#include "playerInfo.h"
#include "gameGlobalInfo.h"
#include "instabilityScreen.h"

#include "screenComponents/alertOverlay.h"
#include "screenComponents/customShipFunctions.h"
#include "screenComponents/systemEffectsList.h"

#include "gui/gui2_keyvaluedisplay.h"
#include "gui/gui2_autolayout.h"
#include "gui/gui2_togglebutton.h"
#include "gui/gui2_slider.h"
#include "gui/gui2_progressbar.h"
#include "gui/gui2_progressslider.h"
#include "gui/gui2_image.h"
#include "gui/gui2_arrow.h"
#include "gui/gui2_panel.h"
#include "gui/gui2_selector.h"
#include "gui/gui2_element.h"

InstabilityControlScreen::InstabilityControlScreen(GuiContainer* owner, ECrewPosition crew_position)
: GuiOverlay(owner, "SYSTEM_SCREEN", colorConfig.background), selected_system(SYS_None)
{
    // Render the background decorations.
    background_crosses = new GuiOverlay(this, "BACKGROUND_CROSSES", sf::Color::White);
    background_crosses->setTextureTiled("gui/BackgroundCrosses");

    // Render the alert level color overlay.
    (new AlertLevelOverlay(this));

    GuiAutoLayout* system_layout = new GuiAutoLayout(this, "LEFT_LAYOUT", GuiAutoLayout::LayoutVerticalTopToBottom);
    system_layout->setPosition(25, 25, ATopLeft)->setSize(300, GuiElement::GuiSizeMax);

    for(int n=0; n<SYS_COUNT; n++)
    {
        ESystem system = ESystem(n);
        
        system_selector[n] = new GuiToggleButton(system_layout, "", getLocaleSystemName(system), [this, n](bool value) {
            selectSystem(ESystem(n));
        });
        system_selector[n]->setSize(GuiElement::GuiSizeMax, 40);        
    }
    
    system_effects_container = new GuiSystemEffectsList(this,"",GuiAutoLayout::LayoutVerticalBottomToTop);
    system_effects_container->setPosition(20, -520, ABottomLeft)->setSize(270, 400);
    system_effects_container->selectSystem(selected_system);
    
    system_info = new GuiAutoLayout(this, "SYSTEM_INFO", GuiAutoLayout::LayoutHorizontalLeftToRight);
    system_info->setPosition(0, 0, ABottomLeft);
    system_info->setSize(500, 500);
    
    damage_title = new GuiLabel(system_info, "DAMAGE_TITLE", tr("slider", "Damage"), 30);
    damage_title->setVertical()->setAlignment(ACenterLeft)->setSize(30, 400);
    damage_bar = new GuiProgressbar(system_info, "SYSTEM_DAMAGE", 0.0, 1.0, 0.0);
    damage_bar->setSize(80, GuiElement::GuiSizeMax);
    damage_icon = new GuiImage(damage_bar, "", "gui/icons/system_health");
    damage_icon->setColor(colorConfig.log_generic)->setPosition(0, 50, ACenter)->setSize(GuiElement::GuiSizeMax, GuiElement::GuiSizeMatchWidth);
    damage_label = new GuiLabel(damage_bar, "SYSTEM_DAMAGE_LABEL", "...", 30);
    damage_label->setSize(GuiElement::GuiSizeMax, GuiElement::GuiSizeMax);
    heat_title = new GuiLabel(system_info, "HEAT_TITLE", tr("slider", "Heat"), 30);
    heat_title->setVertical()->setAlignment(ACenterLeft)->setSize(30, 400);
    heat_bar = new GuiProgressbar(system_info, "SYSTEM_HEAT", 0.0, 1.0, 0.0);
    heat_bar->setSize(80, GuiElement::GuiSizeMax);
    heat_icon = new GuiImage(heat_bar, "", "gui/icons/status_overheat");
    heat_icon->setColor(colorConfig.log_generic)->setPosition(0, 50, ACenter)->setSize(GuiElement::GuiSizeMax, GuiElement::GuiSizeMatchWidth);
    heat_label = new GuiLabel(heat_bar, "SYSTEM_HEAT_LABEL", "...", 30);
    heat_label->setSize(GuiElement::GuiSizeMax, GuiElement::GuiSizeMax);
    coolant_title = new GuiLabel(system_info, "COOLANT_TITLE", tr("slider", "Coolant"), 30);
    coolant_title->setVertical()->setAlignment(ACenterLeft)->setSize(30, 400);
    coolant_bar = new GuiProgressbar(system_info, "SYSTEM_COOLANT", 0.0, 10.0, 0.0);
    coolant_bar->setSize(80, GuiElement::GuiSizeMax);
    coolant_icon = new GuiImage(coolant_bar, "", "gui/icons/coolant");
    coolant_icon->setColor(colorConfig.log_generic)->setPosition(0, 50, ACenter)->setSize(GuiElement::GuiSizeMax, GuiElement::GuiSizeMatchWidth);
    coolant_label = new GuiLabel(coolant_bar, "SYSTEM_COOLANT_LABEL", "...", 30);
    coolant_label->setSize(GuiElement::GuiSizeMax, GuiElement::GuiSizeMax);
    if (gameGlobalInfo->use_nano_repair_crew)
    {
        repair_title = new GuiLabel(system_info, "REPAIR_TITLE", tr("slider", "Repair"), 30);
        repair_title->setVertical()->setAlignment(ACenterLeft)->setSize(30, 400);
        repair_bar = new GuiProgressbar(system_info, "SYSTEM_REPAIR", 0.0, 10.0, 0.0);
        repair_bar->setSize(80, GuiElement::GuiSizeMax);
        repair_icon = new GuiImage(repair_bar, "", "gui/icons/system_health");
        repair_icon->setColor(colorConfig.log_generic)->setPosition(0, 50, ACenter)->setSize(GuiElement::GuiSizeMax, GuiElement::GuiSizeMatchWidth);
        repair_label = new GuiLabel(repair_bar, "SYSTEM_REPAIR_LABEL", "...", 30);
        repair_label->setSize(GuiElement::GuiSizeMax, GuiElement::GuiSizeMax);    
    }
    power_title = new GuiLabel(system_info, "POWER_TITLE", tr("slider", "Power"), 30);
    power_title->setVertical()->setAlignment(ACenterLeft)->setSize(30, 400);
    power_bar = new GuiProgressbar(system_info, id + "SYSTEM_POWER", 0.0, 3.0, 1.0);
    power_bar->setSize(80, GuiElement::GuiSizeMax);
    power_icon = new GuiImage(power_bar, "", "gui/icons/energy");
    power_icon->setColor(colorConfig.log_generic)->setPosition(0, 50, ACenter)->setSize(GuiElement::GuiSizeMax, GuiElement::GuiSizeMatchWidth);
    power_label = new GuiLabel(power_bar, "SYSTEM_POWER_LABEL", "...", 30);
    power_label->setSize(GuiElement::GuiSizeMax, GuiElement::GuiSizeMax);
    effectiveness_title = new GuiLabel(system_info, "EFFECTIVENESS_TITLE", tr("slider", "Effectiveness"), 30);
    effectiveness_title->setVertical()->setAlignment(ACenterLeft)->setSize(30, 400);
    effectiveness_bar = new GuiProgressbar(system_info, "SYSTEM_EFFECTIVENESS", 0.0, 3.0, 0.0);
    effectiveness_bar->setSize(80, GuiElement::GuiSizeMax);
    effectiveness_icon = new GuiImage(effectiveness_bar, "", "gui/icons/system_reactor");
    effectiveness_icon->setColor(colorConfig.log_generic)->setPosition(0, 50, ACenter)->setSize(GuiElement::GuiSizeMax, GuiElement::GuiSizeMatchWidth);
    effectiveness_label = new GuiLabel(effectiveness_bar, "SYSTEM_EFFECTIVENESS_LABEL", "...", 30);
    effectiveness_label->setSize(GuiElement::GuiSizeMax, GuiElement::GuiSizeMax);
    
    // Instability Info and Slider
    instability_container = new GuiElement(this, "");
    instability_container->setPosition(-50, 0, ACenterRight)->setSize(500, 500);
    
    instability_info = new GuiAutoLayout(instability_container, "SYSTEM_INFO", GuiAutoLayout::LayoutHorizontalRows);
    instability_info->setSize(350, 250);
    instability_info->setPosition(0, 0, ACenter);
    
    instability_sum_label = new GuiLabel(instability_info, "INSTABILITY_SUM_LABEL", tr("Instability Level : "), 30);
   
    for(int n=0; n<4; n++)
    {
        instability_label[n] = new GuiLabel(instability_info, "INSTABILITY_LABEL", "", 30);
        instability_slider[n] = new GuiSlider(instability_info, "", -1.0, 1.0, 0.0, [this, n](float value) {
            if (my_spaceship) my_spaceship->commandSetSystemInstability(selected_system, n+1, value);
        });
        instability_slider[n]->setSize(340, 25);
    }

    // Select the first system
    selectSystem(SYS_None);
    for(int n=0; n<SYS_COUNT; n++)
    {        
        if (my_spaceship && my_spaceship->hasSystem(ESystem(n)) && my_spaceship->systems[n].instability_factor > 0.0 && selected_system == SYS_None)
        {
            selectSystem(ESystem(n));
            system_selector[n]->setValue(true);
        }
    }
    
    (new GuiCustomShipFunctions(this, crew_position, "", my_spaceship))->setPosition(-20, 120, ATopRight)->setSize(250, GuiElement::GuiSizeMax);
}

void InstabilityControlScreen::onDraw(sf::RenderTarget& window)
{
    GuiOverlay::onDraw(window);

    if (my_spaceship)
    {
        // System selector update
        for(int n=0; n<SYS_COUNT; n++)
        {
            system_selector[n]->setVisible(my_spaceship->hasSystem(ESystem(n)) && my_spaceship->systems[n].instability_factor > 0.0);
            system_selector[n]->setValue(ESystem(n) == selected_system);
        }
    
        if (selected_system != SYS_None)
        {
            ShipSystem system = my_spaceship->systems[selected_system];

            for(int n=0; n<4; n++)
            {
                instability_label[n]->setVisible(n < system.instability_difficulty);
                instability_label[n]->setText(system.instability_label[n]);
                
                instability_slider[n]->setVisible(n < system.instability_difficulty);
                instability_slider[n]->setValue(my_spaceship->getSystemInstabilityValue(selected_system,n+1));
            }

            float health = system.health;
            if (health < 0.0)
                damage_bar->setValue(-health)->setColor(sf::Color(128, 32, 32, 192));
            else
                damage_bar->setValue(health)->setColor(sf::Color(64, 128 * health, 64 * health, 192));
            damage_label->setText(string(systemHealthToPercent(health)) + "%");

            float heat = system.heat_level;
            heat_bar->setValue(heat)->setColor(sf::Color(128, 32 + 96 * (1.0 - heat), 32, 192));
            heat_label->setText(string(int(heat * 100)) + "%");

            float power = system.power_level;
            power_bar->setValue(power)->setColor(sf::Color(128, 128, 0, 192));
            power_label->setText(string(int(power * 100)) + "%");
            
            float coolant = system.coolant_level;
            coolant_bar->setValue(coolant)->setColor(sf::Color(0, 128, 128, 192));
            coolant_label->setText(string(int(coolant * 10)) + "%");
            
            if (gameGlobalInfo->use_nano_repair_crew)
            {
                float repair = system.repair_level;
                repair_bar->setValue(repair)->setColor(sf::Color(64, 128, 64, 192));
                repair_label->setText(string(int(coolant * 10)) + "%");
            }
            
            float effectiveness = my_spaceship->getSystemEffectiveness(selected_system);
            effectiveness_bar->setValue(effectiveness)->setColor(sf::Color(64, 128 * health, 64 * health, 192));
            effectiveness_label->setText(string(int(effectiveness * 100)) + "%");
            
            updateInstability(window, instability_container);
            instability_sum_label->setText(tr("Instability Level : ")+ string(int(system.instability_level * 100)) + "%");
        }
    }
}

void InstabilityControlScreen::updateInstability(sf::RenderTarget& window, GuiElement* container)
{
    if (!my_spaceship)
        return;

    // Cap the number of signature points, which determines the raw data's
    // resolution.
    sf::FloatRect rect = container->getRect();
    const int point_count = 512;
    float width = rect.width;
    float height = rect.height;
    float left = rect.left;
    float top = rect.top;
    
    float radius = std::min(width, height) / 2.0f;
    
    // Create a vertex array containing each data point.
    sf::VertexArray a_r(sf::LinesStrip, point_count+1);

    // For each data point ...
    for(int n = 0; n < point_count; n++)
    {
        // ... make some noise ...
        float r = random(-1.0, 1.0);
        float instability_level = 0.0;
        if (my_spaceship)
            instability_level= my_spaceship->systems[selected_system].instability_level;
            
        r *= instability_level * 30;

        // ... and add vectors for each point.
        a_r[n].position.x = left + width / 2.0f;
        a_r[n].position.y = top + height / 2.0f;
        a_r[n].position += sf::vector2FromAngle(float(n) / float(point_count) * 360.0f ) * (radius * (0.95f - r / 300));
        a_r[n].color = sf::Color(255, 0, 0, 255*(instability_level));
    }

    // Set a zero value at the "end" of the data point array.
    a_r[point_count] = a_r[0];
    
    // Draw each band as a line.
    window.draw(a_r, sf::BlendAdd);
}

void InstabilityControlScreen::selectSystem(ESystem system)
{
    if (!my_spaceship)
        return;
    if (!my_spaceship->hasSystem(system))
        return;
    
    selected_system = system;
    system_effects_container->selectSystem(system);
}

void InstabilityControlScreen::onHotkey(const HotkeyResult& key)
{
    if (key.category == "INSTABILITY" && my_spaceship)
    {
        if (key.hotkey == "SELECT_REACTOR") selectSystem(SYS_Reactor);
        if (key.hotkey == "SELECT_BEAM_WEAPONS") selectSystem(SYS_BeamWeapons);
        if (key.hotkey == "SELECT_MISSILE_SYSTEM") selectSystem(SYS_MissileSystem);
        if (key.hotkey == "SELECT_MANEUVER") selectSystem(SYS_Maneuver);
        if (key.hotkey == "SELECT_IMPULSE") selectSystem(SYS_Impulse);
        if (key.hotkey == "SELECT_WARP") selectSystem(SYS_Warp);
        if (key.hotkey == "SELECT_JUMP_DRIVE") selectSystem(SYS_JumpDrive);
        if (key.hotkey == "SELECT_FRONT_SHIELDS") selectSystem(SYS_FrontShield);
        if (key.hotkey == "SELECT_REAR_SHIELDS") selectSystem(SYS_RearShield);

        // Don't act if the selected system doesn't exist.
        if (!my_spaceship->hasSystem(selected_system))
            return;

        ShipSystem& system = my_spaceship->systems[selected_system];

        for(int n=1; n<5; n++)
        {
            if (system.instability_difficulty >= n && key.hotkey == "INCREASE_SLIDER_" + string(n))
                my_spaceship->commandSetSystemInstability(selected_system, n, instability_slider[n-1]->getValue() + 0.05);
            if (system.instability_difficulty >= n && key.hotkey == "DECREASE_SLIDER_" + string(n))
                my_spaceship->commandSetSystemInstability(selected_system, n, instability_slider[n-1]->getValue() - 0.05);
        }  
    }
}

