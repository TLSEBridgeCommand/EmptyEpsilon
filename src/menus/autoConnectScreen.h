#ifndef AUTO_CONNECT_SCREEN_H
#define AUTO_CONNECT_SCREEN_H

#include "gui/gui2_canvas.h"
#include "playerInfo.h"

class GuiOverlay;
class GuiPanel;
class GuiButton;
class GuiToggleButton;
class GuiTextEntry;
class GuiLabel;
class GuiControlNumericEntryPanel;

class AutoConnectScreen : public GuiCanvas
{
private:
    float connect_timeout;
    const float MAX_CONNECT_WAIT;
    int connect_attempts;
    const int MAX_ATTEMPTS;
    bool waiting_for_scenario;
    bool waiting_for_password;
    
    gui2::Panel* password_overlay;
    gui2::Panel* control_code_numeric_panel;

public:
    AutoConnectScreen(string autoconnect);
    virtual ~AutoConnectScreen() {}
    
    virtual void update(float delta) override;
    virtual void onGui() override;
    virtual void destroy() override;
    
private:
    void connectToMyShip();
    bool is_integer(const string& s);
};

#endif//AUTO_CONNECT_SCREEN_H
