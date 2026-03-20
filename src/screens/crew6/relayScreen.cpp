#include "relayScreen.h"
#include "gameGlobalInfo.h"
#include "playerInfo.h"
#include "spaceObjects/playerSpaceship.h"
#include "spaceObjects/scanProbe.h"
#include "scriptInterface.h"

#include "screenComponents/radarView.h"
#include "screenComponents/openCommsButton.h"
#include "screenComponents/commsOverlay.h"
#include "screenComponents/shipsLogControl.h"
#include "screenComponents/hackingDialog.h"
#include "screenComponents/customShipFunctions.h"

#include "gui/gui2_autolayout.h"
#include "gui/gui2_keyvaluedisplay.h"
#include "gui/gui2_selector.h"
#include "gui/gui2_slider.h"
#include "gui/gui2_label.h"
#include "gui/gui2_togglebutton.h"


RelayScreen::RelayScreen(GuiContainer* owner, bool allow_comms, bool allow_alert)
: GuiOverlay(owner, "RELAY_SCREEN", colorConfig.background), mode(TargetSelection)
{
    targets.setAllowWaypointSelection();
    radar = new GuiRadarView(this, "RELAY_RADAR", 50000.0f, &targets, my_spaceship);
    radar->longRange()->enableWaypoints()->enableRoutes()->enableWarpLayer()->enableCallsigns()->setStyle(GuiRadarView::Rectangular)->setFogOfWarStyle(GuiRadarView::FriendlysShortRangeFogOfWar);
    radar->setAutoCentering(false);
    radar->setPosition(0, 0, ATopLeft)->setSize(GuiElement::GuiSizeMax, GuiElement::GuiSizeMax);
    radar->setCallbacks(
        [this](sf::Vector2f position) { //down
            if (mode == TargetSelection && my_spaceship)
            {
                // Check for waypoint dragging
                if (targets.getWaypointIndex() > -1)
                {
                    if (my_spaceship->waypoints[targets.getRouteIndex()][targets.getWaypointIndex()] < empty_waypoint && sf::length(my_spaceship->waypoints[targets.getRouteIndex()][targets.getWaypointIndex()] - position) < 10 / radar->getScale())
                    {
                        mode = MoveWaypoint;
                        drag_route_index = targets.getRouteIndex();
                        drag_waypoint_index = targets.getWaypointIndex();
                    }
                }
                // Check for route dragging
                else if (targets.getSelectedRouteIndex() > -1)
                {
                    if (my_spaceship->routes[targets.getSelectedRouteIndex()][targets.getSelectedRouteWaypointIndex()] < empty_waypoint && sf::length(my_spaceship->routes[targets.getSelectedRouteIndex()][targets.getSelectedRouteWaypointIndex()] - position) < 10 / radar->getScale())
                    {
                        mode = MoveRoute;
                        drag_route_index = targets.getSelectedRouteIndex();
                        drag_waypoint_index = targets.getSelectedRouteWaypointIndex();
                    }
                }
            }
            mouse_down_position = position;
        },
        [this](sf::Vector2f position) { //drag
            if (mode == TargetSelection)
                radar->setViewPosition(radar->getViewPosition() - (position - mouse_down_position));
            if (mode == MoveWaypoint && my_spaceship)
                if (my_spaceship->waypoints[drag_route_index][drag_waypoint_index] < empty_waypoint)
                    my_spaceship->commandMoveWaypoint(drag_waypoint_index, position, drag_route_index);
            if (mode == MoveRoute && my_spaceship)
                if (my_spaceship->routes[drag_route_index][drag_waypoint_index] < empty_waypoint)
                    my_spaceship->commandMoveRoute(drag_waypoint_index, position, drag_route_index);
        },
        [this](sf::Vector2f position) { //up
            switch(mode)
            {
            case TargetSelection:
                targets.setToClosestTo(position, 10 / radar->getScale(), TargetsContainer::Selectable);
                //targets.setToClosestTo(position, 1000, TargetsContainer::Selectable);
                break;
            case WaypointPlacement:
                if (my_spaceship)
                    my_spaceship->commandAddWaypoint(position, route_index);
                mode = TargetSelection;
                placement_buttons->hide();
                option_buttons->show();
                break;
            case RoutePlacement:
                if (my_spaceship)
                    my_spaceship->commandAddRoute(position, route_index);
                mode = TargetSelection;
                placement_buttons->hide();
                option_buttons->show();
                break;
            case MoveWaypoint:
                mode = TargetSelection;
                targets.setRouteIndex(drag_route_index);
                targets.setWaypointIndex(drag_waypoint_index);
                break;
            case MoveRoute:
                mode = TargetSelection;
                targets.setSelectedRouteIndex(drag_route_index);
                targets.setSelectedRouteWaypointIndex(drag_waypoint_index);
                break;
            case LaunchProbe:
                if (my_spaceship)
                    my_spaceship->commandLaunchProbe(position);
                mode = TargetSelection;
                option_buttons->show();
                break;
            }
        }
    );

    if (my_spaceship)
        radar->setViewPosition(my_spaceship->getPosition());

    GuiAutoLayout* sidebar = new GuiAutoLayout(this, "SIDE_BAR", GuiAutoLayout::LayoutVerticalTopToBottom);
    sidebar->setPosition(-20, 150, ATopRight)->setSize(250, GuiElement::GuiSizeMax);
        
    (new GuiLabel(sidebar, "SPACE", "", 30))->setSize(GuiElement::GuiSizeMax, 30);
    
    info_distance = new GuiKeyValueDisplay(sidebar, "DISTANCE", 0.4, "Distance", "");
    info_distance->setSize(GuiElement::GuiSizeMax, 30);

    info_callsign = new GuiKeyValueDisplay(sidebar, "SCIENCE_CALLSIGN", 0.4, tr("Callsign"), "");
    info_callsign->setSize(GuiElement::GuiSizeMax, 30);

    info_faction = new GuiKeyValueDisplay(sidebar, "SCIENCE_FACTION", 0.4, tr("Faction"), "");
    info_faction->setSize(GuiElement::GuiSizeMax, 30);

    zoom_slider = new GuiSlider(this, "ZOOM_SLIDER", 50000.0f, 6250.0f, 50000.0f, [this](float value) {
        zoom_label->setText(tr("Zoom: {zoom}x").format({{"zoom", string(50000.0f / value, 1.0f)}}));
        radar->setDistance(value);
    });
    zoom_slider->setPosition(20, -70, ABottomLeft)->setSize(250, 50);
    zoom_label = new GuiLabel(zoom_slider, "", "Zoom: 1.0x", 30);
    zoom_label->setSize(GuiElement::GuiSizeMax, GuiElement::GuiSizeMax);

    // Option buttons for comms, waypoints, and probes.
    option_buttons = new GuiAutoLayout(this, "BUTTONS", GuiAutoLayout::LayoutVerticalTopToBottom);
    option_buttons->setPosition(20, 50, ATopLeft)->setSize(250, GuiElement::GuiSizeMax);
    
    // Placement buttons for waypoint/route placement mode.
    placement_buttons = new GuiAutoLayout(this, "PLACEMENT_BUTTONS", GuiAutoLayout::LayoutVerticalTopToBottom);
    placement_buttons->setPosition(20, 50, ATopLeft)->setSize(250, GuiElement::GuiSizeMax);
    placement_buttons->hide();
    
    // Add placement mode UI elements
    (new GuiLabel(placement_buttons, "PLACEMENT_LABEL", tr("Placement Mode"), 30))->addBackground()->setSize(GuiElement::GuiSizeMax, 50);
    
    // Route selector for placement mode
    if (gameGlobalInfo->use_advanced_sector_system)
    {
        placement_route_selector = new GuiSelector(placement_buttons, "PLACEMENT_ROUTE_SELECTOR", [this](int index, string value) {
            if (index < PlayerSpaceship::max_routes && index >= -1){
                route_index = index;
            }
        });
        for(int r = 0; r < PlayerSpaceship::max_routes; r++)
        {
            placement_route_selector->addEntry("Colour " + string(r+1), r);
            placement_route_selector->setEntryColor(r, routeColors[r]);
        }
        placement_route_selector->setSize(GuiElement::GuiSizeMax, 50);
        placement_route_selector->setSelectionIndex(0);
    }
    
    // Cancel button for placement mode
    (new GuiButton(placement_buttons, "CANCEL_PLACEMENT_BUTTON", tr("Cancel"), [this]() {
        mode = TargetSelection;
        placement_buttons->hide();
        option_buttons->show();
    }))->setSize(GuiElement::GuiSizeMax, 50);
    
    (new GuiLabel(option_buttons, "INTERACTION_LABEL", tr("Comms Options"), 30))->addBackground()->setSize(GuiElement::GuiSizeMax, 50);

    // Open comms button.
    if (allow_comms == true)
        (new GuiOpenCommsButton(option_buttons, "OPEN_COMMS_BUTTON", tr("Open Comms"), &targets, my_spaceship))->setSize(GuiElement::GuiSizeMax, 50);
    else
        (new GuiOpenCommsButton(option_buttons, "OPEN_COMMS_BUTTON", tr("Link to Comms"), &targets, my_spaceship))->setSize(GuiElement::GuiSizeMax, 50);

    // Hack target
    hack_target_button = new GuiButton(option_buttons, "HACK_TARGET", tr("Start hacking"), [this](){
        P<SpaceObject> target = targets.get();
        if (my_spaceship && target && target->canBeHackedBy(my_spaceship))
        {
            hacking_dialog->open(target);
        }
    });
    hack_target_button->setSize(GuiElement::GuiSizeMax, 50);

    (new GuiLabel(option_buttons, "SPACE", "", 30))->setSize(GuiElement::GuiSizeMax, 20);
    (new GuiLabel(option_buttons, "PROBE_LABEL", tr("Probe Options"), 30))->addBackground()->setSize(GuiElement::GuiSizeMax, 50);
    
    // Launch probe button.
    launch_probe_button = new GuiButton(option_buttons, "LAUNCH_PROBE_BUTTON", tr("Launch Probe"), [this]() {
        mode = LaunchProbe;
        option_buttons->hide();
    });
    launch_probe_button->setSize(GuiElement::GuiSizeMax, 50)->setVisible(my_spaceship && my_spaceship->getCanLaunchProbe());

    // Link probe to science button.
    link_to_science_button = new GuiToggleButton(option_buttons, "LINK_TO_SCIENCE", tr("Link to Science"), [this](bool value){
        if (value)
            my_spaceship->commandSetScienceLink(targets.get()->getMultiplayerId());
        else
            my_spaceship->commandSetScienceLink(-1);
    });
    link_to_science_button->setSize(GuiElement::GuiSizeMax, 50)->setVisible(my_spaceship && my_spaceship->getCanLaunchProbe());

    (new GuiLabel(option_buttons, "SPACE", "", 30))->setSize(GuiElement::GuiSizeMax, 20);
    (new GuiLabel(option_buttons, "WAYPOINT_LABEL", tr("Waypoint Options"), 30))->addBackground()->setSize(GuiElement::GuiSizeMax, 50);
    
    // Manage waypoints.
    (new GuiButton(option_buttons, "WAYPOINT_PLACE_BUTTON", tr("Place Waypoint"), [this]() {
        mode = WaypointPlacement;
        option_buttons->hide();
        placement_buttons->show();
    }))->setSize(GuiElement::GuiSizeMax, 50);

    delete_waypoint_button = new GuiButton(option_buttons, "WAYPOINT_DELETE_BUTTON", tr("Delete Waypoint"), [this]() {
        if (my_spaceship && targets.getWaypointIndex() >= 0)
        {
            my_spaceship->commandRemoveWaypoint(targets.getWaypointIndex(), targets.getRouteIndex());
        }
    });
    delete_waypoint_button->setSize(GuiElement::GuiSizeMax, 50);

    // Manage routes.
    (new GuiButton(option_buttons, "ROUTE_PLACE_BUTTON", tr("Place Route"), [this]() {
        mode = RoutePlacement;
        option_buttons->hide();
        placement_buttons->show();
    }))->setSize(GuiElement::GuiSizeMax, 50);

    delete_route_button = new GuiButton(option_buttons, "ROUTE_DELETE_BUTTON", tr("Delete Route"), [this]() {
        if (my_spaceship && targets.getSelectedRouteWaypointIndex() >= 0)
        {
            my_spaceship->commandRemoveRoute(targets.getSelectedRouteWaypointIndex(), targets.getSelectedRouteIndex());
        }
    });
    delete_route_button->setSize(GuiElement::GuiSizeMax, 50);
    
    // Manage routes.
    if (gameGlobalInfo->use_advanced_sector_system)
    {
        route_selector = new GuiSelector(option_buttons, "ROUTE_SELECTOR", [this](int index, string value) {
            if (index < PlayerSpaceship::max_routes && index >= -1){
                route_index = index;
            }
        });
        for(int r = 0; r < PlayerSpaceship::max_routes; r++)
        {
            route_selector->addEntry("Colour " + string(r+1), r);
            route_selector->setEntryColor(r, routeColors[r]);
        }
        route_selector->setSize(GuiElement::GuiSizeMax, 50);
        route_selector->setSelectionIndex(0);
        route_selector->hide(); // Hide the route selector from main menu
    }
    
    // Show map layers.
    if (gameGlobalInfo->use_warp_terrain)
    {
        (new GuiLabel(option_buttons, "SPACE", "", 30))->setSize(GuiElement::GuiSizeMax, 20);
        (new GuiLabel(option_buttons, "WARP_LABEL", tr("Warp Frequency"), 30))->addBackground()->setSize(GuiElement::GuiSizeMax, 50);
        
        layer_selector = new GuiSelector(option_buttons, "LAYER_SELECTOR", [this](int index, string value) {
            if (index < 10 && index >= 0){
                radar->setWarpLayer(index);
            }
        });
        for(int n = 0; n < 10; n++)
        {
            if (gameGlobalInfo->layer[n].defined)
                layer_selector->addEntry(gameGlobalInfo->layer[n].title, gameGlobalInfo->layer[n].title);
        }
        layer_selector->setSize(GuiElement::GuiSizeMax, 50);
        layer_selector->setSelectionIndex(0);
    }

    // Bottom layout.
    GuiAutoLayout* layout = new GuiAutoLayout(this, "", GuiAutoLayout::LayoutVerticalBottomToTop);
    layout->setPosition(-20, -70, ABottomRight)->setSize(300, GuiElement::GuiSizeMax);

    // Center on ship
    center_button = new GuiToggleButton(layout, "CENTER_ON_SHIP", tr("Centre on Vessel: Off"), [this](bool value) {
        if(!my_spaceship) return;
        radar->setAutoCentering(value);
        // Toggle label shows what will happen when the player presses the button.
        // - If we're currently in free range mode (value=false), the action is to enable centering.
        // - If we're currently centered (value=true), the action is to disable centering.
        center_button->setText(value ? tr("Centre on Vessel: On") : tr("Centre on Vessel: Off"));
    });
    center_button->setSize(GuiElement::GuiSizeMax, 50);
    center_button->setValue(false);
    center_button->setText(tr("Centre on Vessel: Off"));

    // Alert level buttons.
    if (allow_alert) {
    alert_level_button = new GuiToggleButton(layout, "", tr("Alert level"), [this](bool value)
    {
        for(GuiButton* button : alert_level_buttons)
            button->setVisible(value);
    });
    alert_level_button->setValue(false);
    alert_level_button->setSize(GuiElement::GuiSizeMax, 50);

    for(int level=AL_Normal; level < AL_MAX; level++)
    {
        GuiButton* alert_button = new GuiButton(layout, "", alertLevelToLocaleString(EAlertLevel(level)), [this, level]()
        {
            if (my_spaceship)
                my_spaceship->commandSetAlertLevel(EAlertLevel(level));
            for(GuiButton* button : alert_level_buttons)
                button->setVisible(false);
            alert_level_button->setValue(false);
        });
        alert_button->setVisible(false);
        alert_button->setSize(GuiElement::GuiSizeMax, 50);
        alert_level_buttons.push_back(alert_button);
    }
    }

    (new GuiCustomShipFunctions(this, relayOfficer, "", my_spaceship))->setPosition(-20, 280, ATopRight)->setSize(250, GuiElement::GuiSizeMax);

    hacking_dialog = new GuiHackingDialog(this, "");

    if (allow_comms)
    {
        new ShipsLog(this, relayOfficer);
        (new GuiCommsOverlay(this))->setSize(GuiElement::GuiSizeMax, GuiElement::GuiSizeMax);
    }
}

void RelayScreen::onDraw(sf::RenderTarget& window)
{
    ///Handle mouse wheel
    float mouse_wheel_delta = InputHandler::getMouseWheelDelta();
    if (mouse_wheel_delta != 0.0 && my_spaceship)
    {
        float view_distance = radar->getDistance() * (1.0 - (mouse_wheel_delta * 0.1f));
        if (view_distance > my_spaceship->getFarRangeRadarRange())
            view_distance = my_spaceship->getFarRangeRadarRange();
        if (view_distance < my_spaceship->getShortRangeRadarRange())
            view_distance = my_spaceship->getShortRangeRadarRange();
        radar->setDistance(view_distance);
        // Keep the zoom slider in sync.
        zoom_slider->setValue(view_distance);
        zoom_label->setText(tr("Zoom: {zoom}x").format({{"zoom", string(my_spaceship->getFarRangeRadarRange() / view_distance, 1)}}));
    }
    ///!

    GuiOverlay::onDraw(window);
    
    // Info Distance
    if (my_spaceship)
    {
        float ratio_screen = radar->getRect().width / radar->getRect().height;
        float distance_width = radar->getDistance() * 2.0 * ratio_screen / 1000.0f;
        float distance_height = radar->getDistance() * 2.0 / 1000.0f;
        if (distance_width < 100)
            info_distance -> setValue(string(distance_width, 1.0f) + " U / " + string(distance_height, 1.0f) + " U");
        else
            info_distance -> setValue(string(distance_width, 0.0f) + " U / " + string(distance_height, 0.0f) + " U");
    }

    info_faction->setValue("-");

    if (targets.get() && my_spaceship)
    {
        P<SpaceObject> target = targets.get();
        bool near_friendly = false;
        foreach(SpaceObject, obj, space_object_list)
        {
            if ((!P<SpaceShip>(obj) && !P<SpaceStation>(obj)) || !obj->isFriendly(my_spaceship))
            {
                P<ScanProbe> sp = obj;
                if (!sp || sp->owner_id != my_spaceship->getMultiplayerId())
                {
                    continue;
                }
            }
            if (obj->getPosition() - target->getPosition() < 5000.0f * my_spaceship->getSystemEffectiveness(SYS_Scanner))
            {
                near_friendly = true;
                break;
            }
        }
        if (!near_friendly)
            targets.clear();
    }

    if (targets.get())
    {
        P<SpaceObject> obj = targets.get();
        P<SpaceShip> ship = obj;
        P<SpaceStation> station = obj;
        P<ScanProbe> probe = obj;

        info_callsign->setValue(obj->getCallSign());

        if (ship)
        {
            if (ship->getScannedStateFor(my_spaceship) >= SS_SimpleScan)
            {
                info_faction->setValue(factionInfo[obj->getFactionId()]->getLocaleName());
            }
        }else{
            info_faction->setValue(factionInfo[obj->getFactionId()]->getLocaleName());
        }

        if (probe && my_spaceship && probe->owner_id == my_spaceship->getMultiplayerId() && probe->canBeTargetedBy(my_spaceship))
        {
            link_to_science_button->setValue(my_spaceship->linked_science_probe_id == probe->getMultiplayerId());
            link_to_science_button->enable();
        }
        else
        {
            link_to_science_button->setValue(false);
            link_to_science_button->disable();
        }
        if (my_spaceship && obj->canBeHackedBy(my_spaceship))
        {
            hack_target_button->enable();
        }else{
            hack_target_button->disable();
        }
    }else{
        hack_target_button->disable();
        link_to_science_button->disable();
        link_to_science_button->setValue(false);
        info_callsign->setValue("-");
    }
    if (my_spaceship)
    {
        // Toggle ship capabilities.
        launch_probe_button->setVisible(my_spaceship->getCanLaunchProbe());
        link_to_science_button->setVisible(my_spaceship->getCanLaunchProbe());
        hack_target_button->setVisible(my_spaceship->getCanHack());

        launch_probe_button->setText(tr("Launch Probe") + " (" + string(my_spaceship->scan_probe_stock) + ")");
    }

    // Enable/disable waypoint deletion button based on waypoint selection
    if (targets.getWaypointIndex() >= 0)
        delete_waypoint_button->enable();
    else
        delete_waypoint_button->disable();

    // Enable/disable route deletion button based on route selection
    if (targets.getSelectedRouteIndex() >= 0)
        delete_route_button->enable();
    else
        delete_route_button->disable();
}

void RelayScreen::onHotkey(const HotkeyResult& key)
{
    if (key.category == "RELAY" && my_spaceship)
    {
        if (key.hotkey == "LINK_SCIENCE")
        {
            P<ScanProbe> obj = targets.get();
            if (obj && obj->isFriendly(my_spaceship))
            {
                if (!link_to_science_button->getValue())
                    my_spaceship->commandSetScienceLink(targets.get()->getMultiplayerId());
                else
                my_spaceship->commandSetScienceLink(-1);
            }
        }
        if (key.hotkey == "BEGIN_HACK")
        {
            P<SpaceObject> target = targets.get();
            if (my_spaceship && target && target->canBeHackedBy(my_spaceship))
            {
                hacking_dialog->open(target);
            }
        }
        if (key.hotkey == "ADD_WAYPOINT")
        {
            mode = WaypointPlacement;
            option_buttons->hide();
            placement_buttons->show();
        }
        if (key.hotkey == "DELETE_WAYPOINT")
        {
            if (targets.getWaypointIndex() >= 0)
                my_spaceship->commandRemoveWaypoint(targets.getWaypointIndex(), targets.getRouteIndex());
        }
        if (key.hotkey == "DELETE_ROUTE")
        {
            if (targets.getSelectedRouteWaypointIndex() >= 0)
                my_spaceship->commandRemoveRoute(targets.getSelectedRouteWaypointIndex(), targets.getSelectedRouteIndex());
        }
        if (key.hotkey == "LAUNCH_PROBE")
        {
            mode = LaunchProbe;
            option_buttons->hide();
        }
        if (key.hotkey == "INC_ZOOM")
        {
            float view_distance = radar->getDistance() * 1.1f;
            if (view_distance > my_spaceship->getFarRangeRadarRange())
                view_distance = my_spaceship->getFarRangeRadarRange();
            if (view_distance < my_spaceship->getShortRangeRadarRange())
                view_distance = my_spaceship->getShortRangeRadarRange();
            radar->setDistance(view_distance);
            // Keep the zoom slider in sync.
            zoom_slider->setValue(view_distance);
            zoom_label->setText(tr("Zoom: {zoom}x").format({{"zoom", string(my_spaceship->getFarRangeRadarRange() / view_distance, 1)}}));
        }
        if (key.hotkey == "DEC_ZOOM")
        {
            float view_distance = radar->getDistance() * 0.9f;
            if (view_distance > my_spaceship->getFarRangeRadarRange())
                view_distance = my_spaceship->getFarRangeRadarRange();
            if (view_distance < my_spaceship->getShortRangeRadarRange())
                view_distance = my_spaceship->getShortRangeRadarRange();
            radar->setDistance(view_distance);
            // Keep the zoom slider in sync.
            zoom_slider->setValue(view_distance);
            zoom_label->setText(tr("Zoom: {zoom}x").format({{"zoom", string(my_spaceship->getFarRangeRadarRange() / view_distance, 1)}}));
        }
        if (key.hotkey == "ALERT_NORMAL")
        {
            my_spaceship->commandSetAlertLevel(AL_Normal);
            for(GuiButton* button : alert_level_buttons)
                button->setVisible(false);
            alert_level_button->setValue(false);
        }
        if (key.hotkey == "ALERT_YELLOW")
        {
            my_spaceship->commandSetAlertLevel(AL_YellowAlert);
            for(GuiButton* button : alert_level_buttons)
                button->setVisible(false);
            alert_level_button->setValue(false);
        }
        if (key.hotkey == "ALERT_RED")
        {
            my_spaceship->commandSetAlertLevel(AL_RedAlert);
            for(GuiButton* button : alert_level_buttons)
                button->setVisible(false);
            alert_level_button->setValue(false);
        }
    }
}
