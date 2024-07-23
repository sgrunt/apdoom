//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 1993-2008 Raven Software
// Copyright(C) 2005-2014 Simon Howard
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


// HEADER FILES ------------------------------------------------------------

// haleyjd: removed WATCOMC
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "config.h"

#include "h2def.h"
#include "ct_chat.h"
#include "d_iwad.h"
#include "d_mode.h"
#include "m_misc.h"
#include "s_sound.h"
#include "i_input.h"
#include "i_joystick.h"
#include "i_swap.h" // [crispy] SHORT()
#include "i_system.h"
#include "i_timer.h"
#include "m_argv.h"
#include "m_config.h"
#include "m_controls.h"
#include "net_client.h"
#include "p_local.h"
#include "v_video.h"
#include "w_main.h"
#include "am_map.h"

#include "hexen_icon.c"

#include "apdoom.h"

#include "level_select.h" // [ap]
#include "ap_msg.h"
#include "ap_notif.h"

#define CT_KEY_GREEN    'g'

// MACROS ------------------------------------------------------------------

#define MAXWADFILES 20
#define CT_KEY_BLUE         'b'
#define CT_KEY_RED          'r'
#define CT_KEY_YELLOW       'y'
#define CT_KEY_GREEN        'g'
#define CT_KEY_PLAYER5      'j'     // Jade
#define CT_KEY_PLAYER6      'w'     // White
#define CT_KEY_PLAYER7      'h'     // Hazel
#define CT_KEY_PLAYER8      'p'     // Purple
#define CT_KEY_ALL          't'

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

void R_ExecuteSetViewSize(void);
void D_ConnectNetGame(void);
void D_CheckNetGame(void);
boolean F_Responder(event_t * ev);
void I_StartupKeyboard(void);
void I_StartupJoystick(void);
void I_ShutdownKeyboard(void);
void S_InitScript(void);

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

void H2_AdvanceDemo(void);
void H2_PageTicker(void);

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

static void DrawMessage(void);
static void PageDrawer(void);
static void HandleArgs(void);
static void CheckRecordFrom(void);
static void DrawAndBlit(void);
static void CreateSavePath(void);
static void WarpCheck(void);

static void CrispyDrawStats(void); // [crispy]

// EXTERNAL DATA DECLARATIONS ----------------------------------------------


// PUBLIC DATA DEFINITIONS -------------------------------------------------

GameMode_t gamemode;
static const char *gamedescription;
char *iwadfile;
static char demolumpname[9];    // Demo lump to start playing.
boolean nomonsters;             // checkparm of -nomonsters
boolean respawnparm;            // checkparm of -respawn
boolean randomclass;            // checkparm of -randclass
boolean debugmode;              // checkparm of -debug
boolean ravpic;                 // checkparm of -ravpic
boolean cdrom = false;          // true if cd-rom mode active
boolean cmdfrag;                // true if a CMD_FRAG packet should be sent out
boolean artiskip;               // whether shift-enter skips an artifact
int maxzone = 0x800000;         // Maximum allocated for zone heap (8meg default)
skill_t startskill;
int startepisode;
int startmap;
boolean autostart;
boolean advancedemo;
FILE *debugfile;
int UpdateState;
int maxplayers = MAXPLAYERS;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static int WarpMap;
static int demosequence;
static int pagetic;
static const char *pagename;
static char *SavePathConfig;

// CODE --------------------------------------------------------------------


static const char * const chat_macro_defaults[10] =
{
    HUSTR_CHATMACRO0,
    HUSTR_CHATMACRO1,
    HUSTR_CHATMACRO2,
    HUSTR_CHATMACRO3,
    HUSTR_CHATMACRO4,
    HUSTR_CHATMACRO5,
    HUSTR_CHATMACRO6,
    HUSTR_CHATMACRO7,
    HUSTR_CHATMACRO8,
    HUSTR_CHATMACRO9,
};

void tick_sticky_msgs()
{
    HU_TickAPMessages();
}

void on_ap_message(const char* text) // This string is cached for several seconds
{
    //if (strncmp(text, "Now that you are connected", strlen("Now that you are connected")) == 0) return; // Ignore that message. It fills the screen
    HU_AddAPMessage(text);
    S_StartSound(NULL, SFX_CHAT);
}


void on_ap_victory()
{
    F_StartFinale();
}


//boolean P_GiveArmor(player_t* player, int armortype);
boolean P_GiveArmor(player_t * player, armortype_t armortype, int amount);
boolean P_GiveWeapon(player_t* player, weapontype_t weapon, boolean dropped);
boolean P_GiveWeaponPiece(player_t *player, pclass_t class, int piece);

// Kind of a copy of P_TouchSpecialThing
void on_ap_give_item(int doom_type, int ep, int map)
{
    player_t* player = &players[consoleplayer];
    int sound = SFX_PICKUP_ITEM;

    switch (doom_type)
    {
            // Items
        case 81:         // Item_HealingPotion
            if (!P_GiveBody(player, 10))
            {
                return;
            }
            P_SetMessage(player, TXT_ITEMHEALTH, false);
            break;
        case 8005:
            if (!P_GiveArmor(player, ARMOR_ARMOR, -1))
            {
                return;
            }
            P_SetMessage(player, TXT_ARMOR1, false);
            break;
        case 8006:
            if (!P_GiveArmor(player, ARMOR_SHIELD, -1))
            {
                return;
            }
            P_SetMessage(player, TXT_ARMOR2, false);
            break;
        case 8007:
            if (!P_GiveArmor(player, ARMOR_HELMET, -1))
            {
                return;
            }
            P_SetMessage(player, TXT_ARMOR3, false);
            break;
        case 8008:
            if (!P_GiveArmor(player, ARMOR_AMULET, -1))
            {
                return;
            }
            P_SetMessage(player, TXT_ARMOR4, false);
            break;

            // Keys
        case 8030:
        case 8031:
        case 8032:
        case 8033:
        case 8034:
        case 8035:
        case 8036:
        case 8037:
        case 8038:
        case 8039:
            if (!P_GiveKey(player, doom_type - 8030))
            {
                return;
            }
            P_SetMessage(player, TextKeyMessages[doom_type - 8030],
                         true);
            sound = SFX_PICKUP_KEY;
	    break;

        case 8200:
            if (!P_GiveKey(player, 10))
            {
                return;
            }
            P_SetMessage(player, TextKeyMessages[10],
                         true);
            sound = SFX_PICKUP_KEY;
	    break;

            // Artifacts
        case 82:
            P_GiveArtifact(player, arti_health, NULL);
            return;
        case 83:
            P_GiveArtifact(player, arti_fly, NULL);
            return;
        case 84:
            P_GiveArtifact(player, arti_invulnerability, NULL);
            return;
        case 86:
            P_GiveArtifact(player, arti_summon, NULL);
            return;
        case 30:
            P_GiveArtifact(player, arti_egg, NULL);
            return;
        case 32:
            P_GiveArtifact(player, arti_superhealth, NULL);
            return;
        case 33:
            P_GiveArtifact(player, arti_torch, NULL);
            return;
        case 36:
            P_GiveArtifact(player, arti_teleport, NULL);
            return;
        case 10040:
            P_GiveArtifact(player, arti_teleportother, NULL);
            return;
        case 8000:
            P_GiveArtifact(player, arti_poisonbag, NULL);
            return;
        case 8002:
            P_GiveArtifact(player, arti_speed, NULL);
            return;
        case 8003:
            P_GiveArtifact(player, arti_boostmana, NULL);
            return;
        case 8041:
            P_GiveArtifact(player, arti_boostarmor, NULL);
            return;
        case 10110:
            P_GiveArtifact(player, arti_blastradius, NULL);
            return;

            // Puzzle artifacts
        case 9002:
            P_GiveArtifact(player, arti_puzzskull, NULL);
            return;
        case 9003:
            P_GiveArtifact(player, arti_puzzgembig, NULL);
            return;
        case 9004:
            P_GiveArtifact(player, arti_puzzgemred, NULL);
            return;
        case 9005:
            P_GiveArtifact(player, arti_puzzgemgreen1, NULL);
            return;
        case 9009:
            P_GiveArtifact(player, arti_puzzgemgreen2, NULL);
            return;
        case 9006:
            P_GiveArtifact(player, arti_puzzgemblue1, NULL);
            return;
        case 9010:
            P_GiveArtifact(player, arti_puzzgemblue2, NULL);
            return;
        case 9007:
            P_GiveArtifact(player, arti_puzzbook1, NULL);
            return;
        case 9008:
            P_GiveArtifact(player, arti_puzzbook2, NULL);
            return;
        case 9014:
            P_GiveArtifact(player, arti_puzzskull2, NULL);
            return;
        case 9015:
            P_GiveArtifact(player, arti_puzzfweapon, NULL);
            return;
        case 9016:
            P_GiveArtifact(player, arti_puzzcweapon, NULL);
            return;
        case 9017:
            P_GiveArtifact(player, arti_puzzmweapon, NULL);
            return;
        case 9018:
            P_GiveArtifact(player, arti_puzzgear1, NULL);
            return;
        case 9019:
            P_GiveArtifact(player, arti_puzzgear2, NULL);
            return;
        case 9020:
            P_GiveArtifact(player, arti_puzzgear3, NULL);
            return;
        case 9021:
            P_GiveArtifact(player, arti_puzzgear4, NULL);
            return;

            // Mana
        case 122:
            if (!P_GiveMana(player, MANA_1, 15))
            {
                return;
            }
            P_SetMessage(player, TXT_MANA_1, false);
            break;
        case 124:
            if (!P_GiveMana(player, MANA_2, 15))
            {
                return;
            }
            P_SetMessage(player, TXT_MANA_2, false);
            break;
        case 8004:         // Double Mana Dodecahedron
            if (!P_GiveMana(player, MANA_1, 20))
            {
                if (!P_GiveMana(player, MANA_2, 20))
                {
                    return;
                }
            }
            else
            {
                P_GiveMana(player, MANA_2, 20);
            }
            P_SetMessage(player, TXT_MANA_BOTH, false);
            break;

            // 2nd and 3rd Mage Weapons
        case 53:         // Frost Shards
            P_GiveWeapon(player, PCLASS_MAGE, WP_SECOND);
	    P_SetMessage(player, TXT_WEAPON_M2, false);
            return;
        case 8040:         // Arc of Death
            P_GiveWeapon(player, PCLASS_MAGE, WP_THIRD);
	    P_SetMessage(player, TXT_WEAPON_M3, false);
            return;

            // 2nd and 3rd Fighter Weapons
        case 8010:         // Timon's Axe
            P_GiveWeapon(player, PCLASS_FIGHTER, WP_SECOND);
	    P_SetMessage(player, TXT_WEAPON_F2, false);
            return;
        case 123:         // Hammer of Retribution
            P_GiveWeapon(player, PCLASS_FIGHTER, WP_THIRD);
	    P_SetMessage(player, TXT_WEAPON_F3, false);
            return;

            // 2nd and 3rd Cleric Weapons
        case 10:         // Serpent Staff
            P_GiveWeapon(player, PCLASS_CLERIC, WP_SECOND);
	    P_SetMessage(player, TXT_WEAPON_C2, false);
            return;
        case 8009:         // Firestorm
            P_GiveWeapon(player, PCLASS_CLERIC, WP_THIRD);
	    P_SetMessage(player, TXT_WEAPON_C3, false);
            return;

            // Fourth Weapon Pieces
        case 12:
            P_GiveWeaponPiece(player, PCLASS_FIGHTER, WPIECE1);
            return;
        case 13:
            P_GiveWeaponPiece(player, PCLASS_FIGHTER, WPIECE2);
            return;
        case 16:
            P_GiveWeaponPiece(player, PCLASS_FIGHTER, WPIECE3);
            return;
        case 18:
            P_GiveWeaponPiece(player, PCLASS_CLERIC, WPIECE1);
            return;
        case 19:
            P_GiveWeaponPiece(player, PCLASS_CLERIC, WPIECE2);
            return;
        case 20:
            P_GiveWeaponPiece(player, PCLASS_CLERIC, WPIECE3);
            return;
        case 21:
            P_GiveWeaponPiece(player, PCLASS_MAGE, WPIECE1);
            return;
        case 22:
            P_GiveWeaponPiece(player, PCLASS_MAGE, WPIECE2);
            return;
        case 23:
            P_GiveWeaponPiece(player, PCLASS_MAGE, WPIECE3);
            return;
    }

    S_StartSound(NULL, sound); // [NS] Fallback to itemup.
}

void D_BindVariables(void)
{
    int i;

    M_ApplyPlatformDefaults();

    I_BindInputVariables();
    I_BindVideoVariables();
    I_BindJoystickVariables();
    I_BindSoundVariables();

    M_BindBaseControls();
    M_BindMapControls();
    M_BindMenuControls();
    M_BindWeaponControls();
    M_BindChatControls(MAXPLAYERS);
    M_BindHereticControls();
    M_BindHexenControls();

    key_multi_msgplayer[0] = CT_KEY_BLUE;
    key_multi_msgplayer[1] = CT_KEY_RED;
    key_multi_msgplayer[2] = CT_KEY_YELLOW;
    key_multi_msgplayer[3] = CT_KEY_GREEN;
    key_multi_msgplayer[4] = CT_KEY_PLAYER5;
    key_multi_msgplayer[5] = CT_KEY_PLAYER6;
    key_multi_msgplayer[6] = CT_KEY_PLAYER7;
    key_multi_msgplayer[7] = CT_KEY_PLAYER8;

    NET_BindVariables();

    M_BindIntVariable("graphical_startup",      &graphical_startup);
    M_BindIntVariable("mouse_sensitivity",      &mouseSensitivity);
    M_BindIntVariable("mouse_sensitivity_x2",   &mouseSensitivity_x2);
    M_BindIntVariable("mouse_sensitivity_y",    &mouseSensitivity_y);
    M_BindIntVariable("sfx_volume",             &snd_MaxVolume);
    M_BindIntVariable("music_volume",           &snd_MusicVolume);
    M_BindIntVariable("messageson",             &messageson);
    M_BindIntVariable("screenblocks",           &screenblocks);
    M_BindIntVariable("snd_channels",           &snd_Channels);
    M_BindIntVariable("vanilla_savegame_limit", &vanilla_savegame_limit);
    M_BindIntVariable("vanilla_demo_limit",     &vanilla_demo_limit);

    M_BindStringVariable("savedir", &SavePathConfig);

    // Multiplayer chat macros

    for (i=0; i<10; ++i)
    {
        char buf[12];

        chat_macros[i] = M_StringDuplicate(chat_macro_defaults[i]);
        M_snprintf(buf, sizeof(buf), "chatmacro%i", i);
        M_BindStringVariable(buf, &chat_macros[i]);
    }

    // [crispy] bind "crispness" config variables
    M_BindIntVariable("crispy_automapoverlay",  &crispy->automapoverlay);
    M_BindIntVariable("crispy_automaprotate",   &crispy->automaprotate);
    M_BindIntVariable("crispy_bobfactor",       &crispy->bobfactor);
    M_BindIntVariable("crispy_centerweapon",    &crispy->centerweapon);
    M_BindIntVariable("crispy_defaultskill",    &crispy->defaultskill);
    M_BindIntVariable("crispy_fpslimit",        &crispy->fpslimit);
    M_BindIntVariable("crispy_freelook",        &crispy->freelook_hh);
    M_BindIntVariable("crispy_hires",           &crispy->hires);
    M_BindIntVariable("crispy_mouselook",       &crispy->mouselook);
    M_BindIntVariable("crispy_playercoords",    &crispy->playercoords);
    M_BindIntVariable("crispy_soundmono",       &crispy->soundmono);
    M_BindIntVariable("crispy_smoothscaling",   &crispy->smoothscaling);
    M_BindIntVariable("crispy_vsync",           &crispy->vsync);
    M_BindIntVariable("crispy_widescreen",      &crispy->widescreen);
    M_BindIntVariable("crispy_uncapped",        &crispy->uncapped);
    M_BindIntVariable("crispy_brightmaps",      &crispy->brightmaps);
}

// Set the default directory where hub savegames are saved.

static void D_SetDefaultSavePath(void)
{
    SavePath = M_GetSaveGameDir("hexen.wad");

    if (!strcmp(SavePath, ""))
    {
        // only get hexen.cfg path if one is not already found

        if (SavePathConfig == NULL || !strcmp(SavePathConfig, ""))
        {
            // If we are not using a savegame path (probably because we are on
            // Windows and not using a config dir), behave like Vanilla Hexen
            // and use hexndata/:

            SavePath = malloc(10);
            M_snprintf(SavePath, 10, "hexndata%c", DIR_SEPARATOR);
        }
        else
        {
            SavePath = M_StringDuplicate(SavePathConfig);
        }
    }

    // only set hexen.cfg path if using default handling

    if (!M_ParmExists("-savedir") && !M_ParmExists("-cdrom"))
    {
        SavePathConfig = SavePath;
    }
}

// The Mac version of the Hexen IWAD is different to the "normal" DOS
// version - it doesn't include lumps used by the DOS DMX library.
// This means that we can't do GUS or OPL emulation and need to apply
// a workaround.
static void AdjustForMacIWAD(void)
{
    boolean adjust_music = false;

    switch (snd_musicdevice)
    {
        case SNDDEVICE_ADLIB:
        case SNDDEVICE_SB:
            adjust_music = W_CheckNumForName("GENMIDI") < 0;
            break;

        case SNDDEVICE_GUS:
            adjust_music = W_CheckNumForName("DMXGUS") < 0;
            break;

        default:
            break;
    }

    if (adjust_music)
    {
        printf("** Note: You appear to be using the Mac version of the Hexen\n"
               "** IWAD file. This is missing the lumps required for OPL or\n"
               "** GUS emulation. Your music configuration is being adjusted\n"
               "** to a different setting that won't cause the game to "
               "crash.\n");
        snd_musicdevice = SNDDEVICE_GENMIDI;
    }
}

//
// D_GrabMouseCallback
//
// Called to determine whether to grab the mouse pointer
//

static boolean D_GrabMouseCallback(void)
{
    // when menu is active or game is paused, release the mouse

    if (MenuActive || paused)
        return false;

    // only grab mouse when playing levels (but not demos)

    return (gamestate == GS_LEVEL) && !advancedemo && !demoplayback;
}

// Message displayed when quitting Hexen

static void D_HexenQuitMessage(void)
{
    printf("\nHexen: Beyond Heretic\n");
}

static void D_AddFile(char *filename)
{
    printf("  adding %s\n", filename);

    W_AddFile(filename);
}

// Find out what version of Hexen is playing.

void D_IdentifyVersion(void)
{
    // The Hexen Shareware, ne 4 Level Demo Version, is missing the SKY1 lump
    // and uses the SKY2 lump instead. Let's use this fact and the missing
    // levels from MAP05 onward to identify it and set gamemode accordingly.

    if (W_CheckNumForName("SKY1") == -1 &&
        W_CheckNumForName("MAP05") == -1 )
    {
	gamemode = shareware;
	maxplayers = 4;
    }

    // The v1.0 IWAD file is missing a bunch of lumps that can cause the game
    // to crash, so we exit with an error if the user tries to play with it.
    // But we provide an override command line flag if they really want to
    // do it.

    //!
    // If provided, the check for the v1.0 IWAD file is disabled, even though
    // it will almost certainly cause the game to crash.
    //
    // @category compat
    //

    if (!M_ParmExists("-v10override")
     && gamemode != shareware && W_CheckNumForName("CLUS1MSG") < 0)
    {
        I_Error(
            "You are trying to use the Hexen v1.0 IWAD. This isn't\n"
            "supported by " PACKAGE_NAME ". Please upgrade to the v1.1\n"
            "IWAD file. See here for more information:\n"
            "  https://www.doomworld.com/classicdoom/info/patches.php");
    }
}

// Set the gamedescription string.

void D_SetGameDescription(void)
{
/*
    NB: The 4 Level Demo Version actually prints a four-lined banner
    (and indeed waits for a keypress):

    Hexen:  Beyond Heretic

    4 Level Demo Version
    Press any key to continue.
*/

    if (gamemode == shareware)
    {
	gamedescription = "Hexen: 4 Level Demo Version";
    }
    else
    {
	gamedescription = "Hexen";
    }
}

static const char *const loadparms[] = {"-file", "-merge", NULL}; // [crispy]

//==========================================================================
//
// H2_Main
//
//==========================================================================
void InitMapMusicInfo(void);

void D_DoomMain(void)
{
    GameMission_t gamemission;
    int p;

    ap_settings_t ap_settings;
    memset(&ap_settings, 0, sizeof(ap_settings));

    I_AtExit(D_HexenQuitMessage, false);
    startepisode = 1;
    autostart = false;
    startmap = 1;
    gamemode = commercial;

    I_PrintBanner(PACKAGE_STRING);

    // Initialize subsystems

    ST_Message("V_Init: allocate screens.\n");
    V_Init();

    // Load defaults before initing other systems
    ST_Message("M_LoadDefaults: Load system defaults.\n");
    D_BindVariables();

#ifdef _WIN32

    //!
    // @category obscure
    // @platform windows
    // @vanilla
    //
    // Save configuration data and savegames in c:\hexndata,
    // allowing play from CD.
    //

    cdrom = M_ParmExists("-cdrom");
#endif

    if (cdrom)
    {
        M_SetConfigDir("c:\\hexndata\\");
    }
    else
    {
        M_SetConfigDir(NULL);
    }

    M_SetConfigFilenames("hexen.cfg", PROGRAM_PREFIX "hexen.cfg");
    M_LoadDefaults();

    D_SetDefaultSavePath();

    I_AtExit(M_SaveDefaults, false);

    // [crispy] set defaultskill after loading config
    startskill = (crispy->defaultskill + SKILL_HMP) % NUM_SKILLS;

    // Now that the savedir is loaded, make sure it exists
    CreateSavePath();

    ST_Message("Z_Init: Init zone memory allocation daemon.\n");
    Z_Init();
    
    // Grab parameters for AP
    int apserver_arg_id = M_CheckParmWithArgs("-apserver", 1);
    if (!apserver_arg_id)
	    I_Error("Make sure to launch the game using APDoomLauncher.exe.\nThe '-apserver' parameter requires an argument.");

    int player_is_hex = 0;
    int applayer_arg_id = M_CheckParmWithArgs("-applayer", 1);
    if (!applayer_arg_id)
    {
        applayer_arg_id = M_CheckParmWithArgs("-applayerhex", 1);
        if (!applayer_arg_id)
        {
	        I_Error("Make sure to launch the game using APDoomLauncher.exe.\nThe '-applayer' parameter requires an argument.");
        }
        player_is_hex = 1;
    }

    const char* password = "";
    if (M_CheckParm("-password"))
    {
        int password_arg_id = M_CheckParmWithArgs("-password", 1);
        if (!password_arg_id)
	        I_Error("Make sure to launch the game using APDoomLauncher.exe.\nThe '-password' parameter requires an argument.");
        password = myargv[password_arg_id + 1];
    }

    GameMission_t mission = hexen;
    if (M_CheckParm("-game"))
    {
        int game_arg_id = M_CheckParmWithArgs("-game", 1);
        if (!game_arg_id)
	        I_Error("Make sure to launch the game using APDoomLauncher.exe.\nThe '-game' parameter requires an argument.");
        const char* game_name = myargv[game_arg_id + 1];
        if (strcmp(game_name, "hexen") == 0) mission = hexen;
    }

    int monster_rando_id = M_CheckParmWithArgs("-apmonsterrando", 1);
    if (monster_rando_id)
    {
        ap_settings.override_monster_rando = 1;
        ap_settings.monster_rando = atoi(myargv[monster_rando_id + 1]);
    }

    int item_rando_id = M_CheckParmWithArgs("-apitemrando", 1);
    if (item_rando_id)
    {
        ap_settings.override_item_rando = 1;
        ap_settings.item_rando = atoi(myargv[item_rando_id + 1]);
    }

    int music_rando_id = M_CheckParmWithArgs("-apmusicrando", 1);
    if (music_rando_id)
    {
        ap_settings.override_music_rando = 1;
        ap_settings.music_rando = atoi(myargv[music_rando_id + 1]);
    }

    // Not supported by hexen 
    //int flip_levels_id = M_CheckParmWithArgs("-apfliplevels", 1);
    //if (flip_levels_id)
    //{
    //    ap_settings.override_flip_levels = 1;
    //    ap_settings.flip_levels = myargv[flip_levels_id + 1];
    //}

    if (M_CheckParm("-apdeathlinkoff"))
        ap_settings.force_deathlink_off = 1;

    int reset_level_on_death_id = M_CheckParmWithArgs("-apresetlevelondeath", 1);
    if (reset_level_on_death_id)
    {
        ap_settings.override_reset_level_on_death = 1;
        ap_settings.reset_level_on_death = atoi(myargv[reset_level_on_death_id + 1]) ? 1 : 0;
    }

    // Initialize AP
    ap_settings.ip = myargv[apserver_arg_id + 1];
    if (mission == hexen)
        ap_settings.game = "Hexen";

    char* player_name = myargv[applayer_arg_id + 1];
    if (player_is_hex)
    {
        int len = strlen(player_name) / 2;
        char byte_str[3] = {0};
        for (int i = 0; i < len; ++i)
        {
            memcpy(byte_str, player_name + (i * 2), 2);
            player_name[i] = strtol(byte_str, NULL, 16);
        }
        player_name[len] = '\0';
    }
    ap_settings.player_name = player_name;

    ap_settings.passwd = password;
    ap_settings.message_callback = on_ap_message;
    ap_settings.give_item_callback = on_ap_give_item;
    ap_settings.victory_callback = on_ap_victory;
    if (!apdoom_init(&ap_settings))
    {
	    I_Error("Failed to initialize Archipelago.");
    }

    // haleyjd: removed WATCOMC

    ST_Message("W_Init: Init WADfiles.\n");

    iwadfile = D_FindIWAD(IWAD_MASK_HEXEN, &gamemission);

    if (iwadfile == NULL)
    {
        I_Error("Game mode indeterminate. No IWAD was found. Try specifying\n"
                "one with the '-iwad' command line parameter.");
    }

    D_AddFile(iwadfile);
    W_CheckCorrectIWAD(hexen);
    D_IdentifyVersion();
    D_SetGameDescription();
    AdjustForMacIWAD();

    //!
    // @category game
    // @category mod
    //
    // Mana pickups give 50% more mana. This option is not allowed when recording a
    // demo, playing back a demo or when starting a network game.
    //

    crispy->moreammo = M_ParmExists("-moremana");

    //!
    // @category game
    // @category mod
    //
    // Fast monsters. This option is not allowed when recording a demo,
    // playing back a demo or when starting a network game.
    //

    crispy->fast = M_ParmExists("-fast");

    //!
    // @category game
    // @category mod
    //
    // Automatic use of Quartz flasks and Mystic urns.
    //

    crispy->autohealth = M_ParmExists("-autohealth");

    //!
    // @category mod
    //
    // Disable auto-loading of .wad files.
    //
    if (!M_ParmExists("-noautoload") && gamemode != shareware)
    {
        char *autoload_dir;
        autoload_dir = M_GetAutoloadDir("hexen.wad", true);
        if (autoload_dir != NULL)
        {
            // TODO? DEH_AutoLoadPatches(autoload_dir);
            W_AutoLoadWADs(autoload_dir);
            free(autoload_dir);
        }
    }

    HandleArgs();

    // Generate the WAD hash table.  Speed things up a bit.
    W_GenerateHashTable();

    I_PrintStartupBanner(gamedescription);

    ST_Message("MN_Init: Init menu system.\n");
    MN_Init();

    ST_Message("CT_Init: Init chat mode data.\n");
    CT_Init();

    InitMapMusicInfo();         // Init music fields in mapinfo

    ST_Message("S_InitScript\n");
    S_InitScript();

    ST_Message("SN_InitSequenceScript: Registering sound sequences.\n");
    SN_InitSequenceScript();
    ST_Message("I_Init: Setting up machine state.\n");
    I_CheckIsScreensaver();
    I_InitTimer();
    I_InitJoystick();
    I_InitSound(false);
    I_InitMusic();

    ST_Message("NET_Init: Init networking subsystem.\n");
    NET_Init();
    D_ConnectNetGame();

    S_Init();
    S_Start();

    ST_Message("ST_Init: Init startup screen.\n");
    ST_Init();

    // Show version message now, so it's visible during R_Init()
    ST_Message("R_Init: Init Hexen refresh daemon");
    R_Init();
    ST_Message("\n");

    //if (M_CheckParm("-net"))
    //    ST_NetProgress();       // Console player found

    ST_Message("P_Init: Init Playloop state.\n");
    P_Init();

    // Check for command line warping. Follows P_Init() because the
    // MAPINFO.TXT script must be already processed.
    WarpCheck();

    ST_Message("D_CheckNetGame: Checking network game status.\n");
    D_CheckNetGame();

    ST_Message("SB_Init: Loading patches.\n");
    SB_Init();

    ST_Done();

    if (autostart)
    {
        ST_Message("Warp to Map %d (\"%s\":%d), Skill %d\n",
                   WarpMap, P_GetMapName(startmap), startmap, startskill + 1);
    }

    CheckRecordFrom();

    //!
    // @arg <x>
    // @category demo
    // @vanilla
    //
    // Record a demo named x.lmp.
    //

    p = M_CheckParm("-record");
    if (p && p < myargc - 1)
    {
        G_RecordDemo(startskill, 1, startepisode, startmap, myargv[p + 1]);
        H2_GameLoop();          // Never returns
    }

    p = M_CheckParmWithArgs("-playdemo", 1);
    if (p)
    {
        singledemo = true;      // Quit after one demo
        G_DeferedPlayDemo(demolumpname);
        H2_GameLoop();          // Never returns
    }

    p = M_CheckParmWithArgs("-timedemo", 1);
    if (p)
    {
        G_TimeDemo(demolumpname);
        H2_GameLoop();          // Never returns
    }

    //!
    // @category game
    // @arg <s>
    // @vanilla
    //
    // Load the game in savegame slot s.
    //

    p = M_CheckParmWithArgs("-loadgame", 1);
    if (p)
    {
        G_LoadGame(atoi(myargv[p + 1]));
    }

    if (gameaction != ga_loadgame)
    {
        UpdateState |= I_FULLSCRN;
        BorderNeedRefresh = true;
        if (autostart || netgame)
        {
            G_StartNewInit();
            G_InitNew(startskill, startepisode, startmap);
        }
        else
        {
            H2_StartTitle();
        }
    }
    H2_GameLoop();              // Never returns
}

//==========================================================================
//
// HandleArgs
//
//==========================================================================

static void HandleArgs(void)
{
    int p;

    //!
    // @category game
    // @vanilla
    //
    // Disable monsters.
    //

    nomonsters = M_ParmExists("-nomonsters");

    //!
    // @category game
    // @vanilla
    //
    // Monsters respawn after being killed.
    //

    respawnparm = M_ParmExists("-respawn");

    //!
    // @vanilla
    // @category net
    //
    // In deathmatch mode, change a player's class each time the
    // player respawns.
    //

    randomclass = M_ParmExists("-randclass");

    //!
    // @vanilla
    //
    // Take screenshots when F1 is pressed.
    //

    ravpic = M_ParmExists("-ravpic");

    //!
    // @category obscure
    // @vanilla
    //
    // Don't allow artifacts to be used when the run key is held down.
    //

    artiskip = M_ParmExists("-artiskip");

    debugmode = M_ParmExists("-debug");

    //!
    // @vanilla
    // @category net
    //
    // Start a deathmatch game.
    //

    deathmatch = M_ParmExists("-deathmatch");

    // currently broken or unused:
    cmdfrag = M_ParmExists("-cmdfrag");

    // Always merge Archipelago WAD
    // W_MergeFile("APHERETIC.WAD");

    // Check WAD file command line options
    W_ParseCommandLine();

    //!
    // @category obscure
    // @vanilla
    // @arg <path>
    //
    // Development option to specify path to level scripts.
    //

    p = M_CheckParmWithArgs("-scripts", 1);

    if (p)
    {
        sc_FileScripts = true;
        sc_ScriptsDir = myargv[p+1];
    }

    //!
    // @category game
    // @arg <skill>
    // @vanilla
    //
    // Set the game skill, 1-5 (1: easiest, 5: hardest).  A skill of
    // 0 disables all monsters.
    //

    p = M_CheckParmWithArgs("-skill", 1);

    if (p)
    {
        startskill = myargv[p+1][0] - '1';
        autostart = true;
    }

    // [crispy] add wad files from autoload PWAD directories

    if (!M_ParmExists("-noautoload") && gamemode != shareware)
    {
        int i;

        for (i = 0; loadparms[i]; i++)
        {
            int p;
            p = M_CheckParmWithArgs(loadparms[i], 1);
            if (p)
            {
                while (++p != myargc && myargv[p][0] != '-')
                {
                    char *autoload_dir;
                    if ((autoload_dir = M_GetAutoloadDir(M_BaseName(myargv[p]), false)))
                    {
                        W_AutoLoadWADs(autoload_dir);
                        free(autoload_dir);
                    }
                }
            }
        }
    }

    //!
    // @arg <demo>
    // @category demo
    // @vanilla
    //
    // Play back the demo named demo.lmp.
    //

    p = M_CheckParmWithArgs("-playdemo", 1);

    if (!p)
    {
        //!
        // @arg <demo>
        // @category demo
        // @vanilla
        //
        // Play back the demo named demo.lmp, determining the framerate
        // of the screen.
        //

        p = M_CheckParmWithArgs("-timedemo", 1);
    }

    if (p)
    {
        char *uc_filename;
        char file[256];

        M_StringCopy(file, myargv[p+1], sizeof(file));

        // With Vanilla Hexen you have to specify the file without
        // extension, but make that optional.
        uc_filename = strdup(myargv[p + 1]);
        M_ForceUppercase(uc_filename);

        if (!M_StringEndsWith(uc_filename, ".LMP"))
        {
            M_StringConcat(file, ".lmp", sizeof(file));
        }

        free(uc_filename);

        if (W_AddFile(file) != NULL)
        {
            M_StringCopy(demolumpname, lumpinfo[numlumps - 1]->name,
                         sizeof(demolumpname));
        }
        else
        {
            // The file failed to load, but copy the original arg as a
            // demo name to make tricks like -playdemo demo1 possible.
            M_StringCopy(demolumpname, myargv[p+1], sizeof(demolumpname));
        }

        ST_Message("Playing demo %s.\n", myargv[p+1]);
    }

    //!
    // @category demo
    //
    // Record or playback a demo, automatically quitting
    // after either level exit or player respawn.
    //

    demoextend = (!M_ParmExists("-nodemoextend"));
    //[crispy] make demoextend the default

    if (M_ParmExists("-testcontrols"))
    {
        autostart = true;
        testcontrols = true;
    }
}

//==========================================================================
//
// WarpCheck
//
//==========================================================================

static void WarpCheck(void)
{
    int p;
    int map;

    //!
    // @category game
    // @arg x
    // @vanilla
    //
    // Start a game immediately, warping to MAPx.
    //

    p = M_CheckParm("-warp");
    if (p && p < myargc - 1)
    {
        WarpMap = atoi(myargv[p + 1]);
        map = P_TranslateMap(WarpMap);
        if (map == -1)
        {                       // Couldn't find real map number
            startmap = 1;
            ST_Message("-WARP: Invalid map number.\n");
        }
        else
        {                       // Found a valid startmap
            startmap = map;
            autostart = true;
        }
    }
    else
    {
        WarpMap = 1;
        startmap = P_TranslateMap(1);
        if (startmap == -1)
        {
            startmap = 1;
        }
    }
}

//==========================================================================
//
// H2_GameLoop
//
//==========================================================================

void H2_GameLoop(void)
{
    // [crispy] update the "singleplayer" variable
    CheckCrispySingleplayer(!demorecording && gameaction != ga_playdemo && !netgame);

    if (!crispy->singleplayer)
    {
        int i;

        const struct {
            boolean feature;
            const char *option;
        } custom_options[] = {
            {crispy->moreammo,   "-moremana"},
            {crispy->fast,       "-fast"},
            {crispy->autohealth, "-autohealth"},
        };

        for (i = 0; i < arrlen(custom_options); i++)
        {
            if (custom_options[i].feature)
            {
                I_Error("The %s option is not supported\n"
                        "for demos and network play.",
                        custom_options[i].option);
            }
        }
    }

    if (M_CheckParm("-debugfile"))
    {
        char filename[20];
        M_snprintf(filename, sizeof(filename), "debug%i.txt", consoleplayer);
        debugfile = M_fopen(filename, "w");
    }
    I_SetWindowTitle(gamedescription);
    I_GraphicsCheckCommandLine();
    I_SetGrabMouseCallback(D_GrabMouseCallback);
    I_RegisterWindowIcon(hexen_icon_data, hexen_icon_w, hexen_icon_h);
    I_InitGraphics();

    while (1)
    {
        static int oldgametic;
        // Frame syncronous IO operations
        I_StartFrame();

        // Process one or more tics
        // Will run at least one tic
        TryRunTics();

        if (oldgametic < gametic)
        {
            // Move positional sounds
            S_UpdateSounds(players[displayplayer].mo);
            oldgametic = gametic;
        }

        DrawAndBlit();

        // [crispy] post-rendering function pointer to apply config changes
        // that affect rendering and that are better applied after the current
        // frame has finished rendering
        if (crispy->post_rendering_hook)
        {
            crispy->post_rendering_hook();
            crispy->post_rendering_hook = NULL;
        }
    }
}

//==========================================================================
//
// H2_ProcessEvents
//
// Send all the events of the given timestamp down the responder chain.
//
//==========================================================================

void H2_ProcessEvents(void)
{
    event_t *ev;

    for (;;)
    {
        ev = D_PopEvent();

        if (ev == NULL)
        {
            break;
        }

        if (F_Responder(ev))
        {
            continue;
        }
        if (MN_Responder(ev))
        {
            continue;
        }
        G_Responder(ev);
    }
}

//==========================================================================
//
// DrawAndBlit
//
//==========================================================================

static void DrawAndBlit(void)
{
    // Change the view size if needed
    if (setsizeneeded)
    {
        R_ExecuteSetViewSize();
    }

    // Do buffered drawing
    switch (gamestate)
    {
        case GS_LEVEL:
            if (!gametic)
            {
                break;
            }
            if (automapactive && !crispy->automapoverlay)
            {
                // [crispy] update automap while playing
                R_RenderPlayerView(&players[displayplayer]);
                AM_Drawer();
            }
            else
            {
                R_RenderPlayerView(&players[displayplayer]);
            }
            if (automapactive && crispy->automapoverlay)
            {
                AM_Drawer();
                BorderNeedRefresh = true;
            }
            CT_Drawer();
            UpdateState |= I_FULLVIEW;
            SB_Drawer();
            CrispyDrawStats();
            break;
        case GS_INTERMISSION:
            IN_Drawer();
            break;
        case GS_FINALE:
            F_Drawer();
            break;
        case GS_DEMOSCREEN:
            PageDrawer();
            break;
        case GS_LEVEL_SELECT:
            DrawLevelSelect();
            break;
    }

    if (testcontrols)
    {
        V_DrawMouseSpeedBox(testcontrols_mousespeed);
    }

    if (paused && !MenuActive && !askforquit && gamestate != GS_LEVEL_SELECT)
    {
        if (!netgame)
        {
            V_DrawPatch(160, (viewwindowy >> crispy->hires) + 5, W_CacheLumpName("PAUSED",
                                                              PU_CACHE));
        }
        else
        {
            V_DrawPatch(160, 70, W_CacheLumpName("PAUSED", PU_CACHE));
        }
    }

    // Draw current message
    if (gamestate != GS_LEVEL_SELECT)
        DrawMessage();

    // Draw Menu
    MN_Drawer();

    if (gamestate != GS_FINALE)
    {
        ap_notif_draw();
        HU_DrawAPMessages();   // [AP] Sticky messages on top of everything
    }

    // Send out any new accumulation
    NetUpdate();

    // Flush buffered stuff to screen
    I_FinishUpdate();
}

//==========================================================================
//
// DrawMessage
//
//==========================================================================

static void DrawMessage(void)
{
    player_t *player;

    player = &players[consoleplayer];
    if (player->messageTics <= 0)
    {                           // No message
        return;
    }
    if (player->yellowMessage)
    {
        MN_DrTextAYellow(player->message,
                         160 - MN_TextAWidth(player->message) / 2, 1);
    }
    else
    {
        MN_DrTextA(player->message, 160 - MN_TextAWidth(player->message) / 2,
                   1);
    }
}

int right_widget_w, right_widget_h; // [crispy]

static void CrispyDrawStats (void)
{
    static short height, coord_x, coord_w;
    char str[32];
    player_t *const player = &players[consoleplayer];
    int right_widget_x;

    if (!height || !coord_x || !coord_w)
    {
        const int FontABaseLump = W_GetNumForName("FONTA_S") + 1;
        const patch_t *const p = W_CacheLumpNum(FontABaseLump + 'A' - 33, PU_CACHE);

        height = SHORT(p->height) + 1;
        coord_w = 7 * SHORT(p->width);
        coord_x = ORIGWIDTH - coord_w;
    }

    right_widget_w = 0;
    right_widget_h = 0;
    right_widget_x = coord_x + WIDESCREENDELTA;

    if (crispy->playercoords == WIDGETS_ALWAYS || (automapactive && crispy->playercoords == WIDGETS_AUTOMAP))
    {
        right_widget_w = coord_w;
        right_widget_h = 3 * height + 2;

        M_snprintf(str, sizeof(str), "X %-5d", player->mo->x>>FRACBITS);
        MN_DrTextA(str, right_widget_x, 1*height);

        M_snprintf(str, sizeof(str), "Y %-5d", player->mo->y>>FRACBITS);
        MN_DrTextA(str, right_widget_x, 2*height);

        M_snprintf(str, sizeof(str), "A %-5d", player->mo->angle/ANG1);
        MN_DrTextA(str, right_widget_x, 3*height);

        if (player->cheats & CF_SHOWFPS)
        {
            right_widget_h += height + 1;
            M_snprintf(str, sizeof(str), "%d FPS", crispy->fps);
            MN_DrTextA(str, right_widget_x, 4*height + 1);
        }
    }
    else if (player->cheats & CF_SHOWFPS)
    {
        right_widget_w = coord_w;
        right_widget_h = height + 2;

        M_snprintf(str, sizeof(str), "%d FPS", crispy->fps);
        MN_DrTextA(str, right_widget_x, 1*height);
    }
}

//==========================================================================
//
// H2_PageTicker
//
//==========================================================================

void H2_PageTicker(void)
{
    if (--pagetic < 0)
    {
        H2_AdvanceDemo();
    }
}

//==========================================================================
//
// PageDrawer
//
//==========================================================================

static void PageDrawer(void)
{
    V_DrawFullscreenRawOrPatch(W_GetNumForName(pagename)); // [crispy]

    if (demosequence == 1)
    {
        V_DrawPatch(4, 160, W_CacheLumpName("ADVISOR", PU_CACHE));
    }
    UpdateState |= I_FULLSCRN;
}

//==========================================================================
//
// H2_AdvanceDemo
//
// Called after each demo or intro demosequence finishes.
//
//==========================================================================

void H2_AdvanceDemo(void)
{
    advancedemo = true;
}

//==========================================================================
//
// H2_DoAdvanceDemo
//
//==========================================================================

void H2_DoAdvanceDemo(void)
{
    players[consoleplayer].playerstate = PST_LIVE;      // don't reborn
    advancedemo = false;
    usergame = false;           // can't save/end game here
    paused = false;
    gameaction = ga_nothing;
    demosequence = (demosequence + 1) % 7;
    switch (demosequence)
    {
        case 0:
            pagetic = 280;
            gamestate = GS_DEMOSCREEN;
            pagename = "TITLE";
            S_StartSongName("hexen", true);
            break;
        case 1:
            pagetic = 210;
            gamestate = GS_DEMOSCREEN;
            pagename = "TITLE";
            break;
        case 2:
            BorderNeedRefresh = true;
            UpdateState |= I_FULLSCRN;
            G_DeferedPlayDemo("demo1");
            break;
        case 3:
            pagetic = 200;
            gamestate = GS_DEMOSCREEN;
            pagename = "CREDIT";
            break;
        case 4:
            BorderNeedRefresh = true;
            UpdateState |= I_FULLSCRN;
            G_DeferedPlayDemo("demo2");
            break;
        case 5:
            pagetic = 200;
            gamestate = GS_DEMOSCREEN;
            pagename = "CREDIT";
            break;
        case 6:
            BorderNeedRefresh = true;
            UpdateState |= I_FULLSCRN;
            G_DeferedPlayDemo("demo3");
            break;
    }
}

//==========================================================================
//
// H2_StartTitle
//
//==========================================================================

void H2_StartTitle(void)
{
    gameaction = ga_nothing;
    demosequence = -1;
    H2_AdvanceDemo();
}

//==========================================================================
//
// CheckRecordFrom
//
// -recordfrom <savegame num> <demoname>
//
//==========================================================================

static void CheckRecordFrom(void)
{
    int p;

    //!
    // @vanilla
    // @category demo
    // @arg <savenum> <demofile>
    //
    // Record a demo, loading from the given filename. Equivalent
    // to -loadgame <savenum> -record <demofile>.
    //
    p = M_CheckParm("-recordfrom");
    if (!p || p > myargc - 2)
    {                           // Bad args
        return;
    }
    G_LoadGame(atoi(myargv[p + 1]));
    G_DoLoadGame();             // Load the gameskill etc info from savegame
    G_RecordDemo(gameskill, 1, gameepisode, gamemap, myargv[p + 2]);

    H2_GameLoop();              // Never returns
}

// haleyjd: removed WATCOMC
/*
void CleanExit(void)
{
	union REGS regs;

	I_ShutdownKeyboard();
	regs.x.eax = 0x3;
	int386(0x10, &regs, &regs);
	printf("Exited from HEXEN: Beyond Heretic.\n");
	exit(1);
}
*/

//==========================================================================
//
// CreateSavePath
//
//==========================================================================

static void CreateSavePath(void)
{
    M_MakeDirectory(SavePath);
}
