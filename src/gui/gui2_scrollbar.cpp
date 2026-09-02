#include "gui2_scrollbar.h"
#include "gui2_arrowbutton.h"

#include <algorithm>

GuiScrollbar::GuiScrollbar(GuiContainer* owner, string id, int min_value, int max_value, int start_value, func_t func, bool horizontal)
: GuiElement(owner, id), min_value(min_value), max_value(max_value), desired_value(start_value), value_size(1), func(func), horizontal(horizontal), back_arrow(nullptr), forward_arrow(nullptr)
{
    if (!horizontal)
    {
        back_arrow = new GuiArrowButton(this, id + "_UP_ARROW", 90, [this]() {
            setValue(getValue() - 1);
        });
        back_arrow->setPosition(0, 0, ATopRight)->setSize(GuiSizeMax, GuiSizeMatchWidth);

        forward_arrow = new GuiArrowButton(this, id + "_DOWN_ARROW", -90, [this]() {
            setValue(getValue() + 1);
        });
        forward_arrow->setPosition(0, 0, ABottomRight)->setSize(GuiSizeMax, GuiSizeMatchWidth);
    }
}

void GuiScrollbar::onDraw(sf::RenderTarget& window)
{
    drawStretched(window, rect, "gui/ScrollbarBackground");

    int range = (max_value - min_value);
    if (range <= 0)
        return;

    if (horizontal)
    {
        float move_width = rect.width;
        float bar_size = move_width * float(value_size) / float(range);
        if (bar_size > move_width)
            bar_size = move_width;
        drawStretched(window, sf::FloatRect(rect.left + move_width * float(getValue()) / float(range), rect.top, bar_size, rect.height), "gui/ScrollbarSelection", sf::Color::White);
    }
    else
    {
        float arrow_size = rect.width / 2.0f;
        float move_height = rect.height - arrow_size * 2;
        float bar_size = move_height * float(value_size) / float(range);
        if (bar_size > move_height)
            bar_size = move_height;
        drawStretched(window, sf::FloatRect(rect.left, rect.top + arrow_size + move_height * float(getValue()) / float(range), rect.width, bar_size), "gui/ScrollbarSelection", sf::Color::White);
    }
}

bool GuiScrollbar::onMouseDown(sf::Vector2f position)
{
    int range = (max_value - min_value);
    if (range <= 0)
        return true;

    if (horizontal)
    {
        float move_width = rect.width;
        float bar_size = move_width * float(value_size) / float(range);
        if (bar_size > move_width)
            bar_size = move_width;
        float bar_x = rect.left + move_width * float(getValue()) / float(range);
        if (position.x >= bar_x && position.x <= bar_x + bar_size)
        {
            drag_scrollbar = true;
            drag_select_offset = position.x - bar_x;
        }else{
            drag_scrollbar = false;
        }
    }
    else
    {
        float arrow_size = rect.width / 2.0f;
        float move_height = rect.height - arrow_size * 2;
        float bar_size = move_height * float(value_size) / float(range);
        if (bar_size > move_height)
            bar_size = move_height;
        float bar_y = rect.top + arrow_size + move_height * float(getValue()) / float(range);
        if (position.y >= bar_y && position.y <= bar_y + bar_size)
        {
            drag_scrollbar = true;
            drag_select_offset = position.y - bar_y;
        }else{
            drag_scrollbar = false;
        }
    }
    return true;
}

void GuiScrollbar::onMouseDrag(sf::Vector2f position)
{
    if (!drag_scrollbar)
        return;

    int range = (max_value - min_value);
    if (range <= 0)
        return;

    if (horizontal)
    {
        float move_width = rect.width;
        float bar_size = move_width * float(value_size) / float(range);
        if (bar_size > move_width)
            bar_size = move_width;

        float target_x_offset = position.x - drag_select_offset - rect.left;
        target_x_offset = std::max(target_x_offset, 0.0f);
        target_x_offset = std::min(target_x_offset, move_width - bar_size);

        if (bar_size < move_width)
            setValue(int(target_x_offset / move_width * range + 0.5f));
    }
    else
    {
        float arrow_size = rect.width / 2.0f;
        float move_height = rect.height - arrow_size * 2;
        float bar_size = move_height * float(value_size) / float(range);
        if (bar_size > move_height)
            bar_size = move_height;

        float target_y_offset = position.y - drag_select_offset - (rect.top + arrow_size);
        target_y_offset = std::max(target_y_offset, 0.0f);
        target_y_offset = std::min(target_y_offset, move_height - bar_size);

        if (bar_size < move_height)
            setValue(int(target_y_offset / move_height * range + 0.5f));
    }
}

void GuiScrollbar::onMouseUp(sf::Vector2f position)
{
    if (drag_scrollbar)
        return;

    int range = (max_value - min_value);
    if (range <= 0)
        return;

    if (horizontal)
    {
        float move_width = rect.width;
        float bar_size = move_width * float(value_size) / float(range);
        if (bar_size > move_width)
            bar_size = move_width;

        float target_x_offset = position.x - bar_size / 2.0f - rect.left;
        target_x_offset = std::max(target_x_offset, 0.0f);
        target_x_offset = std::min(target_x_offset, move_width - bar_size);

        if (bar_size < move_width)
            setValue(int(target_x_offset / move_width * range + 0.5f));
    }
    else
    {
        float arrow_size = rect.width / 2.0f;
        float move_height = rect.height - arrow_size * 2;
        float bar_size = move_height * float(value_size) / float(range);
        if (bar_size > move_height)
            bar_size = move_height;

        float target_y_offset = position.y - bar_size / 2.0f - (rect.top + arrow_size);
        target_y_offset = std::max(target_y_offset, 0.0f);
        target_y_offset = std::min(target_y_offset, move_height - bar_size);

        if (bar_size < move_height)
            setValue(int(target_y_offset / move_height * range + 0.5f));
    }
}

void GuiScrollbar::setRange(int min_value, int max_value)
{
    this->min_value = min_value;
    this->max_value = max_value;
}

void GuiScrollbar::setValueSize(int size)
{
    value_size = size;
}

void GuiScrollbar::setValue(int value)
{
    if (this->desired_value == value)
        return;

    this->desired_value = value;

    if (func)
        func(getValue());
}

int GuiScrollbar::getValue() const
{
    auto value = desired_value;
    if (value > max_value - value_size)
        value = max_value - value_size;
    if (value < min_value)
        value = min_value;
    return value;
}

int GuiScrollbar::getMax() const
{
    return max_value;
}

int GuiScrollbar::getMin() const
{
    return min_value;
}
