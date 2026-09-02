#ifndef FACTIONS_H
#define FACTIONS_H

#include "gui/gui2_overlay.h"

class GuiLabel;
class GuiPanel;
class GuiSelector;
class GuiButton;
class GuiListbox;
class GuiToggleButton;
class GuiClipPanel;
class GuiElement;
class GuiScrollbar;

class GuiFactions : public GuiOverlay
{
  private:
    static constexpr int margin = 10;
    static constexpr int cell_size = 50;
    static constexpr int label_size = 160;
    static constexpr int scroll_bar_size = 25;
    static constexpr int toggle_row_height = 45;

    int faction_a;
    int faction_b;
    bool table_mode;

    float panel_width;
    float panel_height;
    float table_viewport_width;
    float table_viewport_height;
    int table_content_width;
    int table_content_height;

    GuiPanel* box;
    GuiToggleButton* view_toggle;
    GuiElement* table_view;
    GuiClipPanel* table_viewport;
    GuiElement* table_content;
    GuiScrollbar* table_h_scroll;
    GuiScrollbar* table_v_scroll;
    GuiElement* selector_view_panel;

    // Table (matrix) view
    std::vector<GuiLabel*> h_labels;
    std::vector<GuiLabel*> v_labels;
    std::vector<GuiButton*> buttons;
    GuiElement* edit_panel;
    GuiLabel* faction_a_edit_label;
    GuiLabel* faction_b_edit_label;
    GuiSelector* edit_selector;

    // Selector view
    GuiListbox* faction_a_list;
    GuiListbox* faction_b_list;
    GuiLabel* relation_label;
    GuiSelector* relation_selector;

    void setViewMode(bool table);
    void updateTableScroll();
    void rebuildFactionLists();
    void refreshRelation();
    void deSelectFactions();
    void onSelectFactions(unsigned int i, unsigned int j);

  public:
    GuiFactions(GuiContainer *owner);

    virtual void onDraw(sf::RenderTarget &window) override;
    void onClose();
};

#endif //FACTIONS_H
