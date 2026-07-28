#ifndef FACTIONS_H
#define FACTIONS_H

#include "gui/gui2_overlay.h"

class GuiLabel;
class GuiPanel;
class GuiSelector;
class GuiListbox;

class GuiFactions : public GuiOverlay
{
  private:
    int faction_a;
    int faction_b;

    GuiListbox* faction_a_list;
    GuiListbox* faction_b_list;
    GuiLabel* relation_label;
    GuiSelector* relation_selector;

    void rebuildFactionLists();
    void refreshRelation();

  public:
    GuiFactions(GuiContainer *owner);

    virtual void onDraw(sf::RenderTarget &window) override;
    void onClose();
};

#endif //FACTIONS_H
