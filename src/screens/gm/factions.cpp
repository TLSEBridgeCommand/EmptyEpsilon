#include "factions.h"
#include "engine.h"
#include "windowManager.h"
#include "gui/gui2_panel.h"
#include "gui/gui2_label.h"
#include "gui/gui2_button.h"
#include "gui/gui2_selector.h"
#include "gui/gui2_listbox.h"
#include "gui/gui2_togglebutton.h"
#include "gui/gui2_scrollbar.h"
#include "gui/gui2_clippanel.h"
#include "factionInfo.h"
#include "GMActions.h"

#include <algorithm>

GuiFactions::GuiFactions(GuiContainer *owner)
    : GuiOverlay(owner, "FACTIONS_OVERLAY", sf::Color(0, 0, 0, 128)), faction_a(-1), faction_b(-1), table_mode(false)
{
    this->setBlocking(true);

    P<WindowManager> window_manager = engine->getObject("windowManager");
    sf::Vector2i virtual_size = window_manager->getVirtualSize();
    panel_width = std::min(900.f, virtual_size.x * 0.85f);
    panel_height = std::min(600.f, virtual_size.y * 0.85f);
    table_viewport_width = panel_width - margin * 2 - scroll_bar_size;
    table_viewport_height = panel_height - toggle_row_height - margin - scroll_bar_size;

    int cells_size = int(factionInfo.size()) * cell_size;
    table_content_width = margin + label_size + cells_size;
    table_content_height = margin + label_size + cells_size;

    box = new GuiPanel(this, "PANEL");
    box->setPosition(0, 0, ACenter)->setSize(panel_width, panel_height);
    (new GuiButton(box, "", "X", [this]() { this->onClose(); }))->setPosition(-10, 10, ATopRight)->setSize(30, 30);

    view_toggle = new GuiToggleButton(box, "FACTIONS_VIEW_TOGGLE", "Table view", [this](bool value) {
        setViewMode(value);
    });
    view_toggle->setPosition(10, 10, ATopLeft)->setSize(150, 30);
    view_toggle->setValue(false);

    // Table (matrix) view — scrollable inside a fixed panel
    table_view = new GuiElement(box, "FACTIONS_TABLE_VIEW");
    table_view->setPosition(0, toggle_row_height, ATopLeft)->setSize(panel_width, panel_height - toggle_row_height)->hide();

    table_viewport = new GuiClipPanel(table_view, "FACTIONS_TABLE_VIEWPORT");
    table_viewport->setPosition(margin, 0, ATopLeft)->setSize(table_viewport_width, table_viewport_height);

    table_content = new GuiElement(table_viewport, "FACTIONS_TABLE_CONTENT");
    table_content->setPosition(0, 0, ATopLeft)->setSize(table_content_width, table_content_height);

    for (unsigned int i = 0; i < factionInfo.size(); i++)
    {
        GuiLabel *label = new GuiLabel(table_content, "", factionInfo[i]->getName(), 30);
        label->setAlignment(ACenterRight)->setPosition(margin, margin + label_size + i * cell_size, ATopLeft)->setSize(label_size - margin, cell_size - margin);
        h_labels.push_back(label);
        label = new GuiLabel(table_content, "", factionInfo[i]->getName(), 30);
        label->setAlignment(ACenterLeft)->setVertical()->setPosition(margin + label_size + i * cell_size, margin, ATopLeft)->setSize(cell_size - margin, label_size - margin);
        v_labels.push_back(label);
        for (unsigned int j = 0; j < factionInfo.size(); j++)
        {
            if (i == j)
            {
                buttons.push_back(nullptr);
            }
            else
            {
                GuiButton *button = new GuiButton(table_content, "", "", [this, i, j]() { this->onSelectFactions(i, j); });
                button->setPosition(margin + label_size + j * cell_size, margin + label_size + i * cell_size, ATopLeft)->setSize(cell_size - margin, cell_size - margin);
                buttons.push_back(button);
            }
        }
    }

    table_h_scroll = new GuiScrollbar(table_view, "FACTIONS_TABLE_H_SCROLL", 0, table_content_width, 0, [this](int value) {
        updateTableScroll();
    }, true);
    table_h_scroll->setPosition(margin, table_viewport_height, ATopLeft)->setSize(table_viewport_width, scroll_bar_size);
    table_h_scroll->setValueSize(int(table_viewport_width));

    table_v_scroll = new GuiScrollbar(table_view, "FACTIONS_TABLE_V_SCROLL", 0, table_content_height, 0, [this](int value) {
        updateTableScroll();
    });
    table_v_scroll->setPosition(margin + table_viewport_width, 0, ATopLeft)->setSize(scroll_bar_size, table_viewport_height);
    table_v_scroll->setValueSize(int(table_viewport_height));

    updateTableScroll();

    edit_panel = new GuiElement(box, "EDIT_PANEL");
    edit_panel->hide();
    edit_panel->setPosition(-margin, toggle_row_height, ATopRight)->setSize(2 * margin + label_size, table_viewport_height);

    faction_a_edit_label = new GuiLabel(edit_panel, "", "", 30);
    faction_a_edit_label->setAlignment(ACenter)->setPosition(0, margin, ATopCenter)->setSize(label_size - margin, cell_size - margin);

    (new GuiLabel(edit_panel, "", "consider", 15))->setAlignment(ACenter)->setPosition(0, margin * 2 + cell_size, ATopCenter)->setSize(label_size - margin, cell_size - margin);

    faction_b_edit_label = new GuiLabel(edit_panel, "", "", 30);
    faction_b_edit_label->setAlignment(ACenter)->setPosition(0, margin * 3 + cell_size * 2, ATopCenter)->setSize(label_size - margin, cell_size - margin);

    (new GuiLabel(edit_panel, "", "to be", 15))->setAlignment(ACenter)->setPosition(0, margin * 4 + cell_size * 3, ATopCenter)->setSize(label_size - margin, cell_size - margin);

    edit_selector = new GuiSelector(edit_panel, "", [this](int selection_index, string value) {
        if (faction_a >= 0 && faction_b >= 0)
            gameMasterActions->commandSetFactionsState(faction_a, faction_b, selection_index);
    });
    edit_selector->setPosition(0, margin * 5 + cell_size * 4, ATopCenter)->setSize(label_size - margin, cell_size - margin);
    edit_selector->setOptions({
        getFactionVsFactionStateName(FVF_Friendly),
        getFactionVsFactionStateName(FVF_Neutral),
        getFactionVsFactionStateName(FVF_Enemy)
    });

    // Selector view — default
    selector_view_panel = new GuiElement(box, "FACTIONS_SELECTOR_VIEW");
    selector_view_panel->setPosition(0, toggle_row_height, ATopLeft)->setSize(panel_width, panel_height - toggle_row_height);

    (new GuiLabel(selector_view_panel, "", "Faction A", 25))->setPosition(20, 10, ATopLeft)->setSize(250, 30);
    (new GuiLabel(selector_view_panel, "", "Faction B", 25))->setPosition(-20, 10, ATopRight)->setSize(250, 30);

    faction_a_list = new GuiListbox(selector_view_panel, "FACTION_A_LIST", [this](int index, string value) {
        faction_a = value.toInt();
        rebuildFactionLists();
        refreshRelation();
    });
    faction_a_list->setButtonHeight(40)->setTextSize(20);
    faction_a_list->setPosition(20, 45, ATopLeft)->setSize(250, panel_height - toggle_row_height - 130);

    faction_b_list = new GuiListbox(selector_view_panel, "FACTION_B_LIST", [this](int index, string value) {
        faction_b = value.toInt();
        rebuildFactionLists();
        refreshRelation();
    });
    faction_b_list->setButtonHeight(40)->setTextSize(20);
    faction_b_list->setPosition(-20, 45, ATopRight)->setSize(250, panel_height - toggle_row_height - 130);

    rebuildFactionLists();

    relation_label = new GuiLabel(selector_view_panel, "", "Select two different factions", 20);
    relation_label->setAlignment(ACenter)->setPosition(0, -90, ABottomCenter)->setSize(panel_width - 40, 30);

    relation_selector = new GuiSelector(selector_view_panel, "RELATION_SELECTOR", [this](int selection_index, string value) {
        if (faction_a >= 0 && faction_b >= 0 && faction_a != faction_b)
            gameMasterActions->commandSetFactionsState(faction_a, faction_b, selection_index);
    });
    relation_selector->setPosition(0, -40, ABottomCenter)->setSize(300, 40);
    relation_selector->setOptions({
        getFactionVsFactionStateName(FVF_Friendly),
        getFactionVsFactionStateName(FVF_Neutral),
        getFactionVsFactionStateName(FVF_Enemy)
    });
    relation_selector->disable();

    setViewMode(false);
}

void GuiFactions::updateTableScroll()
{
    table_content->setPosition(-float(table_h_scroll->getValue()), -float(table_v_scroll->getValue()), ATopLeft);

    if (table_content_width <= int(table_viewport_width))
        table_h_scroll->hide();
    else
        table_h_scroll->show();

    if (table_content_height <= int(table_viewport_height))
        table_v_scroll->hide();
    else
        table_v_scroll->show();
}

void GuiFactions::setViewMode(bool table)
{
    table_mode = table;
    view_toggle->setValue(table);
    table_view->setVisible(table);
    selector_view_panel->setVisible(!table);

    deSelectFactions();
    faction_a = -1;
    faction_b = -1;
    rebuildFactionLists();
    refreshRelation();
    updateTableScroll();
}

void GuiFactions::rebuildFactionLists()
{
    std::vector<string> names_a;
    std::vector<string> values_a;
    std::vector<string> names_b;
    std::vector<string> values_b;

    for (unsigned int n = 0; n < factionInfo.size(); n++)
    {
        if (int(n) != faction_b)
        {
            names_a.push_back(factionInfo[n]->getName());
            values_a.push_back(string(n));
        }
        if (int(n) != faction_a)
        {
            names_b.push_back(factionInfo[n]->getName());
            values_b.push_back(string(n));
        }
    }

    faction_a_list->setOptions(names_a, values_a);
    faction_b_list->setOptions(names_b, values_b);
    faction_a_list->setSelectionIndex(faction_a_list->indexByValue(string(faction_a)));
    faction_b_list->setSelectionIndex(faction_b_list->indexByValue(string(faction_b)));
}

void GuiFactions::refreshRelation()
{
    if (faction_a < 0 || faction_b < 0 || faction_a == faction_b
        || faction_a >= int(factionInfo.size()) || faction_b >= int(factionInfo.size()))
    {
        relation_label->setText("Select two different factions");
        relation_selector->disable();
        return;
    }

    relation_label->setText(factionInfo[faction_a]->getName() + " and " + factionInfo[faction_b]->getName() + " consider each other");
    relation_selector->enable();
    relation_selector->setSelectionIndex(factionInfo[faction_a]->states[faction_b]);
}

void GuiFactions::onSelectFactions(unsigned int i, unsigned int j)
{
    deSelectFactions();
    faction_a = int(i);
    faction_b = int(j);

    buttons[faction_a * factionInfo.size() + faction_b]->setActive(true);
    h_labels[faction_a]->addBackground();
    v_labels[faction_b]->addBackground();
    edit_panel->show();
    faction_a_edit_label->setText(factionInfo[faction_a]->getName());
    faction_b_edit_label->setText(factionInfo[faction_b]->getName());
    edit_selector->setSelectionIndex(factionInfo[i]->states[j]);
}

void GuiFactions::deSelectFactions()
{
    if (faction_a >= 0 && faction_b >= 0 && faction_a != faction_b
        && faction_a < int(factionInfo.size()) && faction_b < int(factionInfo.size()))
    {
        buttons[faction_a * factionInfo.size() + faction_b]->setActive(false);
        h_labels[faction_a]->removeBackground();
        v_labels[faction_b]->removeBackground();
    }
    edit_panel->hide();
}

void GuiFactions::onClose()
{
    deSelectFactions();
    faction_a = -1;
    faction_b = -1;
    this->hide();
}

void GuiFactions::onDraw(sf::RenderTarget &window)
{
    GuiOverlay::onDraw(window);

    if (table_mode)
    {
        for (unsigned int i = 0; i < factionInfo.size(); i++)
        {
            for (unsigned int j = 0; j < factionInfo.size(); j++)
            {
                if (i != j)
                {
                    buttons[i * factionInfo.size() + j]->setText(getFactionVsFactionStateName(factionInfo[i]->states[j])[0]);
                }
            }
        }
    }
    else
    {
        refreshRelation();
    }
}
