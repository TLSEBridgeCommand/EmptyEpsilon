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

AutoConnectScreen::AutoConnectScreen(string autoconnect)
{
    // Initialize new variables
    connect_timeout = 0.0f;
    connect_attempts = 0;
    waiting_for_scenario = false;
    waiting_for_password = false;

    // Existing connection setup
    if (game_client)
        game_client->destroy();
    
    new GameClient(autoconnect, 35666);
    
    if (!game_client->getConnected())
    {
        LOG(WARNING) << "Failed to connect to " << autoconnect;
        destroy();
        new MainMenuScreen();
        return;
    }

    // Start waiting for scenario
    waiting_for_scenario = true;
}

void AutoConnectScreen::onGui()
{
    // Add loading indicator while waiting
    if (waiting_for_scenario)
    {
        gui2::Box* box = new gui2::Box(sf::FloatRect(0, 0, 800, 600), gui2::Theme::WindowBackground);
        box->setPosition(sf::Vector2f(getWindowSize().x / 2 - 400, getWindowSize().y / 2 - 300));
        
        (new gui2::Label(sf::FloatRect(0, 250, 800, 50), "Waiting for scenario to load...", AlignCenter))->setSize(30);
        
        if (connect_attempts > 0)
        {
            string attempt_text = "Attempt " + string(connect_attempts + 1) + " of " + string(MAX_ATTEMPTS);
            (new gui2::Label(sf::FloatRect(0, 300, 800, 30), attempt_text, AlignCenter))->setSize(20);
        }
        return;
    }

    // ... rest of existing onGui code ...
}

void AutoConnectScreen::destroy()
{
    // Cleanup if we're still waiting
    if (waiting_for_scenario)
    {
        if (game_client)
        {
            game_client->destroy();
            game_client = nullptr;
        }
    }
    
    // Existing destroy code
    if (password_overlay)
        password_overlay->destroy();
    if (control_code_numeric_panel)
        control_code_numeric_panel->destroy();
        
    GuiCanvas::destroy();
}
