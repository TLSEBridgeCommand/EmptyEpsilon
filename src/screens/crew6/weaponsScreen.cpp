#include <i18n.h>
#include "playerInfo.h"
#include "gameGlobalInfo.h"
#include "weaponsScreen.h"
#include "preferenceManager.h"

#include "screenComponents/missileTubeControls.h"
#include "screenComponents/aimLock.h"
#include "screenComponents/fireLock.h"
#include "screenComponents/beamFrequencySelector.h"
#include "screenComponents/beamTargetSelector.h"
#include "screenComponents/powerDamageIndicator.h"
#include "screenComponents/shieldFreqencySelect.h"
#include "screenComponents/shieldsEnableButton.h"
#include "screenComponents/alertOverlay.h"
#include "screenComponents/customShipFunctions.h"
#include "screenComponents/tractorBeamControl.h"
#include "screenComponents/commsOverlay.h"
#include "screenComponents/shipsLogControl.h"

#include "gui/gui2_rotationdial.h"
#include "gui/gui2_label.h"
#include "gui/gui2_keyvaluedisplay.h"

WeaponsScreen::WeaponsScreen(GuiContainer* owner)
: GuiOverlay(owner, "WEAPONS_SCREEN", colorConfig.background)
{
    // Render the radar shadow and background decorations.
    background_gradient = new GuiOverlay(this, "BACKGROUND_GRADIENT", sf::Color::White);
    background_gradient->setTextureCenter("gui/BackgroundGradient");

    background_crosses = new GuiOverlay(this, "BACKGROUND_CROSSES", sf::Color::White);
    background_crosses->setTextureTiled("gui/BackgroundCrosses");

    // Render the alert level color overlay.
    (new AlertLevelOverlay(this));

    radar = new GuiRadarView(this, "HELMS_RADAR", &targets, my_spaceship);
    radar->setPosition(0, 0, ACenter)->setSize(GuiElement::GuiSizeMatchHeight, 800);
    radar->setRangeIndicatorStepSize(1000.0)->shortRange()->enableCallsigns()->enableHeadingIndicators()->setStyle(GuiRadarView::Circular);
    radar->setCallbacks(
        [this](sf::Vector2f position) {
            targets.setToClosestTo(position, 250, TargetsContainer::Targetable);
            if (my_spaceship && targets.get())
                my_spaceship->commandSetTarget(targets.get(), PreferencesManager::get("weapons_specific_station", "0").toInt());
            else if (my_spaceship)
                my_spaceship->commandSetTarget(NULL, PreferencesManager::get("weapons_specific_station", "0").toInt());
        }, nullptr, nullptr
    );
    radar->setAutoRotating(PreferencesManager::get("weapons_radar_lock","0")=="1");

    missile_aim = new AimLock(this, "MISSILE_AIM", radar, -90, 360 - 90, 0, [this](float value){
        tube_controls->setMissileTargetAngle(value);
    });
    missile_aim->setPosition(0, 0, ACenter)->setSize(GuiElement::GuiSizeMatchHeight, 850);

    tube_controls = new GuiMissileTubeControls(this, "MISSILE_TUBES", my_spaceship);
    tube_controls->setPosition(20, -20, ABottomLeft);
    radar->enableTargetProjections(tube_controls);

    lock_aim = new AimLockButton(this, "LOCK_AIM", tube_controls, missile_aim, my_spaceship);
    lock_aim->setPosition(250, 20, ATopCenter)->setSize(130, 50);
    bool lock_is_visible = PreferencesManager::get("weapons_specific_station", "0").toInt() == 0 || my_spaceship->getLockButtonStation() == PreferencesManager::get("weapons_specific_station", "0").toInt();
    lock_aim->setVisible(lock_is_visible);
    
    if (gameGlobalInfo->all_can_be_targeted)
    {
        lock_fire = new FireLockButton(this, "LOCK_FIRE", my_spaceship);
        lock_fire->setPosition(-250, 20, ATopCenter)->setSize(130, 50);
        bool is_visible = PreferencesManager::get("weapons_specific_station", "0").toInt() == 0 || my_spaceship->getBeamsButtonStation() == PreferencesManager::get("weapons_specific_station", "0").toInt();
        LOG(INFO) << " NatDebug " << "is_visible " << is_visible << ", weapons_specific_station " << PreferencesManager::get("weapons_specific_station", "0").toInt() << ", getBeamsButtonStation() " << my_spaceship->getBeamsButtonStation();
        lock_fire->setVisible(is_visible);
    }
    
    if (gameGlobalInfo->use_beam_shield_frequencies || gameGlobalInfo->use_system_damage)
    {
        GuiElement* beam_info_box = new GuiElement(this, "BEAM_INFO_BOX");
        beam_info_box->setPosition(-20, -200, ABottomRight)->setSize(280, 150);
        (new GuiLabel(beam_info_box, "BEAM_INFO_LABEL", tr("Beam info"), 30))->addBackground()->setPosition(0, -200, ABottomRight)->setSize(GuiElement::GuiSizeMax, 50);
        (new GuiLabel(beam_info_box, "BEAM_TARGET_LABEL", tr("Target"), 30))->addBackground()->setPosition(0, -150, ABottomRight)->setSize(GuiElement::GuiSizeMax, 50);
        (new GuiPowerDamageIndicator(beam_info_box, "", SYS_BeamWeapons, ACenterLeft, my_spaceship))->setSize(GuiElement::GuiSizeMax, 50);
        (new GuiBeamTargetSelector(beam_info_box, "BEAM_TARGET_SELECTOR"))->setPosition(0, -100, ABottomRight)->setSize(GuiElement::GuiSizeMax, 50);
        (new GuiLabel(beam_info_box, "BEAM_FREQUENCY_LABEL", tr("Frequency"), 30))->addBackground()->setPosition(0, -50, ABottomRight)->setSize(GuiElement::GuiSizeMax, 50);
        (new GuiBeamFrequencySelector(beam_info_box, "BEAM_FREQUENCY_SELECTOR"))->setPosition(0, 0, ABottomRight)->setSize(GuiElement::GuiSizeMax, 50);

        if (!gameGlobalInfo->use_beam_shield_frequencies)
        {   //If we do have system damage, but no shield frequencies, we can partially overlap this with the shield button.
            //So move the beam configuration a bit down.
            beam_info_box->setPosition(-20, -50, ABottomRight);
        }
    }

    GuiAutoLayout* stats = new GuiAutoLayout(this, "WEAPONS_STATS", GuiAutoLayout::LayoutVerticalTopToBottom);
    stats->setPosition(20, 100, ATopLeft)->setSize(240, 180);
	
    station_display = new GuiKeyValueDisplay(stats, "STATION_DISPLAY", 0.45, tr("Weapons", "Station"), "");
    station_display->setIcon("gui/icons/station-weapons")->setTextSize(20)->setSize(240, 40);
    energy_display = new GuiKeyValueDisplay(stats, "ENERGY_DISPLAY", 0.45, tr("Energy"), "");
    energy_display->setIcon("gui/icons/energy")->setTextSize(20)->setSize(240, 40);
    front_shield_display = new GuiKeyValueDisplay(stats, "FRONT_SHIELD_DISPLAY", 0.45, tr("shields","Front"), "");
    front_shield_display->setIcon("gui/icons/shields-fore")->setTextSize(20)->setSize(240, 40);
    rear_shield_display = new GuiKeyValueDisplay(stats, "REAR_SHIELD_DISPLAY", 0.45, tr("shields", "Rear"), "");
    rear_shield_display->setIcon("gui/icons/shields-aft")->setTextSize(20)->setSize(240, 40);

    if (PreferencesManager::get("weapons_specific_station", "0").toInt() == 0 || my_spaceship->getShieldsStation() == PreferencesManager::get("weapons_specific_station", "0").toInt())
    {
        GuiElement* shield_info_box = new GuiElement(this, "SHIELD_INFO_BOX");
        shield_info_box->setPosition(-20, -120, ABottomRight)->setSize(280, 150);
        (new GuiLabel(shield_info_box, "SHIELDS_INFO_LABEL", tr("Shield info"), 30))->addBackground()->setPosition(0, 0, ABottomRight)->setSize(GuiElement::GuiSizeMax, 50);
        if (gameGlobalInfo->use_beam_shield_frequencies)
        {
            //The shield frequency selection includes a shield enable button.
            (new GuiShieldFrequencySelect(this, "SHIELD_FREQ", my_spaceship))->setPosition(-20, -20, ABottomRight)->setSize(280, 100);
        }else{
            (new GuiShieldsEnableButton(this, "SHIELDS_ENABLE", my_spaceship))->setPosition(-20, -20, ABottomRight)->setSize(280, 50);
        }
    }

    (new GuiCustomShipFunctions(this, weaponsOfficer, "", my_spaceship))->setPosition(-20, 120, ATopRight)->setSize(250, GuiElement::GuiSizeMax);

    // new ShipsLog(this, weaponsOfficer);
}

void WeaponsScreen::onDraw(sf::RenderTarget& window)
{
    if (my_spaceship)
    {
        energy_display->setValue(string(int(my_spaceship->energy_level)));
        front_shield_display->setValue(string(my_spaceship->getShieldPercentage(0)) + "%");
        if (my_spaceship->hasSystem(SYS_FrontShield))
        {
            front_shield_display->show();
        } else {
            front_shield_display->hide();
        }
        rear_shield_display->setValue(string(my_spaceship->getShieldPercentage(1)) + "%");
		if (my_spaceship->hasSystem(SYS_RearShield))
        {
            rear_shield_display->show();
        } else {
            rear_shield_display->hide();
        }

        targets.set(my_spaceship->getTarget(PreferencesManager::get("weapons_specific_station", "0").toInt()));
        station_display->setValue(string(PreferencesManager::get("weapons_specific_station_name", "")))->setVisible(string(PreferencesManager::get("weapons_specific_station_name", "")) != "");

        missile_aim->setVisible(tube_controls->getManualAim());
    }
    GuiOverlay::onDraw(window);
}

void WeaponsScreen::onHotkey(const HotkeyResult& key)
{
    if (key.category == "WEAPONS" && my_spaceship)
    {
        if (key.hotkey == "NEXT_ENEMY_TARGET")
        {
            bool current_found = false;
            foreach(SpaceObject, obj, space_object_list)
            {
                if (obj == targets.get())
                {
                    current_found = true;
                    continue;
                }
                if (current_found && sf::length(obj->getPosition() - my_spaceship->getPosition()) < my_spaceship->getShortRangeRadarRange() && my_spaceship->isEnemy(obj) && my_spaceship->getScannedStateFor(obj) >= SS_FriendOrFoeIdentified && obj->canBeTargetedBy(my_spaceship))
                {
                    targets.set(obj);
                    my_spaceship->commandSetTarget(targets.get(), PreferencesManager::get("weapons_specific_station", "0").toInt());
                    return;
                }
            }
            foreach(SpaceObject, obj, space_object_list)
            {
                if (obj == targets.get())
                {
                    continue;
                }
                if (my_spaceship->isEnemy(obj) && sf::length(obj->getPosition() - my_spaceship->getPosition()) < my_spaceship->getShortRangeRadarRange() && my_spaceship->getScannedStateFor(obj) >= SS_FriendOrFoeIdentified && obj->canBeTargetedBy(my_spaceship))
                {
                    targets.set(obj);
                    my_spaceship->commandSetTarget(targets.get(), PreferencesManager::get("weapons_specific_station", "0").toInt());
                    return;
                }
            }
        }
        if (key.hotkey == "NEXT_TARGET")
        {
            bool current_found = false;
            foreach(SpaceObject, obj, space_object_list)
            {
                if (obj == targets.get())
                {
                    current_found = true;
                    continue;
                }
                if (obj == my_spaceship)
                    continue;
                if (current_found && sf::length(obj->getPosition() - my_spaceship->getPosition()) < my_spaceship->getShortRangeRadarRange() && obj->canBeTargetedBy(my_spaceship))
                {
                    targets.set(obj);
                    my_spaceship->commandSetTarget(targets.get(), PreferencesManager::get("weapons_specific_station", "0").toInt());
                    return;
                }
            }
            foreach(SpaceObject, obj, space_object_list)
            {
                if (obj == targets.get() || obj == my_spaceship)
                    continue;
                if (sf::length(obj->getPosition() - my_spaceship->getPosition()) < my_spaceship->getShortRangeRadarRange() && obj->canBeTargetedBy(my_spaceship))
                {
                    targets.set(obj);
                    my_spaceship->commandSetTarget(targets.get(), PreferencesManager::get("weapons_specific_station", "0").toInt());
                    return;
                }
            }
        }
        if (key.hotkey == "AIM_MISSILE_LEFT")
        {
            missile_aim->setValue(missile_aim->getValue() - 5.0f);
            tube_controls->setMissileTargetAngle(missile_aim->getValue());
        }
        if (key.hotkey == "AIM_MISSILE_RIGHT")
        {
            missile_aim->setValue(missile_aim->getValue() + 5.0f);
            tube_controls->setMissileTargetAngle(missile_aim->getValue());
        }
    }
}
bool WeaponsScreen::onJoystickAxis(const AxisAction& axisAction){
    if (axisAction.category == "WEAPONS" && my_spaceship){
        if (axisAction.action == "AIM_MISSILE"){
            missile_aim->setValue(axisAction.value * 180);
            tube_controls->setMissileTargetAngle(missile_aim->getValue());
            return true;
        }
    }
    return false;
}
