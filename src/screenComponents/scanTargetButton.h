#ifndef SCAN_TARGET_BUTTON_H
#define SCAN_TARGET_BUTTON_H

#include "gui/gui2_element.h"

class GuiButton;
class GuiProgressbar;
class TargetsContainer;
class GuiPowerDamageIndicator;

class GuiScanTargetButton : public GuiElement
{
private:
    TargetsContainer* targets;
    GuiButton* button;
    GuiProgressbar* progress;
    GuiPowerDamageIndicator* pdi;
public:
    GuiScanTargetButton(GuiContainer* owner, string id, TargetsContainer* targets);

    virtual void onUpdate() override;
    virtual void onDraw(sf::RenderTarget& window) override;
};

#endif//SCAN_TARGET_BUTTON_H
