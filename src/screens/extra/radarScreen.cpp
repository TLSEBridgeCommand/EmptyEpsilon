#include "radarScreen.h"
#include "playerInfo.h"
#include "gameGlobalInfo.h"
#include "spaceObjects/playerSpaceship.h"

#include "screenComponents/indicatorOverlays.h"
#include "screenComponents/selfDestructIndicator.h"
#include "screenComponents/globalMessage.h"
#include "screenComponents/jumpIndicator.h"
#include "screenComponents/commsOverlay.h"
#include "screenComponents/shipDestroyedPopup.h"

#include "screenComponents/radarView.h"
#include "screenComponents/alertOverlay.h"
#include "gui/gui2_overlay.h"

RadarScreen::RadarScreen(GuiContainer* owner, string type)
: GuiOverlay(owner, "RADAR_SCREEN", colorConfig.background), type(type)
{
    radar = new GuiRadarView(this, "TACTICAL_RADAR", 1000.0f, nullptr, my_spaceship);
    radar->setPosition(0, 0, ATopLeft)->setSize(GuiElement::GuiSizeMax, GuiElement::GuiSizeMax);
    radar->enableCallsigns()->enableWaypoints()->show();
    
    if (type == "tactical")
    {   
        radar->setDistance(my_spaceship->getShortRangeRadarRange());
        radar->setRangeIndicatorStepSize(1000.0f)->shortRange()->enableHeadingIndicators();
    }
    
    if (type == "long_range"){
        radar->setDistance(my_spaceship->getLongRangeRadarRange());
        radar->setRangeIndicatorStepSize(5000.0f)->longRange()->enableHeadingIndicators();
        radar->setFogOfWarStyle(GuiRadarView::NebulaFogOfWar);
    }

    if (type == "far_range"){
      radar->setDistance(my_spaceship->getLongRangeRadarRange());
      radar->setAutoCentering(true)->setStyle(GuiRadarView::Rectangular);
      radar->longRange()->setFogOfWarStyle(GuiRadarView::FriendlysShortRangeFogOfWar);
    }

    new GuiJumpIndicator(this);
    new GuiSelfDestructIndicator(this);
    new GuiGlobalMessage(this);
    new GuiIndicatorOverlays(this);
}

void RadarScreen::onDraw(sf::RenderTarget& window)
{
    ///Handle mouse wheel
    float mouse_wheel_delta = InputHandler::getMouseWheelDelta();
    if (mouse_wheel_delta != 0.0 && my_spaceship)
    {
        float distance_min = 1000.0f;
        float distance_max = my_spaceship->getShortRangeRadarRange();
        if (type == "long_range"){
            distance_min = my_spaceship->getShortRangeRadarRange();
            distance_max = my_spaceship->getLongRangeRadarRange();
        }
        if (type == "far_range"){
            distance_min = my_spaceship->getShortRangeRadarRange();
            distance_max = my_spaceship->getFarRangeRadarRange();
        }
        float view_distance = radar->getDistance() * (1.0 - (mouse_wheel_delta * 0.1f));
        if (view_distance > distance_max)
            view_distance = distance_max;
        if (view_distance < distance_min)
            view_distance = distance_min;
        radar->setDistance(view_distance);
    }
}

void RadarScreen::onHotkey(const HotkeyResult& key)
{
    if (key.category == "RADAR" && my_spaceship)
    {
        float distance_min = 1000.0f;
        float distance_max = my_spaceship->getShortRangeRadarRange();
        if (type == "long_range"){
            distance_min = my_spaceship->getShortRangeRadarRange();
            distance_max = my_spaceship->getLongRangeRadarRange();
        }
        if (type == "far_range"){
            distance_min = my_spaceship->getShortRangeRadarRange();
            distance_max = my_spaceship->getFarRangeRadarRange();
        }
        if (key.hotkey == "INC_ZOOM")
        {
            float view_distance = radar->getDistance() * 1.1f;
            if (view_distance > distance_max)
                view_distance = distance_max;
            if (view_distance < distance_min)
                view_distance = distance_min;
            radar->setDistance(view_distance);
        }
        if (key.hotkey == "DEC_ZOOM")
        {
            float view_distance = radar->getDistance() * 0.9f;
            if (view_distance > distance_max)
                view_distance = distance_max;
            if (view_distance < distance_min)
                view_distance = distance_min;
            radar->setDistance(view_distance);
        }
    }
}
