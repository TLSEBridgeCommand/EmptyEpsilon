#ifndef GUI2_CLIPPANEL_H
#define GUI2_CLIPPANEL_H

#include "gui2_element.h"

class GuiClipPanel : public GuiElement
{
private:
    sf::RenderTexture render_texture;

protected:
    virtual void drawElements(sf::FloatRect parent_rect, sf::RenderTarget& window) override;
    virtual GuiElement* getClickElement(sf::Vector2f mouse_position) override;

public:
    GuiClipPanel(GuiContainer* owner, string id);
};

#endif//GUI2_CLIPPANEL_H
