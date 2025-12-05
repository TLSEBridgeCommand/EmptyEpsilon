#include <SFML/OpenGL.hpp>

#include "main.h"
#include "nebula.h"
#include "playerInfo.h"

#include "scriptInterface.h"

/// Nebulae block long-range radar in a 5U range.
REGISTER_SCRIPT_SUBCLASS(Nebula, SpaceObject)
{
    REGISTER_SCRIPT_CLASS_FUNCTION(Nebula, setEffectRadius);
    REGISTER_SCRIPT_CLASS_FUNCTION(Nebula, setTexture); 
}

PVector<Nebula> Nebula::nebula_list;

REGISTER_MULTIPLAYER_CLASS(Nebula, "Nebula")
Nebula::Nebula()
: SpaceObject(5000, "Nebula"), custom_texture("") 
{
    // Nebulae need a large radius to render properly from a distance, but
    // collision isn't important, so set the collision radius to a tiny range.
    setCollisionRadius(1);
    setRotation(random(0, 360));
    radar_visual = irandom(1, 3);
    setRadarSignatureInfo(0.0, 0.8, -1.0);

    registerMemberReplication(&radar_visual);

    for(int n=0; n<cloud_count; n++)
    {
        clouds[n].size = random(512, 1024 * 4);
        clouds[n].texture = irandom(1, 3);
        float dist_min = clouds[n].size / 2.0f;
        float dist_max = getEffectRadius() * 0.7f  - clouds[n].size;
        clouds[n].offset = sf::vector2FromAngle(float(n * 360 / cloud_count)) * random(dist_min, dist_max);
    }

    nebula_list.push_back(this);
}

P<Nebula> Nebula::setEffectRadius(float r)
{
        r = std::max(0.0f, r);
    if (effect_radius > 1e-3f && r > 1e-3f)
    {
        float s = r / effect_radius;
        for (int n = 0; n < cloud_count; ++n)
        {
            clouds[n].offset *= s; 
            clouds[n].size   *= s; 
        }
    }
    effect_radius = r;
    return this;
}

P<Nebula> Nebula::setTexture(string texture_name)
{
    custom_texture = texture_name;
    return this;
}

#if FEATURE_3D_RENDERING
void Nebula::draw3DTransparent()
{
    glRotatef(getRotation(), 0, 0, -1);
    glTranslatef(-getPosition().x, -getPosition().y, 0);
    for(int n=0; n<cloud_count; n++)
    {
        NebulaCloud& cloud = clouds[n];

        sf::Vector3f position = sf::Vector3f(getPosition().x, getPosition().y, 0) + sf::Vector3f(cloud.offset.x, cloud.offset.y, 0);
        float size = cloud.size;
        float distance = sf::length(camera_position - position);
        float alpha = 1.0 - (distance / 10000.0f);
        float density = 0.8f;         
        float r = getEffectRadius();  
        if (r > 10000.0f) density = 1.5f;
        if (r > 30000.0f) density = 1.8f;   
        if (r > 50000.0f) density = 2.5f;   
        alpha *= density;
        if (alpha < 0.0)
            continue;

        string tex;
        if (!custom_texture.empty())
        {
            tex = custom_texture;
        }
        else
        {
            tex = "Nebula" + string(cloud.texture) + ".png";
        }

        ShaderManager::getShader("billboardShader")->setUniform(
            "textureMap",
            *textureManager.getTexture(tex)
        );
        sf::Shader::bind(ShaderManager::getShader("billboardShader"));

        // ShaderManager::getShader("billboardShader")->setUniform(
        //     "textureMap",
        //     *textureManager.getTexture("Nebula" + string(cloud.texture) + ".png")
        // );
        
        glBegin(GL_QUADS);
        glColor4f(alpha * 0.8, alpha * 0.8, alpha * 0.8, size);
        glTexCoord2f(0, 0);
        glVertex3f(position.x, position.y, position.z);
        glTexCoord2f(1, 0);
        glVertex3f(position.x, position.y, position.z);
        glTexCoord2f(1, 1);
        glVertex3f(position.x, position.y, position.z);
        glTexCoord2f(0, 1);
        glVertex3f(position.x, position.y, position.z);
        glEnd();
    }
}
#endif//FEATURE_3D_RENDERING

void Nebula::drawOnRadar(sf::RenderTarget& window, sf::Vector2f position, float scale, float rotation, bool long_range)
{
    sf::Sprite object_sprite;
    string tex;
    if (!custom_texture.empty())
    {
        tex = custom_texture;
    }
    else
    {
        tex = "Nebula" + string(radar_visual) + ".png";
    }
    textureManager.setTexture(object_sprite, tex);
    object_sprite.setRotation(getRotation()-rotation);
    object_sprite.setPosition(position);
    float size = getEffectRadius() * scale / object_sprite.getTextureRect().width * 3.0;
    object_sprite.setScale(size, size);
    object_sprite.setColor(sf::Color(255, 255, 255));
    window.draw(object_sprite, sf::BlendAdd);
}

void Nebula::drawOnGMRadar(sf::RenderTarget& window, sf::Vector2f position, float scale, float rotation, bool long_range)
{
    sf::CircleShape range_circle(getEffectRadius() * scale);
    range_circle.setOrigin(getEffectRadius() * scale, getEffectRadius() * scale);
    range_circle.setPosition(position);
    range_circle.setFillColor(sf::Color::Transparent);
    range_circle.setOutlineColor(sf::Color(255, 255, 255, 64));
    range_circle.setOutlineThickness(2.0);
    window.draw(range_circle);
}

bool Nebula::inNebula(sf::Vector2f position)
{
    foreach(Nebula, n, nebula_list)
    {
        if ((n->getPosition() - position) < n->getEffectRadius())
            return true;
    }
    return false;
}

bool Nebula::blockedByNebula(sf::Vector2f start, sf::Vector2f end)
{
    sf::Vector2f startEndDiff = end - start;
    float startEndLength = sf::length(startEndDiff);
    // if (startEndLength < 5000.0f)
    //     return false;

    foreach(Nebula, n, nebula_list)
    {
        //Calculate point q, which is a point on the line start-end that is closest to n->getPosition
        float f = sf::dot(startEndDiff, n->getPosition() - start) / startEndLength;
        if (f < 0.0f)
            f = 0.0f;
        if (f > startEndLength)
            f = startEndLength;
        sf::Vector2f q = start + startEndDiff / startEndLength * f;
        if ((q - n->getPosition()) < n->getEffectRadius())
        {
            return true;
        }
    }
    return false;
}

sf::Vector2f Nebula::getFirstBlockedPosition(sf::Vector2f start, sf::Vector2f end)
{
    sf::Vector2f startEndDiff = end - start;
    float startEndLength = sf::length(startEndDiff);
    P<Nebula> first_nebula;
    float first_nebula_f = startEndLength;
    sf::Vector2f first_nebula_q;
    foreach(Nebula, n, nebula_list)
    {
        float f = sf::dot(startEndDiff, n->getPosition() - start) / startEndLength;
        if (f < 0.0)
            f = 0;
        sf::Vector2f q = start + startEndDiff / startEndLength * f;
        if ((q - n->getPosition()) < n->getEffectRadius())
        {
            if (!first_nebula || f < first_nebula_f)
            {
                first_nebula = n;
                first_nebula_f = f;
                first_nebula_q = q;
            }
        }
    }
    if (!first_nebula)
        return end;

    float d = sf::length(first_nebula_q - first_nebula->getPosition());
    return first_nebula_q + sf::normalize(start - end) * sqrtf(first_nebula->getEffectRadius() * first_nebula->getEffectRadius() - d * d);
}

PVector<Nebula> Nebula::getNebulas()
{
    return nebula_list;
}
