#include "tractorBeam.h"
#include "spaceObjects/spaceship.h"
#include "spaceObjects/playerSpaceship.h"
#include "spaceObjects/beamEffect.h"
#include "spaceObjects/spaceObject.h"
#include <math.h>

TractorBeam::TractorBeam() : max_area(0), drag_per_second(0), parent(nullptr), mode(TBM_Off), arc(0), direction(0), range(0) {}

void TractorBeam::setParent(SpaceShip* parent)
{
    assert(!this->parent);
    this->parent = parent;

    parent->registerMemberReplication(&max_area);
    parent->registerMemberReplication(&drag_per_second);
    parent->registerMemberReplication(&arc);
    parent->registerMemberReplication(&direction);
    parent->registerMemberReplication(&range);
    parent->registerMemberReplication(&mode);
    parent->registerMemberReplication(&targetInRange);
}

void TractorBeam::setMode(ETractorBeamMode mode)
{
    this->mode = mode;
}

ETractorBeamMode TractorBeam::getMode()
{
    return mode;
}

void TractorBeam::setMaxArea(float max_area)
{
    this->max_area = max_area;
}

void TractorBeam::setMaxRange(float max_range)
{
    this->max_area = (max_range * max_range * M_PI * 6.0) / 360;
}

float TractorBeam::getMaxArea()
{
    return max_area;
}

void TractorBeam::setDragPerSecond(float drag_per_second)
{
    this->drag_per_second = drag_per_second;
}

float TractorBeam::getDragPerSecond()
{
    return drag_per_second;
}

float TractorBeam::getMaxRange(float arc)
{
    // M_PI * range * range * arc / 360 <= max_area
    return sqrtf((max_area * 360) / (M_PI * std::max(1.0f, arc)));
}

void TractorBeam::setArc(float arc)
{
    this->arc = arc;
}

float TractorBeam::getArc()
{
    return arc;
}

void TractorBeam::setDirection(float direction)
{
     while(direction < 0)
        direction += 360;
    while(direction > 360)
        direction -= 360;
    this->direction = direction;
}

float TractorBeam::getDirection()
{
    return direction;
}


float TractorBeam::getMaxArc(float range)
{
    // M_PI * range * range * arc / 360 <= max_area
    return (max_area * 360) / (M_PI * std::max(1.0f, range * range));
}
void TractorBeam::setRange(float range)
{
    this->range = range;
}

float TractorBeam::getRange()
{
    return range;
}

float TractorBeam::getDragSpeed()
{
    return getDragPerSecond() * parent->getSystemEffectiveness(SYS_Docks);
}

bool TractorBeam::isTargetInRange()
{
    return targetInRange;
}

void TractorBeam::update(float delta)
{
    if (game_server && mode > TBM_Off && range > 0.0 && delta > 0)
    {
        float dragCapability = delta * getDragSpeed();
        static constexpr float hold_drag_multiplier = 2.0f;
        bool tmpTrackingTarget = false;
        std::set<SpaceShip*> tmpTargets;
        foreach(SpaceObject, target, space_object_list)
        {
            if (target != parent) {
                // Get the angle to the target.

                sf::Vector2f diff = target->getPosition() - parent->getPosition();
                const float dist_parent_target = sf::length(diff);
                float angle_diff = fabsf(sf::angleDifference(direction + parent->getRotation(), sf::vector2ToAngle(diff)));

                // Target must be within range and inside the beam arc (same volume for all modes including Hold).
                const bool in_tractor_volume = dist_parent_target < range && angle_diff < arc / 2.0f;
                if (in_tractor_volume)
                {
                    tmpTrackingTarget = true;
                    sf::Vector2f destination;
                    switch(mode) {
                        case TBM_Pull : 
                            destination = parent->getPosition();
                            break;
                        case TBM_Push :
                            destination = parent->getPosition() + normalize(target->getPosition() - parent->getPosition()) * (range * 2);
                            break;
                        case TBM_Hold :
                        {
                            // Near the far end of the beam along the cone axis, but slightly inward (not flush on the limit).
                            const sf::Vector2f beam = sf::vector2FromAngle(direction + parent->getRotation());
                            const float edge_inset = std::max(1.0f, target->getRadius() + 1.0f);
                            static constexpr float hold_inward_margin = 80.0f;
                            const float along = std::max(0.f, range - edge_inset - hold_inward_margin);
                            destination = parent->getPosition() + beam * along;
                        }
                            break;
                        case TBM_Off :
                        default:
                            break;
                    }
                    // SpaceObject* targetPtr = &target;
                    // if(mode != TBM_Off && (SpaceShip* shipPtr = dynamic_cast<SpaceShip*>(targetPtr)))
                    // {
                    //     shipPtr->addAsTractorBeamTargeter(mode);
                    //     tmpTargets.insert(shipPtr);
                    // }
                    const float ideal_sep = parent->getRadius() + target->getRadius();
                    diff = target->getPosition() - destination;
                    const float dlen = sf::length(diff);
                    float target_distance;
                    if (mode == TBM_Hold)
                        target_distance = dlen;
                    else
                        target_distance = std::max(0.0f, dlen - ideal_sep);

                    float effective_cap = dragCapability;
                    if (mode == TBM_Hold)
                        effective_cap *= hold_drag_multiplier;
                    float distanceToDrag = std::min(target_distance, effective_cap);
                    if (parent->useEnergy(parent->systems[SYS_Docks].power_user_factor)) //uses cargo docks energy usage // ~~ sweet tractor beam ooh ooh ooh ~~ 
                    {
                        P<PlayerSpaceship> target_ship = target;
                        if (target_distance < dragCapability && target_ship && mode == TBM_Pull)
                        {
                            // if tractor beam is dragging a ship into parent, force docking
                            target_ship->requestDock(parent);
                        }
                        distanceToDrag *= (100 / target->getRadius());
                        if (dlen > 0.001f)
                            target->setPosition(target->getPosition() - (distanceToDrag * normalize(diff)));
                    }
                }
            }
        }
        targetInRange = tmpTrackingTarget;
        parent->forceMemberReplicationUpdate(&targetInRange);
        // for(auto it = targets.begin(); it != targets.end(); ++it)
        // {
        //     bool tmpInTmpTargets = false;
        //     for(auto tmpIt = tmpTargets.begin(); tmpIt != tmpTargets.end(); ++tmpIt)
        //     {
        //         if(*it == *tmpIt)
        //         {
        //             tmpInTmpTargets = true;
        //             break;
        //         }
        //     }
        //     if(!tmpInTmpTargets)
        //     {
        //         it->removeAsTractorBeamTargeter(this);
        //         targets.erase(it);
        //     }
        // }
    }
}

string getTractorBeamModeName(ETractorBeamMode mode)
{
    switch(mode)
    {
    case TBM_Off:
        return "Off";
    case TBM_Pull:
        return "Pull";
    case TBM_Push:
        return "Push";
    case TBM_Hold:
        return "Hold";
    default:
        return "UNK: " + string(int(mode));
    }
}

#ifndef _MSC_VER
// MFC: GCC does proper external template instantiation, VC++ doesn't.
#include "tractorBeam.hpp"
#endif
