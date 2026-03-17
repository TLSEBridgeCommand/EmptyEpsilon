#ifndef DOCKING_MENU_H
#define DOCKING_MENU_H

#include "gui/gui2_overlay.h"
#include "spaceObjects/playerSpaceship.h"

class GuiListbox;
class GuiButton;
class GuiLabel;
class GuiAutoLayout;

class GuiDockingMenu : public GuiOverlay
{
private:
    P<PlayerSpaceship> target_spaceship;
    GuiListbox* docking_targets_list;
    GuiButton* dock_button;
    GuiButton* cancel_button;
    GuiLabel* title_label;
    GuiLabel* no_targets_label;
    GuiAutoLayout* button_layout;

public:
    GuiDockingMenu(GuiContainer* owner, P<PlayerSpaceship> targetSpaceship);
    
    void setTargetSpaceship(P<PlayerSpaceship> targetSpaceship);
    virtual void onUpdate() override;
    void populateDockingTargets();
    void dockWithSelectedTarget();
    void cancelDocking();
};

#endif // DOCKING_MENU_H 