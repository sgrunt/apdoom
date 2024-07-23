//
// Copyright(C) 2023 David St-Louis
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
//
// *Level select feature for archipelago*
//

#include "level_select.h"
#include "h2def.h"
#include "s_sound.h"
#include "z_zone.h"
#include "v_video.h"
#include "doomkeys.h"
#include "apdoom.h"
#include "i_video.h"
#include "m_misc.h"
#include "ap_msg.h"
#include "m_controls.h"
#include "i_timer.h"

extern boolean automapactive;


typedef struct
{
    int x;
    int y;
    char* urhere_lump_name;
    int urhere_x_offset;
    int urhere_y_offset;
    int display_as_line;
} level_pos_t;

const char* level_names[5] = {
    "SEVEN PORTALS",
    "SHADOW WOOD",
    "HERESIARCH'S SEMINARY",
    "CASTLE OF GRIEF",
    "NECROPOLIS"
};


extern int cursor_x;
extern int cursor_y;

static level_pos_t level_pos_infos[5] =
{
        { 40, 15 + 21 * 0, "IN_YAH", 0, 0, 1 },
        { 40, 15 + 21 * 1, "IN_YAH", 0, 0, 1 },
        { 40, 15 + 21 * 2, "IN_YAH", 0, 0, 1 },
        { 40, 15 + 21 * 3, "IN_YAH", 0, 0, 1 },
        { 40, 15 + 21 * 4, "IN_YAH", 0, 0, 1 }
};


int selected_level = 0;
int urh_anim = 0;
int activating_level_select_anim = 0; //200;

static int map_for_level[5] = { 1, 13, 27, 22, 35 };

static const char* YELLOW_DIGIT_LUMP_NAMES[] = {
    "SMALLIN0", "SMALLIN1", "SMALLIN2", "SMALLIN3", "SMALLIN4", 
    "SMALLIN5", "SMALLIN6", "SMALLIN7", "SMALLIN8", "SMALLIN9"
};


void print_right_aligned_yellow_digit(int x, int y, int digit)
{
    x -= 4;

    if (!digit)
    {
        V_DrawPatch(x, y, W_CacheLumpName(YELLOW_DIGIT_LUMP_NAMES[0], PU_CACHE));
        return;
    }

    while (digit)
    {
        int i = digit % 10;
        V_DrawPatch(x, y, W_CacheLumpName(YELLOW_DIGIT_LUMP_NAMES[i], PU_CACHE));
        x -= 4;
        digit /= 10;
    }
}


void print_left_aligned_yellow_digit(int x, int y, int digit)
{
    if (!digit)
    {
        x += 4;
    }

    int len = 0;
    int d = digit;
    while (d)
    {
        len++;
        d /= 10;
    }
    print_right_aligned_yellow_digit(x + len * 4, y, digit);
}


void play_level(int lvl)
{
    // Check if level has a save file first
    char filename[260];
    snprintf(filename, 260, "%s/hex0.hxs", apdoom_get_seed());
    if (M_FileExists(filename))
    {
        // We load
        gameaction = ga_loadgame;
        G_DoLoadGame();
	G_TeleportNewMap(lvl, 0);
    }
    else
    {
        // If none, load it fresh
	PlayerClass[consoleplayer] = ap_state.player_class;
        G_DeferedInitNew(gameskill, 1, lvl);
    }

    //apdoom_check_victory(); // In case we had pending victory

    HU_ClearAPMessages();
}

static void level_select_prev_episode()
{
}


static void level_select_next_episode()
{
}


void select_map_dir(int dir)
{
    int from = selected_level;
    float fromx = (float)level_pos_infos[from].x;
    float fromy = (float)level_pos_infos[from].y;

    int best = from;
    int top_most = 0; //200;
    int top_most_idx = -1;
    int bottom_most = 0;
    int bottom_most_idx = -1;
    float best_score = 0.0f;
    
    int map_count = 5; //ap_get_map_count(selected_ep + 1);
    for (int i = 0; i < map_count; ++i)
    {
        if (level_pos_infos[i].y < top_most)
        {
            top_most = level_pos_infos[i].y;
            top_most_idx = i;
        }
        if (level_pos_infos[i].y > bottom_most)
        {
            bottom_most = level_pos_infos[i].y;
            bottom_most_idx = i;
        }
        if (i == from) continue;

        float tox = (float)level_pos_infos[i].x;
        float toy = (float)level_pos_infos[i].y;
        float score = 0.0f;
        float dist = 10000.0f;

        switch (dir)
        {
            case 0: // Left
                if (tox >= fromx) continue;
                dist = fromx - tox;
                break;
            case 1: // Right
                if (tox <= fromx) continue;
                dist = tox - fromx;
                break;
            case 2: // Up
                if (toy >= fromy) continue;
                dist = fromy - toy;
                break;
            case 3: // Down
                if (toy <= fromy) continue;
                dist = toy - fromy;
                break;
        }
        score = 1.0f / dist;

        if (score > best_score)
        {
            best_score = score;
            best = i;
        }
    }

    // Are we at the top? go to the bottom
    if (from == top_most_idx && dir == 2)
    {
        best = bottom_most_idx;
    }
    else if (from == bottom_most_idx && dir == 3)
    {
        best = top_most_idx;
    }

    if (best != from)
    {
        urh_anim = 0;
        S_StartSound(NULL, SFX_PICKUP_KEY);
        selected_level = best;
    }
}


static void level_select_nav_enter()
{
    if (ap_get_level_state(ap_make_level_index(selected_level + 1, 1))->unlocked)
    {
        S_StartSound(NULL, SFX_DOOR_METAL_CLOSE);
        play_level(map_for_level[selected_level]);
    }
    else
    {
        S_StartSound(NULL, SFX_ARTIFACT_USE);
    }
}


boolean LevelSelectResponder(event_t* ev)
{
    if (activating_level_select_anim) return true;

    int ep_count = 0;

    switch (ev->type)
    {
        case ev_joystick:
        {
            if (ev->data4 < 0 || ev->data2 < 0)
            {
                select_map_dir(0);
                joywait = I_GetTime() + 5;
            }
            else if (ev->data4 > 0 || ev->data2 > 0)
            {
                select_map_dir(1);
                joywait = I_GetTime() + 5;
            }
            else if (ev->data3 < 0)
            {
                //select_map_dir(2);
                level_select_prev_episode();
                joywait = I_GetTime() + 5;
            }
            else if (ev->data3 > 0)
            {
                //select_map_dir(3);
                joywait = I_GetTime() + 5;
                level_select_next_episode();
            }

#define JOY_BUTTON_MAPPED(x) ((x) >= 0)
#define JOY_BUTTON_PRESSED(x) (JOY_BUTTON_MAPPED(x) && (ev->data1 & (1 << (x))) != 0)

            if (JOY_BUTTON_PRESSED(joybfire)) level_select_nav_enter();

            if (JOY_BUTTON_PRESSED(joybprevweapon)) level_select_prev_episode();
            else if (JOY_BUTTON_PRESSED(joybnextweapon)) level_select_next_episode();

            break;
        }
        case ev_keydown:
        {
            if (ev->data1 == key_left || ev->data1 == key_alt_strafeleft || ev->data1 == key_strafeleft) level_select_prev_episode();
            if (ev->data1 == key_right || ev->data1 == key_alt_straferight || ev->data1 == key_straferight) level_select_next_episode();
            if (ev->data1 == key_up || ev->data1 == key_alt_up) select_map_dir(2);
            if (ev->data1 == key_down || ev->data1 == key_alt_down) select_map_dir(3);
            if (ev->data1 == key_menu_forward || ev->data1 == key_use) level_select_nav_enter();
            break;
        }
    }

    return true;
}

void G_DoSaveGame(void);

void ShowLevelSelect()
{
    HU_ClearAPMessages();

    // If in a level, save current level
    if (gamestate == GS_LEVEL)
        G_DoSaveGame(); 

    if (crispy->ap_levelselectmusic)
        S_StartSong(mus_intr, true);
    else
    {
        extern int Mus_Song;
        Mus_Song = -1;
        I_StopSong();
    }

    gameaction = ga_nothing;
    gamestate = GS_LEVEL_SELECT;
    viewactive = false;
    automapactive = false;

    activating_level_select_anim = 200;
    ap_state.ep = 0;
    ap_state.map = 0;
    players[consoleplayer].message[0] = '\0';

    selected_level = 1;

/*    while (!ap_state.episodes[selected_level])
    {
        selected_level = (selected_level + 1) % ap_episode_count;
        if (selected_level == 0) // oops;
            break;
    } */
}


void TickLevelSelect()
{
    if (activating_level_select_anim > 0)
    {
        activating_level_select_anim -= 6;
        if (activating_level_select_anim < 0)
            activating_level_select_anim = 0;
        else
            return;
    }
    urh_anim = (urh_anim + 1) % 35;
}


void draw_legend_line_right_aligned(const char* text, int x, int y)
{
    int w = MN_TextAWidth_len(text, strlen(text));
    MN_DrTextA(text, x - w, y);
}


void draw_legend_line(const char* text, int x, int y)
{
    MN_DrTextA(text, x, y);
}


void DrawEpisodicLevelSelectStats()
{
    int x, y;
    const int key_spacing = 5;
    const int stat_y_offset = 8;
    
    int map_count = 5; //ap_get_map_count(selected_ep + 1);
    for (int i = 0; i < map_count; ++i)
    {
        level_pos_t* level_pos = &level_pos_infos[i];
        ap_level_info_t* ap_level_info = ap_get_level_info(ap_make_level_index(i + 1, 1));
        ap_level_state_t* ap_level_state = ap_get_level_state(ap_make_level_index(i + 1, 1));

        x = level_pos->x;
        y = level_pos->y;
        
        if (level_pos->display_as_line)
        {
            // Text
            const char* level_name = level_names[i];

            // Remove the '- E1M1' at the end
            char name[80];
            snprintf(name, 80, "%s", level_name);
/*            for (int i = 0; i < 80; ++i)
            {
                if (name[i] == '-')
                {
                    name[i] = '\0';
                    break;
                }
            }*/

            int text_w = MN_TextBWidth(name);
            MN_DrTextB(name, x + 20, y - 10);
        
            // Level complete splash
            if (ap_level_state->completed)
                V_DrawPatch(x, y, W_CacheLumpName("FONTA56", PU_CACHE));

            // Lock
            if (!ap_level_state->unlocked)
                V_DrawPatch(x, y, W_CacheLumpName("FONTAY03", PU_CACHE));

            // Progress
            print_right_aligned_yellow_digit(x + 30 + text_w - 4, y - 1, ap_level_state->check_count);
            V_DrawPatch(x + 30 + text_w - 3, y - 1, W_CacheLumpName("FONTA15", PU_CACHE));
//            print_left_aligned_yellow_digit(x + 30 + text_w + 3, y - 1, ap_state.check_sanity ? ap_level_info->check_count : ap_level_info->check_count - ap_level_info->sanity_check_count);
        }
        else
        {
            // Level complete splash
            if (ap_level_state->completed)
                V_DrawPatch(x, y, W_CacheLumpName("FONTA56", PU_CACHE));

            // Lock
            if (!ap_level_state->unlocked)
                V_DrawPatch(x, y, W_CacheLumpName("FONTAY03", PU_CACHE));

            // Progress
            print_right_aligned_yellow_digit(x - 4, y + stat_y_offset, ap_level_state->check_count);
            V_DrawPatch(x - 3, y + stat_y_offset, W_CacheLumpName("FONTA15", PU_CACHE));
//            print_left_aligned_yellow_digit(x + 3, y + stat_y_offset, ap_state.check_sanity ? ap_level_info->check_count : ap_level_info->check_count - ap_level_info->sanity_check_count);
        }
    }

    level_pos_t* selected_level_pos = &level_pos_infos[selected_level];
    if (!selected_level_pos->display_as_line)
    {
        // "You are here"
        if (urh_anim < 25)
        {
            x = selected_level_pos->x;
            y = selected_level_pos->y;
            int x_offset = 2;
            int y_offset = -2;
            V_DrawPatch(x + x_offset + selected_level_pos->urhere_x_offset, 
                        y + y_offset + selected_level_pos->urhere_y_offset, 
                        W_CacheLumpName(selected_level_pos->urhere_lump_name, PU_CACHE));
        }

        // Level name
        const char* level_name = level_names[selected_level];
        int text_x = 160 - MN_TextBWidth(level_name) / 2;
        int text_y = 200 - 20;
        MN_DrTextB(level_name, text_x, text_y);
    }
    else
    {
        x = selected_level_pos->x;
        y = selected_level_pos->y;
        V_DrawPatch(x - 20, y - 6, W_CacheLumpName("INVGEMR1", PU_CACHE));
    }

    // Legend
    //int lx = legendes[selected_ep].x;
    //int ly = legendes[selected_ep].y;
    //typedef void (*draw_legend_line_fn_t)(const char* text, int x, int y);
    //draw_legend_line_fn_t draw_legend_line_fn = draw_legend_line;
    //if (legendes[selected_ep].right_align) draw_legend_line_fn = draw_legend_line_right_aligned;
    //draw_legend_line_fn("~2Change map: ~3Arrows", lx, ly);
    //draw_legend_line_fn("~2Change episode: ~3(~2, ~3)", lx, ly + 8);
    //draw_legend_line_fn("~2Enter map: ~3Enter", lx, ly + 16);
}


void DrawLevelSelectStats()
{
    DrawEpisodicLevelSelectStats();
}


static const char* WIN_MAP = "INTERPIC";


void DrawLevelSelect()
{
    int x_offset = 0;

    // [crispy] fill pillarboxes in widescreen mode
    if (SCREENWIDTH != NONWIDEWIDTH)
    {
        V_DrawFilledBox(0, 0, SCREENWIDTH, SCREENHEIGHT, 0);
    }

    V_DrawFullscreenRawOrPatch(W_GetNumForName("INTERPIC"));
    //V_DrawPatch(x_offset, activating_level_select_anim, W_CacheLumpName(lump_name, PU_CACHE));
    if (activating_level_select_anim == 0)
        DrawLevelSelectStats();
}
