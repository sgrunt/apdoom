#include <onut/onut.h>
#include <onut/Settings.h>
#include <onut/Renderer.h>
#include <onut/PrimitiveBatch.h>
#include <onut/Window.h>
#include <onut/Input.h>
#include <onut/SpriteBatch.h>
#include <onut/Texture.h>

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


struct bb_t
{
    int x1, y1, x2, y2;
    bool operator!=(const bb_t& other)
    {
        return x1 != other.x1 || x2 != other.x2 || y1 != other.y1 || y2 != other.y2;
    }
};


struct map_state_t
{
    Vector2 pos;
    float angle = 0.0f;
    std::vector<bb_t> bbs;
    int selected_bb = -1;
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
static Vector2 mouse_pos;
static Vector2 mouse_pos_on_down;
static Vector2 cam_pos_on_down;
static bb_t bb_on_down;
static map_state_t map_state;
static map_view_t* map_view = nullptr;
static map_history_t* map_history = nullptr;
static OTextureRef ap_icon;
static int mouse_hover_bb = -1;
static int moving_edge = -1;


void update_window_title()
{
    oWindow->setCaption(level_names[active_ep][active_map]);
}


void select_map(int ep, int map)
{
    active_ep = ep;
    active_map = map;
    map_state = map_states[active_ep][active_map];
    map_view = &map_views[active_ep][active_map];
    map_history = &map_histories[active_ep][active_map];
    update_window_title();
}


// Undo/Redo shit
void push_undo()
{
    if (map_history->history_point < (int)map_histories[active_ep][active_map].history.size() - 1)
        map_history->history.erase(map_history->history.begin() + (map_history->history_point + 1), map_history->history.end());
    map_history->history.push_back(map_state);
    map_history->history_point = (int)map_history->history.size() - 1;
}


void undo()
{
    if (map_history->history_point > 0)
    {
        map_history->history_point--;
        map_state = map_history->history[map_history->history_point];
    }
}


void redo()
{
    if (map_history->history_point < (int)map_history->history.size() - 1)
    {
        map_history->history_point++;
        map_state = map_history->history[map_history->history_point];
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


void init()
{
    ap_icon = OGetTexture("ap.png");

    init_maps();
    select_map(0, 0);

    for (int ep = 0; ep < EP_COUNT; ++ep)
    {
        for (int lvl = 0; lvl < MAP_COUNT; ++lvl)
        {
            auto k = ep * 9 + lvl;
            map_states[ep][lvl].pos = Vector2(
                -16384 + 3000 + ((k % 5) * 6000),
                -16384 + 3000 + ((k / 5) * 6000)
            );

            auto map = &maps[ep][lvl];
            map_views[ep][lvl].cam_pos = Vector2((map->bb[2] + map->bb[0]) / 2, -(map->bb[3] + map->bb[1]) / 2);
        }
    }
}


void shutdown() // lol
{
}


void add_bounding_box()
{
    auto map = &maps[active_ep][active_map];
    map_state.bbs.push_back({map->bb[0], map->bb[1], map->bb[2], map->bb[3]});
    map_state.selected_bb = (int)map_state.bbs.size() - 1;
    push_undo();
}


void update_shortcuts()
{
    auto ctrl = OInputPressed(OKeyLeftControl);
    auto shift = OInputPressed(OKeyLeftShift);
    auto alt = OInputPressed(OKeyLeftAlt);

    if (ctrl && !shift && !alt && OInputJustPressed(OKeyZ)) undo();
    if (ctrl && shift && !alt && OInputJustPressed(OKeyZ)) redo();
    if (!ctrl && !shift && !alt && OInputJustPressed(OKeyB)) add_bounding_box();
}


int get_bb_at(const Vector2& pos, float zoom, int &edge)
{
    edge = -1;
    float edge_size = 32.0f / zoom;
    for (int i = 0; i < (int)map_state.bbs.size(); ++i)
    {
        const auto& bb = map_state.bbs[i];
        if (pos.x >= bb.x1 - edge_size && pos.x <= bb.x1 + edge_size && pos.y <= -bb.y1 && pos.y >= -bb.y2)
        {
            edge = 0;
            return i;
        }
        if (pos.y <= -bb.y1 + edge_size && pos.y >= -bb.y1 - edge_size && pos.x >= bb.x1 && pos.x <= bb.x2)
        {
            edge = 1;
            return i;
        }
        if (pos.x >= bb.x2 - edge_size && pos.x <= bb.x2 + edge_size && pos.y <= -bb.y1 && pos.y >= -bb.y2)
        {
            edge = 2;
            return i;
        }
        if (pos.y <= -bb.y2 + edge_size && pos.y >= -bb.y2 - edge_size && pos.x >= bb.x1 && pos.x <= bb.x2)
        {
            edge = 3;
            return i;
        }
        if (pos.x >= bb.x1 && pos.x <= bb.x2 && pos.y <= -bb.y1 && pos.y >= -bb.y2)
        {
            return i;
        }
    }
    return -1;
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
            mouse_hover_bb = get_bb_at(mouse_pos, map_view->cam_zoom, moving_edge);
            update_shortcuts();
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
            else if (OInputJustPressed(OMouse1))
            {
                if (mouse_hover_bb != -1)
                {
                    map_state.selected_bb = mouse_hover_bb;
                    state = state_t::move_bb;
                    bb_on_down = map_state.bbs[mouse_hover_bb];
                    mouse_pos_on_down = OGetMousePos();
                }
                else if (map_state.selected_bb != -1)
                {
                    map_state.selected_bb = -1;
                    push_undo();
                }
            }
            break;
        }
        case state_t::panning:
        {
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
                    map_state.bbs[map_state.selected_bb].x1 = bb_on_down.x1 + diff.x;
                    map_state.bbs[map_state.selected_bb].x2 = bb_on_down.x2 + diff.x;
                    map_state.bbs[map_state.selected_bb].y1 = bb_on_down.y1 - diff.y;
                    map_state.bbs[map_state.selected_bb].y2 = bb_on_down.y2 - diff.y;
                    break;
                case 0:
                    map_state.bbs[map_state.selected_bb].x1 = bb_on_down.x1 + diff.x;
                    break;
                case 1:
                    map_state.bbs[map_state.selected_bb].y1 = bb_on_down.y1 - diff.y;
                    break;
                case 2:
                    map_state.bbs[map_state.selected_bb].x2 = bb_on_down.x2 + diff.x;
                    break;
                case 3:
                    map_state.bbs[map_state.selected_bb].y2 = bb_on_down.y2 - diff.y;
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

    pb->draw(Vector2(-16384, -16384), Color(1, 0, 0)); pb->draw(Vector2(-16384, 16384), Color(1, 0, 0));
    pb->draw(Vector2(-16384, 16384), Color(1, 0, 0)); pb->draw(Vector2(16384, 16384), Color(1, 0, 0));
    pb->draw(Vector2(16384, 16384), Color(1, 0, 0)); pb->draw(Vector2(16384, -16384), Color(1, 0, 0));
    pb->draw(Vector2(16384, -16384), Color(1, 0, 0)); pb->draw(Vector2(-16384, -16384), Color(1, 0, 0));
}


void draw_level(int ep, int lvl, const Vector2& pos, float angle)
{
    Color bound_color(1.0f);
    Color step_color(0.35f);
    Color bb_color(1, 1, 0, 1);

    auto pb = oPrimitiveBatch.get();
    auto sb = oSpriteBatch.get();
    auto map = &maps[ep][lvl];

    auto transform = 
              Matrix::CreateRotationZ(angle) *
              Matrix::CreateTranslation(pos) *
              Matrix::Create2DTranslationZoom(OScreenf, map_view->cam_pos, map_view->cam_zoom);

    // Geometry
    pb->begin(OPrimitiveLineList, nullptr, transform);

    // Bounding box
    //pb->draw(Vector2(map->bb[0], -map->bb[1]), bb_color); pb->draw(Vector2(map->bb[0], -map->bb[3]), bb_color);
    //pb->draw(Vector2(map->bb[0], -map->bb[3]), bb_color); pb->draw(Vector2(map->bb[2], -map->bb[3]), bb_color);
    //pb->draw(Vector2(map->bb[2], -map->bb[3]), bb_color); pb->draw(Vector2(map->bb[2], -map->bb[1]), bb_color);
    //pb->draw(Vector2(map->bb[2], -map->bb[1]), bb_color); pb->draw(Vector2(map->bb[0], -map->bb[1]), bb_color);

    // Geometry
    for (const auto& line : map->linedefs)
    {
        Color color = bound_color;
        if (line.back_sidedef != -1) color = step_color;
        
        pb->draw(Vector2(map->vertexes[line.start_vertex].x, -map->vertexes[line.start_vertex].y), color);
        pb->draw(Vector2(map->vertexes[line.end_vertex].x, -map->vertexes[line.end_vertex].y), color);
    }

    // Bounding boxes
    int i = 0;
    for (const auto& bb : map_state.bbs)
    {
        Color color = bb_color;
        if (i == map_state.selected_bb) color = Color(1, 0, 0);
        pb->draw(Vector2(bb.x1, -bb.y1), bb_color); pb->draw(Vector2(bb.x1, -bb.y2), bb_color);
        pb->draw(Vector2(bb.x1, -bb.y2), bb_color); pb->draw(Vector2(bb.x2, -bb.y2), bb_color);
        pb->draw(Vector2(bb.x2, -bb.y2), bb_color); pb->draw(Vector2(bb.x2, -bb.y1), bb_color);
        pb->draw(Vector2(bb.x2, -bb.y1), bb_color); pb->draw(Vector2(bb.x1, -bb.y1), bb_color);
        ++i;
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
                    draw_level(ep, map, map_state.pos, map_state.angle);
                }
            }
            break;
        }
        default:
        {
            draw_level(active_ep, active_map, {0, 0}, 0);
            break;
        }
    }
}


void renderUI()
{
    ImGui::BeginMainMenuBar();
    if (ImGui::BeginMenu("File"))
    {
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
                if (ImGui::MenuItem(level_names[ep][map]))
                {
                    select_map(ep, map);
                }
            }
        }
        ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
}


void postRender()
{
}
