#include "localMessage.h"

GuiLocalMessage::GuiLocalMessage(GuiContainer* owner, P<PlayerSpaceship> ship)
: GuiElement(owner, "LOCAL_MESSAGE")
, ship(ship)
{
    setSize(GuiElement::GuiSizeMax, GuiElement::GuiSizeMax);

    // Slightly lower than global so they don't fight for space
    box = new GuiPanel(owner, "LOCAL_MESSAGE_BOX");
    box->setSize(800, 100)->setPosition(0, 360, ATopCenter);

    label = new GuiLabel(box, "LOCAL_MESSAGE_LABEL", "...", 40);
    label->setSize(GuiElement::GuiSizeMax, GuiElement::GuiSizeMax)->setPosition(0, 0, ACenter);
}

void GuiLocalMessage::onDraw(sf::RenderTarget& window)
{
    GuiElement::onDraw(window);

    if (!ship || ship->local_message_timeout <= 0.0f || ship->local_message.empty())
    {
        box->hide();
        return;
    }

    box->show();
    label->setText(ship->local_message);
}