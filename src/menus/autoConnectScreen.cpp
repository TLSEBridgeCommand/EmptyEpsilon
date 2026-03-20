#include <algorithm>
#include <cmath>
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
#include "gui/gui2_button.h"
#include "../screenComponents/numericEntryPanel.h"

AutoConnectScreen::AutoConnectScreen(ECrewPosition crew_position, int auto_mainscreen, bool control_main_screen, string ship_filter)
: crew_position(crew_position), auto_mainscreen(auto_mainscreen), control_main_screen(control_main_screen), cancel_button(nullptr), cancel_button_y_set(false), update_timer(0.0f), ship_selection_delay(1.0f)
{
    if (!game_client)
    {
        scanner = new ServerScanner(VERSION_NUMBER);
        scanner->scanLocalNetwork();
    }

    new GuiNoiseOverlay(this);

    status_label = new GuiLabel(this, "STATUS", tr("autoconnect", "Searching for server..."), 50);
    status_label->setPosition(0, 300, ATopCenter)->setSize(0, 50);

    // Cancel autoconnect and take the player to ShipSelectionScreen (like ESC after autoconnect succeeds).
    // Vertical position: ~2/3 down the viewport (applied in update() once virtual size is known).
    cancel_button = new GuiButton(this, "AUTOCONNECT_CANCEL_BUTTON", tr("button", "Cancel"), [this]() {
        if (!game_client || game_client->getStatus() == GameClient::Disconnected || !my_player_info)
        {
            disconnectFromServer();
            destroy();
            // Same as successful autoconnect exit: do not show AutoConnect again until restart.
            setAutoconnectSessionDone(true);
            returnToMainMenu(true);
            return;
        }

        setAutoconnectSessionDone(true);
        destroy();
        returnToMainMenu(false);
    });
    cancel_button->setPosition(0, 350, ATopCenter)->setSize(220, 50);

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
        {
            string val = key_value[1].strip();
            ship_filters[key] = val;
            if (key == "server")
                autoconnect_server_names.push_back(val);
        }
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
    if (!cancel_button_y_set && cancel_button && engine)
    {
        P<WindowManager> wm = engine->getObject("windowManager");
        float h = wm ? static_cast<float>(wm->getVirtualSize().y) : 1080.f;
        cancel_button->setPosition(0, h * 2.f / 3.f, ATopCenter);
        cancel_button_y_set = true;
    }

    if (scanner)
    {
        std::vector<ServerScanner::ServerInfo> serverList = scanner->getServerList();
        string autoconnect_address = PreferencesManager::get("autoconnect_address", "");
        bool multi_server_mode = PreferencesManager::get("multi_server_mode", "0").toInt() > 0;
        float try_server_seconds = PreferencesManager::get("autoconnect_server_try_seconds", "10").toFloat();
        if (try_server_seconds < 1.f)
            try_server_seconds = 10.f;

        if (autoconnect_address != "") {
            status_label->setText(tr("autoconnect", "Using autoconnect server {address}").format({{"address", autoconnect_address}}));
            connect_to_address = autoconnect_address;
            if (game_client)
                disconnectFromServer();
            new GameClient(VERSION_NUMBER, autoconnect_address);
            scanner->destroy();
        } else {
            const bool multi_name_fallback = multi_server_mode && autoconnect_server_names.size() > 1;
            if (multi_name_fallback)
            {
                multi_server_try_elapsed += delta;
                if (multi_server_try_elapsed >= try_server_seconds)
                {
                    multi_server_try_elapsed = 0.f;
                    multi_server_name_index = (multi_server_name_index + 1) % autoconnect_server_names.size();
                }
            }

            if (serverList.size() > 0) {
                if (multi_server_mode) {
                    string target;
                    bool have_target = false;
                    if (!autoconnect_server_names.empty())
                    {
                        target = autoconnect_server_names[multi_server_name_index];
                        have_target = true;
                    }
                    else if (ship_filters.find("server") != ship_filters.end())
                    {
                        target = ship_filters["server"];
                        have_target = true;
                    }

                    if (have_target)
                    {
                        bool found_server = false;
                        for (const auto& server : serverList)
                        {
                            if (server.name == target)
                            {
                                status_label->setText(tr("autoconnect", "Found server {name}").format({{"name", server.name}}));
                                connect_to_address = server.address;
                                if (game_client)
                                    disconnectFromServer();
                                new GameClient(VERSION_NUMBER, server.address);
                                scanner->destroy();
                                found_server = true;
                                break;
                            }
                        }
                        if (!found_server)
                        {
                            if (multi_name_fallback)
                            {
                                int secs_left = std::max(0, int(std::ceil(try_server_seconds - multi_server_try_elapsed)));
                                status_label->setText(tr("autoconnect", "Looking for server \"{name}\". Next option in {seconds} s.").format({{"name", target}, {"seconds", string(secs_left)}}));
                            }
                            else
                                status_label->setText(tr("autoconnect", "Searching for matching server..."));
                        }
                    }
                    else
                        status_label->setText(tr("autoconnect", "Searching for matching server..."));
                } else {
                    // In single-server mode, just connect to the first available server
                    status_label->setText(tr("autoconnect", "Found server {name}").format({{"name", serverList[0].name}}));
                    connect_to_address = serverList[0].address;
                    if (game_client)
                        disconnectFromServer();
                    new GameClient(VERSION_NUMBER, serverList[0].address);
                    scanner->destroy();
                }
            } else {
                if (multi_server_mode && !autoconnect_server_names.empty())
                {
                    string target = autoconnect_server_names[multi_server_name_index];
                    if (multi_name_fallback)
                    {
                        int secs_left = std::max(0, int(std::ceil(try_server_seconds - multi_server_try_elapsed)));
                        status_label->setText(tr("autoconnect", "No servers on LAN. Looking for \"{name}\" — next option in {seconds} s.").format({{"name", target}, {"seconds", string(secs_left)}}));
                    }
                    else
                        status_label->setText(tr("autoconnect", "Searching for server..."));
                }
                else
                    status_label->setText(tr("autoconnect", "Searching for server..."));
            }
        }
    }else{
        // Connected: check scenario/ship and optionally spawn UI, then this screen is destroyed
        if (!game_client)
        {
            disconnectFromServer();
            returnToMainMenu(true);
            return;
        }
        switch(game_client->getStatus())
        {
        case GameClient::ReadyToConnect:
        case GameClient::Connecting:
        case GameClient::Authenticating:
            // With a busy server (heavy scenario Lua), the client can stay here until the server
            // sends auth/replication; avoid assuming a timeout — show "Connecting..." until then.
            status_label->setText(tr("autoconnect", "Connecting: {address}").format({{"address", connect_to_address.toString()}}));
            break;
        case GameClient::WaitingForPassword:
            status_label->setText(tr("autoconnect", "Server requires password. Please use manual connection."));
            disconnectFromServer();
            returnToMainMenu(true);
            break;
        case GameClient::Disconnected:
        {
            string autoconnect_address = PreferencesManager::get("autoconnect_address", "");
            // If an explicit autoconnect_address is configured, keep waiting and retrying instead of
            // returning to the main menu when the server is not up yet.
            if (autoconnect_address != "")
            {
                disconnectFromServer();
                // Restart scanner so the update() loop goes back into the "scanner" branch,
                // which will attempt to connect to the configured address again.
                if (!scanner)
                {
                    scanner = new ServerScanner(VERSION_NUMBER);
                    scanner->scanLocalNetwork();
                }
                status_label->setText(tr("autoconnect", "Waiting for server {address}...").format({{"address", autoconnect_address}}));
            }
            else
            {
                disconnectFromServer();
                returnToMainMenu(true);
            }
            break;
        }
        case GameClient::Connected:
            if (game_client->getClientId() > 0)
            {
                my_player_info = nullptr;
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

                    if (!my_spaceship)
                    {
                        // When connecting to an already-running server, GameGlobalInfo::playerShipId may replicate
                        // before the actual PlayerSpaceship objects (and their templates). Wait until at least one
                        // real (non-drone) ship resolves with a template before attempting to pick a ship.
                        // Requiring 2 ships can deadlock on servers with a single playable ship.
                        bool any_ready_ship = false;
                        for (int n = 0; n < GameGlobalInfo::max_player_ships; n++)
                        {
                            P<PlayerSpaceship> ship = gameGlobalInfo->getPlayerShip(n);
                            if (ship && ship->ship_template && ship->ship_template->getType() != ShipTemplate::TemplateType::Drone)
                            {
                                any_ready_ship = true;
                                break;
                            }
                        }
                        if (!any_ready_ship)
                        {
                            status_label->setText(tr("autoconnect", "Scenario started. Waiting for ship list..."));
                            return;
                        }

                        status_label->setText(tr("autoconnect", "Looking for available ships..."));
                        int preferred_index = PreferencesManager::get("autoconnect_ship_index", "1").toInt();
                        preferred_index = preferred_index - 1;
                        if (preferred_index < 0) preferred_index = 0;
                        if (preferred_index >= GameGlobalInfo::max_player_ships) preferred_index = GameGlobalInfo::max_player_ships - 1;

                        bool connected_to_ship = false;
                        if (isValidShip(preferred_index))
                        {
                            connectToShip(preferred_index);
                            connected_to_ship = true;
                        }
                        else
                        {
                            for(int n=0; n<GameGlobalInfo::max_player_ships; n++)
                            {
                                if (n != preferred_index && isValidShip(n))
                                {
                                    connectToShip(n);
                                    connected_to_ship = true;
                                    break;
                                }
                            }
                        }

                        // If we found ships but none were valid (already occupied, drone-only, etc.),
                        // don't get stuck on this screen. Drop the user into ship selection.
                        if (!connected_to_ship)
                        {
                            status_label->setText(tr("autoconnect", "No available ships. Opening ship selection..."));
                            setAutoconnectSessionDone(true);
                            destroy();
                            returnToMainMenu(false);
                            return;
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

    // Already on this ship (replication edge case)
    if (my_player_info->ship_id == ship->getMultiplayerId())
        return false;

    // Stations are non-exclusive for autoconnect (multiple clients may use the same role, e.g. LRR).
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
            setAutoconnectSessionDone(true);
            returnToMainMenu(true);
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

    // Match ship-selection toggles so spawnUI() opens MainScreen vs crew tabs correctly.
    my_player_info->commandSetMainScreen(auto_mainscreen == 1);
    my_player_info->commandSetMainScreenControl(control_main_screen);

    if (auto_mainscreen != 1 && crew_position != max_crew_positions)
        my_player_info->commandSetCrewPosition(crew_position, true);

    string autostationslist = PreferencesManager::get("autostationslist", "");
    if (autostationslist != "")
    {
        // Same 1-based numbering as autoconnect: 1=Helms, 2=Weapons, 3=Engineering, etc. Comma-separated.
        std::vector<string> stations = autostationslist.split(",");
        for(string station : stations)
        {
            int one_based = station.toInt();
            if (one_based >= 1 && one_based <= int(max_crew_positions))
                my_player_info->commandSetCrewPosition(ECrewPosition(one_based - 1), true);
        }
    }

    destroy();
    // Mark autoconnect as done for this session so ESC in scenario goes to ship selection; disconnect goes to main menu.
    setAutoconnectSessionDone(true);
    // Defer UI spawn by one frame so the main screen 3D viewport gets valid layout/state (fixes black screen on first load).
    my_player_info->ui_spawn_pending = true;
    my_player_info->ui_spawn_delay_frames = 1;
}
