#ifndef INSTANCE_NAME_DISPLAY_H
#define INSTANCE_NAME_DISPLAY_H

#include "stringImproved.h"

/** Stored preference: add this to purely numeric instance_name for on-screen / title (0 or 2). Set by autoconnect when 2s1u_client_auto matches; cleared on disconnect / exit. */
extern const char* kPrefsInstanceNameIndexOffset;
/** If "1", autoconnect adds +2 to ship slot and display offset when joined server's broadcast name contains 2S1U (same exe; enable only on second-ship clients). */
extern const char* kPrefsAuto2S1uClientOffset;

bool broadcastServerNameIs2S1UMissionLayout(const string& server_name);

/** When auto flag is on and server name matches, set session display offset +2 and refresh title. */
void applyAuto2S1uSessionDisplayOffset(const string& server_broadcast_name);

void clearInstanceNameIndexOffsetOnSessionEnd();

int getInstanceNameIndexOffset();

/** Numeric instance_name only: adds offset; non-numeric names unchanged. */
string formatInstanceNameForDisplay(const string& raw_instance_name);

void refreshWindowTitleWithInstanceDisplay();

#endif
