#include "playerInfo.h"
#include "gameGlobalInfo.h"
#include "systemEffectsList.h"

#include "gui/gui2_autolayout.h"
#include "gui/gui2_keyvaluedisplay.h"

GuiSystemEffectsList::GuiSystemEffectsList(GuiContainer* owner, string id, ELayoutMode layout)
: GuiAutoLayout(owner, id, layout), selected_system(SYS_None), primary_info_visible(false), background(true)
{
    selected_system = SYS_None;
    setPosition(0, 0, ABottomRight)->setSize(GuiElement::GuiSizeMax, GuiElement::GuiSizeMax);
}

void GuiSystemEffectsList::onDraw(sf::RenderTarget& window)
{
    if (!my_spaceship)
        return;

    if (selected_system != SYS_None)
    {
        system_effects_index = 0;
        
        float effectiveness = my_spaceship->getSystemEffectiveness(selected_system);
        float health_max = my_spaceship->getSystemHealthMax(selected_system);
        float health = my_spaceship->getSystemHealth(selected_system);
        float heat = my_spaceship->getSystemHeat(selected_system);
        float coolant = my_spaceship->getSystemCoolant(selected_system);
        float repair = my_spaceship->getSystemRepair(selected_system);
        float power = my_spaceship->getSystemPower(selected_system);
        float instability = my_spaceship->getSystemInstabilityLevel(selected_system);
        
        if (primary_info_visible)
        {
            addSystemEffect(tr("Health"), string(systemHealthToPercent(health)) + "%");
            addSystemEffect(tr("Power"), string(int(power * 100)) + "%");
            addSystemEffect(tr("Heat"), string(int(heat * 100)) + "%");
            addSystemEffect(tr("Coolant"), string(coolant,1));
            if (gameGlobalInfo->use_nano_repair_crew)
                addSystemEffect(tr("Repair"), string(repair,1));
            if (my_spaceship->getSystemInstabilityFactor(selected_system) > 0.0)
                addSystemEffect(tr("Instability"), string(int(instability * 100)) + "%");
        } 
        
        addSystemEnergy(selected_system);

        if (health_max < 1.0)
            addSystemEffect(tr("Maximal health"), string(systemHealthToPercent(health_max)) + "%");
        switch(selected_system)
        {
        case SYS_Reactor:
            if (effectiveness > 1.0f)
                effectiveness = (1.0f + effectiveness) / 2.0f;
            break;
        case SYS_Oxygen:
            for(unsigned int n=0; n<10; n++)
            {
                OxygenZone zone = my_spaceship->oxygen_zones[n];
                if (zone.oxygen_max > 0.0 || zone.label != "")
                {
                    float oxygen_rate = my_spaceship->getZoneRechargeRate(n) * 60;
                    if (oxygen_rate >= 0.0)
                        addSystemEffect(tr("Oxygen: ") + zone.label, "+" + string(oxygen_rate, 1) + tr("oxygen_rate","/min"));
                    else
                        addSystemEffect(tr("Oxygen: ") + zone.label, string(oxygen_rate, 1) + tr("oxygen_rate","/min"));
                }
            }
            break;
        case SYS_BeamWeapons:
            addSystemEffect(tr("Firing rate"), string(int(effectiveness * 100)) + "%");
            // If the ship has a turret, also note that the rotation rate
            // is affected.
            for(int n = 0; n < max_beam_weapons; n++)
            {
                if (my_spaceship->beam_weapons[n].getTurretArc() > 0)
                {
                    addSystemEffect(tr("beams","Turret rotation rate"), string(int(effectiveness * 100)) + "%");
                    break;
                }
            }
            break;
        case SYS_MissileSystem:
            addSystemEffect(tr("missile","Reload rate"), string(int(effectiveness * 100)) + "%");
            break;
        case SYS_Maneuver:
            addSystemEffect(tr("Turning speed"), string(int(effectiveness * 100)) + "%");
            if (my_spaceship->combat_maneuver_boost_speed > 0.0 || my_spaceship->combat_maneuver_strafe_speed)
                addSystemEffect(tr("Combat recharge rate"), string(int(((my_spaceship->getSystemEffectiveness(SYS_Maneuver) + my_spaceship->getSystemEffectiveness(SYS_Impulse)) / 2.0) * 100)) + "%");
            break;
        case SYS_Impulse:
            addSystemEffect(tr("Impulse speed"), string(int(effectiveness * 100)) + "%");
            if (my_spaceship->combat_maneuver_boost_speed > 0.0 || my_spaceship->combat_maneuver_strafe_speed)
                addSystemEffect(tr("Combat recharge rate"), string(int(((my_spaceship->getSystemEffectiveness(SYS_Maneuver) + my_spaceship->getSystemEffectiveness(SYS_Impulse)) / 2.0) * 100)) + "%");
            break;
        case SYS_Warp:
            addSystemEffect(tr("Warp drive speed"), string(int(effectiveness * 100)) + "%");
            break;
        case SYS_JumpDrive:
            addSystemEffect(tr("Jump drive recharge rate"), string(int(my_spaceship->getJumpDriveRechargeRate() * 100)) + "%");
            addSystemEffect(tr("Jump drive jump speed"), string(int(effectiveness * 100)) + "%");
            break;
        case SYS_FrontShield:
            if (gameGlobalInfo->use_beam_shield_frequencies)
                addSystemEffect(tr("shields","Calibration speed"), string(int((my_spaceship->getSystemEffectiveness(SYS_FrontShield) + my_spaceship->getSystemEffectiveness(SYS_RearShield)) / 2.0 * 100)) + "%");
            addSystemEffect(tr("shields","Charge rate"), string(int(effectiveness * 100)) + "%");
            {
                DamageInfo di;
                di.type = DT_Kinetic;
                float damage_negate = 1.0f - my_spaceship->getShieldDamageFactor(di, 0);
                if (damage_negate < 0.0)
                    addSystemEffect(tr("Extra damage"), string(int(-damage_negate * 100)) + "%");
                else
                    addSystemEffect(tr("Damage negate"), string(int(damage_negate * 100)) + "%");
            }
            break;
        case SYS_RearShield:
            if (gameGlobalInfo->use_beam_shield_frequencies)
                addSystemEffect(tr("shields","Calibration speed"), string(int((my_spaceship->getSystemEffectiveness(SYS_FrontShield) + my_spaceship->getSystemEffectiveness(SYS_RearShield)) / 2.0 * 100)) + "%");
            addSystemEffect(tr("shields","Charge rate"), string(int(effectiveness * 100)) + "%");
            {
                DamageInfo di;
                di.type = DT_Kinetic;
                float damage_negate = 1.0f - 
    my_spaceship->getShieldDamageFactor(di, my_spaceship->shield_count - 1);
                if (damage_negate < 0.0)
                    addSystemEffect(tr("Extra damage"), string(int(-damage_negate * 100)) + "%");
                else
                    addSystemEffect(tr("Damage negate"), string(int(damage_negate * 100)) + "%");
            }
            break;
        case SYS_Docks:
            addSystemEffect(tr("Drone transfert"), string(int(effectiveness * 100)) + "%");
            break;
        case SYS_Scanner:
            addSystemEffect(tr("Drone range control"), string(my_spaceship->getDronesControlRange() / 1000.0f,1) + "U");
            addSystemEffect(tr("Relay radar range"), string(5000.0 * my_spaceship->getSystemEffectiveness(SYS_Scanner)  / 1000.0f,1) + "U");
            {
                float noise = my_spaceship->scanning_noise + (1.0 - my_spaceship->getSystemEffectiveness(SYS_Scanner));
                if (my_spaceship->getSystemEffectiveness(SYS_Scanner) < 0.1)
                    addSystemEffect(tr("Scanner disabled"), "");
                else if (noise > 0.0)
                    addSystemEffect(tr("Scanning pertubation"), string(int(noise * 100)) + "%");
                else if (noise < 0.0)
                    addSystemEffect(tr("Scanning help"), string(int(-noise * 100)) + "%");
                
            }
            break;
        default:
            break;
        }
        for(unsigned int idx=system_effects_index; idx<system_effects.size(); idx++)
            system_effects[idx]->hide();
    }
}

void GuiSystemEffectsList::addSystemEnergy(ESystem system)
{
    if (!my_spaceship)
        return;

    float energy = 0.0;
    float effectiveness = my_spaceship->getSystemEffectiveness(selected_system);
    energy = effectiveness * my_spaceship->systems[selected_system].power_user_factor * 60.0;
    
    // Specific systems : Reactor
    if (selected_system == SYS_Reactor)
    {
        if (effectiveness > 1.0f)
            effectiveness = (1.0f + effectiveness) / 2.0f;
        energy = effectiveness * my_spaceship->systems[selected_system].power_user_factor * 60.0;
    }
    
    // Specific systems : Shields
    if ((selected_system == SYS_FrontShield || selected_system == SYS_RearShield) && my_spaceship->shields_active)
    {
        if (my_spaceship->hasSystem(SYS_FrontShield) && my_spaceship->hasSystem(SYS_RearShield))
            energy += PlayerSpaceship::energy_shield_use_per_second * 60.0 / 2;
        else
            energy += PlayerSpaceship::energy_shield_use_per_second * 60.0;
    }

    // Specific systems : WARP
    if (selected_system == SYS_Warp && my_spaceship->current_warp > 0.0)
        energy += PlayerSpaceship::energy_warp_per_second * 60.0 * powf(my_spaceship->current_warp, 1.2f) * (my_spaceship->shields_active ? 1.5 : 1.0);

    // Specific systems : Jump
    if (selected_system == SYS_JumpDrive && my_spaceship->jump_drive_charge < my_spaceship->jump_drive_max_distance && my_spaceship->jump_delay == 0.0)
    {
        energy += (60.0 /  my_spaceship->jump_drive_charge_time *  my_spaceship->jump_drive_max_distance) * my_spaceship->getJumpDriveRechargeRate() *  my_spaceship->jump_drive_energy_per_km_charge / 1000.0;
        if (energy < 0.0) energy = 0.0;
    }

     // If energy < 0, then production of energy (Reactor)
     if (energy < 0.0)
        addSystemEffect(tr("Energy production"),  tr("{energy}/min").format({{"energy", string(-energy, 1)}}));
     else
        addSystemEffect(tr("Energy consumption"),  tr("{energy}/min").format({{"energy", string(energy, 1)}}));
}

void GuiSystemEffectsList::selectSystem(ESystem system)
{
    selected_system = system;
}

void GuiSystemEffectsList::setPrimaryInfoVisible(bool visible)
{
    primary_info_visible = visible;
}

void GuiSystemEffectsList::isBackground(bool background)
{
    this->background = background;
}

void GuiSystemEffectsList::addSystemEffect(string key, string value)
{
    if (system_effects_index == system_effects.size())
    {
        GuiKeyValueDisplay* item = new GuiKeyValueDisplay(this, "", 0.7, key, value);
        item->setTextSize(20)->setSize(GuiElement::GuiSizeMax, 35);
        system_effects.push_back(item);
    }else{
        system_effects[system_effects_index]->setKey(key);
        system_effects[system_effects_index]->setValue(value);
        system_effects[system_effects_index]->isBackground(background);
        system_effects[system_effects_index]->show();
    }
    system_effects_index++;
}
