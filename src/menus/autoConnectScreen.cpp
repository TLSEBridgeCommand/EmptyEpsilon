#include <i18n.h>
#include "main.h"
#include "autoConnectScreen.h"
#include "preferenceManager.h"
#include "screenComponents/noiseOverlay.h"
#include "epsilonServer.h"
#include "gameGlobalInfo.h"
#include "playerInfo.h"

#include "gui/gui2_label.h"
#include "gui/gui2_overlay.h"
#include "gui/gui2_panel.h"
#include "gui/gui2_textentry.h"
#include "gui/gui2_togglebutton.h"
#include "../screenComponents/numericEntryPanel.h"

AutoConnectScreen::AutoConnectScreen(ECrewPosition crew_position, int auto_mainscreen, bool control_main_screen, string ship_filter)
: crew_position(crew_position), auto_mainscreen(auto_mainscreen) , control_main_screen(control_main_screen), update_timer(0.0f)
{
    if (!game_client)
    {
        scanner = new ServerScanner(VERSION_NUMBER);
        scanner->scanLocalNetwork();
    }

    new GuiNoiseOverlay(this);

    status_label = new GuiLabel(this, "STATUS", tr("autoconnect", "Searching for server..."), 50);
    status_label->setPosition(0, 300, ATopCenter)->setSize(0, 50);

    string position_name = tr("autoconnect", "Main screen");
    if (crew_position < max_crew_positions)
        position_name = getCrewPositionName(crew_position);
    if (auto_mainscreen == 1)
        position_name = tr("autoconnect", "Main screen");

    (new GuiLabel(this, "POSITION", position_name, 50))->setPosition(0, 400, ATopCenter)->setSize(0, 30);
    
    filter_label = new GuiLabel(this, "FILTER", "", 20);
    filter_label->setPosition(0, 30, ATopCenter)->setSize(0, 10);

    for(string filter : ship_filter.split(";"))
    {
        std::vector<string> key_value = filter.split("=", 1);
        string key = key_value[0].strip().lower();
        if (key.length() < 1)
            continue;

        if (key_value.size() == 1)
            ship_filters[key] = "1";
        else if (key_value.size() == 2)
            ship_filters[key] = key_value[1].strip();
        filter_label->setText(filter_label->getText() + key + " : " + ship_filters[key] + " ");
    }

    if (PreferencesManager::get("instance_name") != "")
    {
        (new GuiLabel(this, "", PreferencesManager::get("instance_name"), 25))->setAlignment(ACenterLeft)->setPosition(20, 20, ATopLeft)->setSize(0, 18);
    }
}

bool AutoConnectScreen::is_integer(const std::string& string)
{
    return !string.empty() && std::find_if(string.begin(), string.end(), [](char c) { return !std::isdigit(c); }) == string.end();
};

AutoConnectScreen::~AutoConnectScreen()
{
    if (scanner)
        scanner->destroy();
}

void AutoConnectScreen::update(float delta)
{
    if (scanner)
    {
        std::vector<ServerScanner::ServerInfo> serverList = scanner->getServerList();
        string autoconnect_address = PreferencesManager::get("autoconnect_address", "");
        bool multi_server_mode = PreferencesManager::get("multi_server_mode", "0").toInt() > 0;

        if (autoconnect_address != "") {
            status_label->setText(tr("autoconnect", "Using autoconnect server {address}").format({{"address", autoconnect_address}}));
            connect_to_address = autoconnect_address;
            new GameClient(VERSION_NUMBER, autoconnect_address);
            scanner->destroy();
        } else if (serverList.size() > 0) {
            // In multi-server mode, we need to find the correct server based on ship filters
            if (multi_server_mode) {
                bool found_server = false;
                for (const auto& server : serverList) {
                    // Check if this server matches our ship filters
                    if (ship_filters.find("server") != ship_filters.end()) {
                        if (server.name == ship_filters["server"]) {
                            status_label->setText(tr("autoconnect", "Found server {name}").format({{"name", server.name}}));
                            connect_to_address = server.address;
                            new GameClient(VERSION_NUMBER, server.address);
                            scanner->destroy();
                            found_server = true;
                            break;
                        }
                    }
                }
                if (!found_server) {
                    status_label->setText(tr("autoconnect", "Searching for matching server..."));
                }
            } else {
                // In single-server mode, just connect to the first available server
                status_label->setText(tr("autoconnect", "Found server {name}").format({{"name", serverList[0].name}}));
                connect_to_address = serverList[0].address;
                new GameClient(VERSION_NUMBER, serverList[0].address);
                scanner->destroy();
            }
        } else {
            status_label->setText(tr("autoconnect", "Searching for server..."));
        }
    }else{
        // Connected: check scenario/ship and optionally spawn UI, then this screen is destroyed
        if (!game_client)
        {
            disconnectFromServer();
            returnToMainMenu();
            return;
        }
        switch(game_client->getStatus())
        {
        case GameClient::ReadyToConnect:
        case GameClient::Connecting:
        case GameClient::Authenticating:
            status_label->setText(tr("autoconnect", "Connecting: {address}").format({{"address", connect_to_address.toString()}}));
            break;
        case GameClient::WaitingForPassword:
            status_label->setText(tr("autoconnect", "Server requires password. Please use manual connection."));
            disconnectFromServer();
            returnToMainMenu();
            break;
        case GameClient::Disconnected:
            disconnectFromServer();
            returnToMainMenu();
            break;
        case GameClient::Connected:
            if (game_client->getClientId() > 0)
            {
                foreach(PlayerInfo, i, player_info_list)
                    if (i->client_id == game_client->getClientId())
                        my_player_info = i;
                if (my_player_info && gameGlobalInfo)
                {
                    // Check if scenario has started
                    if (!gameGlobalInfo->scenario_started) {
                        status_label->setText(tr("autoconnect", "Connected to server. Waiting for scenario to start..."));
                        return;
                    }

                    status_label->setText(tr("autoconnect", "Scenario started. Looking for available ships..."));
                    if (!my_spaceship)
                    {
                        int preferred_index = PreferencesManager::get("autoconnect_ship_index", "1").toInt();
                        preferred_index = preferred_index - 1;
                        if (preferred_index < 0) preferred_index = 0;
                        if (preferred_index >= GameGlobalInfo::max_player_ships) preferred_index = GameGlobalInfo::max_player_ships - 1;

                        if (isValidShip(preferred_index))
                        {
                            connectToShip(preferred_index);
                        }
                        else
                        {
                            for(int n=0; n<GameGlobalInfo::max_player_ships; n++)
                            {
                                if (n != preferred_index && isValidShip(n))
                                {
                                    connectToShip(n);
                                    break;
                                }
                            }
                        }
                    } else {
                        if (my_spaceship->getMultiplayerId() == my_player_info->ship_id && (auto_mainscreen == 1 || crew_position == max_crew_positions || my_player_info->crew_position[crew_position]))
                        {
                            if (auto_mainscreen == 1)
                            {
                                for(int n=0; n<max_crew_positions; n++)
                                    my_player_info->commandSetCrewPosition(crew_position, false);
                            }

                            if(!waiting_for_password) {
                                status_label->hide();
                                int ship_index = gameGlobalInfo->findPlayerShip(my_spaceship);
                                if (ship_index >= 0) {
                                    my_player_info->ui_spawn_pending = true;
                                    connectToShip(ship_index);
                                }
                            }
                        }
                    }
                } else {
                    status_label->setText(tr("autoconnect", "Connected, waiting for game data..."));
                }
            }
            break;
        }
    }
}

bool AutoConnectScreen::isValidShip(int index)
{
    P<PlayerSpaceship> ship = gameGlobalInfo->getPlayerShip(index);
    if (!ship || !ship->ship_template || ship->ship_template->getType() == ShipTemplate::TemplateType::Drone)
        return false;

    // Check if this ship is already crewed by this player
    if (my_player_info->ship_id == ship->getMultiplayerId())
        return false;

    // Check if this ship is already crewed by another player
    foreach(PlayerInfo, i, player_info_list)
    {
        if (i->ship_id == ship->getMultiplayerId())
            return false;
    }

    return true;
}

void AutoConnectScreen::connectToShip(int index)
{
    P<PlayerSpaceship> ship = gameGlobalInfo->getPlayerShip(index);
    if (!ship) return;

    my_player_info->commandSetShipId(ship->getMultiplayerId());
    
    if (ship->control_code.length() > 0 && PreferencesManager::get("autoconnect_control_code_bypass", "0") != "1")
    {
        if (control_code_numeric_panel) { control_code_numeric_panel->destroy(); control_code_numeric_panel = nullptr; }
        
        waiting_for_password = true;
        
        control_code_numeric_panel = new GuiControlNumericEntryPanel(this, "CODE_ENTRY", tr("autoconnect", "Enter this ship's control code"));
        control_code_numeric_panel->setPosition(0, 0, ACenter);
        control_code_numeric_panel->enterCallback([this, ship](int value) {
            if (ship->control_code.toInt() == value) {
                waiting_for_password = false;
                autoConnectPasswordEntryOnOkClick(ship);
            } else {
                control_code_numeric_panel->setPrompt(tr("autoconnect", "Incorrect Control Code"));
                control_code_numeric_panel->clearCode();
            }
        });
        control_code_numeric_panel->clearCallback([this](int) {
            waiting_for_password = false;
            // Don't destroy the panel - we're inside its callback. Just clear ref and destroy parent.
            control_code_numeric_panel = nullptr;
            destroy();
            returnToMainMenu();
        });
    } else {
        autoConnectPasswordEntryOnOkClick(ship);
    }
}

void AutoConnectScreen::autoConnectPasswordEntryOnOkClick(P<PlayerSpaceship> ship_param)
{
    P<PlayerSpaceship> ship = ship_param ? ship_param : my_spaceship;
    if (!ship)
    {
        destroy();
        return;
    }

    // Don't destroy the panel here - we're likely inside its callback. Destroying the
    // parent screen will clean up the panel; explicitly destroying it during its
    // own callback causes a crash (Access Violation in GuiElement::destroy).
    control_code_numeric_panel = nullptr;

    my_player_info->commandSetShipId(ship->getMultiplayerId());

    for(int n = 0; n < max_crew_positions; n++)
        my_player_info->commandSetCrewPosition(ECrewPosition(n), false);

    if (auto_mainscreen != 1 && crew_position != max_crew_positions)
        my_player_info->commandSetCrewPosition(crew_position, true);

    string autostationslist = PreferencesManager::get("autostationslist", "");
    if (autostationslist != "")
    {
        std::vector<string> stations = autostationslist.split(",");
        for(string station : stations)
        {
            int station_id = station.toInt();
            if (station_id >= 0 && station_id < max_crew_positions)
                my_player_info->commandSetCrewPosition(ECrewPosition(station_id), true);
        }
    }

    destroy();
    // Mark autoconnect as done for this session so ESC/return goes to main menu, not AutoConnectScreen.
    PreferencesManager::set("autoconnect_session_done", "1");
    // Defer UI spawn to GameGlobalInfo::update() so it runs when my_spaceship is
    // resolved (same or next frame). Fixes station UI not appearing when autoconnect
    // connects before unpause or with replication delay.
    my_player_info->ui_spawn_pending = true;
}
