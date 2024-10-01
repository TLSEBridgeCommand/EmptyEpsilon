#include <i18n.h>
#include "fireLock.h"

#include "playerInfo.h"
#include "spaceObjects/playerSpaceship.h"

FireLockButton::FireLockButton(GuiContainer* owner, string id, P<PlayerSpaceship> targetSpaceship)
: GuiToggleButton(owner, id, tr("missile","Beams"), [this](bool value){setFireLock(value);}), target_spaceship(targetSpaceship)
{
    setValue(true);
    setIcon("gui/icons/lock");
}

void FireLockButton::onDraw(sf::RenderTarget& window)
{
    if (target_spaceship)
    {
        setValue(target_spaceship->lock_fire);
    }
    GuiButton::onDraw(window);
}

void FireLockButton::onHotkey(const HotkeyResult& key)
{
    if (key.category == "WEAPONS" && target_spaceship)
    {
        if (key.hotkey == "TOGGLE_FIRE_LOCK")
        {
            setFireLock(!getValue());
            setValue(!getValue());
        }
        if (key.hotkey == "ENABLE_FIRE_LOCK")
        {
            setFireLock(true);
            setValue(true);
        }
        if (key.hotkey == "DISABLE_FIRE_LOCK")
        {
            setFireLock(false);
            setValue(false);
        }
    }
}

void FireLockButton::setFireLock(bool value)
{
    if (target_spaceship)
	target_spaceship->commandLockFire(value);
}
