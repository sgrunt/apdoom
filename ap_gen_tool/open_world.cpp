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

#include "maps.h"
#include "generate.h"


enum class state_t
{
    idle,
    panning,
    gen,
    gen_panning,
    move_bb
};


enum class tool_t : int
{
    bb,
    region
};


struct bb_t
{
    int x1, y1, x2, y2;

    bool operator!=(const bb_t& other) const
    {
        return x1 != other.x1 || x2 != other.x2 || y1 != other.y1 || y2 != other.y2;
    }

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
    std::vector<int> sectors;
    std::vector<std::string> required_items_or;
    std::vector<std::string> required_items_and;
    Color tint = Color::White;
    int selected_item_or = -1;
    int selected_item_and = -1;
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
static tool_t tool = tool_t::bb;
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
static Json::Value json;
static Json::Value* map_json = nullptr;
static bool generating = true;
static map_state_t* flat_levels[EP_COUNT * MAP_COUNT];
static int gen_step_count = 0;


void save()
{
    for (int ep = 0; ep < EP_COUNT; ++ep)
    {
        for (int lvl = 0; lvl < MAP_COUNT; ++lvl)
        {
            auto& _map_json = json[level_names[ep][lvl]];
            Json::Value bbs_json(Json::arrayValue);
            for (const auto& bb : map_states[ep][lvl].bbs)
            {
                Json::Value bb_json(Json::arrayValue);
                bb_json.append(bb.x1);
                bb_json.append(bb.y1);
                bb_json.append(bb.x2);
                bb_json.append(bb.y2);
                bbs_json.append(bb_json);
            }
            _map_json["bbs"] = bbs_json;
        }
    }

    std::string filename = OArguments[2] + std::string("\\regions.json");
    onut::saveJson(json, filename);
}


void load()
{
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
            auto& _map_json = json[level_names[ep][lvl]];
            const auto& bbs_json = _map_json["bbs"];
            for (const auto& bb_json : bbs_json)
            {
                _map_state.bbs.push_back({
                    bb_json[0].asInt(),
                    bb_json[1].asInt(),
                    bb_json[2].asInt(),
                    bb_json[3].asInt()
                });
            }
            auto region_names = _map_json["Regions"].getMemberNames();
            for (const auto& region_name : region_names)
            {
                const auto& region_json = _map_json["Regions"][region_name];
                region_t region;
                region.name = region_name;
                region.connects_to_exit = region_json.get("connects_to_exit", false).asBool();
                region.connects_to_hub = region_json.get("connects_to_hub", false).asBool();
                region.required_items_or = onut::deserializeStringArray(region_json["required_items_or"]);
                region.required_items_and = onut::deserializeStringArray(region_json["required_items_and"]);
                _map_state.regions.push_back(region);
            }
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
    active_ep = ep;
    active_map = map;
    map_state = &map_states[active_ep][active_map];
    map_view = &map_views[active_ep][active_map];
    map_history = &map_histories[active_ep][active_map];

    map_json = nullptr;
    auto map_names = json.getMemberNames();
    for (const auto& map_name : map_names)
    {
        auto& _map_json = json[map_name];
        if (_map_json["episode"].asInt() == active_ep + 1 &&
            _map_json["map"].asInt() == active_map + 1)
        {
            map_json = &_map_json;
            break;
        }
    }

    assert(map_json);

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
            auto mid = Vector2((map->bb[2] + map->bb[0]) / 2, -(map->bb[3] + map->bb[1]) / 2);
            map_states[ep][lvl].pos = -mid + onut::rand2f(Vector2(-1000, -1000), Vector2(1000, 1000));
        }
    }
}


void init()
{
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
            map_views[ep][lvl].cam_pos = Vector2((map->bb[2] + map->bb[0]) / 2, -(map->bb[3] + map->bb[1]) / 2);
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


void delete_selected()
{
    if (map_state->selected_bb != -1)
    {
        map_state->bbs.erase(map_state->bbs.begin() + map_state->selected_bb);
        map_state->selected_bb = -1;
        push_undo();
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


void update_gen()
{
    bool overlapped = false;
    auto dt = 0.01f;
    // TODO: Optimize that with chunks? Only care about bounding boxes and not levels?
    for (int i = 0; i < EP_COUNT * MAP_COUNT; ++i)
    {
        for (int k = 0; k < (int)flat_levels[i]->bbs.size(); ++k)
        {
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
                    
                    if (OInputPressed(OMouse1))
                    {
                        // "paint" selected region
                    }
                    else if (OInputJustPressed(OKeyF))
                    {
                        // Fill selected region
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
                    map_state->bbs[map_state->selected_bb].x1 = bb_on_down.x1 + diff.x;
                    map_state->bbs[map_state->selected_bb].x2 = bb_on_down.x2 + diff.x;
                    map_state->bbs[map_state->selected_bb].y1 = bb_on_down.y1 - diff.y;
                    map_state->bbs[map_state->selected_bb].y2 = bb_on_down.y2 - diff.y;
                    break;
                case 0:
                    map_state->bbs[map_state->selected_bb].x1 = bb_on_down.x1 + diff.x;
                    break;
                case 1:
                    map_state->bbs[map_state->selected_bb].y1 = bb_on_down.y1 - diff.y;
                    break;
                case 2:
                    map_state->bbs[map_state->selected_bb].x2 = bb_on_down.x2 + diff.x;
                    break;
                case 3:
                    map_state->bbs[map_state->selected_bb].y2 = bb_on_down.y2 - diff.y;
                    break;
            }
            if (OInputJustReleased(OMouse1))
            {
                push_undo();
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


void draw_level(int ep, int lvl, const Vector2& pos, float angle, bool draw_tools)
{
    Color bound_color(1.0f);
    Color step_color(0.35f);
    Color bb_color(1, 1, 0, 1);

    auto pb = oPrimitiveBatch.get();
    auto sb = oSpriteBatch.get();
    auto map = &maps[ep][lvl];

    auto transform = 
              Matrix::CreateRotationZ(angle) *
              Matrix::CreateTranslation(Vector2(pos.x, -pos.y)) *
              Matrix::Create2DTranslationZoom(OScreenf, map_view->cam_pos, map_view->cam_zoom);

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
            if (i == map_state->selected_bb) color = Color(1, 0, 0);
            pb->draw(Vector2(bb.x1, -bb.y1), color); pb->draw(Vector2(bb.x1, -bb.y2), color);
            pb->draw(Vector2(bb.x1, -bb.y2), color); pb->draw(Vector2(bb.x2, -bb.y2), color);
            pb->draw(Vector2(bb.x2, -bb.y2), color); pb->draw(Vector2(bb.x2, -bb.y1), color);
            pb->draw(Vector2(bb.x2, -bb.y1), color); pb->draw(Vector2(bb.x1, -bb.y1), color);
            ++i;
        }
    }

    pb->end();

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
                    draw_level(ep, map, map_states[ep][map].pos, map_states[ep][map].angle, false);
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
            ImGui::Combo("Tool", &tooli, "Bounding Box\0Region\0");
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
                    if (ImGui::Selectable(region.name.c_str(), selected, 0, ImVec2(150, 22)))
                    {
                        map_state->selected_region = i;
                    }
                    if (map_state->selected_region == i)
                    {
                        ImGui::SameLine(); if (ImGui::Button(" ^ ")) to_move_up = i;
                        ImGui::SameLine(); if (ImGui::Button(" v ")) to_move_down = i;
                        ImGui::SameLine(); if (ImGui::Button("X")) to_delete = i;
                    }
                }
                if (to_move_up > 0)
                {
                    region_t region = map_state->regions[to_move_up];
                    map_state->regions.erase(map_state->regions.begin() + to_move_up);
                    map_state->regions.insert(map_state->regions.begin() + (to_move_up - 1), region);
                    map_state->selected_region = to_move_up - 1;
                    push_undo();
                }
                if (to_move_down != -1 && to_move_down < (int)map_state->regions.size() - 1)
                {
                    region_t region = map_state->regions[to_move_down];
                    map_state->regions.erase(map_state->regions.begin() + to_move_down);
                    map_state->regions.insert(map_state->regions.begin() + (to_move_down + 1), region);
                    map_state->selected_region = to_move_down + 1;
                    push_undo();
                }
                if (to_delete != -1)
                {
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
    }

    ImGui::EndMainMenuBar();
}


void postRender()
{
}
