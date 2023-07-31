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
#include "doomdef.h"
#include "doomstat.h"
#include "d_main.h"
#include "s_sound.h"
#include "z_zone.h"
#include "v_video.h"
#include "d_player.h"
#include "doomkeys.h"
#include "apdoom.h"
#include "i_video.h"
#include "g_game.h"
#include "m_misc.h"
#include "hu_lib.h"
#include "hu_stuff.h"


void WI_initAnimatedBack(void);
void WI_updateAnimatedBack(void);
void WI_drawAnimatedBack(void);
void WI_initVariables(wbstartstruct_t* wbstartstruct);
void WI_loadData(void);


typedef struct
{
    int x;
    int y;
    int keys_offset;
    char* urhere_lump_name;
    int urhere_x_offset;
    int urhere_y_offset;

} level_pos_t;


extern int cursor_x;
extern int cursor_y;


static level_pos_t level_pos_infos[AP_EPISODE_COUNT][AP_LEVEL_COUNT] =
{
    // Episode 1
    {
        { 185, 164 + 10, 22, "WIURH0", 0, 0 },	// location of level 0 (CJ)
	    { 148, 143, 18, "WIURH0", 0, 0 },	// location of level 1 (CJ)
	    { 69, 122, 18, "WIURH1", 0, 0 },	// location of level 2 (CJ)
	    { 209 + 20, 102, 22, "WIURH0", 0, 0 },	// location of level 3 (CJ)
	    { 116, 89, 26, "WIURH2", 0, 0 },	// location of level 4 (CJ)
	    { 166 + 10, 55 - 2, 22, "WIURH0", 0, 0 },	// location of level 5 (CJ)
	    { 71, 56, 18, "WIURH1", 0, 0 },	// location of level 6 (CJ)
	    { 135, 29, 18, "WIURH0", -2, 4 },	// location of level 7 (CJ)
	    { 71, 24, 22, "WIURH1", 0, 0 }	// location of level 8 (CJ)
    },

#ifndef FIRST_EP_ONLY
    // Episode 2
    {
	    { 254, 25, 18, "WIURH2", 0, 0 },	// location of level 0 (CJ)
	    { 97, 50, 22, "WIURH0", 0, 0 },	// location of level 1 (CJ)
	    { 188, 64, 18, "WIURH0", 0, 0 },	// location of level 2 (CJ)
	    { 128, 78 + 5, 22, "WIURH3", 0, 0 },	// location of level 3 (CJ)
	    { 214, 92, 22, "WIURH0", 0, 0 },	// location of level 4 (CJ)
	    { 133, 130, 20, "WIURH0", 0, 0 },	// location of level 5 (CJ)
	    { 208, 136 - 1, 18, "WIURH0", 0, 0 },	// location of level 6 (CJ)
	    { 148, 140 + 20, 22, "WIURH2", 0, 0 },	// location of level 7 (CJ)
	    { 235, 158 + 10, 18, "WIURH2", 0, 0 }	// location of level 8 (CJ)
    },

    // Episode 3
    {
	    { 156, 168, 22, "WIURH0", 0, 0 },	// location of level 0 (CJ)
	    { 48, 154, 22, "WIURH0", 0, 0 },	// location of level 1 (CJ)
	    { 174, 95, -26, "WIURH0", 0, 0 },	// location of level 2 (CJ)
	    { 265, 75, 22, "WIURH3", 0, 0 },	// location of level 3 (CJ)
	    { 130, 48 + 4, -24, "WIURH3", 0, 0 },	// location of level 4 (CJ)
	    { 279, 23, -26, "WIURH1", 8, 0 },	// location of level 5 (CJ)
	    { 198, 48, 18, "WIURH3", 0, 0 },	// location of level 6 (CJ)
	    { 140, 25, 22, "WIURH1", 0, 0 },	// location of level 7 (CJ)
	    { 281, 136, -26, "WIURH3", 0, 0 }	// location of level 8 (CJ)
    },

    // Episode 4
    {
        { 101, 177, 22, "WIURH1", 0, 0 },	// location of level 0 (CJ)
	    { 183, 148, 18, "WIURH0", 0, 0 },	// location of level 1 (CJ)
	    { 172, 97, 18, "WIURH2", 0, 0 },	// location of level 2 (CJ)
	    { 78, 85, 22, "WIURH2", 0, 0 },	// location of level 3 (CJ)
	    { 251, 85, 26, "WIURH2", 0, 0 },	// location of level 4 (CJ)
	    { 33 + 4, 24, -24, "WIURH2", 3, -20 },	// location of level 5 (CJ)
	    { 260, 47, 18, "WIURH0", -5, -4 },	// location of level 6 (CJ)
	    { 191, 24, 28, "WIURH3", 0, 0 },	// location of level 7 (CJ)
	    { 88, 58, 17, "WIURH0", 0, 0 }	// location of level 8 (CJ)
    }
#endif
};


static const char* d2_level_names[AP_D2_LEVEL_COUNT] = {
    // Episode 1: The Space Station
    "Entryway (MAP01)",
    "Underhalls (MAP02)",
    "The Gantlet (MAP03)",
    "The Focus (MAP04)",
    "The Waste Tunnels (MAP05)",
    "The Crusher (MAP06)",
    "Dead Simple (MAP07)",
    "Tricks and Traps (MAP08)",
    "The Pit (MAP09)",
    "Refueling Base (MAP10)",
    "Circle of Death (MAP11)", // 'O' of Destruction!1 ?
    
    // Episode 2: The City
    "The Factory (MAP12)",
    "Downtown (MAP13)",
    "The Inmost Dens (MAP14)",
    "Industrial Zone (MAP15)", // (Exit to secret level)
    "Suburbs (MAP16)",
    "Tenements (MAP17)",
    "The Courtyard (MAP18)",
    "The Citadel (MAP19)",
    "Gotcha! (MAP20)",

    // Episode 3: Hell
    "Nirvana (MAP21)",
    "The Catacombs (MAP22)",
    "Barrels o Fun (MAP23)",
    "The Chasm (MAP24)",
    "Bloodfalls (MAP25)",
    "The Abandoned Mines (MAP26)",
    "Monster Condo (MAP27)",
    "The Spirit World (MAP28)",
    "The Living End (MAP29)",
    "Icon of Sin (MAP30)",

    // Secret levels:
    "Wolfenstein2 (MAP31)", // (Exit to super secret level, IDKFA in BFG Edition)
    "Grosse2 (MAP32)" // (Keen in BFG Edition)
};


static wbstartstruct_t wiinfo;

extern int bcnt;

int selected_level[AP_EPISODE_COUNT] = {0};
int selected_ep = 0;
int prev_ep = 0;
int ep_anim = 0;
int urh_anim = 0;

static hu_textline_t level_lines[AP_D2_LEVEL_COUNT][3];

static const char* YELLOW_DIGIT_LUMP_NAMES[] = {
    "STYSNUM0", "STYSNUM1", "STYSNUM2", "STYSNUM3", "STYSNUM4", 
    "STYSNUM5", "STYSNUM6", "STYSNUM7", "STYSNUM8", "STYSNUM9"
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


void restart_wi_anims()
{
    wiinfo.epsd = selected_ep;
    WI_initVariables(&wiinfo);
    WI_loadData();
    WI_initAnimatedBack();
}

void HU_ClearAPMessages();

void play_level(int ep, int lvl)
{
    // Check if level has a save file first
    char filename[260];
    if (gamemode == commercial)
        snprintf(filename, 260, "%s/save_E%iM%i.dsg", apdoom_get_seed(), ep + 1, lvl + 1);
    else
        snprintf(filename, 260, "%s/save_MAP%02i.dsg", apdoom_get_seed(), lvl + 1);
    if (M_FileExists(filename))
    {
        // We load
        extern char savename[256];
        snprintf(savename, 256, "%s", filename);
        ap_state.player_state.powers[pw_strength] = 0;
        gameaction = ga_loadgame;
        //G_DoLoadGame();
    }
    else
    {
        // If none, load it fresh
        G_DeferedInitNew(gameskill, ep + 1, lvl + 1);
    }

    apdoom_check_victory(); // In case we had pending victory

    HU_ClearAPMessages();
}


boolean LevelSelectResponder(event_t* ev)
{
    if (ep_anim) return true;

    int ep_count = 0;
    if (gamemode != commercial)
        for (int i = 0; i < AP_EPISODE_COUNT; ++i)
            if (ap_state.episodes[i])
                ep_count++;

    switch (ev->type)
    {
        case ev_keydown:
        {
            switch (ev->data1)
            {
#ifndef FIRST_EP_ONLY
                case KEY_LEFTARROW:
                    if (gamemode == commercial)
                    {
                        selected_level[selected_ep] -= 16;
                        if (selected_level[selected_ep] < 0) selected_level[selected_ep] += AP_D2_LEVEL_COUNT;
                        urh_anim = 0;
                        S_StartSoundOptional(NULL, sfx_mnusli, sfx_stnmov);
                    }
                    else if (gamemode != shareware && ep_count > 1)
                    {
                        prev_ep = selected_ep;
                        ep_anim = -10;
                        selected_ep--;
                        if (selected_ep < 0) selected_ep = AP_EPISODE_COUNT - 1;
                        while (!ap_state.episodes[selected_ep])
                        {
                            selected_ep--;
                            if (selected_ep < 0) selected_ep = AP_EPISODE_COUNT - 1;
                            if (selected_ep == prev_ep) // oops;
                                break;
                        }
                        restart_wi_anims();
                        urh_anim = 0;
                        S_StartSoundOptional(NULL, sfx_mnucls, sfx_swtchx);
                    }
                    break;
                case KEY_RIGHTARROW:
                    if (gamemode == commercial)
                    {
                        selected_level[selected_ep] += 16;
                        if (selected_level[selected_ep] >= AP_D2_LEVEL_COUNT) selected_level[selected_ep] -= 32;
                        urh_anim = 0;
                        S_StartSoundOptional(NULL, sfx_mnusli, sfx_stnmov);
                    }
                    else if (gamemode != shareware && ep_count > 1)
                    {
                        prev_ep = selected_ep;
                        ep_anim = 10;
                        selected_ep = (selected_ep + 1) % AP_EPISODE_COUNT;
                        while (!ap_state.episodes[selected_ep])
                        {
                            selected_ep = (selected_ep + 1) % AP_EPISODE_COUNT;
                            if (selected_ep == prev_ep) // oops;
                                break;
                        }
                        restart_wi_anims();
                        urh_anim = 0;
                        S_StartSoundOptional(NULL, sfx_mnucls, sfx_swtchx);
                    }
                    break;
#endif
                case KEY_UPARROW:
                    if (gamemode == commercial)
                    {
                        if (selected_level[selected_ep] - 1 < (selected_level[selected_ep] / 16) * 16)
                            selected_level[selected_ep] = (selected_level[selected_ep] / 16) * 16 + 15;
                        else
                            selected_level[selected_ep]--;
                    }
                    else if (selected_ep == 1)
                    {
                        selected_level[selected_ep]--;
                        if (selected_level[selected_ep] < 0) selected_level[selected_ep] = AP_LEVEL_COUNT - 1;
                    }
                    else
                        selected_level[selected_ep] = (selected_level[selected_ep] + 1) % AP_LEVEL_COUNT;
                    urh_anim = 0;
                    S_StartSoundOptional(NULL, sfx_mnusli, sfx_stnmov);
                    break;
                case KEY_DOWNARROW:
                    if (gamemode == commercial)
                    {
                        if (selected_level[selected_ep] + 1 > (selected_level[selected_ep] / 16) * 16 + 15)
                            selected_level[selected_ep] = (selected_level[selected_ep] / 16) * 16;
                        else
                            selected_level[selected_ep]++;
                    }
                    else if (selected_ep == 1)
                        selected_level[selected_ep] = (selected_level[selected_ep] + 1) % AP_LEVEL_COUNT;
                    else
                    {
                        selected_level[selected_ep]--;
                        if (selected_level[selected_ep] < 0) selected_level[selected_ep] = AP_LEVEL_COUNT - 1;
                    }
                    urh_anim = 0;
                    S_StartSoundOptional(NULL, sfx_mnusli, sfx_stnmov);
                    break;
                case KEY_ENTER:
                    if (ap_get_level_state(selected_ep + 1, selected_level[selected_ep] + 1)->unlocked)
                    {
                        S_StartSoundOptional(NULL, sfx_mnusli, sfx_swtchn);
                        play_level(selected_ep, selected_level[selected_ep]);
                    }
                    else
                    {
                        S_StartSoundOptional(NULL, sfx_mnusli, sfx_noway);
                    }
                    break;
            }
            break;
        }
    }

    return true;
}


void ShowLevelSelect()
{
    // If in a level, save current level
    if (gamestate == GS_LEVEL)
        G_DoSaveGame(); 

    S_ChangeMusic(mus_read_m, true);

    gameaction = ga_nothing;
    gamestate = GS_LEVEL_SELECT;
    viewactive = false;
    automapactive = false;

    ap_state.ep = 0;
    ap_state.map = 0;

    if (gamemode == commercial)
    {
        selected_ep = 0;
    }
    else
    {
        while (!ap_state.episodes[selected_ep])
        {
            selected_ep = (selected_ep + 1) % AP_EPISODE_COUNT;
            if (selected_ep == 0) // oops;
                break;
        }
    }

    wiinfo.epsd = selected_ep;
    wiinfo.didsecret = false;
    wiinfo.last = -1;
    wiinfo.next = -1;
    wiinfo.maxkills = 0;
    wiinfo.maxitems = 0;
    wiinfo.maxsecret = 0;
    wiinfo.maxfrags = 0;
    wiinfo.partime = 0;
    wiinfo.pnum = 0;

    if (gamemode == commercial && !level_lines[0][0].f)
    {
        for (int i = 0; i < AP_D2_LEVEL_COUNT; ++i)
        {
            for (int j = 0; j < 3; ++j)
            {
                hu_textline_t* level_line = &level_lines[i][j];

                HUlib_initTextLine(
                    level_line,
                    level_line->x = 26 + (i / 16) * ORIGWIDTH / 2, 
                    level_line->y = 20 + (i % 16) * 11,
		            hu_font,
		            HU_FONTSTART);
            
                char map_name[20];
                if (j == 0) // Locked
                    sprintf(map_name, "MAP%02i", i + 1);
                else if (j == 1) // Unlocked
                    sprintf(map_name, "~2MAP%02i", i + 1);
                else // Completed
                    sprintf(map_name, "~3MAP%02i", i + 1);
                char* m = map_name;
	            while (*m)
	                HUlib_addCharToTextLine(level_line, *(m++));
            }
        }
    }
    
    restart_wi_anims();
    bcnt = 0;
}


void TickLevelSelect()
{
    if (ep_anim > 0)
        ep_anim -= 1;
    else if (ep_anim < 0)
        ep_anim += 1;
    bcnt++;
    urh_anim = (urh_anim + 1) % 35;
    WI_updateAnimatedBack();
}


void DrawEpisodicLevelSelectStats()
{
    int x, y;
    const int key_spacing = 8;
    const int start_y_offset = 10;

    for (int i = 0; i < AP_LEVEL_COUNT; ++i)
    {
        level_pos_t* level_pos = &level_pos_infos[selected_ep][i];
        ap_level_info_t* ap_level_info = ap_get_level_info(selected_ep + 1, i + i);
        ap_level_state_t* ap_level_state = ap_get_level_state(selected_ep + 1, i + i);

        x = level_pos->x;
        y = level_pos->y;

        int key_count = 0;
        for (int i = 0; i < 3; ++i)
            if (ap_level_info->keys[i])
                key_count++;

        const int key_start_offset = -key_spacing * key_count / 2;
        
        // Level complete splash
        if (ap_level_state->completed)
            V_DrawPatch(x, y, W_CacheLumpName("WISPLAT", PU_CACHE));

        // Lock
        if (!ap_level_state->unlocked)
            V_DrawPatch(x, y, W_CacheLumpName("WILOCK", PU_CACHE));

        // Keys
        const char* key_lump_names[] = {"STKEYS0", "STKEYS1", "STKEYS2"};
        const char* key_skull_lump_names[] = {"STKEYS3", "STKEYS4", "STKEYS5"};
        int key_y = y + key_start_offset;
        int key_x = x + level_pos->keys_offset;
        for (int k = 0; k < 3; ++k)
        {
            if (ap_level_info->keys[k])
            {
                const char* key_lump_name = key_lump_names[k];
                if (ap_level_info->use_skull[k])
                    key_lump_name = key_skull_lump_names[k];
                V_DrawPatch(key_x, key_y, W_CacheLumpName("KEYBG", PU_CACHE));
                V_DrawPatch(key_x + 2, key_y + 1, W_CacheLumpName(key_lump_name, PU_CACHE));
                if (ap_level_state->keys[k])
                {
                    if (level_pos->keys_offset < 0)
                    {
                        V_DrawPatch(key_x - 12, key_y - 1, W_CacheLumpName("CHECKMRK", PU_CACHE));
                    }
                    else
                    {
                        V_DrawPatch(key_x + 12, key_y - 1, W_CacheLumpName("CHECKMRK", PU_CACHE));
                    }
                }
                key_y += key_spacing;
            }
        }

        // Progress
        print_right_aligned_yellow_digit(x - 4, y + start_y_offset, ap_level_state->check_count);
        V_DrawPatch(x - 3, y + start_y_offset, W_CacheLumpName("STYSLASH", PU_CACHE));
        print_left_aligned_yellow_digit(x + 4, y + start_y_offset, ap_level_info->check_count);

        // "You are here"
        if (i == selected_level[selected_ep] && urh_anim < 25)
        {
            int x_offset = 2;
            int y_offset = -2;
            if (level_pos->urhere_lump_name[5] == '1')
            {
                x_offset = -2;
            }
            if ((level_pos->urhere_lump_name[5] == '0' && level_pos->keys_offset > 0) ||
                (level_pos->urhere_lump_name[5] == '1' && level_pos->keys_offset < 0))
            {
                y_offset += key_start_offset;
            }
            if (level_pos->urhere_lump_name[5] == '2' ||
                level_pos->urhere_lump_name[5] == '3')
            {
                y_offset = 16;
            }
            V_DrawPatch(x + x_offset + level_pos->urhere_x_offset, 
                        y + y_offset + level_pos->urhere_y_offset, 
                        W_CacheLumpName(level_pos->urhere_lump_name, PU_CACHE));
        }
    }

    // Level name
    char name[9];
    snprintf(name, 9, "WILV%d%d", selected_ep, selected_level[selected_ep]);
    if (W_CheckNumForName(name) != -1)
    {
        patch_t* finished = W_CacheLumpName(name, PU_STATIC);
        V_DrawPatch((ORIGWIDTH - finished->width) / 2, 2, finished);
    }

    // Mouse Cursor
    //V_DrawPatch(cursor_x, cursor_y, W_CacheLumpName("CURSOR", PU_STATIC));
}


void DrawNonEpisodicLevelSelectStats()
{
    ap_level_info_t* ap_level_info;
    ap_level_state_t* ap_level_state;
    hu_textline_t* level_line;

    for (int i = 0; i < AP_D2_LEVEL_COUNT; ++i)
    {
        ap_level_info = ap_get_level_info(selected_ep + 1, i + 1);
        ap_level_state = ap_get_level_state(selected_ep + 1, i + 1);

        // Map id
        level_line = &level_lines[i][ap_level_state->completed ? 2 : ap_level_state->unlocked];
        HUlib_drawTextLine(level_line, false);
        
        // Progress
        int progress_offset = 58;
        print_right_aligned_yellow_digit(level_line->x + progress_offset - 4, level_line->y + 1, ap_level_state->check_count);
        V_DrawPatch(level_line->x + progress_offset - 3, level_line->y + 1, W_CacheLumpName("STYSLASH", PU_CACHE));
        print_left_aligned_yellow_digit(level_line->x + progress_offset + 4, level_line->y + 1, ap_level_info->check_count);

        // Keys
        const char* key_lump_names[] = {"STKEYS0", "STKEYS1", "STKEYS2"};
        const char* key_skull_lump_names[] = {"STKEYS3", "STKEYS4", "STKEYS5"};
        int key_x = level_line->x + 80;
        int key_y = level_line->y - 1;
        for (int k = 0; k < 3; ++k)
        {
            if (ap_level_info->keys[k])
            {
                const char* key_lump_name = key_lump_names[k];
                if (ap_level_info->use_skull[k])
                    key_lump_name = key_skull_lump_names[k];
                V_DrawPatch(key_x, key_y, W_CacheLumpName("KEYBG", PU_CACHE));
                if (ap_level_state->keys[k])
                {
                    V_DrawPatch(key_x + 2, key_y + 1, W_CacheLumpName(key_lump_name, PU_CACHE));
                }
                key_x += 12;
            }
        }
    }

    ap_level_info = ap_get_level_info(selected_ep + 1, selected_level[selected_ep] + 1);
    ap_level_state = ap_get_level_state(selected_ep + 1, selected_level[selected_ep] + 1);
    level_line = &level_lines[selected_level[selected_ep]][ap_level_state->completed ? 2 : ap_level_state->unlocked];

    // Cursor
    patch_t* cursor = W_CacheLumpName(urh_anim < 16 ? "M_SKULL1" : "M_SKULL2", PU_STATIC);
    V_DrawPatch(level_line->x - 24, level_line->y - 8, cursor);

    // Level name
    char name[9];
    snprintf(name, 9, "CWILV%02i", selected_level[selected_ep]);
    if (W_CheckNumForName(name) != -1)
    {
        patch_t* finished = W_CacheLumpName(name, PU_STATIC);
        V_DrawPatch((ORIGWIDTH - finished->width) / 2, 2, finished);
    }
}


void DrawLevelSelectStats()
{
    if (gamemode == commercial)
        DrawNonEpisodicLevelSelectStats();
    else
        DrawEpisodicLevelSelectStats();
}


static const char* WIN_MAPS[4] = {
    "WIMAP0",
    "WIMAP1",
    "WIMAP2",
    "WIMAP3"
};

static const char* D2_WIN_MAP = "INTERPIC";


const char* get_win_map(int ep)
{
    if (gamemode == commercial) return D2_WIN_MAP;
    else return WIN_MAPS[ep];
}


void DrawLevelSelect()
{
    int x_offset = ep_anim * 32;

    char lump_name[9];
    snprintf(lump_name, 9, "%s", get_win_map(selected_ep));
    
    // [crispy] fill pillarboxes in widescreen mode
    if (SCREENWIDTH != NONWIDEWIDTH)
    {
        V_DrawFilledBox(0, 0, SCREENWIDTH, SCREENHEIGHT, 0);
    }

    V_DrawPatch(x_offset, 0, W_CacheLumpName(lump_name, PU_CACHE));
    if (ep_anim == 0)
    {
        WI_drawAnimatedBack();

        DrawLevelSelectStats();
    }
    else
    {
        snprintf(lump_name, 9, "%s", get_win_map(prev_ep));
        if (ep_anim > 0)
            x_offset = -(10 - ep_anim) * 32;
        else
            x_offset = (10 + ep_anim) * 32;
        V_DrawPatch(x_offset, 0, W_CacheLumpName(lump_name, PU_CACHE));
    }
}
