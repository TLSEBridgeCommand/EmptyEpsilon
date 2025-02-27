#include "main.h"
#include "invisibleMine.h"
// #include "playerInfo.h"
#include "particleEffect.h"
#include "explosionEffect.h"
// #include "pathPlanner.h"

#include "scriptInterface.h"


/// An InvisibleMine. Should explode and do damage but not drawn opside of GM radar. Not for player use.
REGISTER_SCRIPT_SUBCLASS(InvisibleMine, SpaceObject)
{
  // Set a function that will be called if the InvisibleMine explodes.
  // First argument is the InvisibleMine, second argument is the InvisibleMine's owner/instigator (or nil).
  REGISTER_SCRIPT_CLASS_FUNCTION(InvisibleMine, onDestruction);
}

REGISTER_MULTIPLAYER_CLASS(InvisibleMine, "InvisibleMine");
InvisibleMine::InvisibleMine()
: SpaceObject(50, "InvisibleMine")
{
    setCollisionRadius(trigger_range);
    triggered = false;
    triggerTimeout = triggerDelay;
    // ejectTimeout = 0.0;
    particleTimeout = 0.0;
    setRadarSignatureInfo(0.0, 0.05, 0.0);

    // PathPlannerManager::getInstance()->addAvoidObject(this, trigger_range * 1.2f);
}

void InvisibleMine::draw3D()
{
}

void InvisibleMine::draw3DTransparent()
{
}

void InvisibleMine::drawOnRadar(sf::RenderTarget& window, sf::Vector2f position, float scale, float rotation, bool long_range)
{
    // sf::Sprite objectSprite;
    // textureManager.setTexture(objectSprite, "RadarBlip.png");
    // objectSprite.setRotation(getRotation());
    // objectSprite.setPosition(position);
    // objectSprite.setScale(0.3, 0.3);
    // window.draw(objectSprite);
}

void InvisibleMine::drawOnGMRadar(sf::RenderTarget& window, sf::Vector2f position, float scale, float rotation, bool long_range)
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

void InvisibleMine::update(float delta)
{
    if (particleTimeout > 0)
    {
        particleTimeout -= delta;
    }else{
        sf::Vector3f pos = sf::Vector3f(getPosition().x, getPosition().y, getPositionZ());
        ParticleEngine::spawn(pos, pos + sf::Vector3f(random(-100, 100), random(-100, 100), random(-100, 100)), sf::Vector3f(1, 1, 1), sf::Vector3f(0, 0, 1), 30, 0, 10.0);
        particleTimeout = 0.4;
    }

    // if (ejectTimeout > 0.0)
    // {
    //     ejectTimeout -= delta;
    //     setVelocity(sf::vector2FromAngle(getRotation()) * data.speed);
    // }else{
    //     setVelocity(sf::Vector2f(0, 0));
    // }
    // if (position_z < 0)
    //     setPositionZ(getPositionZ() + 0.5);
    // if (position_z > 0)
    //     setPositionZ(getPositionZ() - 0.5);
    if (!triggered)
        return;
    triggerTimeout -= delta;
    if (triggerTimeout <= 0)
    {
        explode();
    }
}

void InvisibleMine::collide(Collisionable* target, float force)
{
    if (!game_server || triggered)
        return;
    P<SpaceObject> hitObject = P<Collisionable>(target);
    if (!hitObject || !hitObject->canBeTargetedBy(nullptr))
        return;

    triggered = true;
}

// void InvisibleMine::eject()
// {
//     ejectTimeout = data.lifetime;
// }

void InvisibleMine::explode()
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
            on_destruction.call(P<InvisibleMine>(this), P<SpaceObject>(info.instigator));
        }
        else
        {
            on_destruction.call(P<InvisibleMine>(this));
        }
    }
    destroy();
}

void InvisibleMine::onDestruction(ScriptSimpleCallback callback)
{
    this->on_destruction = callback;
}