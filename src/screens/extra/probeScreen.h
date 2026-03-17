#ifndef PROBE_SCREEN_H
#define PROBE_SCREEN_H

#include "engine.h"
#include "screenComponents/targetsContainer.h"
#include "gui/gui2_overlay.h"
#include "spaceObjects/scanProbe.h"

class GuiViewport3D;

class ProbeScreen : public GuiOverlay, public Updatable
{
private:
    GuiOverlay* background_crosses;
    GuiViewport3D* viewport;
    float angle;
    float rotatetime;
    P<ScanProbe> probe;
public:
    ProbeScreen(GuiContainer* owner);

    virtual void update(float delta) override;

    virtual void onHotkey(const HotkeyResult& key) override;

};

#endif//PROBE_SCREEN_H
