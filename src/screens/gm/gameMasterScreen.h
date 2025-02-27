#ifndef GAME_MASTER_SCREEN_H
#define GAME_MASTER_SCREEN_H

#include "engine.h"
#include "gui/gui2_panel.h"
#include "gui/gui2_scrolltext.h"
#include "gui/gui2_canvas.h"
#include "gui/gui2_overlay.h"
#include "screenComponents/targetsContainer.h"
#include "preferenceManager.h"

class GuiGlobalMessageEntry;
class GuiObjectCreationScreen;
class GuiObjectTweak;
class GuiRadarView;
class GuiOverlay;
class GuiSelector;
class GuiAutoLayout;
class GuiKeyValueDisplay;
class GuiListbox;
class GuiButton;
class GuiToggleButton;
class GuiTextEntry;
class GameMasterChatDialog;
class GuiObjectCreationView;
class GuiGlobalMessageEntryView;
class GuiFactions;
class GameMasterScreen : public GuiCanvas, public Updatable
{
private:
    TargetsContainer targets;
    P<SpaceObject> target;
    GuiRadarView* main_radar;
    GuiOverlay* box_selection_overlay;
    GuiSelector* faction_selector;
    GuiSelector* personality_selector;

    GuiElement* chat_layer;
    std::vector<GameMasterChatDialog*> chat_dialog_per_ship;
    GuiGlobalMessageEntryView* global_message_entry;
    GuiObjectCreationView* object_creation_view;
    GuiObjectTweak* player_tweak_dialog;
    GuiObjectTweak* ship_tweak_dialog;
    GuiObjectTweak* object_tweak_dialog;
    GuiObjectTweak* station_tweak_dialog;
    GuiObjectTweak* jammer_tweak_dialog;
    GuiFactions* factions_dialog;

    GuiAutoLayout* info_layout;
    std::vector<GuiKeyValueDisplay*> info_items;
    GuiKeyValueDisplay* info_clock;
    GuiListbox* gm_script_options;
    GuiAutoLayout* order_layout;
    GuiButton* player_comms_hail;
    GuiButton* global_message_button;
    GuiToggleButton* pause_button;
    GuiSelector* speed_selector;
    GuiToggleButton* intercept_comms_button;
    GuiButton* tweak_button;
    GuiButton* factions_button;
    GuiButton* copy_scenario_button;
    GuiButton* copy_selected_button;
    GuiSelector* player_ship_selector;

    GuiPanel* message_frame;
    GuiScrollText* message_text;
    GuiButton* message_close_button;

    enum EClickAndDragState
    {
        CD_None,
        CD_DragViewOrOrder,
        CD_DragView,
        CD_BoxSelect,
        CD_DragObjects
    } click_and_drag_state;
    sf::Vector2f drag_start_position;
    sf::Vector2f drag_previous_position;

    GuiButton* create_button;
    GuiButton* cancel_action_button;
public:

    GameMasterScreen();
    virtual ~GameMasterScreen();

    virtual void update(float delta);

    void onMouseDown(sf::Vector2f position);
    void onMouseDrag(sf::Vector2f position);
    void onMouseUp(sf::Vector2f position);
    const float max_distance = PreferencesManager::get("gm_radar_max_distance", "50000").toFloat();
    const float min_distance = 5000.0f;

    virtual void onKey(sf::Event::KeyEvent key, int unicode);

    PVector<SpaceObject> getSelection();

    string getScriptExport(bool selected_only);
};


#endif//GAME_MASTER_SCREEN_H
