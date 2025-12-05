#ifndef LOCAL_MESSAGE_H
#define LOCAL_MESSAGE_H

#include "gui/gui2_element.h"
#include "gui/gui2_panel.h"
#include "gui/gui2_label.h"
#include "spaceObjects/playerSpaceship.h"

class GuiLocalMessage : public GuiElement
{
public:
    GuiLocalMessage(GuiContainer* owner, P<PlayerSpaceship> ship);

    virtual void onDraw(sf::RenderTarget& window) override;

private:
    P<PlayerSpaceship> ship;
    GuiPanel* box;
    GuiLabel* label;
};

#endif // LOCAL_MESSAGE_H