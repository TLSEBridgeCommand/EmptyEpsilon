#ifndef AUTO_CONNECT_SCREEN_H
#define AUTO_CONNECT_SCREEN_H

#include <vector>

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
    /** Ordered names from repeated server= in autoconnectship (multi_server_mode). */
    std::vector<string> autoconnect_server_names;
    size_t multi_server_name_index = 0;
    float multi_server_try_elapsed = 0.f;
    GuiOverlay* control_code_numeric_panel_overlay;
    GuiControlNumericEntryPanel* control_code_numeric_panel;

    GuiLabel* status_label;
    GuiLabel* filter_label;
    GuiButton* cancel_button;
    bool cancel_button_y_set;
    float update_timer;
    /** Delay after scenario_started before picking ship by index, so replication can settle. */
    float ship_selection_delay;
    /** LAN server display name from scanner (empty if joining by raw IP only). */
    string connect_broadcast_name_for_index;
    bool applied_auto_2s1u_session_ = false;
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
