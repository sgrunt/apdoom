#include <onut/onut.h>
#include <onut/Settings.h>
#include <onut/Renderer.h>
#include <onut/PrimitiveBatch.h>
#include <onut/Window.h>
#include <onut/Input.h>
#include <onut/SpriteBatch.h>
#include <onut/Texture.h>
#include <onut/Json.h>
#include <onut/Dialogs.h>
#include <onut/Random.h>
#include <onut/Timing.h>
#include <onut/Font.h>

#include <imgui/imgui.h>

#include <vector>
#include <set>

#include "maps.h"
#include "generate.h"
#include "defs.h"


enum class state_t
{
    idle,
    panning,
    gen,
    gen_panning,
    move_bb,
    move_rule,
    connecting_rule,
    set_rules
};


enum class tool_t : int
{
    bb,
    region,
    rules,
    access
};


struct rule_connection_t
{
    int target_region = -1;
    std::vector<int> requirements_or;
    std::vector<int> requirements_and;
};


struct rule_region_t
{
    int x = 0, y = 0;
    std::vector<rule_connection_t> connections;
};


#define RULES_W 1024
#define RULES_H 400
#define RULE_CONNECTION_OFFSET 64.0f
#define BIG_DOOR_W 128
#define BIG_DOOR_H 128
#define SMALL_DOOR_W 64
#define SMALL_DOOR_H 72


struct bb_t
{
    int x1, y1, x2, y2;
    int region = -1;

    int overlaps(const bb_t& other) const
    {
        auto d1 = other.x2 - x1;
        if (d1 < 0) return 0;

        auto d2 = x2 - other.x1;
        if (d2 < 0) return 0;

        auto d3 = other.y2 - y1;
        if (d3 < 0) return 0;

        auto d4 = y2 - other.y1;
        if (d4 < 0) return 0;

        return onut::max(d1, d2, d3, d4);

        //return (x1 <= other.x2 && x2 >= other.x1 && 
        //        y1 <= other.y2 && y2 >= other.y1);
    }

    bb_t operator+(const Vector2& v) const
    {
        return {
            (int)(x1 + v.x),
            (int)(y1 + v.y),
            (int)(x2 + v.x),
            (int)(y2 + v.y)
        };
    }

    Vector2 center() const
    {
        return {
            (float)(x1 + x2) * 0.5f,
            (float)(y1 + y2) * 0.5f
        };
    }
};


struct region_t
{
    std::string name;
    bool connects_to_exit = false;
    bool connects_to_hub = false;
    std::set<int> sectors;
    std::vector<std::string> required_items_or;
    std::vector<std::string> required_items_and;
    Color tint = Color::White;
    int selected_item_or = -1;
    int selected_item_and = -1;
    rule_region_t rules;
};


static region_t world_region = {
    "World",
    false,
    false,
    {},
    {},
    {},
    Color(0.6f, 0.6f, 0.6f, 1.0f),
    -1,
    -1,
    {}
};

static region_t exit_region = {
    "Exit",
    false,
    false,
    {},
    {},
    {},
    Color(0.6f, 0.6f, 0.6f, 1.0f),
    -1,
    -1,
    {}
};


struct item_t
{
    bool death_logic = false;
};


struct map_state_t
{
    Vector2 pos;
    float angle = 0.0f;
    int selected_bb = -1;
    int selected_region = -1;
    std::vector<bb_t> bbs;
    std::vector<region_t> regions;
    std::vector<item_t> items;
    rule_region_t world_rules;
    rule_region_t exit_rules;
    std::set<int> accesses;
};


struct map_view_t
{
    Vector2 cam_pos;
    float cam_zoom = 0.25f;
};


struct map_history_t
{
    std::vector<map_state_t> history;
    int history_point = 0;
};


static int active_ep = 0;
static int active_map = 0;
static map_state_t map_states[EP_COUNT][MAP_COUNT];
static map_view_t map_views[EP_COUNT][MAP_COUNT];
static map_history_t map_histories[EP_COUNT][MAP_COUNT];
static state_t state = state_t::idle;
static tool_t tool = tool_t::access;
static Vector2 mouse_pos;
static Vector2 mouse_pos_on_down;
static Vector2 cam_pos_on_down;
static bb_t bb_on_down;
static map_state_t* map_state = nullptr;
static map_view_t* map_view = nullptr;
static map_history_t* map_history = nullptr;
static OTextureRef ap_icon;
static int mouse_hover_bb = -1;
static int mouse_hover_sector = -1;
static int moving_edge = -1;
static HCURSOR arrow_cursor = 0;
static HCURSOR we_cursor = 0;
static HCURSOR ns_cursor = 0;
static HCURSOR nswe_cursor = 0;
static bool generating = true;
static map_state_t* flat_levels[EP_COUNT * MAP_COUNT];
static int gen_step_count = 0;
static bool painted = false;
static int moving_rule = -3;
static int mouse_hover_connection = -1;
static int mouse_hover_connection_rule = -3;
static Point rule_pos_on_down;
static int mouse_hover_rule = -3;
static int connecting_rule_from = -3;
static int set_rule_rule = -3;
static int set_rule_connection = -1;
static int mouse_hover_access = -1;

static std::map<int, OTextureRef> REQUIREMENT_TEXTURES;
static const std::vector<int> REQUIREMENTS = {
    5, 40, 6, 39, 13, 38,
    2005, 2001, 2002, 2003, 2004, 2006
};


Json::Value serialize_rules(const rule_region_t& rules)
{
    Json::Value json;

    json["x"] = rules.x;
    json["y"] = rules.y;

    Json::Value connections_json(Json::arrayValue);
    for (const auto& connection : rules.connections)
    {
        Json::Value connection_json;

        connection_json["target_region"] = connection.target_region;

        {
            Json::Value requirements_json(Json::arrayValue);
            for (auto requirement : connection.requirements_or)
            {
                requirements_json.append(requirement);
            }
            connection_json["requirements_or"] = requirements_json;
        }

        {
            Json::Value requirements_json(Json::arrayValue);
            for (auto requirement : connection.requirements_and)
            {
                requirements_json.append(requirement);
            }
            connection_json["requirements_and"] = requirements_json;
        }

        connections_json.append(connection_json);
    }
    json["connections"] = connections_json;

    return json;
}


rule_region_t deserialize_rules(const Json::Value& json)
{
    rule_region_t rules;

    rules.x = json.get("x", 0).asInt();
    rules.y = json.get("y", 0).asInt();

    const auto& connections_json = json["connections"];
    for (const auto& connection_json : connections_json)
    {
        rule_connection_t connection;

        connection.target_region = connection_json.get("target_region", -1).asInt();
        
        {
            const auto& requirements_json = connection_json["requirements_or"];
            for (const auto& requirement_json : requirements_json)
            {
                connection.requirements_or.push_back(requirement_json.asInt());
            }
        }
        {
            const auto& requirements_json = connection_json["requirements_and"];
            for (const auto& requirement_json : requirements_json)
            {
                connection.requirements_and.push_back(requirement_json.asInt());
            }
        }

        rules.connections.push_back(connection);
    }

    return rules;
}


void save()
{
    Json::Value _json;

    Json::Value eps_json(Json::arrayValue);
    for (int ep = 0; ep < EP_COUNT; ++ep)
    {
        Json::Value maps_json(Json::arrayValue);
        for (int lvl = 0; lvl < MAP_COUNT; ++lvl)
        {
            Json::Value _map_json;
            Json::Value bbs_json(Json::arrayValue);
            for (const auto& bb : map_states[ep][lvl].bbs)
            {
                Json::Value bb_json(Json::arrayValue);
                bb_json.append(bb.x1);
                bb_json.append(bb.y1);
                bb_json.append(bb.x2);
                bb_json.append(bb.y2);
                bb_json.append(bb.region);
                bbs_json.append(bb_json);
            }
            _map_json["bbs"] = bbs_json;

            Json::Value regions_json(Json::arrayValue);
            for (const auto& region : map_states[ep][lvl].regions)
            {
                Json::Value region_json;
                region_json["name"] = region.name;
                region_json["connects_to_exit"] = region.connects_to_exit;
                region_json["connects_to_hub"] = region.connects_to_hub;
                region_json["tint"] = onut::serializeFloat4(&region.tint.r);

                Json::Value sectors_json(Json::arrayValue);
                for (auto sectori : region.sectors)
                    sectors_json.append(sectori);
                region_json["sectors"] = sectors_json;

                region_json["required_items_or"] = onut::serializeStringArray(region.required_items_or);
                region_json["required_items_and"] = onut::serializeStringArray(region.required_items_and);

                region_json["rules"] = serialize_rules(region.rules);

                regions_json.append(region_json);
            }
            _map_json["regions"] = regions_json;

            Json::Value accesses_json(Json::arrayValue);
            for (auto access : map_states[ep][lvl].accesses)
            {
                accesses_json.append(access);
            }
            _map_json["accesses"] = accesses_json;

            _map_json["world_rules"] = serialize_rules(map_states[ep][lvl].world_rules);
            _map_json["exit_rules"] = serialize_rules(map_states[ep][lvl].exit_rules);

            maps_json.append(_map_json);
        }
        eps_json.append(maps_json);
    }

    _json["episodes"] = eps_json;

    std::string filename = OArguments[2] + std::string("\\regions.json");
    onut::saveJson(_json, filename, false);
}


void load()
{
    Json::Value json;
    std::string filename = OArguments[2] + std::string("\\regions.json");
    if (!onut::loadJson(json, filename))
    {
        onut::showMessageBox("Warning", "Warning: File not found. Saving will break shit.\n" + filename);
        return;
    }

    for (int ep = 0; ep < EP_COUNT; ++ep)
    {
        for (int lvl = 0; lvl < MAP_COUNT; ++lvl)
        {
            auto& _map_state = map_states[ep][lvl];
            auto& _map_json = json["episodes"][ep][lvl];

            const auto& bbs_json = _map_json["bbs"];
            for (const auto& bb_json : bbs_json)
            {
                _map_state.bbs.push_back({
                    bb_json[0].asInt(),
                    bb_json[1].asInt(),
                    bb_json[2].asInt(),
                    bb_json[3].asInt(),
                    bb_json.isValidIndex(4) ? bb_json[4].asInt() : -1,
                });
            }

            const auto& regions_json = _map_json["regions"];
            for (const auto& region_json : regions_json)
            {
                region_t region;

                region.name = region_json.get("name", "BAD_NAME").asString();
                region.connects_to_exit = region_json.get("connects_to_exit", false).asBool();
                region.connects_to_hub = region_json.get("connects_to_hub", false).asBool();
                onut::deserializeFloat4(&region.tint.r, region_json["tint"]);

                const auto& sectors_json = region_json["sectors"];
                for (const auto& sector_json : sectors_json)
                    region.sectors.insert(sector_json.asInt());

                region.required_items_or = onut::deserializeStringArray(region_json["required_items_or"]);
                region.required_items_and = onut::deserializeStringArray(region_json["required_items_and"]);

                region.rules = deserialize_rules(region_json["rules"]);

                _map_state.regions.push_back(region);
            }

            const auto& accesses_json = _map_json["accesses"];
            for (const auto& access_json : accesses_json)
            {
                _map_state.accesses.insert(access_json.asInt());
            }

            _map_state.world_rules = deserialize_rules(_map_json["world_rules"]);
            _map_state.exit_rules = deserialize_rules(_map_json["exit_rules"]);
        }
    }
}


void update_window_title()
{
    oWindow->setCaption(level_names[active_ep][active_map]);
}


// Undo/Redo shit
void push_undo()
{
    if (map_history->history_point < (int)map_histories[active_ep][active_map].history.size() - 1)
        map_history->history.erase(map_history->history.begin() + (map_history->history_point + 1), map_history->history.end());
    map_history->history.push_back(*map_state);
    map_history->history_point = (int)map_history->history.size() - 1;
}


void select_map(int ep, int map)
{
    mouse_hover_sector = -1;
    mouse_hover_bb = -1;
    set_rule_rule = -3;
    set_rule_connection = -1;
    active_ep = ep;
    active_map = map;
    map_state = &map_states[active_ep][active_map];
    map_view = &map_views[active_ep][active_map];
    map_history = &map_histories[active_ep][active_map];

    update_window_title();
    if (map_history->history.empty())
        push_undo();
}


void undo()
{
    if (map_history->history_point > 0)
    {
        map_history->history_point--;
        *map_state = map_history->history[map_history->history_point];
    }
}


void redo()
{
    if (map_history->history_point < (int)map_history->history.size() - 1)
    {
        map_history->history_point++;
        *map_state = map_history->history[map_history->history_point];
    }
}


void initSettings()
{
    oSettings->setGameName("APDOOM Gen Tool");
    oSettings->setResolution({ 1600, 900 });
    oSettings->setIsResizableWindow(true);
    oSettings->setIsFixedStep(false);
    oSettings->setShowFPS(false);
    oSettings->setAntiAliasing(true);
    oSettings->setShowOnScreenLog(false);
    oSettings->setStartMaximized(true);
}


void regen()
{
    gen_step_count = 0;
    generating = true;

    for (int ep = 0; ep < EP_COUNT; ++ep)
    {
        for (int lvl = 0; lvl < MAP_COUNT; ++lvl)
        {
            auto map = &maps[ep][lvl];
            auto mid = Vector2((float)(map->bb[2] + map->bb[0]) / 2, (float)(map->bb[3] + map->bb[1]) / 2);
            map_states[ep][lvl].pos = -mid + onut::rand2f(Vector2(-1000, -1000), Vector2(1000, 1000));
        }
    }
}


void init()
{
    REQUIREMENT_TEXTURES[5] = OGetTexture("Blue keycard.png");
    REQUIREMENT_TEXTURES[40] = OGetTexture("Blue skull key.png");
    REQUIREMENT_TEXTURES[6] = OGetTexture("Yellow keycard.png");
    REQUIREMENT_TEXTURES[39] = OGetTexture("Yellow skull key.png");
    REQUIREMENT_TEXTURES[13] = OGetTexture("Red keycard.png");
    REQUIREMENT_TEXTURES[38] = OGetTexture("Red skull key.png");
    REQUIREMENT_TEXTURES[2005] = OGetTexture("Chainsaw.png");
    REQUIREMENT_TEXTURES[2001] = OGetTexture("Shotgun.png");
    REQUIREMENT_TEXTURES[2002] = OGetTexture("Chaingun.png");
    REQUIREMENT_TEXTURES[2003] = OGetTexture("Rocket launcher.png");
    REQUIREMENT_TEXTURES[2004] = OGetTexture("Plasma gun.png");
    REQUIREMENT_TEXTURES[2006] = OGetTexture("BFG9000.png");

    arrow_cursor = LoadCursor(nullptr, IDC_ARROW);
    nswe_cursor = LoadCursor(nullptr, IDC_SIZEALL);
    we_cursor = LoadCursor(nullptr, IDC_SIZEWE);
    ns_cursor = LoadCursor(nullptr, IDC_SIZENS);
    
    ap_icon = OGetTexture("ap.png");

    init_maps();

    for (int ep = 0; ep < EP_COUNT; ++ep)
    {
        for (int lvl = 0; lvl < MAP_COUNT; ++lvl)
        {
            //auto k = ep * 9 + lvl;
            //map_states[ep][lvl].pos = Vector2(
            //    -16384 + 3000 + ((k % 5) * 6000),
            //    -16384 + 3000 + ((k / 5) * 6000)
            //);

            auto map = &maps[ep][lvl];
            flat_levels[ep * MAP_COUNT + lvl] = &map_states[ep][lvl];
            map_views[ep][lvl].cam_pos = Vector2((float)(map->bb[2] + map->bb[0]) / 2, -(float)(map->bb[3] + map->bb[1]) / 2);
        }
    }

    // Load states
    load();
    select_map(0, 0);

    regen();

    sector_at(0, 0, &maps[0][0]);
    // 56508168
}


void shutdown() // lol
{
}


void add_bounding_box()
{
    auto map = &maps[active_ep][active_map];
    map_state->bbs.push_back({map->bb[0], map->bb[1], map->bb[2], map->bb[3]});
    map_state->selected_bb = (int)map_state->bbs.size() - 1;
    push_undo();
}


rule_region_t* get_rules(int idx)
{
    switch (idx)
    {
        case -1: return &map_state->world_rules;
        case -2: return &map_state->exit_rules;
        default: return &map_state->regions[idx].rules;
    }
}


void delete_selected()
{
    switch (tool)
    {
        case tool_t::bb:
            if (map_state->selected_bb != -1)
            {
                map_state->bbs.erase(map_state->bbs.begin() + map_state->selected_bb);
                map_state->selected_bb = -1;
                push_undo();
            }
            break;
        case tool_t::rules:
            if (set_rule_rule != -3 && set_rule_connection != -1)
            {
                auto rules = get_rules(set_rule_rule);
                if (rules)
                {
                    rules->connections.erase(rules->connections.begin() + set_rule_connection);
                    set_rule_rule = -3;
                    set_rule_connection = -1;
                    push_undo();
                }
            }
            break;
    }
}


void update_shortcuts()
{
    auto ctrl = OInputPressed(OKeyLeftControl);
    auto shift = OInputPressed(OKeyLeftShift);
    auto alt = OInputPressed(OKeyLeftAlt);

    if (ctrl && !shift && !alt && OInputJustPressed(OKeyZ)) undo();
    if (ctrl && shift && !alt && OInputJustPressed(OKeyZ)) redo();
    if (!ctrl && !shift && !alt && OInputJustPressed(OKeyB)) add_bounding_box();
    if (!ctrl && !shift && !alt && OInputJustPressed(OKeyDelete)) delete_selected();
    if (ctrl && !shift && !alt && OInputJustPressed(OKeyS)) save();
    if (!ctrl && !shift && !alt && OInputJustPressed(OKeySpaceBar)) regen();
}


bool test_bb(const bb_t& bb, const Vector2& pos, float zoom, int &edge)
{
    float edge_size = 32.0f / zoom;
    if (pos.x >= bb.x1 - edge_size && pos.x <= bb.x1 + edge_size && pos.y <= -bb.y1 && pos.y >= -bb.y2)
    {
        edge = 0;
        return true;
    }
    if (pos.y <= -bb.y1 + edge_size && pos.y >= -bb.y1 - edge_size && pos.x >= bb.x1 && pos.x <= bb.x2)
    {
        edge = 1;
        return true;
    }
    if (pos.x >= bb.x2 - edge_size && pos.x <= bb.x2 + edge_size && pos.y <= -bb.y1 && pos.y >= -bb.y2)
    {
        edge = 2;
        return true;
    }
    if (pos.y <= -bb.y2 + edge_size && pos.y >= -bb.y2 - edge_size && pos.x >= bb.x1 && pos.x <= bb.x2)
    {
        edge = 3;
        return true;
    }
    if (pos.x >= bb.x1 && pos.x <= bb.x2 && pos.y <= -bb.y1 && pos.y >= -bb.y2)
    {
        return true;
    }
    return false;
}


int get_bb_at(const Vector2& pos, float zoom, int &edge)
{
    edge = -1;
    if (map_state->selected_bb != -1)
    {
        if (test_bb(map_state->bbs[map_state->selected_bb], pos, zoom, edge))
            return map_state->selected_bb;
    }
    for (int i = 0; i < (int)map_state->bbs.size(); ++i)
    {
        if (test_bb(map_state->bbs[i], pos, zoom, edge))
            return i;
    }
    return -1;
}


// -1 = world, -2 = exit, -3 = not found
int get_rule_at(const Vector2& pos)
{
    if (pos.x >= (float)map_state->world_rules.x - RULES_W * 0.5f &&
        pos.x <= (float)map_state->world_rules.x + RULES_W * 0.5f &&
        pos.y <= -(float)map_state->world_rules.y + RULES_H * 0.5f &&
        pos.y >= -(float)map_state->world_rules.y - RULES_H * 0.5f)
        return -1;

    if (pos.x >= (float)map_state->exit_rules.x - RULES_W * 0.5f &&
        pos.x <= (float)map_state->exit_rules.x + RULES_W * 0.5f &&
        pos.y <= -(float)map_state->exit_rules.y + RULES_H * 0.5f &&
        pos.y >= -(float)map_state->exit_rules.y - RULES_H * 0.5f)
        return -2;

    for (int i = (int)map_state->regions.size() - 1; i >= 0; --i)
    {
        const auto& region = map_state->regions[i];
        if (pos.x >= (float)region.rules.x - RULES_W * 0.5f &&
            pos.x <= (float)region.rules.x + RULES_W * 0.5f &&
            pos.y <= -(float)region.rules.y + RULES_H * 0.5f &&
            pos.y >= -(float)region.rules.y - RULES_H * 0.5f)
            return i;
    }

    return -3;
}


Vector2 get_rect_edge_pos(Vector2 from, Vector2 to, float side_offset, bool invert_offset)
{
    const auto RECT_HW = RULES_W * 0.5f + 32.0f;
    const auto RECT_HH = RULES_H * 0.5f + 32.0f;

    Vector2 dir = to - from;
    Vector2 right(-dir.y, dir.x);
    right.Normalize();

    Vector2 offset = right * side_offset;
    if (invert_offset) offset = -offset;

    // Left
    if (dir.x + offset.x < -RECT_HW)
    {
        auto d1 = offset.x + RECT_HW;
        auto d2 = -RECT_HW - (dir.x + offset.x);
        auto t = d1 / (d1 + d2);
        auto p = offset + dir * t;
        if (p.y >= -RECT_HH && p.y < RECT_HH) return from + p;
    }

    // Right
    if (dir.x + offset.x > RECT_HW)
    {
        auto d1 = RECT_HW - offset.x;
        auto d2 = (dir.x + offset.x) - RECT_HW;
        auto t = d1 / (d1 + d2);
        auto p = offset + dir * t;
        if (p.y >= -RECT_HH && p.y < RECT_HH) return from + p;
    }

    // Top
    if (dir.y + offset.y < -RECT_HH)
    {
        auto d1 = offset.y + RECT_HH;
        auto d2 = -RECT_HH - (dir.y + offset.y);
        auto t = d1 / (d1 + d2);
        auto p = offset + dir * t;
        if (p.x >= -RECT_HW && p.x < RECT_HW) return from + p;
    }

    // Bottom
    if (dir.y + offset.y > RECT_HH)
    {
        auto d1 = RECT_HH - offset.y;
        auto d2 = (dir.y + offset.y) - RECT_HH;
        auto t = d1 / (d1 + d2);
        auto p = offset + dir * t;
        if (p.x >= -RECT_HW && p.x < RECT_HW) return from + p;
    }

    // Shouldn't happen
    return from;
}


// Thanks, chatGPT
double segment_point_distance(const Vector2& p1, const Vector2& p2, const Vector2& point)
{
    double segmentLength = sqrt(pow(p2.x - p1.x, 2) + pow(p2.y - p1.y, 2));
    double u = ((point.x - p1.x) * (p2.x - p1.x) + (point.y - p1.y) * (p2.y - p1.y)) / (segmentLength * segmentLength);

    if (u < 0.0) {
        return sqrt(pow(point.x - p1.x, 2) + pow(point.y - p1.y, 2));
    }
    if (u > 1.0) {
        return sqrt(pow(point.x - p2.x, 2) + pow(point.y - p2.y, 2));
    }

    double intersectionX = p1.x + u * (p2.x - p1.x);
    double intersectionY = p1.y + u * (p2.y - p1.y);

    return sqrt(pow(point.x - intersectionX, 2) + pow(point.y - intersectionY, 2));
}


// -1 = not found
int get_connection_at(const Vector2& pos, const rule_region_t& rules)
{
    Vector2 center((float)rules.x, -(float)rules.y);

    int i = 0;
    for (const auto& connection : rules.connections)
    {
        auto other_rules = get_rules(connection.target_region);
        if (!other_rules) continue;

        Vector2 other_center((float)other_rules->x, -(float)other_rules->y);
        Vector2 from = get_rect_edge_pos(center, other_center, RULE_CONNECTION_OFFSET, false);
        Vector2 to = get_rect_edge_pos(other_center, center, RULE_CONNECTION_OFFSET, true);
        auto d = segment_point_distance(from, to, {pos.x, pos.y});
        if (d <= 24.0f / map_view->cam_zoom) return i;
        i++;
    }
    return -1;
}

void get_connection_at(const Vector2& pos, int& rule, int& connection)
{
    connection = get_connection_at(pos, map_state->world_rules);
    if (connection != -1)
    {
        rule = -1;
        return;
    }

    for (int i = 0; i < (int)map_state->regions.size(); ++i)
    {
        connection = get_connection_at(pos, map_state->regions[i].rules);
        if (connection != -1)
        {
            rule = i;
            return;
        }
    }

    connection = get_connection_at(pos, map_state->exit_rules);
    if (connection != -1)
    {
        rule = -2;
        return;
    }

    rule = -3;
    connection = -1;
}


void update_gen()
{
    bool overlapped = false;
    auto dt = 0.01f;
    // TODO: Optimize that with chunks? Only care about bounding boxes and not levels?
    for (int i = 0; i < EP_COUNT * MAP_COUNT; ++i)
    {
        for (int k = 0; k < (int)flat_levels[i]->bbs.size(); ++k)
        {
            // try to pull back to the middle
            //if (gen_step_count > 100 && gen_step_count < 200)
            //    flat_levels[i]->pos *= (1.0f - dt * 0.5f);

            for (int j = 0; j < EP_COUNT * MAP_COUNT; ++j)
            {
                if (j == i) continue;
                for (int l = 0; l < (int)flat_levels[j]->bbs.size(); ++l)
                {
                    auto bb1 = flat_levels[i]->bbs[k] + flat_levels[i]->pos;
                    auto bb2 = flat_levels[j]->bbs[l] + flat_levels[j]->pos;
                    auto penetration = bb1.overlaps(bb2);
                    if (penetration)
                    {
                        overlapped = true;
                        auto dir = bb1.center() - bb2.center();
                        dir.Normalize();
                        dir += onut::rand2f(Vector2(-1, -1), Vector2(1, 1));
                        dir *= (float)penetration;
                        flat_levels[i]->pos += dir * dt;
                        flat_levels[j]->pos -= dir * dt;
                    }
                }
            }
        }
    }
    if (overlapped) gen_step_count++;
    //else regen();
}


void update()
{
    // Update mouse pos in world
    auto cam_matrix = Matrix::Create2DTranslationZoom(OScreenf, map_view->cam_pos, map_view->cam_zoom);
    auto inv_cam_matrix = cam_matrix.Invert();
    mouse_pos = Vector2::Transform(OGetMousePos(), inv_cam_matrix);

    // Update shortcuts
    switch (state)
    {
        case state_t::idle:
        {
            if (!ImGui::GetIO().WantCaptureMouse)
            {
                if (OInputJustPressed(OMouse3))
                {
                    state = state_t::panning;
                    mouse_pos_on_down = OGetMousePos();
                    cam_pos_on_down = map_view->cam_pos;
                }
                else if (oInput->getStateValue(OMouseZ) > 0.0f)
                {
                    map_view->cam_zoom *= 1.2f;
                }
                else if (oInput->getStateValue(OMouseZ) < 0.0f)
                {
                    map_view->cam_zoom /= 1.2f;
                }
                else if (OInputJustPressed(OKeyTab))
                {
                    state = state_t::gen;
                }
                else if (tool == tool_t::bb)
                {
                    mouse_hover_bb = get_bb_at(mouse_pos, map_view->cam_zoom, moving_edge);
                    if (mouse_hover_bb != -1)
                    {
                        switch (moving_edge)
                        {
                            case -1: SetCursor(nswe_cursor); break;
                            case 0: SetCursor(we_cursor); break;
                            case 1: SetCursor(ns_cursor); break;
                            case 2: SetCursor(we_cursor); break;
                            case 3: SetCursor(ns_cursor); break;
                        }
                    }
                    else
                    {
                        SetCursor(arrow_cursor);
                    }

                    for (int i = 0; i < 9; ++i)
                        if (OInputJustPressed((onut::Input::State)((int)OKey1 + i)) && map_state->selected_bb != -1)
                            map_state->bbs[map_state->selected_bb].region = i;
                    if (OInputJustPressed(OKey0) && map_state->selected_bb != -1)
                        map_state->bbs[map_state->selected_bb].region = -1;

                    if (OInputJustPressed(OMouse1))
                    {
                        if (mouse_hover_bb != -1)
                        {
                            map_state->selected_bb = mouse_hover_bb;
                            state = state_t::move_bb;
                            bb_on_down = map_state->bbs[mouse_hover_bb];
                            mouse_pos_on_down = OGetMousePos();
                        }
                        else if (map_state->selected_bb != -1)
                        {
                            map_state->selected_bb = -1;
                            push_undo();
                        }
                    }
                }
                else if (tool == tool_t::region)
                {
                    int x = (int)mouse_pos.x;
                    int y = (int)-mouse_pos.y;
                    mouse_hover_sector = sector_at(x, y, &maps[active_ep][active_map]);

                    if ((OInputJustReleased(OMouse1) || OInputJustReleased(OMouse2)) && painted)
                    {
                        painted = false;
                        push_undo();
                    }
                    
                    if (OInputPressed(OMouse1))
                    {
                        // "paint" selected region
                        if (mouse_hover_sector != -1 && map_state->selected_region != -1)
                        {
                            for (auto& region : map_state->regions) region.sectors.erase(mouse_hover_sector);
                            map_state->regions[map_state->selected_region].sectors.insert(mouse_hover_sector);
                            painted = true;
                        }
                    }
                    else if (OInputPressed(OMouse2))
                    {
                        // "erase" selected region
                        if (mouse_hover_sector != -1)
                        {
                            for (auto& region : map_state->regions) region.sectors.erase(mouse_hover_sector);
                            painted = true;
                        }
                    }
                    else if (OInputJustPressed(OKeyF) && map_state->selected_region != -1)
                    {
                        // Fill selected region
                        for (auto& region : map_state->regions) region.sectors.clear();
                        for (int i = 0, len = (int)maps[active_ep][active_map].sectors.size(); i < len; ++i)
                            map_state->regions[map_state->selected_region].sectors.insert(i);
                        painted = true;
                    }
                }
                else if (tool == tool_t::rules)
                {
                    mouse_hover_rule = get_rule_at(mouse_pos);
                    if (mouse_hover_rule != -3)
                    {
                        mouse_hover_connection_rule = -3;
                        mouse_hover_connection = -1;
                    }
                    else
                    {
                        get_connection_at(mouse_pos, mouse_hover_connection_rule, mouse_hover_connection);
                    }

                    if (OInputJustPressed(OMouse1))
                    {
                        if (mouse_hover_rule != -3)
                        {
                            moving_rule = mouse_hover_rule;
                            state = state_t::move_rule;
                            mouse_pos_on_down = OGetMousePos();
                            auto rules = get_rules(mouse_hover_rule);
                            rule_pos_on_down.x = rules->x;
                            rule_pos_on_down.y = rules->y;
                            mouse_hover_rule = -3;
                            set_rule_rule = -3;
                            set_rule_connection = -1;
                        }
                        else if (mouse_hover_connection != -1)
                        {
                            set_rule_rule = mouse_hover_connection_rule;
                            set_rule_connection = mouse_hover_connection;
                        }
                        else
                        {
                            set_rule_rule = -3;
                            set_rule_connection = -1;
                        }
                        //else
                        //{
                        //    box_to = box_from = mouse_pos;
                        //    state = state_t::selecting_rules;
                        //}
                    }
                    else if (OInputJustPressed(OMouse2))
                    {
                        if (mouse_hover_rule != -3)
                        {
                            set_rule_rule = -3;
                            set_rule_connection = -1;
                            state = state_t::connecting_rule;
                            connecting_rule_from = mouse_hover_rule;
                        }
                    }
                }
            }
            if (!ImGui::GetIO().WantCaptureKeyboard)
            {
                update_shortcuts();
            }
            break;
        }
        case state_t::panning:
        {
            ImGui::GetIO().WantCaptureMouse = false;
            ImGui::GetIO().WantCaptureKeyboard = false;
            auto diff = OGetMousePos() - mouse_pos_on_down;
            map_view->cam_pos = cam_pos_on_down - diff / map_view->cam_zoom;
            if (OInputJustReleased(OMouse3))
                state = state_t::idle;
            break;
        }
        case state_t::move_bb:
        {
            auto diff = (OGetMousePos() - mouse_pos_on_down) / map_view->cam_zoom;
            switch (moving_edge)
            {
                case -1:
                    map_state->bbs[map_state->selected_bb].x1 = bb_on_down.x1 + (int)diff.x;
                    map_state->bbs[map_state->selected_bb].x2 = bb_on_down.x2 + (int)diff.x;
                    map_state->bbs[map_state->selected_bb].y1 = bb_on_down.y1 - (int)diff.y;
                    map_state->bbs[map_state->selected_bb].y2 = bb_on_down.y2 - (int)diff.y;
                    break;
                case 0:
                    map_state->bbs[map_state->selected_bb].x1 = bb_on_down.x1 + (int)diff.x;
                    break;
                case 1:
                    map_state->bbs[map_state->selected_bb].y1 = bb_on_down.y1 - (int)diff.y;
                    break;
                case 2:
                    map_state->bbs[map_state->selected_bb].x2 = bb_on_down.x2 + (int)diff.x;
                    break;
                case 3:
                    map_state->bbs[map_state->selected_bb].y2 = bb_on_down.y2 - (int)diff.y;
                    break;
            }
            if (OInputJustReleased(OMouse1))
            {
                push_undo();
                state = state_t::idle;
            }
            break;
        }
        case state_t::move_rule:
        {
            auto diff = (OGetMousePos() - mouse_pos_on_down) / map_view->cam_zoom;
            auto rules = get_rules(moving_rule);
            rules->x = rule_pos_on_down.x + (int)diff.x;
            rules->y = rule_pos_on_down.y - (int)diff.y;
            if (OInputJustReleased(OMouse1))
            {
                push_undo();
                state = state_t::idle;
            }
            break;
        }
        case state_t::connecting_rule:
        {
            mouse_hover_rule = get_rule_at(mouse_pos);

            if (OInputJustReleased(OMouse2))
            {
                if (mouse_hover_rule != -3 && connecting_rule_from != mouse_hover_rule)
                {
                    auto rules_from = get_rules(connecting_rule_from);
                    auto rules_to = get_rules(mouse_hover_rule);

                    // Check if connection not already there
                    bool already_connected = false;
                    for (const auto& connection : rules_from->connections)
                    {
                        if (connection.target_region == mouse_hover_rule)
                        {
                            already_connected = true;
                            break;
                        }
                    }

                    if (!already_connected)
                    {
                        rule_connection_t connection;
                        connection.target_region = mouse_hover_rule;
                        rules_from->connections.push_back(connection);
                        push_undo();

                        set_rule_rule = connecting_rule_from;
                        set_rule_connection = (int)rules_from->connections.size() - 1;
                    }
                }
                state = state_t::idle;
            }
            break;
        }
        case state_t::gen:
        {
            update_shortcuts();
            update_gen();
            if (OInputJustPressed(OMouse3))
            {
                state = state_t::gen_panning;
                mouse_pos_on_down = OGetMousePos();
                cam_pos_on_down = map_view->cam_pos;
            }
            else if (oInput->getStateValue(OMouseZ) > 0.0f)
            {
                map_view->cam_zoom *= 1.2f;
            }
            else if (oInput->getStateValue(OMouseZ) < 0.0f)
            {
                map_view->cam_zoom /= 1.2f;
            }
            else if (OInputJustPressed(OKeyTab))
            {
                state = state_t::idle;
            }
            break;
        }
        case state_t::gen_panning:
        {
            update_gen();
            auto diff = OGetMousePos() - mouse_pos_on_down;
            map_view->cam_pos = cam_pos_on_down - diff / map_view->cam_zoom;
            if (OInputJustReleased(OMouse3))
                state = state_t::gen;
            break;
        }
    }
}


void draw_guides()
{
    auto pb = oPrimitiveBatch.get();

    pb->draw(Vector2(0, -16384), Color(0.15f)); pb->draw(Vector2(0, 16384), Color(0.15f));
    pb->draw(Vector2(-16384, 0), Color(0.15f)); pb->draw(Vector2(16384, 0), Color(0.15f));

    pb->draw(Vector2(-16384, -16384), Color(1, 0.5f, 0)); pb->draw(Vector2(-16384, 16384), Color(1, 0.5f, 0));
    pb->draw(Vector2(-16384, 16384), Color(1, 0.5f, 0)); pb->draw(Vector2(16384, 16384), Color(1, 0.5f, 0));
    pb->draw(Vector2(16384, 16384), Color(1, 0.5f, 0)); pb->draw(Vector2(16384, -16384), Color(1, 0.5f, 0));
    pb->draw(Vector2(16384, -16384), Color(1, 0.5f, 0)); pb->draw(Vector2(-16384, -16384), Color(1, 0.5f, 0));

    pb->draw(Vector2(-32768, -32768), Color(1, 0, 0)); pb->draw(Vector2(-32768, 32768), Color(1, 0, 0));
    pb->draw(Vector2(-32768, 32768), Color(1, 0, 0)); pb->draw(Vector2(32768, 32768), Color(1, 0, 0));
    pb->draw(Vector2(32768, 32768), Color(1, 0, 0)); pb->draw(Vector2(32768, -32768), Color(1, 0, 0));
    pb->draw(Vector2(32768, -32768), Color(1, 0, 0)); pb->draw(Vector2(-32768, -32768), Color(1, 0, 0));
}


region_t* get_region_for_sector(int ep, int lvl, int sector)
{
    auto& map_state = map_states[ep][lvl];
    for (auto& region : map_state.regions)
    {
        if (region.sectors.count(sector))
        {
            return &region;
        }
    }
    return nullptr;
}


void draw_rules(const rule_region_t& rules, const region_t& region, bool mouse_hover)
{
    auto sb = oSpriteBatch.get();

    Rect rect(rules.x - RULES_W * 0.5f, -rules.y - RULES_H * 0.5f, RULES_W, RULES_H);
    sb->drawRect(nullptr, rect, Color(0, 0, 0, 0.75f));
    sb->drawInnerOutlineRect(rect, 1.0f / map_view->cam_zoom * 2.0f, region.tint);
    if (mouse_hover)
    {
        sb->drawInnerOutlineRect(rect.Grow(1.0f / map_view->cam_zoom * 2.0f), 1.0f / map_view->cam_zoom * 2.0f, Color(0, 1, 1));
    }
}


void draw_rules_name(const rule_region_t& rules, const region_t& region)
{
    auto sb = oSpriteBatch.get();
    auto pFont = OGetFont("font.fnt");

    sb->begin(
        Matrix::CreateScale(10.0f) * 
        Matrix::CreateTranslation(Vector2(rules.x, -rules.y)) *
        Matrix::Create2DTranslationZoom(OScreenf, map_view->cam_pos, map_view->cam_zoom)
    );
    sb->drawText(pFont, region.name, Vector2::Zero, OCenter, Color::White);
    sb->end();
}


void draw_connections(const rule_region_t& rules, int rule_idx)
{
    auto pb = oPrimitiveBatch.get();

    Vector2 center((float)rules.x, -(float)rules.y);

    int i = 0;
    for (const auto& connection : rules.connections)
    {
        auto other_rules = get_rules(connection.target_region);
        if (!other_rules) continue;
        Rect other_rect(other_rules->x - RULES_W * 0.5f, -other_rules->y - RULES_H * 0.5f, RULES_W, RULES_H);
        other_rect.Grow(128);

        Vector2 other_center((float)other_rules->x, -(float)other_rules->y);
        Vector2 from = get_rect_edge_pos(center, other_center, RULE_CONNECTION_OFFSET, false);
        Vector2 to = get_rect_edge_pos(other_center, center, RULE_CONNECTION_OFFSET, true);
        Vector2 dir = to - from;
        dir.Normalize();
        Vector2 right(-dir.y, dir.x);

        Color color = Color::White;
        if (mouse_hover_connection_rule == rule_idx && i == mouse_hover_connection)
        {
            color = Color(0, 1, 1);
        }
        if (rule_idx == set_rule_rule && i == set_rule_connection)
        {
            color = Color(1, 0, 0);
        }               

        pb->draw(from, color); pb->draw(to, color);
        pb->draw(to, color); pb->draw(to - dir * 32.0f - right * 32.0f, color);
        pb->draw(to, color); pb->draw(to - dir * 32.0f + right * 32.0f, color);

        ++i;
    }
}


#define REQUIREMENT_SIZE 96.0f


void draw_connections_requirements(const rule_region_t& rules, int rule_idx)
{
    auto sb = oSpriteBatch.get();

    Vector2 center((float)rules.x, -(float)rules.y);

    int i = 0;
    for (const auto& connection : rules.connections)
    {
        auto other_rules = get_rules(connection.target_region);
        if (!other_rules) continue;
        Rect other_rect(other_rules->x - RULES_W * 0.5f, -other_rules->y - RULES_H * 0.5f, RULES_W, RULES_H);
        other_rect.Grow(128);

        Vector2 other_center((float)other_rules->x, -(float)other_rules->y);
        Vector2 from = get_rect_edge_pos(center, other_center, RULE_CONNECTION_OFFSET, false);
        Vector2 to = get_rect_edge_pos(other_center, center, RULE_CONNECTION_OFFSET, true);
        Vector2 dir = to - from;
        dir.Normalize();
        Vector2 right(-dir.y, dir.x);

        Vector2 pos = (to - from) * 0.5f + right * REQUIREMENT_SIZE - dir * ((float)(connection.requirements_or.size() + connection.requirements_and.size()) * 0.5f * REQUIREMENT_SIZE - 0.5f * REQUIREMENT_SIZE);
        for (auto requirement : connection.requirements_or)
        {
            sb->drawSprite(REQUIREMENT_TEXTURES[requirement], pos + from, Color::White, 0.0f, 2.0f);
            pos += dir * REQUIREMENT_SIZE;
        }
        for (auto requirement : connection.requirements_and)
        {
            sb->drawSprite(REQUIREMENT_TEXTURES[requirement], pos + from, Color::White, 0.0f, 2.0f);
            pos += dir * REQUIREMENT_SIZE;
        }

        ++i;
    }
}


void draw_rules()
{
    int i = 0;

    auto sb = oSpriteBatch.get();
    auto pb = oPrimitiveBatch.get();

    auto transform = Matrix::Create2DTranslationZoom(OScreenf, map_view->cam_pos, map_view->cam_zoom);

    // Draw connections
    pb->begin(OPrimitiveLineList, nullptr, transform);
    draw_connections(map_state->world_rules, -1);
    i = 0;
    for (const auto& region : map_state->regions)
    {
        draw_connections(region.rules, i);
        ++i;
    }
    draw_connections(map_state->exit_rules, -2);
    pb->end();

    // Draw connection requirements
    sb->begin(transform);
    draw_connections_requirements(map_state->world_rules, -1);
    i = 0;
    for (const auto& region : map_state->regions)
    {
        draw_connections_requirements(region.rules, i);
        ++i;
    }
    draw_connections_requirements(map_state->exit_rules, -2);
    sb->end();


    // Draw boxes
    sb->begin(transform);
    draw_rules(map_state->world_rules, world_region, mouse_hover_rule == -1);
    i = 0;
    for (const auto& region : map_state->regions)
    {
        draw_rules(region.rules, region, mouse_hover_rule == i);
        ++i;
    }
    draw_rules(map_state->exit_rules, exit_region, mouse_hover_rule == -2);
    sb->end();

    // Draw names
    draw_rules_name(map_state->world_rules, world_region);
    for (const auto& region : map_state->regions)
    {
        draw_rules_name(region.rules, region);
    }
    draw_rules_name(map_state->exit_rules, exit_region);
}


void draw_level(int ep, int lvl, const Vector2& pos, float angle, bool draw_tools)
{
    Color bound_color(1.0f);
    Color step_color(0.35f);
    Color bb_color(0.5f);

    auto pb = oPrimitiveBatch.get();
    auto sb = oSpriteBatch.get();
    auto map = &maps[ep][lvl];
    oRenderer->renderStates.backFaceCull = false;

    auto transform = 
              Matrix::CreateRotationZ(angle) *
              Matrix::CreateTranslation(Vector2(pos.x, -pos.y)) *
              Matrix::Create2DTranslationZoom(OScreenf, map_view->cam_pos, map_view->cam_zoom);

    // Sectors
    if (draw_tools)
    {
        pb->begin(OPrimitiveTriangleList, nullptr, transform);
        int i = 0;
        for (const auto& sector : map->sectors)
        {
            region_t* region = get_region_for_sector(ep, lvl, i);
            if (region)
            {
                Color color = region->tint * 0.5f;
                for (int i = 0, len = (int)sector.vertices.size(); i < len; i += 3)
                {
                    const auto& v1 = map->vertexes[sector.vertices[i + 0]];
                    const auto& v2 = map->vertexes[sector.vertices[i + 1]];
                    const auto& v3 = map->vertexes[sector.vertices[i + 2]];

                    pb->draw(Vector2(v1.x, -v1.y), color);
                    pb->draw(Vector2(v2.x, -v2.y), color);
                    pb->draw(Vector2(v3.x, -v3.y), color);
                }
            }
            ++i;
        }
        pb->end();
    }

    // Geometry
    pb->begin(OPrimitiveLineList, nullptr, transform);

    // Bounding box
    //pb->draw(Vector2(map->bb[0], -map->bb[1]), bb_color); pb->draw(Vector2(map->bb[0], -map->bb[3]), bb_color);
    //pb->draw(Vector2(map->bb[0], -map->bb[3]), bb_color); pb->draw(Vector2(map->bb[2], -map->bb[3]), bb_color);
    //pb->draw(Vector2(map->bb[2], -map->bb[3]), bb_color); pb->draw(Vector2(map->bb[2], -map->bb[1]), bb_color);
    //pb->draw(Vector2(map->bb[2], -map->bb[1]), bb_color); pb->draw(Vector2(map->bb[0], -map->bb[1]), bb_color);

    // Geometry
    int i = 0;
    for (const auto& line : map->linedefs)
    {
        Color color = bound_color;
        if (line.back_sidedef != -1) color = step_color;

        if (draw_tools)
        {
            if (line.special_type == LT_DR_DOOR_RED_OPEN_WAIT_CLOSE ||
                line.special_type == LT_D1_DOOR_RED_OPEN_STAY ||
                line.special_type == LT_SR_DOOR_RED_OPEN_STAY_FAST ||
                line.special_type == LT_S1_DOOR_RED_OPEN_STAY_FAST)
                color = Color(1, 0, 0);
            else if (line.special_type == LT_DR_DOOR_YELLOW_OPEN_WAIT_CLOSE ||
                line.special_type == LT_D1_DOOR_YELLOW_OPEN_STAY ||
                line.special_type == LT_SR_DOOR_YELLOW_OPEN_STAY_FAST ||
                line.special_type == LT_S1_DOOR_YELLOW_OPEN_STAY_FAST)
                color = Color(1, 1, 0);
            else if (line.special_type == LT_DR_DOOR_BLUE_OPEN_WAIT_CLOSE ||
                line.special_type == LT_D1_DOOR_BLUE_OPEN_STAY ||
                line.special_type == LT_SR_DOOR_BLUE_OPEN_STAY_FAST ||
                line.special_type == LT_S1_DOOR_BLUE_OPEN_STAY_FAST)
                color = Color(0, 0, 1);
        }

        if (draw_tools && tool == tool_t::region)
        {
            if (mouse_hover_sector != -1)
            {
                if ((line.back_sidedef != -1 && map->sidedefs[line.back_sidedef].sector == mouse_hover_sector) ||
                    (line.front_sidedef != -1 && map->sidedefs[line.front_sidedef].sector == mouse_hover_sector))
                {
                    color = Color(0, 1, 1);
                }
            }
        }
        
        pb->draw(Vector2(map->vertexes[line.start_vertex].x, -map->vertexes[line.start_vertex].y), color);
        pb->draw(Vector2(map->vertexes[line.end_vertex].x, -map->vertexes[line.end_vertex].y), color);

        ++i;
    }

    // Bounding boxes
    if (draw_tools && tool == tool_t::bb)
    {
        int i = 0;
        for (const auto& bb : map_states[ep][lvl].bbs)
        {
            Color color = bb_color;
            if (bb.region != -1 && bb.region < (int)map_states[ep][lvl].regions.size()) color = map_states[ep][lvl].regions[bb.region].tint;
            //if (i == map_state->selected_bb) color = Color(1, 0, 0);
            pb->draw(Vector2(bb.x1, -bb.y1), color); pb->draw(Vector2(bb.x1, -bb.y2), color);
            pb->draw(Vector2(bb.x1, -bb.y2), color); pb->draw(Vector2(bb.x2, -bb.y2), color);
            pb->draw(Vector2(bb.x2, -bb.y2), color); pb->draw(Vector2(bb.x2, -bb.y1), color);
            pb->draw(Vector2(bb.x2, -bb.y1), color); pb->draw(Vector2(bb.x1, -bb.y1), color);
            ++i;
        }
    }

    pb->end();

    // Selected bb
    if (draw_tools && tool == tool_t::bb)
    {
        sb->begin(transform);
        if (map_states[ep][lvl].selected_bb != -1)
        {
            const auto& bb = map_states[ep][lvl].bbs[map_states[ep][lvl].selected_bb];
            sb->drawRect(nullptr, Rect(bb.x1, -bb.y1 - (bb.y2 - bb.y1), bb.x2 - bb.x1, bb.y2 - bb.y1), Color(0.5f, 0, 0, 0.5f));
        }
        sb->end();
    }

    // Vertices
    //sb->begin(transform);
    //for (const auto& v : map->vertexes)
    //{
    //    sb->drawSprite(nullptr, Vector2(v.x, -v.y), bound_color, 0.0f, 3.0f);
    //}
    //sb->end();

    // Items
    sb->begin(transform);
    oRenderer->renderStates.sampleFiltering = OFilterNearest;
    for (const auto& thing : map->things)
    {
        if (thing.flags & 0x0010) continue; // Thing is not in single player
        switch (thing.type)
        {
            case 5:
            case 40:
            case 6:
            case 39:
            case 13:
            case 38:
            case 2018:
            case 8:
            case 2019:
            case 2023:
            case 2022:
            case 2024:
            case 2013:
            case 2006:
            case 2002:
            case 2005:
            case 2004:
            case 2003:
            case 2001:
            case 2026:
                sb->drawSprite(ap_icon, Vector2(thing.x, -thing.y));
                break;
        }
    }
    sb->end();
}


void render()
{
    oRenderer->clear(Color::Black);
    auto pb = oPrimitiveBatch.get();
    auto sb = oSpriteBatch.get();

    pb->begin(OPrimitiveLineList, nullptr, Matrix::Create2DTranslationZoom(OScreenf, map_view->cam_pos, map_view->cam_zoom));
    draw_guides();
    pb->end();

    switch (state)
    {
        case state_t::gen:
        case state_t::gen_panning:
        {
            for (int ep = 0; ep < EP_COUNT; ++ep)
            {
                for (int map = 0; map < MAP_COUNT; ++map)
                {
                    draw_level(ep, map, map_states[ep][map].pos, map_states[ep][map].angle, true);
                }
            }
            sb->begin();
            sb->drawText(OGetFont("font.fnt"), "Step Count = " + std::to_string(gen_step_count), Vector2(0, 50));
            sb->end();
            break;
        }
        default:
        {
            draw_level(active_ep, active_map, {0, 0}, 0, true);
            draw_rules();
            break;
        }
    }

}


std::string unique_name(const Json::Value& dict, const std::string& name)
{
    auto names = dict.getMemberNames();
    int i = 1;
    bool found = false;
    std::string new_name = name;
    while (true)
    {
        for (const auto& other_name : names)
        {
            if (other_name == name)
            {
                i++;
                new_name = name + " " + std::to_string(i);
                continue;
            }
        }
        break;
    }
    return new_name;
}


void renderUI()
{
    ImGui::BeginMainMenuBar();
    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("Save")) save();
        ImGui::Separator();
        if (ImGui::MenuItem("Generate")) generate();
        ImGui::Separator();
        if (ImGui::MenuItem("Exit")) OQuit();
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Edit"))
    {
        if (ImGui::MenuItem("Add Bounding Box")) add_bounding_box();
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Maps"))
    {
        for (int ep = 0; ep < EP_COUNT; ++ep)
        {
            for (int map = 0; map < MAP_COUNT; ++map)
            {
                bool selected = ep == active_ep && map == active_map;
                if (ImGui::MenuItem(level_names[ep][map], nullptr, &selected))
                {
                    select_map(ep, map);
                }
            }
        }
        ImGui::EndMenu();
    }
    if (state != state_t::gen && state != state_t::gen_panning)
    {
        if (ImGui::Begin("Tools"))
        {
            int tooli = (int)tool;
            ImGui::Combo("Tool", &tooli, "Bounding Box\0Region\0Rules\0Access\0");
            tool = (tool_t)tooli;
        }
        ImGui::End();

        if (ImGui::Begin("Regions"))
        {
            static char region_name[260] = {'\0'};
            ImGui::InputText("##region_name", region_name, 260);
            ImGui::SameLine();
            if (ImGui::Button("Add") && strlen(region_name) > 0)
            {
                region_t region;
                region.name = region_name;
                map_state->regions.push_back(region);
                region_name[0] = '\0';
                map_state->selected_region = (int)map_state->regions.size() - 1;
                push_undo();
            }
            {
                int to_move_up = -1;
                int to_move_down = -1;
                int to_delete = -1;
                for (int i = 0; i < (int)map_state->regions.size(); ++i)
                {
                    const auto& region = map_state->regions[i];
                    bool selected = map_state->selected_region == i;
                    if (ImGui::Selectable((std::to_string(i + 1) + " " + region.name).c_str(), selected, 0, ImVec2(150, 22)))
                    {
                        map_state->selected_region = i;
                    }
                    if (map_state->selected_region == i)
                    {
                        ImGui::SameLine(); if (ImGui::Button(" ^ ")) to_move_up = i;
                        ImGui::SameLine(); if (ImGui::Button(" v ")) to_move_down = i;
                        ImGui::SameLine(); if (ImGui::Button("X")) to_delete = i;
                        if (map_state->selected_bb != -1 && tool == tool_t::bb)
                        {
                            ImGui::SameLine();
                            if (ImGui::Button("Assign"))
                            {
                                map_state->bbs[map_state->selected_bb].region = i;
                            }
                        }
                    }
                }
                if (to_move_up > 0)
                {
                    for (auto& bb : map_state->bbs)
                    {
                        if (bb.region == to_move_up)
                            bb.region--;
                        else if (bb.region == to_move_up - 1)
                            bb.region++;
                    }
                    region_t region = map_state->regions[to_move_up];
                    map_state->regions.erase(map_state->regions.begin() + to_move_up);
                    map_state->regions.insert(map_state->regions.begin() + (to_move_up - 1), region);
                    map_state->selected_region = to_move_up - 1;
                    push_undo();
                }
                if (to_move_down != -1 && to_move_down < (int)map_state->regions.size() - 1)
                {
                    for (auto& bb : map_state->bbs)
                    {
                        if (bb.region == to_move_down)
                            bb.region++;
                        else if (bb.region == to_move_down + 1)
                            bb.region--;
                    }
                    region_t region = map_state->regions[to_move_down];
                    map_state->regions.erase(map_state->regions.begin() + to_move_down);
                    map_state->regions.insert(map_state->regions.begin() + (to_move_down + 1), region);
                    map_state->selected_region = to_move_down + 1;
                    push_undo();
                }
                if (to_delete != -1)
                {
                    for (auto& bb : map_state->bbs)
                    {
                        if (bb.region == to_delete)
                            bb.region = -1;
                    }
                    for (auto& region : map_state->regions)
                    {
                        for (int c = 0; c < (int)region.rules.connections.size(); ++c)
                        {
                            auto& connection = region.rules.connections[c];
                            if (connection.target_region == to_delete)
                            {
                                region.rules.connections.erase(region.rules.connections.begin() + c);
                                --c;
                                continue;
                            }
                            if (connection.target_region > to_delete)
                            {
                                connection.target_region--;
                            }
                        }
                    }
                    map_state->regions.erase(map_state->regions.begin() + to_delete);
                    map_state->selected_region = onut::min((int)map_state->regions.size() - 1, map_state->selected_region);
                    push_undo();
                }
            }
        }
        ImGui::End();

        if (ImGui::Begin("Region"))
        {
            if (map_state->selected_region != -1)
            {
                auto& region = map_state->regions[map_state->selected_region];

                static char region_name[260] = {'\0'};
                snprintf(region_name, 260, "%s", region.name.c_str());
                if (ImGui::InputText("Name", region_name, 260, ImGuiInputTextFlags_EnterReturnsTrue))
                {
                    region.name = region_name;
                    push_undo();
                }

                if (ImGui::Checkbox("Connects to Exit", &region.connects_to_exit)) push_undo();
                if (ImGui::Checkbox("Connects to HUB", &region.connects_to_hub)) push_undo();
                ImGui::ColorEdit4("Tint", &region.tint.r, ImGuiColorEditFlags_NoInputs);
                if (ImGui::IsItemDeactivatedAfterEdit()) push_undo();

                ImGui::Separator();
                {
                    ImGui::Text("Required items OR");

                    static char item_name[260] = {'\0'};
                    ImGui::InputText("##item_name_or", item_name, 260);
                    ImGui::SameLine();
                    if (ImGui::Button("Add##item_or") && strlen(item_name) > 0)
                    {
                        region.required_items_or.push_back(item_name);
                        region.selected_item_or = (int)region.required_items_or.size() - 1;
                        push_undo();
                    }
                    if (ImGui::BeginListBox("##item_or_list", ImVec2(300, 0)))
                    {
                        int to_move_up = -1;
                        int to_move_down = -1;
                        int to_delete = -1;
                        for (int i = 0; i < (int)region.required_items_or.size(); ++i)
                        {
                            const auto& item = region.required_items_or[i];
                            bool selected = region.selected_item_or == i;
                            if (ImGui::Selectable((item + "##item_or_" + std::to_string(i)).c_str(), selected, 0, ImVec2(150, 22)))
                            {
                                region.selected_item_or = i;
                            }
                            if (region.selected_item_or == i)
                            {
                                ImGui::SameLine(); if (ImGui::Button(" ^ ##item_or")) to_move_up = i;
                                ImGui::SameLine(); if (ImGui::Button(" v ##item_or")) to_move_down = i;
                                ImGui::SameLine(); if (ImGui::Button("X##item_or")) to_delete = i;
                            }
                        }
                        if (to_move_up > 0)
                        {
                            auto item = region.required_items_or[to_move_up];
                            region.required_items_or.erase(region.required_items_or.begin() + to_move_up);
                            region.required_items_or.insert(region.required_items_or.begin() + (to_move_up - 1), item);
                            region.selected_item_or = to_move_up - 1;
                            push_undo();
                        }
                        if (to_move_down != -1 && to_move_down < (int)map_state->regions.size() - 1)
                        {
                            auto item = region.required_items_or[to_move_down];
                            region.required_items_or.erase(region.required_items_or.begin() + to_move_down);
                            region.required_items_or.insert(region.required_items_or.begin() + (to_move_down + 1), item);
                            region.selected_item_or = to_move_down + 1;
                            push_undo();
                        }
                        if (to_delete != -1)
                        {
                            region.required_items_or.erase(region.required_items_or.begin() + to_delete);
                            region.selected_item_or = onut::min((int)region.required_items_or.size() - 1, region.selected_item_or);
                            push_undo();
                        }
                        ImGui::EndListBox();
                    }
                }

                ImGui::Separator();
                {
                    ImGui::Text("Required items AND");

                    static char item_name[260] = {'\0'};
                    ImGui::InputText("##item_name_and", item_name, 260);
                    ImGui::SameLine();
                    if (ImGui::Button("Add##item_and") && strlen(item_name) > 0)
                    {
                        region.required_items_and.push_back(item_name);
                        region.selected_item_and = (int)region.required_items_and.size() - 1;
                        push_undo();
                    }
                    if (ImGui::BeginListBox("##item_and_list", ImVec2(300, 0)))
                    {
                        int to_move_up = -1;
                        int to_move_down = -1;
                        int to_delete = -1;
                        for (int i = 0; i < (int)region.required_items_and.size(); ++i)
                        {
                            const auto& item = region.required_items_and[i];
                            bool selected = region.selected_item_and == i;
                            if (ImGui::Selectable((item + "##item_and_" + std::to_string(i)).c_str(), selected, 0, ImVec2(150, 22)))
                            {
                                region.selected_item_and = i;
                            }
                            if (region.selected_item_and == i)
                            {
                                ImGui::SameLine(); if (ImGui::Button(" ^ ##item_and")) to_move_up = i;
                                ImGui::SameLine(); if (ImGui::Button(" v ##item_and")) to_move_down = i;
                                ImGui::SameLine(); if (ImGui::Button("X##item_and")) to_delete = i;
                            }
                        }
                        if (to_move_up > 0)
                        {
                            auto item = region.required_items_and[to_move_up];
                            region.required_items_and.erase(region.required_items_and.begin() + to_move_up);
                            region.required_items_and.insert(region.required_items_and.begin() + (to_move_up - 1), item);
                            region.selected_item_and = to_move_up - 1;
                            push_undo();
                        }
                        if (to_move_down != -1 && to_move_down < (int)map_state->regions.size() - 1)
                        {
                            auto item = region.required_items_and[to_move_down];
                            region.required_items_and.erase(region.required_items_and.begin() + to_move_down);
                            region.required_items_and.insert(region.required_items_and.begin() + (to_move_down + 1), item);
                            region.selected_item_and = to_move_down + 1;
                            push_undo();
                        }
                        if (to_delete != -1)
                        {
                            region.required_items_and.erase(region.required_items_and.begin() + to_delete);
                            region.selected_item_and = onut::min((int)region.required_items_and.size() - 1, region.selected_item_and);
                            push_undo();
                        }
                        ImGui::EndListBox();
                    }
                }
            }
        }
        ImGui::End();

        if (ImGui::Begin("Connection"))
        {
            if (set_rule_rule != -3 && set_rule_connection != -1)
            {
                auto rules = get_rules(set_rule_rule);
                if (rules)
                {
                    if (ImGui::Button("Remove"))
                    {
                        rules->connections.erase(rules->connections.begin() + set_rule_connection);
                        set_rule_rule = -3;
                        set_rule_connection = -1;
                        push_undo();
                    }
                    else
                    {
                        auto& connection = rules->connections[set_rule_connection];

                        ImGui::Columns(2);
                        ImGui::Text("OR");
                        ImGui::NextColumn();
                        ImGui::Text("AND");
                        ImGui::NextColumn();

                        for (auto requirement : REQUIREMENTS)
                        {
                            {
                                ImVec4 tint(0.25f, 0.25f, 0.25f, 1);
                                bool has_requirement = false;
                                auto it = std::find(connection.requirements_or.begin(), connection.requirements_or.end(), requirement);
                                if (it != connection.requirements_or.end())
                                {
                                    has_requirement = true;
                                    tint = {1,1,1,1};
                                }

                                if (ImGui::ImageButton(
                                        ("or_btn_" + std::to_string(requirement)).c_str(), // str_id
                                        &REQUIREMENT_TEXTURES[requirement], // user_texture_id
                                        {64, 64}, // size
                                        {0,0}, // uv0
                                        {1,1}, // uv1
                                        {0, 0, 0, 0},
                                        tint))
                                {
                                    if (has_requirement)
                                    {
                                        connection.requirements_or.erase(it);
                                    }
                                    else
                                    {
                                        connection.requirements_or.push_back(requirement);
                                    }
                                    push_undo();
                                }
                                ImGui::NextColumn();
                            }
                            {
                                ImVec4 tint(0.25f, 0.25f, 0.25f, 1);
                                bool has_requirement = false;
                                auto it = std::find(connection.requirements_and.begin(), connection.requirements_and.end(), requirement);
                                if (it != connection.requirements_and.end())
                                {
                                    has_requirement = true;
                                    tint = {1,1,1,1};
                                }
                                if (ImGui::ImageButton(("and_btn_" + std::to_string(requirement)).c_str(), &REQUIREMENT_TEXTURES[requirement], {64, 64}, {0,0}, {1,1}, {0, 0, 0, 0}, tint))
                                {
                                    if (has_requirement)
                                    {
                                        connection.requirements_and.erase(it);
                                    }
                                    else
                                    {
                                        connection.requirements_and.push_back(requirement);
                                    }
                                    push_undo();
                                }
                                ImGui::NextColumn();
                            }
                        }
                    }
                }
            }
        }
        ImGui::End();

        if (ImGui::Begin("Map"))
        {

        }
        ImGui::End();
    }

    ImGui::EndMainMenuBar();
}


void postRender()
{
}
