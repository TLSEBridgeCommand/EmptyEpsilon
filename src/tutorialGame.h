#ifndef TUTORIAL_GAME_H
#define TUTORIAL_GAME_H

#include "epsilonServer.h"
#include "gui/gui2_canvas.h"

class PlayerSpaceship;
class GuiRadarView;
class GuiPanel;
class GuiButton;
class GuiScrollText;
class GuiOverlay;
class GuiLabel;

class TutorialGame : public Updatable, public GuiCanvas
{
    GuiElement* viewport;
    GuiRadarView* tactical_radar;
    GuiRadarView* long_range_radar;
    GuiElement* station_screen[13];

    P<ScriptObject> script;
    GuiPanel* frame;
    GuiScrollText* text;
    GuiButton* next_button;

    bool repeated_tutorial;
    
    // Inactivity timeout tracking
    float inactivity_timer;
    float warning_timer;
    bool warning_shown;
    GuiOverlay* warning_overlay;
    GuiPanel* warning_panel;
    GuiLabel* warning_label;
    GuiButton* continue_button;
    GuiButton* end_button;
    
    // End tutorial confirmation dialog
    GuiButton* end_tutorial_button;
    GuiOverlay* confirm_overlay;
    GuiPanel* confirm_panel;
    GuiLabel* confirm_label;
    GuiButton* confirm_yes_button;
    GuiButton* confirm_cancel_button;
public:
    ScriptSimpleCallback _onNext;

    TutorialGame(bool repeated_tutorial = false, string filename = "tutorial.lua");

    virtual void update(float delta) override;
    virtual void onKey(sf::Event::KeyEvent key, int unicode) override;
    virtual void onClick(sf::Vector2f mouse_position) override;
    virtual void handleJoystickButton(unsigned int joystickId, unsigned int button, bool state) override;

    void setPlayerShip(P<PlayerSpaceship> ship);

    void showMessage(string message, bool show_next);
    void switchViewToMainScreen();
    void switchViewToTactical();
    void switchViewToLongRange();
    void switchViewToScreen(int n);
    void setMessageToTopPosition();
    void setMessageToBottomPosition();

    void onNext(ScriptSimpleCallback callback) { _onNext = callback; }
    void finish();
private:
    void setDefaultsFromPreferences();
    void hideAllScreens();
    void createScreens();
    void resetInactivityTimer();
    void showInactivityWarning();
    void hideInactivityWarning();
    void showEndTutorialConfirmation();
    void hideEndTutorialConfirmation();
};

class LocalOnlyGame : public EpsilonServer
{
public:
    //Overide the update function from the game server, so no actuall socket communication is done.
    virtual void update(float delta) override;
};

#endif//TUTORIAL_GAME_H
