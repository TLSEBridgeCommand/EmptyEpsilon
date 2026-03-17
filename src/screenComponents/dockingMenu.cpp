#include <i18n.h>
#include "engine.h"
#include "playerInfo.h"
#include "spaceObjects/playerSpaceship.h"
#include "spaceObjects/spaceStation.h"
#include "spaceObjects/spaceship.h"
#include "dockingMenu.h"

#include "gui/gui2_listbox.h"
#include "gui/gui2_button.h"
#include "gui/gui2_label.h"
#include "gui/gui2_autolayout.h"

GuiDockingMenu::GuiDockingMenu(GuiContainer* owner, P<PlayerSpaceship> targetSpaceship)
: GuiOverlay(owner, "DOCKING_MENU", sf::Color(0, 0, 0, 128)), target_spaceship(targetSpaceship)
{
    // Create a semi-transparent background
    GuiOverlay* background = new GuiOverlay(this, "BACKGROUND", sf::Color(0, 0, 0, 128));
    background->setSize(GuiElement::GuiSizeMax, GuiElement::GuiSizeMax);

    // Main container for the menu
    GuiAutoLayout* main_layout = new GuiAutoLayout(this, "MAIN_LAYOUT", GuiAutoLayout::LayoutVerticalTopToBottom);
    main_layout->setPosition(0, 0, ACenter)->setSize(400, 500)->setMargins(20, 20, 20, 20);

    // Title
    title_label = new GuiLabel(main_layout, "TITLE", tr("Select Docking Target"), 30);
    title_label->addBackground()->setAlignment(ACenter)->setSize(GuiElement::GuiSizeMax, 50);

    // List of docking targets
    docking_targets_list = new GuiListbox(main_layout, "DOCKING_TARGETS", [this](int index, string value) {
        // Enable dock button when an item is selected
        dock_button->setEnable(index >= 0);
        // Debug: print selection info
        if (index >= 0)
        {
            LOG(INFO) << "Docking menu: Selected index " << index << " with value " << value;
        }
    });
    docking_targets_list->setSize(GuiElement::GuiSizeMax, 350);
    
    // Label for when there are no docking targets
    no_targets_label = new GuiLabel(main_layout, "NO_TARGETS", tr("No docking targets in range"), 20);
    no_targets_label->addBackground()->setAlignment(ACenter)->setSize(GuiElement::GuiSizeMax, 50);
    no_targets_label->setVisible(false);

    // Button layout
    button_layout = new GuiAutoLayout(main_layout, "BUTTON_LAYOUT", GuiAutoLayout::LayoutHorizontalLeftToRight);
    button_layout->setSize(GuiElement::GuiSizeMax, 50);

     // Cancel button
     cancel_button = new GuiButton(button_layout, "CANCEL_BUTTON", tr("Cancel"), [this]() {
        cancelDocking();
    });
    cancel_button->setSize(180, 50);

    // Dock button
    dock_button = new GuiButton(button_layout, "DOCK_BUTTON", tr("Dock"), [this]() {
        dockWithSelectedTarget();
    });
    dock_button->setSize(180, 50);

    // Populate the list initially
    populateDockingTargets();
}

void GuiDockingMenu::setTargetSpaceship(P<PlayerSpaceship> targetSpaceship)
{
    target_spaceship = targetSpaceship;
    populateDockingTargets();
}

void GuiDockingMenu::onUpdate()
{
    // Only update if the menu is visible
    if (!visible)
        return;
    
    // Periodically update the docking targets list to remove ships that have left range
    static float last_update_time = 0.0f;
    float current_time = engine->getElapsedTime();
    float time_since_update = current_time - last_update_time;
    
    if (time_since_update > 0.5f) // Update every 0.5 seconds
    {
        populateDockingTargets();
        last_update_time = current_time;
    }
    
    // Enable/disable dock button based on selection
    dock_button->setEnable(docking_targets_list->getSelectionIndex() >= 0);
}

void GuiDockingMenu::populateDockingTargets()
{
    if (!target_spaceship)
        return;

    // Store current selection
    string current_selection = "";
    if (docking_targets_list->getSelectionIndex() >= 0)
    {
        current_selection = docking_targets_list->getEntryValue(docking_targets_list->getSelectionIndex());
    }

    // Clear existing entries
    while (docking_targets_list->entryCount() > 0)
    {
        docking_targets_list->removeEntry(0);
    }

    // Only show targets if ship can start docking (same logic as docking button enable check)
    if (!target_spaceship->canStartDocking())
    {
        // Show/hide the "no targets" message
        bool has_targets = false;
        docking_targets_list->setVisible(has_targets);
        if (no_targets_label)
        {
            no_targets_label->setVisible(!has_targets);
        }
        return;
    }

    // Search for docking targets using the exact same logic as docking button's findDockingTarget()
    PVector<Collisionable> obj_list = CollisionManager::queryArea(
        target_spaceship->getPosition() - sf::Vector2f(1000, 1000), 
        target_spaceship->getPosition() + sf::Vector2f(1000, 1000)
    );

    foreach(Collisionable, obj, obj_list)
    {
        P<SpaceObject> dock_object = obj;
        if (!dock_object || dock_object == target_spaceship)
            continue;
            
        // Same logic as docking button's findDockingTarget()
        P<SpaceShip> dock_ship = dock_object;
        if (dock_ship && (dock_ship->getDockingState() == DS_Docking || dock_ship->getDockedWith() == target_spaceship))
            continue;
            
        // Same checks as docking button: canBeDockedBy != None and distance check
        if (dock_object->canBeDockedBy(target_spaceship) != DockStyle::None && 
            sf::length(dock_object->getPosition() - target_spaceship->getPosition()) < 1000.0f + dock_object->getRadius())
        {
            // All checks passed - add to list
            string callsign = dock_object->getCallSign();
            string object_type = "";
            
            // Determine object type for display
            if (P<SpaceStation>(dock_object))
                object_type = " (Station)";
            else if (P<SpaceShip>(dock_object))
                object_type = " (Ship)";
            
            string display_text = callsign + object_type;
            int index = docking_targets_list->addEntry(display_text, callsign);
            
            // Restore selection if this was the previously selected item
            if (callsign == current_selection)
            {
                docking_targets_list->setSelectionIndex(index);
            }
        }
    }
    
    // If we had a selection but it's no longer valid, clear the selection
    if (!current_selection.empty() && docking_targets_list->getSelectionIndex() < 0)
    {
        docking_targets_list->setSelectionIndex(-1);
    }
    
    // Show/hide the "no targets" message based on whether the list is empty
    bool has_targets = docking_targets_list->entryCount() > 0;
    docking_targets_list->setVisible(has_targets);
    if (no_targets_label)
    {
        no_targets_label->setVisible(!has_targets);
    }
}

void GuiDockingMenu::dockWithSelectedTarget()
{
    if (!target_spaceship || docking_targets_list->getSelectionIndex() < 0)
        return;

    string selected_callsign = docking_targets_list->getEntryValue(docking_targets_list->getSelectionIndex());
    
    // Find the selected object by callsign
    PVector<Collisionable> obj_list = CollisionManager::queryArea(
        target_spaceship->getPosition() - sf::Vector2f(1000, 1000), 
        target_spaceship->getPosition() + sf::Vector2f(1000, 1000)
    );

    foreach(Collisionable, obj, obj_list)
    {
        P<SpaceObject> dock_object = obj;
        if (dock_object && 
            dock_object->getCallSign() == selected_callsign &&
            dock_object->canBeDockedBy(target_spaceship) != DockStyle::None)
        {
            target_spaceship->commandDock(dock_object);
            hide();
            return;
        }
    }
}

void GuiDockingMenu::cancelDocking()
{
    hide();
} 