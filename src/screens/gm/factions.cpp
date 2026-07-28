#include "factions.h"
#include "gui/gui2_panel.h"
#include "gui/gui2_label.h"
#include "gui/gui2_button.h"
#include "gui/gui2_selector.h"
#include "gui/gui2_listbox.h"
#include "factionInfo.h"
#include "GMActions.h"

GuiFactions::GuiFactions(GuiContainer *owner)
    : GuiOverlay(owner, "FACTIONS_OVERLAY", sf::Color(0, 0, 0, 128)), faction_a(-1), faction_b(-1)
{
    this->setBlocking(true);

    GuiPanel *box = new GuiPanel(this, "PANEL");
    box->setPosition(0, 0, ACenter)->setSize(700, 450);
    (new GuiButton(box, "", "X", [this]() { this->onClose(); }))->setPosition(-10, 10, ATopRight)->setSize(30, 30);

    (new GuiLabel(box, "", "Faction A", 25))->setPosition(20, 20, ATopLeft)->setSize(250, 30);
    (new GuiLabel(box, "", "Faction B", 25))->setPosition(-20, 20, ATopRight)->setSize(250, 30);

    faction_a_list = new GuiListbox(box, "FACTION_A_LIST", [this](int index, string value) {
        faction_a = value.toInt();
        rebuildFactionLists();
        refreshRelation();
    });
    faction_a_list->setButtonHeight(40)->setTextSize(20);
    faction_a_list->setPosition(20, 55, ATopLeft)->setSize(250, 280);

    faction_b_list = new GuiListbox(box, "FACTION_B_LIST", [this](int index, string value) {
        faction_b = value.toInt();
        rebuildFactionLists();
        refreshRelation();
    });
    faction_b_list->setButtonHeight(40)->setTextSize(20);
    faction_b_list->setPosition(-20, 55, ATopRight)->setSize(250, 280);

    rebuildFactionLists();

    relation_label = new GuiLabel(box, "", "Select two different factions", 20);
    relation_label->setAlignment(ACenter)->setPosition(0, -90, ABottomCenter)->setSize(660, 30);

    relation_selector = new GuiSelector(box, "RELATION_SELECTOR", [this](int selection_index, string value) {
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

void GuiFactions::onClose()
{
    this->hide();
}

void GuiFactions::onDraw(sf::RenderTarget &window)
{
    GuiOverlay::onDraw(window);
    refreshRelation();
}
