#include <i18n.h>
#include "playerInfo.h"
#include "spaceObjects/playerSpaceship.h"
#include "dockingButton.h"
#include "screens/crew6/helmsScreen.h"
#include "screens/crew1/singlePilotView.h"
#include "gui/gui2_element.h"

GuiDockingButton::GuiDockingButton(GuiContainer* owner, string id, P<PlayerSpaceship> targetSpaceship)
: GuiButton(owner, id, "", [this]() { click(); }), target_spaceship(targetSpaceship)
{
    setIcon("gui/icons/docking");
}

void GuiDockingButton::click()
{
    if (!target_spaceship)
        return;
    switch(target_spaceship->docking_state)
    {
    case DS_NotDocking:
        {
            // Show docking menu for both ships and drones (find HelmsScreen or SinglePilotView)
            GuiContainer* owner = getOwner();
            while (owner)
            {
                if (HelmsScreen* helms_screen = dynamic_cast<HelmsScreen*>(owner))
                {
                    helms_screen->showDockingMenu();
                    break;
                }
                if (SinglePilotView* single_pilot = dynamic_cast<SinglePilotView*>(owner))
                {
                    single_pilot->showDockingMenu();
                    break;
                }
                GuiElement* element = dynamic_cast<GuiElement*>(owner);
                if (element)
                    owner = element->getOwner();
                else
                    break;
            }
        }
        break;
    case DS_Docking:
        target_spaceship->commandAbortDock();
        break;
    case DS_Docked:
        target_spaceship->commandUndock();
        break;
    }
}

void GuiDockingButton::onUpdate()
{
    setVisible(target_spaceship && target_spaceship->getCanDock());
}

void GuiDockingButton::onDraw(sf::RenderTarget& window)
{
    if (target_spaceship)
    {
        P<SpaceObject> docking_target = findDockingTarget();
        
        switch(target_spaceship->docking_state)
        {
        case DS_NotDocking:
            setText(tr("Initiate Docking"));
            if (target_spaceship->canStartDocking() && docking_target)
            {
                enable();
            }else{
                disable();
            }
            break;
        case DS_Docking:
            setText(tr("Cancel Docking"));
            enable();
            break;
        case DS_Docked:
            setText(tr("Undock"));
            enable();
            break;
        }
    }
    GuiButton::onDraw(window);
}

void GuiDockingButton::onHotkey(const HotkeyResult& key)
{
    if (key.category == "HELMS" && target_spaceship)
    {
        if (key.hotkey == "DOCK_ACTION")
        {
            switch(target_spaceship->docking_state)
            {
            case DS_NotDocking:
                target_spaceship->commandDock(findDockingTarget());
                break;
            case DS_Docking:
                target_spaceship->commandAbortDock();
                break;
            case DS_Docked:
                target_spaceship->commandUndock();
                break;
            }
        }
        else if (key.hotkey == "DOCK_REQUEST")
        {
            // Find the docking menu (HelmsScreen or SinglePilotView) and show it
            GuiContainer* owner = getOwner();
            while (owner)
            {
                if (HelmsScreen* helms_screen = dynamic_cast<HelmsScreen*>(owner))
                {
                    helms_screen->showDockingMenu();
                    break;
                }
                if (SinglePilotView* single_pilot = dynamic_cast<SinglePilotView*>(owner))
                {
                    single_pilot->showDockingMenu();
                    break;
                }
                GuiElement* element = dynamic_cast<GuiElement*>(owner);
                if (element)
                    owner = element->getOwner();
                else
                    break;
            }
        }
        else if (key.hotkey == "DOCK_ABORT")
            target_spaceship->commandAbortDock();
        else if (key.hotkey == "UNDOCK")
            target_spaceship->commandUndock();
    }
}

P<SpaceObject> GuiDockingButton::findDockingTarget()
{
    PVector<Collisionable> obj_list = CollisionManager::queryArea(target_spaceship->getPosition() - sf::Vector2f(1000, 1000), target_spaceship->getPosition() + sf::Vector2f(1000, 1000));
    P<SpaceObject> dock_object;
    foreach(Collisionable, obj, obj_list)
    {
        dock_object = obj;
        P<SpaceShip> dock_ship = dock_object;
        if ((dock_ship && dock_ship->getDockingState() == 1) || (dock_ship && dock_ship->getDockedWith() == target_spaceship))
        {
            dock_object = NULL;
            continue;
        }
        // SBW: DockingButton in latest Daid code refers to my_spaceship rather than target:
        //      if (dock_object && dock_object != my_spaceship && dock_object->canBeDockedBy(my_spaceship) != DockStyle::None && sf::length(dock_object->getPosition() - my_spaceship->getPosition()) < 1000.0f + dock_object->getRadius())
        // This doesn't make any difference in our fork because target_spaceship is always player_spaceship,
        // but keep it consistent with the rest of the class by referring to target_spaceship
        if (dock_object && dock_object != target_spaceship && dock_object->canBeDockedBy(target_spaceship) != DockStyle::None && (dock_object->getPosition() - target_spaceship->getPosition()) < 1000.0f + dock_object->getRadius())
            break;
        dock_object = NULL;
    }
    return dock_object;
}
