#include "main.h"
#include "mine.h"
#include "playerInfo.h"
#include "particleEffect.h"
#include "explosionEffect.h"
#include "pathPlanner.h"
#include "spaceObjects/missiles/missileWeapon.h"

#include "scriptInterface.h"

/// A mine object. Simple, effective, deadly.
REGISTER_SCRIPT_SUBCLASS(Mine, SpaceObject)
{
  // Set a function that will be called if the mine explodes.
  // First argument is the mine, second argument is the mine's owner/instigator (or nil).
  REGISTER_SCRIPT_CLASS_FUNCTION(Mine, onDestruction);
}

REGISTER_MULTIPLAYER_CLASS(Mine, "Mine");
Mine::Mine()
: SpaceObject(50, "Mine"), data(MissileWeaponData::getDataFor(MW_Mine))
{
    // Keep physics hull small; proximity uses trigger_range in checkProximityTrigger() (same reach as the old oversized hull).
    triggered = false;
    triggerTimeout = triggerDelay;
    ejectTimeout = 0.0;
    particleTimeout = 0.0;
    setRadarSignatureInfo(0.0, 0.05, 0.0);
    setDescriptions("Mine", "Mine");
    addInfos(0, "Type", "Mine");
    addInfos(1, "Proximity", "0,6 u");
    addInfos(2, "Delay", "1 sec");
    PathPlannerManager::getInstance()->addAvoidObject(this, trigger_range * 1.2f);
}

void Mine::draw3D()
{
}

void Mine::draw3DTransparent()
{
}

void Mine::drawOnRadar(sf::RenderTarget& window, sf::Vector2f position, float scale, float rotation, bool long_range)
{
    sf::Sprite objectSprite;
    textureManager.setTexture(objectSprite, "radar_mine.png");
    objectSprite.setRotation(getRotation());
    objectSprite.setPosition(position);
    objectSprite.setScale(0.3, 0.3);
    window.draw(objectSprite);
}

void Mine::drawOnGMRadar(sf::RenderTarget& window, sf::Vector2f position, float scale, float rotation, bool long_range)
{
    sf::CircleShape hitRadius(trigger_range * scale);
    hitRadius.setOrigin(trigger_range * scale, trigger_range * scale);
    hitRadius.setPosition(position);
    hitRadius.setFillColor(sf::Color::Transparent);
    if (triggered)
        hitRadius.setOutlineColor(sf::Color(255, 0, 0, 128));
    else
        hitRadius.setOutlineColor(sf::Color(255, 255, 255, 128));
    hitRadius.setOutlineThickness(3.0);
    window.draw(hitRadius);
}

void Mine::update(float delta)
{
    if (particleTimeout > 0)
    {
        particleTimeout -= delta;
    }else{
        sf::Vector3f pos = sf::Vector3f(getPosition().x, getPosition().y, getPositionZ());
        ParticleEngine::spawn(pos, pos + sf::Vector3f(random(-100, 100), random(-100, 100), random(-100, 100)), sf::Vector3f(1, 1, 1), sf::Vector3f(0, 0, 1), 30, 0, 10.0);
        particleTimeout = 0.4;
    }

    if (ejectTimeout > 0.0)
    {
        ejectTimeout -= delta;
        setVelocity(sf::vector2FromAngle(getRotation()) * data.speed);
    }else{
        setVelocity(sf::Vector2f(0, 0));
    }
    if (position_z < 0)
        setPositionZ(getPositionZ() + 0.5);
    if (position_z > 0)
        setPositionZ(getPositionZ() - 0.5);
    // Proximity (including missiles) only after deployment; during tube eject the fuse is not active.
    if (game_server && !triggered && ejectTimeout <= 0.0f)
        checkProximityTrigger();
    if (!triggered)
        return;
    triggerTimeout -= delta;
    if (triggerTimeout <= 0)
    {
        explode();
    }
}

void Mine::collide(Collisionable* target, float force)
{
    if (!game_server || triggered || ejectTimeout > 0.0)
        return;
    P<SpaceObject> hitObject = P<Collisionable>(target);
    if (!hitObject)
        return;
    if (!hitObject->canBeTargetedBy(nullptr) && !P<MissileWeapon>(hitObject))
        return;

    triggered = true;
}

void Mine::eject()
{
    ejectTimeout = data.lifetime;
}

void Mine::explode()
{
    DamageInfo info(owner, DT_Kinetic, getPosition());
    SpaceObject::damageArea(getPosition(), blastRange, damageAtEdge, damageAtCenter, info, blastRange / 2.0);

    P<ExplosionEffect> e = new ExplosionEffect();
    e->setSize(blastRange);
    e->setPosition(getPosition());
    e->setOnRadar(true);
    e->setRadarSignatureInfo(0.0, 0.0, 0.2);

    if (on_destruction.isSet())
    {
        if (info.instigator)
        {
            on_destruction.call(P<Mine>(this), P<SpaceObject>(info.instigator));
        }else{
            on_destruction.call(P<Mine>(this));
        }
    }
    destroy();
}

void Mine::onDestruction(ScriptSimpleCallback callback)
{
    this->on_destruction = callback;
}

void Mine::checkProximityTrigger()
{
    foreach(SpaceObject, target, space_object_list)
    {
        if (target == this)
            continue;
        P<SpaceObject> hitObject = target;
        if (!hitObject)
            continue;
        // In-flight missiles must arm the fuse when crossing the proximity ring (same as the old large hull),
        // even when "all can be targeted" is off — physics did not use that flag.
        if (!hitObject->canBeTargetedBy(nullptr) && !P<MissileWeapon>(hitObject))
            continue;
        const float dist = sf::length(target->getPosition() - getPosition());
        if (dist >= trigger_range + target->getRadius())
            continue;
        triggered = true;
        return;
    }
}
