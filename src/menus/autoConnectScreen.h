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

class AutoConnectScreen : public GuiCanvas, public Updatable
{
	GuiOverlay* screen_connect;
    P<ServerScanner> scanner;
    sf::IpAddress connect_to_address;
    ECrewPosition crew_position;
    int auto_mainscreen;
    bool control_main_screen;
    bool waiting_for_password;
    std::map<string, string> ship_filters;
    GuiOverlay* control_code_numeric_panel_overlay;
    GuiControlNumericEntryPanel* control_code_numeric_panel;

    GuiLabel* status_label;
    GuiLabel* filter_label;
    float update_timer;
    /** Delay after scenario_started before picking ship by index, so replication can settle. */
    float ship_selection_delay;
public:
    AutoConnectScreen(ECrewPosition crew_position, int auto_mainscreen, bool control_main_screen, string ship_filter);
    void autoConnectPasswordEntryOnOkClick(P<PlayerSpaceship> ship = nullptr);
    void autoConnectPasswordEntryOnEnter(string text);
    virtual ~AutoConnectScreen();

    virtual void update(float delta);

private:
    bool isValidShip(int index);
    void connectToShip(int index);
    bool is_integer(const std::string& string);
};

#endif//AUTO_CONNECT_SCREEN_H
