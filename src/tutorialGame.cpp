#include <i18n.h>
#include "tutorialGame.h"
#include "gameGlobalInfo.h"
#include "menus/tutorialMenu.h"
#include "scriptInterface.h"
#include "playerInfo.h"
#include "spaceObjects/playerSpaceship.h"
#include "preferenceManager.h"
#include "main.h"
#include "input.h"

#include "screenComponents/viewport3d.h"
#include "screenComponents/radarView.h"

#include "screens/crew6/helmsScreen.h"
#include "screens/crew6/weaponsScreen.h"
#include "screens/crew6/engineeringScreen.h"
#include "screens/crew6/scienceScreen.h"
#include "screens/crew6/relayScreen.h"
#include "screens/crew4/tacticalScreen.h"
#include "screens/crew4/operationsScreen.h"
// new screens for BC tutorials
#include "screens/extra/tractorBeamScreen.h"
#include "screens/extra/targetAnalysisScreen.h"
#include "screens/extra/damcon.h"
#include "screens/extra/powerManagement.h"
#include "screens/extra/dockMasterScreen.h"
#include "screens/extra/droneOperatorScreen.h"

#include "screenComponents/indicatorOverlays.h"

#include "gui/gui2_panel.h"
#include "gui/gui2_scrolltext.h"
#include "gui/gui2_button.h"
#include "gui/gui2_overlay.h"
#include "gui/gui2_label.h"
#include "gui/gui2_autolayout.h"

///The TutorialGame object is normally never created.
/// And it only used to setup the special tutorial level.
/// It contains functions to assist in explaining the game, but do not work outside of the tutorial.
REGISTER_SCRIPT_CLASS_NO_CREATE(TutorialGame)
{
    REGISTER_SCRIPT_CLASS_FUNCTION(TutorialGame, setPlayerShip);
    REGISTER_SCRIPT_CLASS_FUNCTION(TutorialGame, switchViewToMainScreen);
    REGISTER_SCRIPT_CLASS_FUNCTION(TutorialGame, switchViewToTactical);
    REGISTER_SCRIPT_CLASS_FUNCTION(TutorialGame, switchViewToLongRange);
    REGISTER_SCRIPT_CLASS_FUNCTION(TutorialGame, switchViewToScreen);
    REGISTER_SCRIPT_CLASS_FUNCTION(TutorialGame, showMessage);
    REGISTER_SCRIPT_CLASS_FUNCTION(TutorialGame, setMessageToTopPosition);
    REGISTER_SCRIPT_CLASS_FUNCTION(TutorialGame, setMessageToBottomPosition);
    REGISTER_SCRIPT_CLASS_FUNCTION(TutorialGame, onNext);
    REGISTER_SCRIPT_CLASS_FUNCTION(TutorialGame, finish);
}

TutorialGame::TutorialGame(bool repeated_tutorial, string filename)
{
    new LocalOnlyGame();

    new GuiOverlay(this, "", colorConfig.background);
    (new GuiOverlay(this, "", sf::Color::White))->setTextureTiled("gui/BackgroundCrosses");

    this->viewport = nullptr;
    this->repeated_tutorial = repeated_tutorial;
    
    // Initialize inactivity tracking
    inactivity_timer = 0.0f;
    warning_timer = 0.0f;
    warning_shown = false;
    warning_overlay = nullptr;
    warning_panel = nullptr;
    warning_label = nullptr;
    continue_button = nullptr;
    end_button = nullptr;
    
    // Initialize end tutorial confirmation dialog
    end_tutorial_button = nullptr;
    confirm_overlay = nullptr;
    confirm_panel = nullptr;
    confirm_label = nullptr;
    confirm_yes_button = nullptr;
    confirm_cancel_button = nullptr;

    i18n::load("locale/" + PreferencesManager::get("language", "en") + ".po");
    i18n::load("locale/tutorial." + PreferencesManager::get("language", "en") + ".po");
    script = new ScriptObject();
    script->registerObject(this, "tutorial");
    script->run(filename);
}

void TutorialGame::createScreens()
{
    viewport = new GuiViewport3D(this, "");
    viewport->setSize(GuiElement::GuiSizeMax, GuiElement::GuiSizeMax)->setPosition(0, 0, ATopLeft);

    tactical_radar = new GuiRadarView(this, "TACTICAL", nullptr, my_spaceship);
    tactical_radar->setPosition(0, 0, ATopLeft)->setSize(GuiElement::GuiSizeMax, GuiElement::GuiSizeMax);
    tactical_radar->setRangeIndicatorStepSize(1000.0f)->shortRange()->enableCallsigns()->hide();
    long_range_radar = new GuiRadarView(this, "TACTICAL", nullptr, my_spaceship);
    long_range_radar->setPosition(0, 0, ATopLeft)->setSize(GuiElement::GuiSizeMax, GuiElement::GuiSizeMax);
    long_range_radar->setRangeIndicatorStepSize(5000.0f)->longRange()->enableCallsigns()->hide();
    long_range_radar->setFogOfWarStyle(GuiRadarView::NebulaFogOfWar);

    setDefaultsFromPreferences();

    station_screen[0] = new HelmsScreen(this);
    station_screen[1] = new WeaponsScreen(this);
    station_screen[2] = new EngineeringScreen(this);
    station_screen[3] = new ScienceScreen(this);
    station_screen[4] = new RelayScreen(this, true, true);
    station_screen[5] = new TacticalScreen(this);
    station_screen[6] = new OperationScreen(this);
    station_screen[7] = new TractorBeamScreen(this);
    station_screen[8] = new TargetAnalysisScreen(this);
    station_screen[9] = new DamageControlScreen(this);
    station_screen[10] = new PowerManagementScreen(this);
    station_screen[11] = new DockMasterScreen(this);
    station_screen[12] = new DroneOperatorScreen(this);
    for(int n=0; n<13; n++)
        station_screen[n]->setSize(GuiElement::GuiSizeMax, GuiElement::GuiSizeMax)->setPosition(0, 0, ATopLeft);

    new GuiIndicatorOverlays(this);

    frame = new GuiPanel(this, "");
    frame->setPosition(0, 0, ATopCenter)->setSize(900, 230)->hide();

    text = new GuiScrollText(frame, "", "");
    text->setTextSize(20)->setPosition(20, 20, ATopLeft)->setSize(900 - 40, 200 - 40);
    next_button = new GuiButton(frame, "", tr("Next"), [this]() {
        _onNext.call();
    });
    next_button->setTextSize(30)->setPosition(-20, -20, ABottomRight)->setSize(300, 30);

    if (repeated_tutorial)
    {
        (new GuiButton(this, "", tr("Reset"), [this]()
        {
            finish();
        }))->setPosition(-16, 20, ATopRight)->setSize(200, 50);
    }

    // Create "End Tutorial" button in top right (inset so it stays inside the canvas)
    end_tutorial_button = new GuiButton(this, "END_TUTORIAL_BUTTON", tr("End Tutorial"), [this]() {
        showEndTutorialConfirmation();
    });
    if (repeated_tutorial)
        end_tutorial_button->setPosition(-16, 78, ATopRight)->setSize(220, 50);
    else
        end_tutorial_button->setPosition(-16, 20, ATopRight)->setSize(220, 50);

    // Create end tutorial confirmation dialog
    confirm_overlay = new GuiOverlay(this, "END_TUTORIAL_CONFIRM", sf::Color(0, 0, 0, 192));
    confirm_overlay->setBlocking(true)->hide();

    confirm_panel = new GuiPanel(confirm_overlay, "CONFIRM_PANEL");
    confirm_panel->setPosition(0, 0, ACenter)->setSize(560, 220);

    confirm_label = new GuiLabel(confirm_panel, "CONFIRM_LABEL", tr("Would you like to end this tutorial?"), 30);
    confirm_label->setPosition(0, 24, ATopCenter)->setSize(520, 96);
    confirm_label->setAlignment(ACenter);

    {
        GuiAutoLayout* confirm_btn_row = new GuiAutoLayout(confirm_panel, "CONFIRM_BTN_ROW", GuiAutoLayout::LayoutVerticalColumns);
        confirm_btn_row->setPosition(0, -20, ABottomCenter)->setSize(520, 54);
        confirm_btn_row->setMargins(20, 0, 20, 20);
        confirm_yes_button = new GuiButton(confirm_btn_row, "CONFIRM_YES_BUTTON", tr("Yes"), [this]() {
            finish();
        });
        confirm_yes_button->setTextSize(28)->setSize(GuiElement::GuiSizeMax, GuiElement::GuiSizeMax);
        confirm_cancel_button = new GuiButton(confirm_btn_row, "CONFIRM_CANCEL_BUTTON", tr("Cancel"), [this]() {
            hideEndTutorialConfirmation();
        });
        confirm_cancel_button->setTextSize(28)->setSize(GuiElement::GuiSizeMax, GuiElement::GuiSizeMax);
    }

    // Create inactivity warning dialog
    warning_overlay = new GuiOverlay(this, "INACTIVITY_WARNING", sf::Color(0, 0, 0, 192));
    warning_overlay->setBlocking(true)->hide();

    warning_panel = new GuiPanel(warning_overlay, "WARNING_PANEL");
    warning_panel->setPosition(0, 0, ACenter)->setSize(640, 270);

    warning_label = new GuiLabel(warning_panel, "WARNING_LABEL", tr("No input detected for 5 minutes.\nThe tutorial will be reset if no action is taken in 30 seconds."), 30);
    warning_label->setPosition(0, 24, ATopCenter)->setSize(600, 132);
    warning_label->setAlignment(ACenter);

    {
        GuiAutoLayout* warning_btn_row = new GuiAutoLayout(warning_panel, "WARNING_BTN_ROW", GuiAutoLayout::LayoutVerticalColumns);
        warning_btn_row->setPosition(0, -20, ABottomCenter)->setSize(600, 54);
        warning_btn_row->setMargins(20, 0, 20, 20);
        continue_button = new GuiButton(warning_btn_row, "CONTINUE_BUTTON", tr("Continue"), [this]() {
            hideInactivityWarning();
            resetInactivityTimer();
        });
        continue_button->setTextSize(28)->setSize(GuiElement::GuiSizeMax, GuiElement::GuiSizeMax);
        end_button = new GuiButton(warning_btn_row, "END_BUTTON", tr("End Tutorial"), [this]() {
            finish();
        });
        end_button->setTextSize(28)->setSize(GuiElement::GuiSizeMax, GuiElement::GuiSizeMax);
    }
    
    hideAllScreens();

    engine->setGameSpeed(1.0);
    
    // Start inactivity timer when screens are created
    resetInactivityTimer();
}

void TutorialGame::update(float delta)
{
    if (my_spaceship)
    {
        float target_camera_yaw = my_spaceship->getRotation();
        switch(my_spaceship->main_screen_setting)
        {
        case MSS_Back: target_camera_yaw += 180; break;
        case MSS_Left: target_camera_yaw -= 90; break;
        case MSS_Right: target_camera_yaw += 90; break;
        default: break;
        }
        camera_pitch = 30.0f;

        const float camera_ship_distance = 420.0f;
        const float camera_ship_height = 420.0f;
        sf::Vector2f cameraPosition2D = my_spaceship->getPosition() + sf::vector2FromAngle(target_camera_yaw) * -camera_ship_distance;
        sf::Vector3f targetCameraPosition(cameraPosition2D.x, cameraPosition2D.y, camera_ship_height);

        camera_position = camera_position * 0.9f + targetCameraPosition * 0.1f;
        camera_yaw += sf::angleDifference(camera_yaw, target_camera_yaw) * 0.1f;
    }
    
    // Handle inactivity timeout - only track if tutorial has started (viewport exists)
    if (viewport != nullptr)
    {
        // Check for any input (mouse/touch press) to reset timer
        // This catches touches anywhere on screen, including on buttons
        if (!warning_shown && (InputHandler::mouseIsPressed(sf::Mouse::Left) || 
                               InputHandler::mouseIsPressed(sf::Mouse::Right) || 
                               InputHandler::mouseIsPressed(sf::Mouse::Middle)))
        {
            resetInactivityTimer();
        }
        
        if (!warning_shown)
        {
            inactivity_timer += delta;
            // Show warning after 5 minutes (300 seconds)
            if (inactivity_timer >= 300.0f)
            {
                showInactivityWarning();
            }
        }
        else
        {
            // If warning is shown, track the 30-second timeout
            warning_timer += delta;
            // Auto-return to menu after 30 seconds
            if (warning_timer >= 30.0f)
            {
                finish();
            }
        }
    }
}

void TutorialGame::onKey(sf::Event::KeyEvent key, int unicode)
{
    // Reset inactivity timer on any key press (except when warning is shown, let buttons handle it)
    if (!warning_shown)
    {
        resetInactivityTimer();
    }
    
    switch(key.code)
    {
    case sf::Keyboard::Escape:
    case sf::Keyboard::Home:
        finish();
        break;
    default:
        break;
    }
}

void TutorialGame::onClick(sf::Vector2f mouse_position)
{
    // Reset inactivity timer on mouse click (except when warning is shown, let buttons handle it)
    if (!warning_shown)
    {
        resetInactivityTimer();
    }
    GuiCanvas::onClick(mouse_position);
}

void TutorialGame::handleJoystickButton(unsigned int joystickId, unsigned int button, bool state)
{
    // Reset inactivity timer on joystick button press (except when warning is shown, let buttons handle it)
    if (!warning_shown && state)
    {
        resetInactivityTimer();
    }
    GuiCanvas::handleJoystickButton(joystickId, button, state);
}

void TutorialGame::setPlayerShip(P<PlayerSpaceship> ship)
{
    my_player_info->commandSetShipId(ship->getMultiplayerId());

    // Player ship information is managed in GameGlobalInfo::update(). However, when we start a tutorial
    // we need to ensure that the player info is set before the screens are created, so forcibly set it here.
    if (my_player_info)
    {
        if ((my_spaceship && my_spaceship->getMultiplayerId() != my_player_info->ship_id) || (my_spaceship && my_player_info->ship_id == -1) || (!my_spaceship && my_player_info->ship_id != -1))
        {
            if (game_server)
                my_spaceship = game_server->getObjectById(my_player_info->ship_id);
            else
                my_spaceship = game_client->getObjectById(my_player_info->ship_id);
        }
    }

    if (viewport == nullptr)
        createScreens();
}

void TutorialGame::showMessage(string message, bool show_next)
{
    if (viewport == nullptr)
        return;

    frame->show();
    text->setText(message);
    if (show_next)
    {
        next_button->show();
        frame->setSize(900, 230);
    }
    else
    {
        next_button->hide();
        frame->setSize(900, 200);
    }
}

void TutorialGame::switchViewToMainScreen()
{
    if (viewport == nullptr)
        return;

    hideAllScreens();
    viewport->show();
}

void TutorialGame::switchViewToTactical()
{
    if (viewport == nullptr)
        return;

    hideAllScreens();
    tactical_radar->show();
}

void TutorialGame::switchViewToLongRange()
{
    if (viewport == nullptr)
        return;

    hideAllScreens();
    long_range_radar->show();
}

void TutorialGame::switchViewToScreen(int n)
{
    if (viewport == nullptr)
        return;

    if (n < 0 || n >= 13)
        return;
    hideAllScreens();
    station_screen[n]->show();
}

void TutorialGame::setMessageToTopPosition()
{
    if (viewport == nullptr)
        return;

    frame->setPosition(0, 0, ATopCenter);
}

void TutorialGame::setMessageToBottomPosition()
{
    if (viewport == nullptr)
        return;

    frame->setPosition(0, -50, ABottomCenter);
}

void TutorialGame::finish()
{
    if (repeated_tutorial)
    {
        foreach(SpaceObject, obj, space_object_list)
            obj->destroy();
        script->destroy();
        hideAllScreens();

        script = new ScriptObject();
        script->registerObject(this, "tutorial");
        script->run("tutorial.lua");
    }else{
        script->destroy();
        destroy();

        disconnectFromServer();
        new TutorialMenu();
    }
}

void TutorialGame::setDefaultsFromPreferences()
{
    // Follow the same setup process as ServerCreationScreen so tutorials respect options in the same way as the server games
    gameGlobalInfo->player_warp_jump_drive_setting = EPlayerWarpJumpDrive(PreferencesManager::get("server_config_warp_jump_drive_setting", "0").toInt());
    gameGlobalInfo->scanning_complexity = EScanningComplexity(PreferencesManager::get("server_config_scanning_complexity", "2").toInt());
    gameGlobalInfo->hacking_difficulty = PreferencesManager::get("server_config_hacking_difficulty", "1").toInt();
    gameGlobalInfo->hacking_games = EHackingGames(PreferencesManager::get("server_config_hacking_games", "2").toInt());
    gameGlobalInfo->use_beam_shield_frequencies = PreferencesManager::get("server_config_use_beam_shield_frequencies", "1").toInt();
    gameGlobalInfo->use_system_damage = PreferencesManager::get("server_config_use_system_damage", "1").toInt();
    gameGlobalInfo->use_advanced_sector_system = PreferencesManager::get("server_config_use_advanced_sector_system", "1").toInt();
    gameGlobalInfo->use_complex_radar_signatures = PreferencesManager::get("server_config_use_complex_radar_signatures", "0").toInt();
    gameGlobalInfo->allow_main_screen_tactical_radar = PreferencesManager::get("server_config_allow_main_screen_tactical_radar", "1").toInt();
    gameGlobalInfo->allow_main_screen_long_range_radar = PreferencesManager::get("server_config_allow_main_screen_long_range_radar", "1").toInt();
    gameGlobalInfo->allow_main_screen_far_range_radar = PreferencesManager::get("server_config_allow_main_screen_far_range_radar", "1").toInt();
    gameGlobalInfo->allow_main_screen_target_analysis = PreferencesManager::get("server_config_allow_main_screen_target_analysis", "1").toInt();
    gameGlobalInfo->use_nano_repair_crew = PreferencesManager::get("server_use_nano_repair_crew", "1").toInt();
    gameGlobalInfo->color_by_faction = PreferencesManager::get("server_config_color_by_faction", "1").toInt();
    gameGlobalInfo->all_can_be_targeted = PreferencesManager::get("server_config_all_can_be_targeted", "1").toInt();
    gameGlobalInfo->logs_by_station = PreferencesManager::get("server_config_logs_by_station", "1").toInt();
    gameGlobalInfo->use_warp_terrain = PreferencesManager::get("server_config_use_warp_terrain", "1").toInt();
}

void TutorialGame::hideAllScreens()
{
    if (viewport == nullptr)
        return;

    viewport->hide();
    tactical_radar->hide();
    long_range_radar->hide();

    for(int n=0; n<13; n++)
    {
        station_screen[n]->hide();
    }
}

void TutorialGame::resetInactivityTimer()
{
    inactivity_timer = 0.0f;
    warning_timer = 0.0f;
}

void TutorialGame::showInactivityWarning()
{
    if (warning_overlay == nullptr)
        return;
    
    warning_shown = true;
    warning_timer = 0.0f;
    warning_overlay->show();
}

void TutorialGame::hideInactivityWarning()
{
    if (warning_overlay == nullptr)
        return;
    
    warning_shown = false;
    warning_overlay->hide();
    resetInactivityTimer();
}

void TutorialGame::showEndTutorialConfirmation()
{
    if (confirm_overlay == nullptr)
        return;
    
    confirm_overlay->show();
}

void TutorialGame::hideEndTutorialConfirmation()
{
    if (confirm_overlay == nullptr)
        return;
    
    confirm_overlay->hide();
}

void LocalOnlyGame::update(float delta)
{
}
