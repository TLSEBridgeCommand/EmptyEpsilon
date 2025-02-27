#include <SFML/OpenGL.hpp>

#include "main.h"
#include "gameGlobalInfo.h"
#include "spaceObjects/nebula.h"
#include "spaceObjects/scanProbe.h"
#include "playerInfo.h"
#include "radarView.h"
#include "missileTubeControls.h"
#include "targetsContainer.h"

GuiRadarView::GuiRadarView(GuiContainer* owner, string id, TargetsContainer* targets, P<PlayerSpaceship> targetSpaceship)
: GuiElement(owner, id),
    next_ghost_dot_update(0.0),
    targets(targets),
    missile_tube_controls(nullptr),
    view_position(sf::Vector2f(0.0f,0.0f)),
    view_rotation(0),
    auto_center_on_my_ship(true),
    auto_rotate_on_my_ship(false),
    auto_distance(true),
    distance(5000.0f),
    target_spaceship(targetSpaceship),
    long_range(false),
    show_ghost_dots(false),
    show_sectors(true),
    show_warp_layer(false),
    show_waypoints(false),
    show_target_projection(false),
    show_missile_tubes(false),
    show_callsigns(false),
    show_heading_indicators(false),
    show_game_master_data(false),
    range_indicator_step_size(0.0f),
    style(Circular),
    fog_style(NoFogOfWar),
    mouse_down_func(nullptr),
    mouse_drag_func(nullptr),
    mouse_up_func(nullptr)
{
    // initialize grid colors for different zoom magnitudes
    for (int scale_magnitude = 0; scale_magnitude < grid_scale_size - 1; scale_magnitude++)
    {
        // warning : the computation is balanced using implicit castings, bit overflows and black magic.
        // seriously it's worse than those job interview questions
        // if you change this code even the slightest, verify that it still produces a veriaty of different colors
        sf::Uint8 colorStep = (-128 / grid_scale_size);
        grid_colors[scale_magnitude] = sf::Color(65 + colorStep * scale_magnitude * 0.5, 65 + colorStep * scale_magnitude * 0.3, 129 + colorStep * scale_magnitude, 128);
    }
    // last color is white
    grid_colors[grid_scale_size - 1] = sf::Color(255, 255, 255, 128);
}

GuiRadarView::GuiRadarView(GuiContainer* owner, string id, float distance, TargetsContainer* targets, P<PlayerSpaceship> targetSpaceship)
: GuiElement(owner, id),
    next_ghost_dot_update(0.0),
    targets(targets),
    missile_tube_controls(nullptr),
    view_position(sf::Vector2f(0.0f,0.0f)),
    view_rotation(0),
    auto_center_on_my_ship(true),
    auto_rotate_on_my_ship(false),
    distance(distance),
    target_spaceship(targetSpaceship),
    long_range(false),
    show_ghost_dots(false),
    show_sectors(true),
    show_warp_layer(false),
    show_waypoints(false),
    show_target_projection(false),
    show_missile_tubes(false),
    show_callsigns(false),
    show_heading_indicators(false),
    show_game_master_data(false),
    range_indicator_step_size(0.0f),
    style(Circular),
    fog_style(NoFogOfWar),
    mouse_down_func(nullptr),
    mouse_drag_func(nullptr),
    mouse_up_func(nullptr)
{
    // initialize grid colors for different zoom magnitudes
    for (int scale_magnitude = 0; scale_magnitude < grid_scale_size - 1; scale_magnitude++)
    {
        // warning : the computation is balanced using implicit castings, bit overflows and black magic.
        // seriously it's worse than those job interview questions
        // if you change this code even the slightest, verify that it still produces a veriaty of different colors
        sf::Uint8 colorStep = (-128 / grid_scale_size);
        grid_colors[scale_magnitude] = sf::Color(65 + colorStep * scale_magnitude * 0.5, 65 + colorStep * scale_magnitude * 0.3, 129 + colorStep * scale_magnitude, 128);
    }
    // last color is white
    grid_colors[grid_scale_size - 1] = sf::Color(255, 255, 255, 128);
}

void GuiRadarView::onDraw(sf::RenderTarget& window)
{
    //We need 3 textures:
    // * background
    // * forground
    // * mask
    // Depending on what type of radar we are rendering we can use the mask to mask out the forground and/or background textures before rendering them
    // to the screen.

    //New rendering method. Render to texture first, so we do not need the stencil buffer, as this causes issues with the post processing effects.
    // Render background to screen
    // Render sectors to screen
    // Render range indicators to screen
    // Clear texture with 0% alpha
    // Render objects to texture
    // Render fog to texture with 0% alpha
    //      make fog result transparent, clearing anything that is in the fog.
    //      We can use different blendmodes to get the effect we want, as we can mask out alphas with that.
    // Render objects that are not effected by fog to texture
    // Render texture to screen

    //Hacky, when not relay and we have a ship, center on it.
    if (target_spaceship && auto_center_on_my_ship) {
        setViewPosition(target_spaceship->getPosition());
    }
    if (target_spaceship && auto_rotate_on_my_ship) {
        view_rotation = target_spaceship->getRotation() + 90;
    }
    if (auto_distance)
    {
        setDistance(long_range ? 30000.0f : 5000.0f);
        if (target_spaceship)
        {
            if (long_range)
                setDistance(target_spaceship->getLongRangeRadarRange());
            else
                setDistance(target_spaceship->getShortRangeRadarRange());
        }
    }

    //Setup our textures for rendering
    adjustRenderTexture(background_texture);
    adjustRenderTexture(forground_texture);
    adjustRenderTexture(mask_texture);

    ///Draw the mask texture, which will be black vs white for masking.
    // White areas will be visible, black areas will be masked away.
    if (fog_style == NebulaFogOfWar)
        drawNebulaBlockedAreas(mask_texture);
    if (fog_style == FriendlysShortRangeFogOfWar)
        drawNoneFriendlyBlockedAreas(mask_texture);

    ///Draw the background texture
    drawBackground(background_texture);
    if (fog_style == NebulaFogOfWar || fog_style == FriendlysShortRangeFogOfWar)    //Mask the background color with the nebula blocked areas, but show the rest.
        drawRenderTexture(mask_texture, background_texture, sf::Color::White, sf::BlendMultiply);
    if (show_sectors)
        drawSectorGrid(background_texture);
    if (show_warp_layer)
        drawWarpLayer(background_texture);
    drawRangeIndicators(background_texture);
    if (show_target_projection)
        drawTargetProjections(background_texture);
    if (show_missile_tubes)
        drawMissileTubes(background_texture);

    ///Start drawing of foreground
    forground_texture.clear(sf::Color::Transparent);
    //Draw things that are masked out by fog-of-war
    if (show_ghost_dots)
    {
        updateGhostDots();
        drawGhostDots(forground_texture);
    }
    drawObjects(forground_texture, background_texture);
    if (show_game_master_data)
        drawObjectsGM(forground_texture);

    //Draw the mask on the drawn objects
    if (fog_style == NebulaFogOfWar || fog_style == FriendlysShortRangeFogOfWar)
    {
        drawRenderTexture(mask_texture, forground_texture, sf::Color::White, sf::BlendMode(
            sf::BlendMode::Zero, sf::BlendMode::SrcAlpha, sf::BlendMode::Add
        ));
    }
    //Post masking
    if (show_waypoints)
        drawWaypoints(forground_texture);
    if (show_heading_indicators)
        drawHeadingIndicators(forground_texture);
    drawTargets(forground_texture);

    if (style == Rectangular && my_spaceship)
    {
        sf::Vector2f ship_offset = (my_spaceship->getPosition() - getViewPosition()) / getDistance() * getRadius();
        if (ship_offset.x < -rect.width / 2.0f || ship_offset.x > rect.width / 2.0f || ship_offset.y < -rect.height / 2.0f || ship_offset.y > rect.height / 2.0f)
        {
            sf::Vector2f position(rect.left + rect.width / 2.0f, rect.top + rect.height / 2.0);
            position += ship_offset / sf::length(ship_offset) * std::min(rect.width, rect.height) * 0.4f;

            sf::Sprite arrow_sprite;
            textureManager.setTexture(arrow_sprite, "waypoint");
            arrow_sprite.setPosition(position);
            arrow_sprite.setRotation(sf::vector2ToAngle(ship_offset) - 90);
            forground_texture.draw(arrow_sprite);
        }
    }

    if (style == CircularMasked || style == Circular)
    {
        //When we have a circular masked radar, use the mask_texture to clear out everything that is not part of the circle.
        mask_texture.clear(sf::Color(0, 0, 0, 0));
        float r = std::min(rect.width, rect.height) / 2.0f - 2.0f;
        sf::CircleShape circle(r, 50);
        circle.setOrigin(r, r);
        circle.setPosition(getCenterPoint());
        circle.setFillColor(sf::Color::Black);
        circle.setOutlineColor(colorConfig.radar_outline);
        circle.setOutlineThickness(2.0);
        mask_texture.draw(circle);

        sf::BlendMode blend_mode(
            sf::BlendMode::One, sf::BlendMode::SrcAlpha, sf::BlendMode::Add,
            sf::BlendMode::Zero, sf::BlendMode::SrcAlpha, sf::BlendMode::Add
        );
        drawRenderTexture(mask_texture, background_texture, sf::Color::White, blend_mode);
        drawRenderTexture(mask_texture, forground_texture, sf::Color::White, blend_mode);
    }

    //Render the final radar
    drawRenderTexture(background_texture, window);
    drawRenderTexture(forground_texture, window);
    //if (style == Circular)
    //    drawRadarCutoff(window);
}

void GuiRadarView::updateGhostDots()
{
    if (next_ghost_dot_update < engine->getElapsedTime())
    {
        next_ghost_dot_update = engine->getElapsedTime() + 5.0;
        foreach(SpaceObject, obj, space_object_list)
        {
            P<SpaceShip> ship = obj;
            if (ship && sf::length(obj->getPosition() - getViewPosition()) < getDistance())
            {
                ghost_dots.push_back(GhostDot(obj->getPosition()));
            }
        }

        for(unsigned int n=0; n < ghost_dots.size(); n++)
        {
            if (ghost_dots[n].end_of_life <= engine->getElapsedTime())
            {
                ghost_dots.erase(ghost_dots.begin() + n);
                n--;
            }
        }
    }
}

void GuiRadarView::drawBackground(sf::RenderTarget& window)
{
    window.clear(sf::Color(20, 20, 20, 255));
}

void GuiRadarView::drawNoneFriendlyBlockedAreas(sf::RenderTarget& window)
{
    window.clear(sf::Color(0, 0, 0, 255));
    if (my_spaceship)
    {
        float scale = std::min(rect.width, rect.height) / 2.0f / getDistance();

        float r = 5000.0 * scale * my_spaceship->getSystemEffectiveness(SYS_Scanner);
        sf::CircleShape circle(r, 50);
        circle.setOrigin(r, r);
        circle.setFillColor(sf::Color(255, 255, 255, 255));

        foreach(SpaceObject, obj, space_object_list)
        {
            if ((P<SpaceShip>(obj) || P<SpaceStation>(obj)) && obj->isFriendly(my_spaceship))
            {
                circle.setPosition(worldToScreen(obj->getPosition()));
                window.draw(circle);
            }
            P<ScanProbe> sp = obj;
            if (sp && sp->owner_id == my_spaceship->getMultiplayerId())
            {
                circle.setPosition(worldToScreen(obj->getPosition()));
                window.draw(circle);
            }
        }
    }
}

void GuiRadarView::drawSectorGrid(sf::RenderTarget& window)
{
    sf::Vector2f radar_screen_center = getCenterPosition();
    const float scale = getScale();
    
    const float factor = std::floor(std::log10(GameGlobalInfo::sector_size * scale));
    const int scale_magnitude = 2 - std::min(2.f, factor);
    
    float sector_size = 20000;
    float sub_sector_size = sector_size / 8;
    
    if (gameGlobalInfo->use_advanced_sector_system)
    {
        sector_size = GameGlobalInfo::sector_size * std::pow(sub_sectors_count, scale_magnitude);
        sub_sector_size = sector_size / sub_sectors_count;
    }
    
    int sector_x_min = floor((view_position.x - (radar_screen_center.x - rect.left) / scale) / sector_size) + 1;
    int sector_x_max = floor((view_position.x + (rect.left + rect.width - radar_screen_center.x) / scale) / sector_size);
    int sector_y_min = floor((view_position.y - (radar_screen_center.y - rect.top) / scale) / sector_size) + 1;
    int sector_y_max = floor((view_position.y + (rect.top + rect.height - radar_screen_center.y) / scale) / sector_size);
    sf::Color color(64, 64, 128, 128);
    for(int sector_x = sector_x_min - 1; sector_x <= sector_x_max; sector_x++)
    {
        float x = sector_x * sector_size;
        for(int sector_y = sector_y_min - 1; sector_y <= sector_y_max; sector_y++)
        {
            float y = sector_y * sector_size;
            sf::Vector2f pos = worldToScreen(sf::Vector2f(x+(30/scale),y+(30/scale)));
            
            if (gameGlobalInfo->use_advanced_sector_system)
            {
                // Sector name
                string sector_name = getSectorName(sf::Vector2f(x + 1, y + 1), 0);
                sf::Color sector_color = grid_colors[scale_magnitude];
                float sector_scale = 30;
                
                // Area name
                string area_name = getSectorName(sf::Vector2f(x + 1, y + 1), 2);
                sf::Color area_color = grid_colors[std::max(scale_magnitude, 2)];               
                float area_scale = std::min(200.0, std::max(200.0 * scale*1800.0, 30.0));
                float area_shift = std::min(50.0, std::max(50.0 * scale*1800.0, 10.0));
                area_color.a = std::max(0.0, std::min(25.0 / (scale*900.0), 128.0));
                float area_x = radar_screen_center.x + (x - view_position.x) * scale + area_shift;
                float area_y = radar_screen_center.y + (y - view_position.y) * scale + area_shift;
                
                // Region name
                string region_name = getSectorName(sf::Vector2f(x + 1, y + 1), 4);
                sf::Color region_color = grid_colors[std::max(scale_magnitude, 4)];
                float region_scale = std::min(200.0, std::max(200.0 * scale*1800.0*64.0, 30.0));
                float region_shift = std::min(50.0, std::max(50.0 * scale*1800.0*64.0, 10.0));
                region_color.a = std::max(0.0, std::min(25.0 / (scale*900.0*64.0), 128.0));
                float region_x = radar_screen_center.x + (x - view_position.x) * scale + region_shift;
                float region_y = radar_screen_center.y + (y - view_position.y) * scale + region_shift;

                // Draw Sector name
                if (scale_magnitude < 2) 
                    drawText(window, sf::FloatRect(pos.x-10, pos.y-10, 20, 20), sector_name, ATopLeft, sector_scale, bold_font, sector_color);
                
                // Draw Area/region name
                if (style == Rectangular)
                {
                    if (scale_magnitude < 4) 
                        drawText(window, sf::FloatRect(area_x, area_y, 20, 20), area_name, ATopLeft, area_scale, bold_font, area_color);
                    drawText(window, sf::FloatRect(region_x, region_y, 20, 20), region_name, ATopLeft, region_scale, bold_font, region_color);
                }
            }
            else
                drawText(window, sf::FloatRect(pos.x-10, pos.y-10, 20, 20), getSectorName(sf::Vector2f(x + sub_sector_size, y + sub_sector_size)), ACenter, 30, bold_font, color);
        }
    }
    
    sf::VertexArray lines_x(sf::Lines, 2 * (sector_x_max - sector_x_min + 1));
    sf::VertexArray lines_y(sf::Lines, 2 * (sector_y_max - sector_y_min + 1));
    for(int sector_x = sector_x_min; sector_x <= sector_x_max; sector_x++)
    {
        float x = sector_x * sector_size;
        lines_x[(sector_x - sector_x_min)*2].position = worldToScreen(sf::Vector2f(x, (sector_y_min-1)*sector_size));
        if (gameGlobalInfo->use_advanced_sector_system)
            color = grid_colors[calcGridScaleMagnitude(scale_magnitude, sector_x)];
        lines_x[(sector_x - sector_x_min)*2].color = color;
        lines_x[(sector_x - sector_x_min)*2+1].position = worldToScreen(sf::Vector2f(x, (sector_y_max+1)*sector_size));
        lines_x[(sector_x - sector_x_min)*2+1].color = color;
    }
    for(int sector_y = sector_y_min; sector_y <= sector_y_max; sector_y++)
    {
        float y = sector_y * sector_size;
        lines_y[(sector_y - sector_y_min)*2].position = worldToScreen(sf::Vector2f((sector_x_min-1)*sector_size, y));
        if (gameGlobalInfo->use_advanced_sector_system)
            color = grid_colors[calcGridScaleMagnitude(scale_magnitude, sector_y)];
        lines_y[(sector_y - sector_y_min)*2].color = color;
        lines_y[(sector_y - sector_y_min)*2+1].position = worldToScreen(sf::Vector2f((sector_x_max+1)*sector_size, y));
        lines_y[(sector_y - sector_y_min)*2+1].color = color;
    }
    window.draw(lines_x);
    window.draw(lines_y);

    color = sf::Color(64, 64, 128, 255);
    if (scale_magnitude > 0)
        color = grid_colors[scale_magnitude - 1];
    int sub_sector_x_min = floor((view_position.x - (radar_screen_center.x - rect.left) / scale) / sub_sector_size) + 1;
    int sub_sector_x_max = floor((view_position.x + (rect.left + rect.width - radar_screen_center.x) / scale) / sub_sector_size);
    int sub_sector_y_min = floor((view_position.y - (radar_screen_center.y - rect.top) / scale) / sub_sector_size) + 1;
    int sub_sector_y_max = floor((view_position.y + (rect.top + rect.height - radar_screen_center.y) / scale) / sub_sector_size);
    sf::VertexArray points(sf::Points, (sub_sector_x_max - sub_sector_x_min + 1) * (sub_sector_y_max - sub_sector_y_min + 1));
    for(int sector_x = sub_sector_x_min; sector_x <= sub_sector_x_max; sector_x++)
    {
        float x = sector_x * sub_sector_size;
        for(int sector_y = sub_sector_y_min; sector_y <= sub_sector_y_max; sector_y++)
        {
            float y = sector_y * sub_sector_size;
            points[(sector_x - sub_sector_x_min) + (sector_y - sub_sector_y_min) * (sub_sector_x_max - sub_sector_x_min + 1)].position = worldToScreen(sf::Vector2f(x,y));
            points[(sector_x - sub_sector_x_min) + (sector_y - sub_sector_y_min) * (sub_sector_x_max - sub_sector_x_min + 1)].color = color;
        }
    }
    window.draw(points);
}

void GuiRadarView::drawNebulaBlockedAreas(sf::RenderTarget& window)
{
    sf::BlendMode blend(
        sf::BlendMode::One, sf::BlendMode::Zero, sf::BlendMode::Add,
        sf::BlendMode::One, sf::BlendMode::Zero, sf::BlendMode::Add
    );
    window.clear(sf::Color(255, 255, 255, 255));
    if (!my_spaceship)
        return;
    sf::Vector2f scan_center = my_spaceship->getPosition();
    sf::Vector2f radar_screen_center(rect.left + rect.width / 2.0f, rect.top + rect.height / 2.0f);
    float scale = std::min(rect.width, rect.height) / 2.0f / getDistance();

    PVector<Nebula> nebulas = Nebula::getNebulas();
    foreach(Nebula, n, nebulas)
    {
        sf::Vector2f diff = n->getPosition() - scan_center;
        float diff_len = sf::length(diff);

        if (diff_len < n->getRadius() + getDistance())
        {
            if (diff_len < n->getRadius())
            {
                sf::RectangleShape background(sf::Vector2f(rect.width, rect.height));
                background.setPosition(rect.left, rect.top);
                background.setFillColor(sf::Color(0, 0, 0, 255));
                window.draw(background, blend);
            }else{
                float r = n->getRadius() * scale;
                sf::CircleShape circle(r, 32);
                circle.setOrigin(r, r);
                circle.setPosition(worldToScreen(n->getPosition()));
                circle.setFillColor(sf::Color(0, 0, 0, 255));
                window.draw(circle, blend);

                float diff_angle = sf::vector2ToAngle(diff);
                float angle = acosf(n->getRadius() / diff_len) / M_PI * 180.0f;

                sf::Vector2f pos_a = n->getPosition() - sf::vector2FromAngle(diff_angle + angle) * n->getRadius();
                sf::Vector2f pos_b = n->getPosition() - sf::vector2FromAngle(diff_angle - angle) * n->getRadius();
                sf::Vector2f pos_c = scan_center + sf::normalize(pos_a - scan_center) * getDistance() * 3.0f;
                sf::Vector2f pos_d = scan_center + sf::normalize(pos_b - scan_center) * getDistance() * 3.0f;
                sf::Vector2f pos_e = scan_center + diff / diff_len * getDistance() * 3.0f;

                sf::VertexArray a(sf::TrianglesStrip, 5);
                a[0].position = worldToScreen(pos_a);
                a[1].position = worldToScreen(pos_b);
                a[2].position = worldToScreen(pos_c);
                a[3].position = worldToScreen(pos_d);
                a[4].position = worldToScreen(pos_e);
                for(int n=0; n<5;n++)
                    a[n].color = sf::Color(0, 0, 0, 255);
                window.draw(a, blend);
            }
        }
    }

    if (my_spaceship)
    {
        float r = 5000.0f * getScale() * my_spaceship->getSystemEffectiveness(SYS_Scanner);
        sf::CircleShape circle(r, 32);
        circle.setOrigin(r, r);
        circle.setPosition(radar_screen_center + (scan_center - getViewPosition()) * getScale());
        circle.setFillColor(sf::Color(255, 255, 255,255));
        window.draw(circle, blend);
    }
}

void GuiRadarView::drawGhostDots(sf::RenderTarget& window)
{
    sf::VertexArray ghost_points(sf::Points, ghost_dots.size());
    for(unsigned int n=0; n<ghost_dots.size(); n++)
    {
        ghost_points[n].position = worldToScreen(ghost_dots[n].position);
        ghost_points[n].color = sf::Color(255, 255, 255, 255 * std::max(((ghost_dots[n].end_of_life - engine->getElapsedTime()) / GhostDot::total_lifetime), 0.f));
    }
    window.draw(ghost_points);
}

void GuiRadarView::drawWaypoints(sf::RenderTarget& window)
{
    if (!my_spaceship)
        return;

    sf::Vector2f radar_screen_center(rect.left + rect.width / 2.0f, rect.top + rect.height / 2.0f);
    float scale = std::min(rect.width, rect.height) / 2.0f / getDistance();
    
    for(int r = 0; r < PlayerSpaceship::max_routes; r++){
        for(int wp = 0; wp < PlayerSpaceship::max_waypoints_in_route; wp++){
            if (my_spaceship->waypoints[r][wp] < empty_waypoint){
                sf::Vector2f screen_position = radar_screen_center + (my_spaceship->waypoints[r][wp] - getViewPosition()) * scale;
                sf::Sprite object_sprite;
                textureManager.setTexture(object_sprite, "waypoint");
                object_sprite.setColor(routeColors[r]);
                object_sprite.setPosition(screen_position - sf::Vector2f(0, 10));
                object_sprite.setScale(0.8, 0.8);
                window.draw(object_sprite);
                drawText(window, sf::FloatRect(screen_position.x, screen_position.y - 10, 0, 0), string(wp + 1), ACenter, 18, bold_font, colorConfig.ship_waypoint_text);
                
                if (style != Rectangular && sf::length(screen_position - radar_screen_center) > std::min(rect.width, rect.height) * 0.5f)
                {
                    screen_position = radar_screen_center + ((screen_position - radar_screen_center) / sf::length(screen_position - radar_screen_center) * std::min(rect.width, rect.height) * 0.4f);

                    object_sprite.setPosition(screen_position);
                    object_sprite.setRotation(sf::vector2ToAngle(screen_position - radar_screen_center) - 90);
                    window.draw(object_sprite);

                    drawText(window, sf::FloatRect(screen_position.x, screen_position.y, 0, 0), string(wp + 1), ACenter, 18, bold_font, colorConfig.ship_waypoint_text);
                }
            }
        }
    }
}

void GuiRadarView::drawRangeIndicators(sf::RenderTarget& window)
{
    if (range_indicator_step_size < 1.0)
        return;

    sf::Vector2f radar_screen_center(rect.left + rect.width / 2.0f, rect.top + rect.height / 2.0f);
    float scale = std::min(rect.width, rect.height) / 2.0f / getDistance();

    for(float circle_size=range_indicator_step_size; circle_size < getDistance(); circle_size+=range_indicator_step_size)
    {
        float s = circle_size * scale;
        sf::CircleShape circle(s, 50);
        circle.setOrigin(s, s);
        circle.setPosition(radar_screen_center);
        circle.setFillColor(sf::Color::Transparent);
        circle.setOutlineColor(sf::Color(255, 255, 255, 16));
        circle.setOutlineThickness(2.0);
        window.draw(circle);
        drawText(window, sf::FloatRect(radar_screen_center.x, radar_screen_center.y - s - 20, 0, 0), string(int(circle_size / 1000.0f + 0.1f)) + DISTANCE_UNIT_1K, ACenter, 20, bold_font, sf::Color(255, 255, 255, 32));
    }
}

void GuiRadarView::drawTargetProjections(sf::RenderTarget& window)
{
    const float seconds_per_distance_tick = 5.0f;
    sf::Vector2f radar_screen_center(rect.left + rect.width / 2.0f, rect.top + rect.height / 2.0f);
    float scale = std::min(rect.width, rect.height) / 2.0f / getDistance();

    if (target_spaceship && missile_tube_controls)
    {
        for(int n=0; n<target_spaceship->weapon_tube_count; n++)
        {
            if (!target_spaceship->weapon_tube[n].isLoaded())
                continue;
            if (PreferencesManager::get("weapons_specific_station", "0").toInt() != 0 && target_spaceship->weapon_tube[n].getStation() != PreferencesManager::get("weapons_specific_station", "0").toInt())
                continue;

            sf::Vector2f fire_position = target_spaceship->getPosition() + sf::rotateVector(target_spaceship->ship_template->model_data->getTubePosition2D(n), target_spaceship->getRotation());
            //sf::Vector2f fire_draw_position = worldToScreen(fire_position);

            const MissileWeaponData& data = MissileWeaponData::getDataFor(target_spaceship->weapon_tube[n].getLoadType());
            float fire_angle = target_spaceship->weapon_tube[n].getDirection() + (target_spaceship->getRotation());
            float missile_target_angle = fire_angle;
            if (data.turnrate > 0.0f)
            {
                if (missile_tube_controls->getManualAim())
                {
                    missile_target_angle = missile_tube_controls->getMissileTargetAngle();
                }else{
                    float firing_solution = target_spaceship->weapon_tube[n].calculateFiringSolution(my_spaceship->getTarget(PreferencesManager::get("weapons_specific_station", "0").toInt()));
                    if (firing_solution != std::numeric_limits<float>::infinity())
                        missile_target_angle = firing_solution;
                }
            }

            float angle_diff = sf::angleDifference(missile_target_angle, fire_angle);
            float turn_radius = ((360.0f / data.turnrate) * data.speed) / (2.0f * M_PI);
            if (data.turnrate == 0.0f)
                turn_radius = 0.0f;

            float left_or_right = 90;
            if (angle_diff > 0)
                left_or_right = -90;

            sf::Vector2f turn_center = sf::vector2FromAngle(fire_angle + left_or_right) * turn_radius;
            sf::Vector2f turn_exit = turn_center + sf::vector2FromAngle(missile_target_angle - left_or_right) * turn_radius;

            float turn_distance = fabs(angle_diff) / 360.0 * (turn_radius * 2.0f * M_PI);
            float lifetime_after_turn = data.lifetime - turn_distance / data.speed;
            float length_after_turn = data.speed * lifetime_after_turn;

            sf::VertexArray a(sf::LinesStrip, 13);
            a[0].position = fire_position;
            for(int cnt=0; cnt<10; cnt++)
                a[cnt + 1].position = fire_position + (turn_center + sf::vector2FromAngle(fire_angle - angle_diff / 10.0f * cnt - left_or_right) * turn_radius);
            a[11].position = fire_position + turn_exit;
            a[12].position = fire_position + (turn_exit + sf::vector2FromAngle(missile_target_angle) * length_after_turn);
            for(int cnt=0; cnt<13; cnt++) {
                a[cnt].position = worldToScreen(a[cnt].position);
                a[cnt].color = sf::Color(255, 255, 255, 128);
            }
            window.draw(a);

            float offset = seconds_per_distance_tick * data.speed;
            for(int cnt=0; cnt<floor(data.lifetime / seconds_per_distance_tick); cnt++)
            {
                sf::Vector2f p;
                sf::Vector2f n;
                if (offset < turn_distance)
                {
                    n = sf::vector2FromAngle(fire_angle - (angle_diff * offset / turn_distance) - left_or_right);
                    p = worldToScreen(fire_position + (turn_center + n * turn_radius));
                }else{
                    p = worldToScreen(fire_position + (turn_exit + sf::vector2FromAngle(missile_target_angle) * (offset - turn_distance)));
                    n = sf::vector2FromAngle(missile_target_angle + 90.0f);
                }
                n = sf::rotateVector(n, -view_rotation);
                n = sf::normalize(n);
                sf::VertexArray a(sf::Lines, 2);
                a[0].position = p - n * 10.0f;
                a[1].position = p + n * 10.0f;
                window.draw(a);

                offset += seconds_per_distance_tick * data.speed;
            }
        }
    }

    if (targets)
    {
        for(P<SpaceObject> obj : targets->getTargets())
        {
            if (obj->getVelocity() < 1.0f)
                continue;

            sf::VertexArray a(sf::Lines, 12);
            a[0].position = worldToScreen(obj->getPosition());
            a[0].color = sf::Color(255, 255, 255, 128);
            a[1].position = worldToScreen(obj->getPosition() + obj->getVelocity() * 60.0f);
            a[1].color = sf::Color(255, 255, 255, 0);
            sf::Vector2f n = sf::normalize(sf::rotateVector(sf::Vector2f(-obj->getVelocity().y, obj->getVelocity().x), -view_rotation));
            for(int cnt=0; cnt<5; cnt++)
            {
                sf::Vector2f p = sf::rotateVector(obj->getVelocity() * (seconds_per_distance_tick * (cnt + 1.0f) * scale), -view_rotation);
                a[2 + cnt * 2].position = a[0].position + p + n * 10.0f;
                a[3 + cnt * 2].position = a[0].position + p - n * 10.0f;
                a[2 + cnt * 2].color = a[3 + cnt * 2].color = sf::Color(255, 255, 255, 128 - cnt * 20);
            }
            window.draw(a);
        }
    }
}

void GuiRadarView::drawMissileTubes(sf::RenderTarget& window)
{
    float scale = std::min(rect.width, rect.height) / 2.0f / getDistance();

    if (target_spaceship)
    {
        sf::VertexArray a(sf::Lines, target_spaceship->weapon_tube_count * 2);
        for(int n=0; n<target_spaceship->weapon_tube_count; n++)
        {
            sf::Vector2f fire_position = target_spaceship->getPosition() + sf::rotateVector(target_spaceship->ship_template->model_data->getTubePosition2D(n), target_spaceship->getRotation());
            sf::Vector2f fire_draw_position = worldToScreen(fire_position);

            float fire_angle = target_spaceship->getRotation() + target_spaceship->weapon_tube[n].getDirection() - view_rotation;

            a[n * 2].position = fire_draw_position;
            a[n * 2 + 1].position = fire_draw_position + (sf::vector2FromAngle(fire_angle) * 1000.0f) * scale;
            a[n * 2].color = sf::Color(128, 128, 128, 128);
            a[n * 2 + 1].color = sf::Color(128, 128, 128, 0);
        }
        window.draw(a);
    }
}

void GuiRadarView::drawObjects(sf::RenderTarget& window_normal, sf::RenderTarget& window_alpha)
{
    float scale = std::min(rect.width, rect.height) / 2.0f / getDistance();

    std::set<SpaceObject*> visible_objects;
    switch(fog_style)
    {
    case NoFogOfWar:
        foreach(SpaceObject, obj, space_object_list)
        {
            visible_objects.insert(*obj);
        }
        break;
    case FriendlysShortRangeFogOfWar:
        if (!my_spaceship)
            return;
        foreach(SpaceObject, obj, space_object_list)
        {
            if (!obj->canHideInNebula())
                visible_objects.insert(*obj);

            if ((!P<SpaceShip>(obj) && !P<SpaceStation>(obj)) || !obj->isFriendly(my_spaceship))
            {
                P<ScanProbe> sp = obj;
                if (!sp || sp->owner_id != my_spaceship->getMultiplayerId())
                {
                    continue;
                }
            }

            sf::Vector2f position = obj->getPosition();
            float radar_range = 5000.0 * my_spaceship->getSystemEffectiveness(SYS_Scanner);
            PVector<Collisionable> obj_list = CollisionManager::queryArea(position - sf::Vector2f(radar_range, radar_range), position + sf::Vector2f(radar_range, radar_range));
            foreach(Collisionable, c_obj, obj_list)
            {
                P<SpaceObject> obj2 = c_obj;
                if (obj2 && (obj->getPosition() - obj2->getPosition()) < radar_range + obj2->getRadius())
                {
                    visible_objects.insert(*obj2);
                }
            }
        }
        break;
    case NebulaFogOfWar:
        foreach(SpaceObject, obj, space_object_list)
        {
            if (obj->canHideInNebula() && my_spaceship && Nebula::blockedByNebula(my_spaceship->getPosition(), obj->getPosition()))
                continue;
            visible_objects.insert(*obj);
        }
        break;
    }

    for(SpaceObject* obj : visible_objects)
    {
        sf::Vector2f object_position_on_screen = worldToScreen(obj->getPosition());
        float r = obj->getRadius() * scale;
        sf::FloatRect object_rect(object_position_on_screen.x - r, object_position_on_screen.y - r, r * 2, r * 2);
        if (obj != *my_spaceship && rect.intersects(object_rect))
        {
            sf::RenderTarget* window = &window_normal;
            if (!obj->canHideInNebula())
                window = &window_alpha;
            obj->drawOnRadar(*window, object_position_on_screen, scale, view_rotation, long_range);
            if (show_callsigns && obj->getCallSign() != "")
                drawText(*window, sf::FloatRect(object_position_on_screen.x, object_position_on_screen.y - 15, 0, 0), obj->getCallSign(), ACenter, 15, bold_font);
        }
    }
    if (my_spaceship)
    {
        sf::Vector2f object_position_on_screen = worldToScreen(my_spaceship->getPosition());
        my_spaceship->drawOnRadar(window_normal, object_position_on_screen, scale, view_rotation, long_range);
    }
}

void GuiRadarView::drawObjectsGM(sf::RenderTarget& window)
{
    float scale = std::min(rect.width, rect.height) / 2.0f / getDistance();

    foreach(SpaceObject, obj, space_object_list)
    {
        sf::Vector2f object_position_on_screen = worldToScreen(obj->getPosition());
        float r = obj->getRadius() * scale;
        sf::FloatRect object_rect(object_position_on_screen.x - r, object_position_on_screen.y - r, r * 2, r * 2);
        if (rect.intersects(object_rect))
        {
            obj->drawOnGMRadar(window, object_position_on_screen, scale, view_rotation, long_range);
        }
    }
}

void GuiRadarView::drawTargets(sf::RenderTarget& window)
{
    float scale = std::min(rect.width, rect.height) / 2.0f / distance;

    if (!targets)
        return;
    sf::Sprite target_sprite;
    textureManager.setTexture(target_sprite, "redicule.png");

    for(P<SpaceObject> obj : targets->getTargets())
    {
        sf::Vector2f object_position_on_screen = worldToScreen(obj->getPosition());
        float r = obj->getRadius() * scale;
        sf::FloatRect object_rect(object_position_on_screen.x - r, object_position_on_screen.y - r, r * 2, r * 2);
        if (obj != my_spaceship && rect.intersects(object_rect))
        {
            target_sprite.setPosition(object_position_on_screen);
            window.draw(target_sprite);
        }
    }
    
    if (my_spaceship){
        sf::Vector2f wp = empty_waypoint;
        if (targets->getRouteIndex() == -1){
            if (targets->getWaypointIndex() > -1 && targets->getWaypointIndex() < PlayerSpaceship::max_waypoints) {
                wp = my_spaceship->waypoints[0][targets->getWaypointIndex()];
            }
        } else {
            if (targets->getWaypointIndex() > -1 && targets->getWaypointIndex() < PlayerSpaceship::max_waypoints_in_route) {
                wp = my_spaceship->waypoints[targets->getRouteIndex()][targets->getWaypointIndex()];
            }
        }
        if (wp < empty_waypoint){
            sf::Vector2f object_position_on_screen = getCenterPosition() + (wp - getViewPosition()) * getScale();
            target_sprite.setPosition(object_position_on_screen - sf::Vector2f(0, 10));
            window.draw(target_sprite);
        }
    }
}

void GuiRadarView::drawHeadingIndicators(sf::RenderTarget& window)
{
    sf::Vector2f radar_screen_center(rect.left + rect.width / 2.0f, rect.top + rect.height / 2.0f);
    float scale = std::min(rect.width, rect.height) / 2.0f;

    sf::VertexArray tigs(sf::Lines, 360/20*2);
    for(unsigned int n=0; n<360; n+=20)
    {
        tigs[n/20*2].position = radar_screen_center + sf::vector2FromAngle(float(n) - 90 - view_rotation) * (scale - 20);
        tigs[n/20*2+1].position = radar_screen_center + sf::vector2FromAngle(float(n) - 90 - view_rotation) * (scale - 40);
    }
    window.draw(tigs);
    sf::VertexArray small_tigs(sf::Lines, 360/5*2);
    for(unsigned int n=0; n<360; n+=5)
    {
        small_tigs[n/5*2].position = radar_screen_center + sf::vector2FromAngle(float(n) - 90 - view_rotation) * (scale - 20);
        small_tigs[n/5*2+1].position = radar_screen_center + sf::vector2FromAngle(float(n) - 90 - view_rotation) * (scale - 30);
    }
    window.draw(small_tigs);
    for(unsigned int n=0; n<360; n+=20)
    {
        sf::Text text(string(n), *main_font, 15);
        text.setPosition(radar_screen_center + sf::vector2FromAngle(float(n) - 90 - view_rotation) * (scale - 45));
        text.setOrigin(text.getLocalBounds().width / 2.0, text.getLocalBounds().height / 2.0);
        text.setRotation(n-view_rotation);
        window.draw(text);
    }
}

void GuiRadarView::drawRadarCutoff(sf::RenderTarget& window)
{
    sf::Vector2f radar_screen_center(rect.left + rect.width / 2.0f, rect.top + rect.height / 2.0f);
    float screen_size = std::min(rect.width, rect.height) / 2.0f;

    sf::Sprite cutOff;
    textureManager.setTexture(cutOff, "radarCutoff.png");
    cutOff.setPosition(radar_screen_center);
    cutOff.setScale(screen_size / float(cutOff.getTextureRect().width) * 2, screen_size / float(cutOff.getTextureRect().width) * 2);
    window.draw(cutOff);

    sf::RectangleShape rectTop(sf::Vector2f(rect.width, radar_screen_center.y - screen_size - rect.top));
    rectTop.setFillColor(sf::Color::Black);
    rectTop.setPosition(rect.left, rect.top);
    window.draw(rectTop);
    sf::RectangleShape rectBottom(sf::Vector2f(rect.width, rect.height - screen_size - (radar_screen_center.y - rect.top)));
    rectBottom.setFillColor(sf::Color::Black);
    rectBottom.setPosition(rect.left, radar_screen_center.y + screen_size);
    window.draw(rectBottom);

    sf::RectangleShape rectLeft(sf::Vector2f(radar_screen_center.x - screen_size - rect.left, rect.height));
    rectLeft.setFillColor(sf::Color::Black);
    rectLeft.setPosition(rect.left, rect.top);
    window.draw(rectLeft);
    sf::RectangleShape rectRight(sf::Vector2f(rect.width - screen_size - (radar_screen_center.x - rect.left), rect.height));
    rectRight.setFillColor(sf::Color::Black);
    rectRight.setPosition(radar_screen_center.x + screen_size, rect.top);
    window.draw(rectRight);
}

sf::Vector2f GuiRadarView::getCenterPosition(){
    return sf::Vector2f(rect.left + rect.width / 2.0f, rect.top + rect.height / 2.0f);
}

sf::Vector2f GuiRadarView::worldToScreen(sf::Vector2f world_position)
{
    sf::Vector2f radar_screen_center(rect.left + rect.width / 2.0f, rect.top + rect.height / 2.0f);
    float scale = std::min(rect.width, rect.height) / 2.0f / distance;

    sf::Vector2f radar_position = sf::rotateVector((world_position - view_position) * scale, -view_rotation);
    return radar_position + radar_screen_center;
}

sf::Vector2f GuiRadarView::screenToWorld(sf::Vector2f screen_position)
{
    sf::Vector2f radar_screen_center(rect.left + rect.width / 2.0f, rect.top + rect.height / 2.0f);
    float scale = std::min(rect.width, rect.height) / 2.0f / distance;

    sf::Vector2f radar_position = sf::rotateVector((screen_position - radar_screen_center) / scale, view_rotation);
    return view_position + radar_position;
}

float GuiRadarView::getRadius(){
    //if(style == CircularSector)
    //    return std::min(rect.width, rect.height) * 0.85f;  // the last 15% has artifacts from the mask rotation
    //else 
        return std::min(rect.width, rect.height) / 2.0f;
}

//sf::Vector2f GuiRadarView::getCenterPosition(){
//    if(style == CircularSector){
//        sf::Vector2f deOrientedCenter(0, std::min(rect.width, rect.height) / 2.0f);
//        return SectorsView::getCenterPosition() + sf::rotateVector(deOrientedCenter, background_texture.getView().getRotation());
//    } else 
//        return SectorsView::getCenterPosition();
//}

float GuiRadarView::getScale(){
    return getRadius() / getDistance();
}

int GuiRadarView::calcGridScaleMagnitude(int scale_magnitude, int position)
{
    for (int i = grid_scale_size - 1; i >= 0; i--)
    {
        if (position % (int)std::pow(sub_sectors_count, i) == 0)
        {
            return std::min(scale_magnitude + i, grid_scale_size - 1);
        }
    }
    return scale_magnitude;
}

void GuiRadarView::drawWarpLayer(sf::RenderTarget &window){
    if (gameGlobalInfo->use_warp_terrain)
    {
        if (gameGlobalInfo->layer[warp_layer].defined){
            MapLayer &layer = gameGlobalInfo->layer[warp_layer];
            sf::Sprite layerMap;
            textureManager.getTexture(layer.textureName)->setSmooth(true);
            textureManager.setTexture(layerMap, layer.textureName);
            layerMap.setPosition(worldToScreen(layer.coordinates));
            layerMap.setScale(getScale() * layer.scale, getScale()* layer.scale);
            float transparency = std::max(0.0, std::min(25.0 / (getScale()*450.0), 128.0));
            layerMap.setColor(sf::Color(255, 255, 255, transparency)); // half transparent
            window.draw(layerMap);
        }
    }
}

bool GuiRadarView::onMouseDown(sf::Vector2f position)
{
    if (style == Circular || style == CircularMasked)
    {
        float radius = std::min(rect.width, rect.height) / 2.0f;
        if (position - getCenterPoint() > radius)
            return false;
    }
    if (!mouse_down_func && !mouse_drag_func && !mouse_up_func)
        return false;
    if (mouse_down_func)
        mouse_down_func(screenToWorld(position));
    return true;
}

void GuiRadarView::onMouseDrag(sf::Vector2f position)
{
    if (mouse_drag_func)
        mouse_drag_func(screenToWorld(position));
}

void GuiRadarView::onMouseUp(sf::Vector2f position)
{
    if (mouse_up_func)
        mouse_up_func(screenToWorld(position));
}
