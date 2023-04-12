#include "level_select.h"
#include "doomdef.h"
#include "doomstat.h"
#include "d_main.h"
#include "s_sound.h"
#include "z_zone.h"
#include "v_video.h"
#include "d_player.h"


void WI_initAnimatedBack(void);
void WI_updateAnimatedBack(void);
void WI_drawAnimatedBack(void);
void WI_initVariables(wbstartstruct_t* wbstartstruct);
void WI_loadData(void);
void WI_getMapLocation(int ep, int map, int* x, int* y);


static wbstartstruct_t wiinfo;

extern int bcnt;


boolean LevelSelectResponder(event_t* ev)
{
    return true;
}


void ShowLevelSelect()
{
    S_ChangeMusic(mus_read_m, true);

    gameaction = ga_nothing;
    gamestate = GS_LEVEL_SELECT;
    viewactive = false;
    automapactive = false;

    wiinfo.epsd = 0;
    wiinfo.didsecret = false;
    wiinfo.last = -1;
    wiinfo.next = -1;
    wiinfo.maxkills = 0;
    wiinfo.maxitems = 0;
    wiinfo.maxsecret = 0;
    wiinfo.maxfrags = 0;
    wiinfo.partime = 0;
    wiinfo.pnum = 0;

    WI_initVariables(&wiinfo);
    WI_loadData();
    WI_initAnimatedBack();
    bcnt = 0;
}


void TickLevelSelect()
{
    bcnt++;
    WI_updateAnimatedBack();
}


void DrawLevelSelectStats()
{
    int x, y;
    const int key_spacing = 8;
    const int key_start_offset = -key_spacing * 3 / 2;
    const int start_y_offset = 10;

    for (int i = 0; i < 9; ++i)
    {
        WI_getMapLocation(wiinfo.epsd, i, &x, &y);
        //V_DrawPatch(x, y, W_CacheLumpName("WISPLAT", PU_CACHE));

        // Progress
        V_DrawPatch(x - 8, y + start_y_offset, W_CacheLumpName("STYSNUM3", PU_CACHE));
        V_DrawPatch(x - 3, y + start_y_offset, W_CacheLumpName("STYSLASH", PU_CACHE));
        V_DrawPatch(x + 4, y + start_y_offset, W_CacheLumpName("STYSNUM5", PU_CACHE));

        // Key BG
        // Key
        // Check mark
        V_DrawPatch(x + 22, y + key_start_offset, W_CacheLumpName("KEYBG", PU_CACHE));
        V_DrawPatch(x + 22 + 2, y + key_start_offset + 1, W_CacheLumpName("STKEYS0", PU_CACHE));
        V_DrawPatch(x + 22 + 12, y + key_start_offset - 1, W_CacheLumpName("CHECKMRK", PU_CACHE));

        V_DrawPatch(x + 22, y + key_start_offset + key_spacing, W_CacheLumpName("KEYBG", PU_CACHE));
        V_DrawPatch(x + 22 + 2, y + key_start_offset + key_spacing + 1, W_CacheLumpName("STKEYS1", PU_CACHE));
        V_DrawPatch(x + 22 + 12, y + key_start_offset + key_spacing - 1, W_CacheLumpName("CHECKMRK", PU_CACHE));

        V_DrawPatch(x + 22, y + key_start_offset + key_spacing * 2, W_CacheLumpName("KEYBG", PU_CACHE));
        V_DrawPatch(x + 22 + 2, y + key_start_offset + key_spacing * 2 + 1, W_CacheLumpName("STKEYS2", PU_CACHE));
        V_DrawPatch(x + 22 + 12, y + key_start_offset + key_spacing * 2 - 1, W_CacheLumpName("CHECKMRK", PU_CACHE));
    }
}


void DrawLevelSelect()
{
    char* lump_name = "WIMAP0";
    lump_name[5] = '0' + wiinfo.epsd;
    V_DrawPatchFullScreen(W_CacheLumpName(lump_name, PU_CACHE), false);
    WI_drawAnimatedBack();

    DrawLevelSelectStats();
}
