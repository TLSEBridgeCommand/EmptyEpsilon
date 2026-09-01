#include <i18n.h>
#include "engine.h"
#include "optionsMenu.h"
#include "main.h"
#include "preferenceManager.h"
#include "instanceNameDisplay.h"
#include "playerInfo.h"

#include "gui/gui2_autolayout.h"
#include "gui/gui2_overlay.h"
#include "gui/gui2_button.h"
#include "gui/gui2_togglebutton.h"
#include "gui/gui2_selector.h"
#include "gui/gui2_label.h"
#include "gui/gui2_slider.h"
#include "gui/gui2_listbox.h"
#include "gui/gui2_keyvaluedisplay.h"
#include "gui/gui2_textentry.h"

#include <algorithm>
#include <functional>
#include <set>
#include <vector>

namespace
{
/**
 * Standard on/off row for boolean options: title, then [Enabled] [Disabled].
 * The Enabled button is highlighted when \a enabled is true.
 */
void addEnabledDisabledRow(GuiAutoLayout* parent, const string& id_prefix, const string& title, bool enabled,
    std::function<void(bool)> onEnabledChanged)
{
    (new GuiLabel(parent, id_prefix + "_TITLE", title, 30))->addBackground()->setSize(GuiElement::GuiSizeMax, 50);
    // LayoutVerticalColumns splits width evenly; HorizontalLeftToRight leaves both buttons at
    // GuiSizeMax width so they overlap and only the top one receives clicks.
    GuiElement* row = new GuiAutoLayout(parent, id_prefix + "_ROW", GuiAutoLayout::LayoutVerticalColumns);
    row->setSize(GuiElement::GuiSizeMax, 50);
    struct BtnPair
    {
        GuiButton* btn_enabled = nullptr;
        GuiButton* btn_disabled = nullptr;
        void sync(bool on)
        {
            btn_enabled->setActive(on);
            btn_disabled->setActive(!on);
        }
    };
    BtnPair* pair = new BtnPair();
    const string lbl_enabled = tr("options", "Enabled");
    const string lbl_disabled = tr("options", "Disabled");
    pair->btn_enabled = new GuiButton(row, id_prefix + "_ENABLED", lbl_enabled, [pair, onEnabledChanged]()
    {
        onEnabledChanged(true);
        pair->sync(true);
    });
    pair->btn_disabled = new GuiButton(row, id_prefix + "_DISABLED", lbl_disabled, [pair, onEnabledChanged]()
    {
        onEnabledChanged(false);
        pair->sync(false);
    });
    pair->btn_enabled->setSize(GuiElement::GuiSizeMax, 50);
    pair->btn_disabled->setSize(GuiElement::GuiSizeMax, 50);
    pair->sync(enabled);
}
} // namespace

OptionsMenu::OptionsMenu()
{
    P<WindowManager> windowManager = engine->getObject("windowManager");

    new GuiOverlay(this, "", colorConfig.background);
    (new GuiOverlay(this, "", sf::Color::White))->setTextureTiled("gui/BackgroundCrosses");

    // Pager across the top; both columns below hold option pages (no soundtrack preview).
    options_pager = new GuiSelector(this, "OPTIONS_PAGER", [this](int index, string value)
    {
        graphics_page->setVisible(index == 0);
        audio_page->setVisible(index == 1);
        interface_page->setVisible(index == 2);
        autoconnect_page->setVisible(index == 3);
        autoconnect_right_page->setVisible(index == 3);
    });
    options_pager->setOptions({tr("Graphics options"), tr("Audio options"), tr("Interface options"), tr("options", "Autoconnect")})->setSelectionIndex(0);
    options_pager->setPosition(0, 50, ATopCenter)->setSize(1100, 50);

    left_container = new GuiAutoLayout(this, "OPTIONS_LEFT_CONTAINER", GuiAutoLayout::LayoutVerticalTopToBottom);
    left_container->setPosition(50, 120, ATopLeft)->setSize(700, GuiElement::GuiSizeMax);

    right_container = new GuiAutoLayout(this, "OPTIONS_RIGHT_CONTAINER", GuiAutoLayout::LayoutVerticalTopToBottom);
    right_container->setPosition(-50, 120, ATopRight)->setSize(700, GuiElement::GuiSizeMax);

    graphics_page = new GuiAutoLayout(left_container, "OPTIONS_GRAPHICS", GuiAutoLayout::LayoutVerticalTopToBottom);
    graphics_page->setPosition(0, 0, ATopLeft)->setSize(GuiElement::GuiSizeMax, GuiElement::GuiSizeMax)->show();
    audio_page = new GuiAutoLayout(left_container, "OPTIONS_AUDIO", GuiAutoLayout::LayoutVerticalTopToBottom);
    audio_page->setPosition(0, 0, ATopLeft)->setSize(GuiElement::GuiSizeMax, GuiElement::GuiSizeMax)->hide();
    interface_page = new GuiAutoLayout(left_container, "OPTIONS_INTERFACE", GuiAutoLayout::LayoutVerticalTopToBottom);
    interface_page->setPosition(0, 0, ATopLeft)->setSize(GuiElement::GuiSizeMax, GuiElement::GuiSizeMax)->hide();
    autoconnect_page = new GuiAutoLayout(left_container, "OPTIONS_AUTOCONNECT", GuiAutoLayout::LayoutVerticalTopToBottom);
    autoconnect_page->setPosition(0, 0, ATopLeft)->setSize(GuiElement::GuiSizeMax, GuiElement::GuiSizeMax)->hide();
    autoconnect_right_page = new GuiAutoLayout(right_container, "OPTIONS_AUTOCONNECT_RIGHT", GuiAutoLayout::LayoutVerticalTopToBottom);
    autoconnect_right_page->setPosition(0, 0, ATopLeft)->setSize(GuiElement::GuiSizeMax, GuiElement::GuiSizeMax)->hide();

    // Graphics options
    // Fullscreen toggle.
    (new GuiButton(graphics_page, "FULLSCREEN_TOGGLE", tr("Fullscreen toggle"), []()
    {
        P<WindowManager> windowManager = engine->getObject("windowManager");
        windowManager->setFullscreen(!windowManager->isFullscreen());
    }))->setSize(GuiElement::GuiSizeMax, 50);

    // FSAA configuration.
    int fsaa = std::max(1, windowManager->getFSAA());
    int fsaa_index = 0;

    // Convert selector index to an FSAA amount.
    switch(fsaa)
    {
        case 8: fsaa_index = 3; break;
        case 4: fsaa_index = 2; break;
        case 2: fsaa_index = 1; break;
        default: fsaa_index = 0; break;
    }

    // FSAA selector.
    (new GuiSelector(graphics_page, "FSAA", [](int index, string value)
    {
        P<WindowManager> windowManager = engine->getObject("windowManager");
        static const int fsaa[] = { 0, 2, 4, 8 };
        windowManager->setFSAA(fsaa[index]);
    }))->setOptions({"FSAA: off", "FSAA: 2x", "FSAA: 4x", "FSAA: 8x"})->setSelectionIndex(fsaa_index)->setSize(GuiElement::GuiSizeMax, 50);

    // Audio optionss
    // Sound volume slider.
    sound_volume_slider = new GuiSlider(audio_page, "SOUND_VOLUME_SLIDER", 0.0f, 100.0f, soundManager->getMasterSoundVolume(), [this](float volume)
    {
        soundManager->setMasterSoundVolume(volume);
        sound_volume_overlay_label->setText(tr("Sound Volume: {volume}%").format({{"volume", string(int(soundManager->getMasterSoundVolume()))}}));
    });
    sound_volume_slider->setSize(GuiElement::GuiSizeMax, 50);

    // Override overlay label.
    sound_volume_overlay_label = new GuiLabel(sound_volume_slider, "SOUND_VOLUME_SLIDER_LABEL", tr("Sound Volume: {volume}%").format({{"volume", string(int(soundManager->getMasterSoundVolume()))}}), 30);
    sound_volume_overlay_label->setSize(GuiElement::GuiSizeMax, GuiElement::GuiSizeMax);

    // Music playback state.
    (new GuiLabel(audio_page, "MUSIC_PLAYBACK_LABEL", tr("Music Playback"), 30))->addBackground()->setSize(GuiElement::GuiSizeMax, 50);

    // Determine when music is enabled.
    int music_enabled_index = PreferencesManager::get("music_enabled", "2").toInt();
    (new GuiSelector(audio_page, "MUSIC_ENABLED", [](int index, string value)
    {
        // 0: Always off
        // 1: Always on
        // 2: On if main screen, off otherwise (default)
        PreferencesManager::set("music_enabled", string(index));
    }))->setOptions({tr("Disabled"), tr("Enabled"), tr("Main Screen only")})->setSelectionIndex(music_enabled_index)->setSize(GuiElement::GuiSizeMax, 50);

    // Music volume slider.
    music_volume_slider = new GuiSlider(audio_page, "MUSIC_VOLUME_SLIDER", 0.0f, 100.0f, soundManager->getMusicVolume(), [this](float volume)
    {
        soundManager->setMusicVolume(volume);
        music_volume_overlay_label->setText(tr("Music Volume: {volume}%").format({{"volume", string(int(soundManager->getMusicVolume()))}}));
    });
    music_volume_slider->setSize(GuiElement::GuiSizeMax, 50);

    // Override overlay label.
    music_volume_overlay_label = new GuiLabel(music_volume_slider, "MUSIC_VOLUME_SLIDER_LABEL", tr("Music Volume: {volume}%").format({{"volume", string(int(soundManager->getMusicVolume()))}}), 30);
    music_volume_overlay_label->setSize(GuiElement::GuiSizeMax, GuiElement::GuiSizeMax);

    // Engine playback state.
    (new GuiLabel(audio_page, "IMPULSE_SOUND_LABEL", tr("Impulse Engine sound"), 30))->addBackground()->setSize(GuiElement::GuiSizeMax, 50);

    // Determine when engine sound effects are enabled.
    int impulse_enabled_index = PreferencesManager::get("impulse_sound_enabled", "2").toInt();
    (new GuiSelector(audio_page, "ENGINE_ENABLED", [](int index, string value)
    {
        // 0: Always off
        // 1: Always on
        // 2: On if main screen, off otherwise (default)
        PreferencesManager::set("impulse_sound_enabled", string(index));
    }))->setOptions({tr("Disabled"), tr("Enabled"), tr("Main Screen only")})->setSelectionIndex(impulse_enabled_index)->setSize(GuiElement::GuiSizeMax, 50);

    // Impulse engine volume slider.
    impulse_volume_slider = new GuiSlider(audio_page, "IMPULSE_VOLUME_SLIDER", 0.0f, 100.0f, PreferencesManager::get("impulse_sound_volume", "50").toInt(), [this](float volume)
    {
        PreferencesManager::set("impulse_sound_volume", volume);
        impulse_volume_overlay_label->setText(tr("Impulse Volume: {volume}%").format({{"volume", string(PreferencesManager::get("impulse_sound_volume", "50").toInt())}}));
    });
    impulse_volume_slider->setSize(GuiElement::GuiSizeMax, 50);

    // Override overlay label.
    impulse_volume_overlay_label = new GuiLabel(impulse_volume_slider, "IMPULSE_VOLUME_SLIDER_LABEL", tr("Impulse Volume: {volume}%").format({{"volume", string(PreferencesManager::get("impulse_sound_volume", "50").toInt())}}), 30);
    impulse_volume_overlay_label->setSize(GuiElement::GuiSizeMax, GuiElement::GuiSizeMax);

    // Interface options
    // Helms rotation lock.
    (new GuiToggleButton(interface_page, "HEMS_RADAR_LOCK", tr("Helms Radar Lock"), [](bool value)
    {
        PreferencesManager::set("helms_radar_lock", value ? "1" : "");
        PreferencesManager::set("tactical_radar_lock", value ? "1" : "");
        PreferencesManager::set("single_pilot_radar_lock", value ? "1" : "");
    }))->setValue(PreferencesManager::get("helms_radar_lock", "0") == "1")->setSize(GuiElement::GuiSizeMax, 50);

    // Helms rotation lock.
    (new GuiToggleButton(interface_page, "WEAPONS_RADAR_LOCK", tr("Weapons Radar Lock"), [](bool value)
    {
        PreferencesManager::set("weapons_radar_lock", value ? "1" : "");
    }))->setValue(PreferencesManager::get("weapons_radar_lock", "0") == "1")->setSize(GuiElement::GuiSizeMax, 50);

    // Helms rotation lock.
    (new GuiToggleButton(interface_page, "SCIENCE_RADAR_LOCK", tr("Science Radar Lock"), [](bool value)
    {
        PreferencesManager::set("science_radar_lock", value ? "1" : "");
        PreferencesManager::set("operations_radar_lock", value ? "1" : "");
    }))->setValue(PreferencesManager::get("science_radar_lock", "0") == "1")->setSize(GuiElement::GuiSizeMax, 50);

    // Default name shown on the LAN when this machine hosts a game (empty = "Server").
    {
        GuiElement* row = new GuiAutoLayout(interface_page, "", GuiAutoLayout::LayoutHorizontalLeftToRight);
        row->setSize(GuiElement::GuiSizeMax, 50);
        (new GuiLabel(row, "DEFAULT_SERVER_NAME_LABEL", tr("options", "Default host server name"), 30))->setAlignment(ACenterRight)->setSize(280, GuiElement::GuiSizeMax);
        (new GuiTextEntry(row, "DEFAULT_SERVER_NAME", PreferencesManager::get("server_name", "")))->callback([](string text) {
            PreferencesManager::set("server_name", text);
        })->setSize(GuiElement::GuiSizeMax, GuiElement::GuiSizeMax);
    }

    (new GuiToggleButton(interface_page, "AUTO_2S1U_CLIENT", tr("options", "On 2S1U +2 Ship Index"), [](bool value)
    {
        PreferencesManager::set(kPrefsAuto2S1uClientOffset, value ? "1" : "0");
    }))->setValue(PreferencesManager::get(kPrefsAuto2S1uClientOffset, "0") == "1")->setSize(GuiElement::GuiSizeMax, 50);

    // Autoconnect options — left column: role, main screen, stations
    (new GuiLabel(autoconnect_page, "AC_SECTION_PRIMARY", tr("options", "Primary Station"), 30))->addBackground()->setSize(GuiElement::GuiSizeMax, 50);
    {
        std::vector<string> ac_options;
        ac_options.push_back(tr("options", "Off"));
        for (int n = 0; n < max_crew_positions; n++)
            ac_options.push_back(getCrewPositionName(ECrewPosition(n)));

        int ac_sel = PreferencesManager::get("autoconnect", "0").toInt();
        if (ac_sel < 0 || ac_sel > max_crew_positions)
            ac_sel = 0;

        (new GuiSelector(autoconnect_page, "AUTOCONNECT_ROLE", [](int index, string value)
        {
            PreferencesManager::set("autoconnect", string(index));
        }))->setOptions(ac_options)->setSelectionIndex(ac_sel)->setSize(GuiElement::GuiSizeMax, 50);
    }

    (new GuiLabel(autoconnect_page, "AC_SECTION_STATIONS", tr("options", "Secondary Stations"), 30))->addBackground()->setSize(GuiElement::GuiSizeMax, 50);

    {
        struct AutostationSync
        {
            GuiListbox* list = nullptr;
            std::set<int> enabled;

            void save()
            {
                std::vector<int> nums(enabled.begin(), enabled.end());
                std::sort(nums.begin(), nums.end());
                string s;
                for (size_t i = 0; i < nums.size(); i++)
                {
                    if (i)
                        s += ",";
                    s += string(nums[i]);
                }
                PreferencesManager::set("autostationslist", s);
            }

            void refreshEntryNames()
            {
                if (!list)
                    return;
                for (int n = 0; n < max_crew_positions; n++)
                {
                    const int station = n + 1;
                    string label = getCrewPositionName(ECrewPosition(n));
                    if (enabled.find(station) != enabled.end())
                        label = tr("options", "[On] {station}").format({{"station", label}});
                    else
                        label = tr("options", "[Off] {station}").format({{"station", label}});
                    list->setEntryName(n, label);
                }
            }

            void toggle(int station_1_based)
            {
                if (station_1_based < 1 || station_1_based > max_crew_positions)
                    return;
                if (enabled.find(station_1_based) != enabled.end())
                    enabled.erase(station_1_based);
                else
                    enabled.insert(station_1_based);
                save();
                refreshEntryNames();
            }
        };

        AutostationSync* sync = new AutostationSync();
        for (string part : PreferencesManager::get("autostationslist", "").split(","))
        {
            int t = part.strip().toInt();
            if (t >= 1 && t <= max_crew_positions)
                sync->enabled.insert(t);
        }

        GuiListbox* station_list = new GuiListbox(autoconnect_page, "AUTO_STATION_LIST", [sync](int index, string value)
        {
            sync->toggle(value.toInt());
            // Keep list selection on the row just toggled for feedback.
            if (sync->list)
                sync->list->setSelectionIndex(index);
        });
        sync->list = station_list;
        station_list->setTextSize(28)->setButtonHeight(45);
        station_list->setSize(GuiElement::GuiSizeMax, 480);

        for (int n = 0; n < max_crew_positions; n++)
            station_list->addEntry(getCrewPositionName(ECrewPosition(n)), string(n + 1));
        sync->refreshEntryNames();
    }

    // Autoconnect options — right column: connection details
    (new GuiLabel(autoconnect_right_page, "AC_SECTION_MORE", tr("options", "Connection details"), 30))->addBackground()->setSize(GuiElement::GuiSizeMax, 50);
    {
        GuiElement* row = new GuiAutoLayout(autoconnect_right_page, "", GuiAutoLayout::LayoutHorizontalLeftToRight);
        row->setSize(GuiElement::GuiSizeMax, 50);
        (new GuiLabel(row, "AC_SHIP_IDX_LABEL", tr("options", "Ship index (1-based)"), 30))->setAlignment(ACenterRight)->setSize(200, GuiElement::GuiSizeMax);
        (new GuiTextEntry(row, "AC_SHIP_IDX", PreferencesManager::get("autoconnect_ship_index", "1")))->callback([](string text)
        {
            PreferencesManager::set("autoconnect_ship_index", text.strip());
        })->setSize(GuiElement::GuiSizeMax, GuiElement::GuiSizeMax);
    }
    {
        GuiElement* row = new GuiAutoLayout(autoconnect_right_page, "", GuiAutoLayout::LayoutHorizontalLeftToRight);
        row->setSize(GuiElement::GuiSizeMax, 50);
        (new GuiLabel(row, "AC_SHIP_FILTER_LABEL", tr("options", "Ship filter (autoconnectship)"), 30))->setAlignment(ACenterRight)->setSize(200, GuiElement::GuiSizeMax);
        (new GuiTextEntry(row, "AC_SHIP_FILTER", PreferencesManager::get("autoconnectship", "solo")))->callback([](string text)
        {
            PreferencesManager::set("autoconnectship", text);
        })->setSize(GuiElement::GuiSizeMax, GuiElement::GuiSizeMax);
    }
    {
        GuiElement* row = new GuiAutoLayout(autoconnect_right_page, "", GuiAutoLayout::LayoutHorizontalLeftToRight);
        row->setSize(GuiElement::GuiSizeMax, 50);
        (new GuiLabel(row, "AC_ADDR_LABEL", tr("options", "Server address (optional)"), 30))->setAlignment(ACenterRight)->setSize(200, GuiElement::GuiSizeMax);
        (new GuiTextEntry(row, "AC_ADDR", PreferencesManager::get("autoconnect_address", "")))->callback([](string text)
        {
            PreferencesManager::set("autoconnect_address", text);
        })->setSize(GuiElement::GuiSizeMax, GuiElement::GuiSizeMax);
    }
    {
        GuiElement* row = new GuiAutoLayout(autoconnect_right_page, "", GuiAutoLayout::LayoutHorizontalLeftToRight);
        row->setSize(GuiElement::GuiSizeMax, 50);
        (new GuiLabel(row, "AC_TRY_SEC_LABEL", tr("options", "Server try interval (seconds)"), 30))->setAlignment(ACenterRight)->setSize(200, GuiElement::GuiSizeMax);
        (new GuiTextEntry(row, "AC_TRY_SEC", PreferencesManager::get("autoconnect_server_try_seconds", "10")))->callback([](string text)
        {
            PreferencesManager::set("autoconnect_server_try_seconds", text.strip());
        })->setSize(GuiElement::GuiSizeMax, GuiElement::GuiSizeMax);
    }

    addEnabledDisabledRow(autoconnect_right_page, "AC_MULTI_SERVER", tr("options", "Multi-server name rotation"),
        PreferencesManager::get("multi_server_mode", "0").toInt() > 0,
        [](bool enabled)
        {
            PreferencesManager::set("multi_server_mode", enabled ? "1" : "0");
        });

    addEnabledDisabledRow(autoconnect_right_page, "AC_CODE_BYPASS", tr("options", "Bypass ship control code"),
        PreferencesManager::get("autoconnect_control_code_bypass", "0") == "1",
        [](bool enabled)
        {
            PreferencesManager::set("autoconnect_control_code_bypass", enabled ? "1" : "0");
        });

    addEnabledDisabledRow(autoconnect_right_page, "AC_NUMERIC_PAD", tr("options", "Prefer numeric pad for control code"),
        PreferencesManager::get("autoconnect_control_code_prefer_numeric_pad", "0").toInt() > 0,
        [](bool enabled)
        {
            PreferencesManager::set("autoconnect_control_code_prefer_numeric_pad", enabled ? "1" : "0");
        });

    (new GuiLabel(autoconnect_right_page, "AC_HINT_RESTART", tr("options", "Restart the game to run autoconnect from options."), 20))->setSize(GuiElement::GuiSizeMax, 36);

    // Bottom GUI.
    // Back button.
    (new GuiButton(this, "BACK", tr("options", "Back"), [this]()
    {
        // Close this menu and return to the main menu.
        destroy();
        soundManager->stopMusic();
        returnToMainMenu();
    }))->setPosition(50, -50, ABottomLeft)->setSize(150, 50);
    // Save options button.
    (new GuiButton(this, "SAVE_OPTIONS", tr("options", "Save"), [this]()
    {
        if (getenv("HOME"))
            PreferencesManager::save(string(getenv("HOME")) + "/.emptyepsilon/options.ini");
        else
            PreferencesManager::save("options.ini");
    }))->setPosition(200, -50, ABottomLeft)->setSize(150, 50);
}

void OptionsMenu::onKey(sf::Event::KeyEvent key, int unicode)
{
    switch(key.code)
    {
    //TODO: This is more generic code and is duplicated.
    case sf::Keyboard::Escape:
    case sf::Keyboard::Home:
        destroy();
        soundManager->stopMusic();
        returnToMainMenu();
        break;
    default:
        break;
    }
}
