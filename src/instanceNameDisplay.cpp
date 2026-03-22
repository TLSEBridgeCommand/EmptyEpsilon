#include "instanceNameDisplay.h"
#include "preferenceManager.h"
#include "main.h"
#include "windowManager.h"

#include <cctype>
#include <cstdlib>

const char* kPrefsInstanceNameIndexOffset = "instance_name_index_offset";
const char* kPrefsAuto2S1uClientOffset = "2s1u_client_auto";

bool broadcastServerNameIs2S1UMissionLayout(const string& server_name)
{
    // string::find returns int (-1 if not found), not size_t/npos.
    return server_name.lower().find("2s1u") >= 0;
}

void applyAuto2S1uSessionDisplayOffset(const string& server_broadcast_name)
{
    if (PreferencesManager::get(kPrefsAuto2S1uClientOffset, "0") != "1")
        return;
    if (!broadcastServerNameIs2S1UMissionLayout(server_broadcast_name))
        return;
    PreferencesManager::set(kPrefsInstanceNameIndexOffset, "2");
    refreshWindowTitleWithInstanceDisplay();
}

static bool isAllDigits(const string& s)
{
    if (s.length() < 1)
        return false;
    for (unsigned n = 0; n < s.length(); n++)
    {
        if (!std::isdigit(static_cast<unsigned char>(s[n])))
            return false;
    }
    return true;
}

void clearInstanceNameIndexOffsetOnSessionEnd()
{
    PreferencesManager::set(kPrefsInstanceNameIndexOffset, "0");
}

int getInstanceNameIndexOffset()
{
    int v = PreferencesManager::get(kPrefsInstanceNameIndexOffset, "0").toInt();
    if (v < 0)
        v = 0;
    if (v > 2)
        v = 2;
    return v;
}

string formatInstanceNameForDisplay(const string& raw_instance_name)
{
    if (!isAllDigits(raw_instance_name))
        return raw_instance_name;
    int base = int(strtol(raw_instance_name.c_str(), nullptr, 10));
    return string(base + getInstanceNameIndexOffset());
}

void refreshWindowTitleWithInstanceDisplay()
{
    if (PreferencesManager::get("headless") != "")
        return;
    P<WindowManager> window_manager = engine->getObject("windowManager");
    if (!window_manager)
        return;
    string iname = PreferencesManager::get("instance_name", "");
    if (iname != "")
        window_manager->setTitle("EmptyEpsilon - " + formatInstanceNameForDisplay(iname));
    else
        window_manager->setTitle("EmptyEpsilon");
}
