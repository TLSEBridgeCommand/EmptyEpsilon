#include "briefingScreen.h"
#include "textureManager.h"
#include "gui/colorConfig.h"
#include "gameGlobalInfo.h"

BriefingScreen::BriefingScreen(GuiContainer* owner)
: GuiOverlay(owner, "BRIEFING_SCREEN", colorConfig.background)
{
    std::cout << "BriefingScreen constructor start" << std::endl;
    GuiOverlay *background_crosses = new GuiOverlay(this, "BACKGROUND_CROSSES", sf::Color::White);
    background_crosses->setTextureTiled("gui/BackgroundCrosses");
    title = new GuiLabel(this, "briefing_title", "Mission Briefing", 35);
    title->setPosition(120, 30, ATopLeft);

    button_prev = new GuiButton(this, "briefing_prev", "Previous", [this]() {
        if (current_page > 0) {
            current_page--;
            updateImage();
        }
    });
    button_prev->setPosition(50, -50, ABottomLeft);
    button_prev->setVisible(false);
    button_prev->setSize(300, 50);

    button_next = new GuiButton(this, "briefing_next", "Next", [this]() {
        if (current_page + 1 < (int)images.size()) {
            current_page++;
            updateImage();
        }
    });
    button_next->setPosition(-50, -50, ABottomRight);
    button_next->setVisible(false);
    button_next->setSize(300, 50);

    current_page = 0;
    if (gameGlobalInfo && !gameGlobalInfo->globalBriefingPages.empty()) {
        setBriefing(gameGlobalInfo->globalBriefingPages);
    }
    std::cout << "NB: BriefingScreen constructed" << std::endl;
}

void BriefingScreen::setBriefing(const std::vector<std::string>& paths)
{
    std::cout << "NB: setBriefing called" << std::endl;

    page_paths = paths;
    current_page = 0;

    for (GuiImage* img : images) {
        img->setVisible(false);
        // delete img;
    }
    // images.clear();

    for (size_t i = 0; i < page_paths.size(); ++i) {
        GuiImage* img = new GuiImage(this, "briefing_image_" + std::to_string(i), page_paths[i]);
        img->setPosition(100, 100);
        img->setSize(1200, 900);
        img->setVisible(false);
        images.push_back(img);
    }

    updateImage();

    std::cout << "NB: setBriefing called" << std::endl;

}

void BriefingScreen::updateImage()
{
    std::cout << "NB: updateImage called, current page: " << current_page << std::endl;

    if (images.empty()) {
        button_prev->setVisible(false);
        button_next->setVisible(false);
        return;
    }

    for (size_t i = 0; i < images.size(); ++i) {
        images[i]->setVisible(i == static_cast<size_t>(current_page));
    }

    button_prev->setVisible(current_page > 0);
    button_next->setVisible(current_page + 1 < (int)images.size());
}