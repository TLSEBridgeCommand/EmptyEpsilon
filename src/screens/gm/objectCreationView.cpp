#include "objectCreationView.h"
#include "GMActions.h"
#include "factionInfo.h"
#include "shipTemplate.h"
#include "gui/gui2_panel.h"
#include "gui/gui2_selector.h"
#include "gameGlobalInfo.h"

#include <regex>

GuiObjectCreationView::GuiObjectCreationView(GuiContainer* owner)
: GuiOverlay(owner, "OBJECT_CREATE_SCREEN", sf::Color(0, 0, 0, 128))
{
    GuiPanel* box = new GuiPanel(this, "FRAME");
    box->setPosition(0, 0, ACenter)->setSize(1000, 500);

    faction_selector = new GuiSelector(box, "FACTION_SELECTOR", nullptr);
    for(P<FactionInfo> info : factionInfo)
        faction_selector->addEntry(info->getLocaleName(), info->getName());
    faction_selector->setSelectionIndex(0);
    faction_selector->setPosition(20, 20, ATopLeft)->setSize(300, 50);

    player_cpu_selector = new GuiSelector(box, "NPC_PC_SELECTOR", [this](int index, string)
    {
        if (index==1)
        {
            cpu_ship_listbox->hide();
            player_ship_listbox->show();    
        }
        else
        {
            cpu_ship_listbox->show();
            player_ship_listbox->hide();
        }
    });
    player_cpu_selector->addEntry("cpu ship","cpu ship");
    player_cpu_selector->addEntry("player ship","player ship");
    player_cpu_selector->setSelectionIndex(0);
    player_cpu_selector->setPosition(20, 70, ATopLeft)->setSize(300, 50);

    float y = 20;
    std::vector<string> template_names = ShipTemplate::getTemplateNameList(ShipTemplate::Station);
    std::sort(template_names.begin(), template_names.end());

    // Listbox
    misc_object_listbox = new GuiListbox(box, "CREATE_MISC_OBJECT", [this](int index, string value)
    {
        string script = misc_object_creation_scripts[index];

        int selected_faction = faction_selector->getSelectionIndex();
        string updated_script = std::regex_replace(script, std::regex("\\$FACTION_ID"), std::to_string(selected_faction));

        setCreateScript(updated_script);
    });

    misc_object_listbox->setTextSize(20)->setButtonHeight(30)->setPosition(-350, y, ATopRight)->setSize(300, 460);
    misc_object_listbox->show();

    // Stations
    for(string template_name : template_names)
    {
        misc_object_listbox->addEntry(template_name, template_name);
        misc_object_creation_scripts.push_back("SpaceStation():setRotation(random(0, 360)):setFactionId($FACTION_ID):setTemplate(\"" + template_name + "\")");
        string script = "SpaceStation():setRotation(random(0, 360)):setFactionId($FACTION_ID):setTemplate(\"" + template_name + "\")";
        y += 30;
    }
    
    // Other objects
    misc_object_listbox->addEntry("ARTIFACT", "Create artifact");
    misc_object_creation_scripts.push_back("Artifact()");
    y += 30;
    
    misc_object_listbox->addEntry("WARP JAMMER", "Create Warp Jammer");
    misc_object_creation_scripts.push_back("WarpJammer():setRotation(random(0, 360)):setFactionId($FACTION_ID)");
    y += 30;

    misc_object_listbox->addEntry("MINE", "Create Mine");
    misc_object_creation_scripts.push_back("Mine():setFactionId($FACTION_ID)");
    y += 30;

    misc_object_listbox->addEntry("INVISIBLE MINE", "Create Invisible Mine");
    misc_object_creation_scripts.push_back("InvisibleMine()");
    y += 30;

    // misc_object_listbox->addEntry("Electronic Explosion", "Create Electronic Explosion Effect");
    // misc_object_creation_scripts.push_back("ElectricExplosionEffect()");
    // y += 30;

    misc_object_listbox->addEntry("SUPPLY DROP", "Supply Drop");
    misc_object_creation_scripts.push_back("SupplyDrop():setFactionId($FACTION_ID):setEnergy(500):setWeaponStorage('Nuke', 1):setWeaponStorage('Homing', 4):setWeaponStorage('Mine', 2):setWeaponStorage('EMP', 1)");
    y += 30;

    misc_object_listbox->addEntry("ASTEROID", "Asteroid");
    misc_object_creation_scripts.push_back("Asteroid()");
    y += 30;

    misc_object_listbox->addEntry("VISUAL ASTEROID", "Visual Asteroid");
    misc_object_creation_scripts.push_back("VisualAsteroid()");
    y += 30;

    misc_object_listbox->addEntry("PLANET", "Planet");
    misc_object_creation_scripts.push_back("Planet()");
    y += 30;

    misc_object_listbox->addEntry("BLACKHOLE", "BlackHole");
    misc_object_creation_scripts.push_back("BlackHole()");
    y += 30;

    misc_object_listbox->addEntry("NEBULA", "Nebula");
    misc_object_creation_scripts.push_back("Nebula()");
    y += 30;

    misc_object_listbox->addEntry("WORM HOLE", "Worm Hole");
    misc_object_creation_scripts.push_back("WormHole()");
    y += 30;

    y = 20;

    template_names = ShipTemplate::getTemplateNameList(ShipTemplate::Ship);
    std::sort(template_names.begin(), template_names.end());
    cpu_ship_listbox = new GuiListbox(box, "CREATE_SHIPS", [this](int index, string value)
    {
        setCreateScript("CpuShip():setRotation(random(0, 360)):setFactionId(" + string(faction_selector->getSelectionIndex()) + "):setTemplate(\"" + value + "\"):orderRoaming()");
    });
    cpu_ship_listbox->setTextSize(20)->setButtonHeight(30)->setPosition(-20, 20, ATopRight)->setSize(300, 460);
    for(string template_name : template_names)
    {
        cpu_ship_listbox->addEntry(template_name, template_name);
    }

    auto player_template_names = ShipTemplate::getTemplateNameList(ShipTemplate::PlayerShip);
    std::sort(player_template_names.begin(), player_template_names.end());
    player_ship_listbox = new GuiListbox(box, "CREATE_PLAYER_SHIPS", [this](int index, string value)
    {
        setCreateScript("PlayerSpaceship():setFactionId(" + string(faction_selector->getSelectionIndex()) + ")",":setTemplate(\"" + value + "\")");
    });
    player_ship_listbox->setTextSize(20)->setButtonHeight(30)->setPosition(-20, 20, ATopRight)->setSize(300, 460);
    for (const auto template_name : player_template_names)
    {
        player_ship_listbox->addEntry(template_name, template_name);
    }
    player_ship_listbox->hide();

    (new GuiButton(box, "CLOSE_BUTTON", tr("button", "Cancel"), [this]() {
        this->hide();
    }))->setPosition(20, -20, ABottomLeft)->setSize(300, 50);
}

void GuiObjectCreationView::onDraw(sf::RenderTarget& target)
{
    if (gameGlobalInfo->allow_new_player_ships)
    {
        player_cpu_selector->show();
    } else {
        player_cpu_selector->hide();
        cpu_ship_listbox->show();
        player_ship_listbox->hide();
    }
}

bool GuiObjectCreationView::onMouseDown(sf::Vector2f position)
{   //Catch clicks.
    return true;
}

void GuiObjectCreationView::setCreateScript(const string create, const string configure)
{
    gameGlobalInfo->on_gm_click = [create, configure] (sf::Vector2f position)
    {
        gameMasterActions->commandRunScript(create + ":setPosition("+string(position.x)+","+string(position.y)+")" + configure);
    };
}
