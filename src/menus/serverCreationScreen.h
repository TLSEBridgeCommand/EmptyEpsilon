#ifndef SERVER_CREATION_SCREEN_H
#define SERVER_CREATION_SCREEN_H

#include "gui/gui2_canvas.h"
#include <map>
#include <vector>

class GuiScrollText;
class GuiAutoLayout;
class GuiSelector;
class GuiListbox;
class GuiButton;
class GuiLabel;

// ServerCreationScreen is only created when you are the server.
class ServerCreationScreen : public GuiCanvas
{
    string selected_scenario_filename;
    GuiScrollText* scenario_description;

    GuiAutoLayout* variation_container;
    std::vector<string> variation_names_list;
    std::vector<string> variation_descriptions_list;
    GuiSelector* variation_selection;
    GuiScrollText* variation_description;

    GuiLabel* scenario_label;
    GuiListbox* scenario_list;
    GuiButton* back_button;
    bool viewing_folders;
    std::vector<string> folder_names_ordered;
    std::map<string, std::vector<string> > scenarios_by_folder;

    void showFolderList();
    void showScenarioList(const string& folder_path);
public:
    ServerCreationScreen();

private:
    void selectScenario(string filename);
    void startScenario();   //Server only
};

#endif//SERVER_CREATION_SCREEN_H
