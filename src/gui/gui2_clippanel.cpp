#include "gui2_clippanel.h"
#include "gui2_canvas.h"
#include "input.h"

GuiClipPanel::GuiClipPanel(GuiContainer* owner, string id)
: GuiElement(owner, id)
{
}

void GuiClipPanel::drawElements(sf::FloatRect parent_rect, sf::RenderTarget& window)
{
    sf::Vector2f mouse_position = InputHandler::getMousePos();
    for(auto it = elements.begin(); it != elements.end(); )
    {
        GuiElement* element = *it;
        if (element->destroyed)
        {
            GuiCanvas* canvas = dynamic_cast<GuiCanvas*>(element->getTopLevelContainer());
            if (canvas)
                canvas->unfocusElementTree(element);

            it = elements.erase(it);

            element->owner = nullptr;
            delete element;
        }else{
            element->updateRect(rect);
            element->hover = element->rect.contains(mouse_position);
            element->onUpdate();
            it++;
        }
    }

    if (!visible)
        return;

    adjustRenderTexture(render_texture);
    render_texture.clear(sf::Color::Transparent);
    sf::View texture_view(rect);
    render_texture.setView(texture_view);

    for(GuiElement* element : elements)
    {
        if (element->visible)
        {
            element->onDraw(render_texture);
            element->drawElements(element->rect, render_texture);
        }
    }

    drawRenderTexture(render_texture, window);
}

GuiElement* GuiClipPanel::getClickElement(sf::Vector2f mouse_position)
{
    if (!rect.contains(mouse_position))
        return nullptr;
    return GuiContainer::getClickElement(mouse_position);
}
