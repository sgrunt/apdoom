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
// *Interface header with the underlying C++ archipelago library*
//

#ifndef _APDOOM_
#define _APDOOM_


#ifdef __cplusplus
extern "C"
{
#endif



#define APDOOM_MAJOR 0
#define APDOOM_MINOR 12
#define APDOOM_PATCH 3
#define APDOOM_STR(x) APDOOM_STR2(x)
#define APDOOM_STR2(x) #x
#define APDOOM_VERSION APDOOM_STR(APDOOM_MAJOR) "." APDOOM_STR(APDOOM_MINOR) "." APDOOM_STR(APDOOM_PATCH)
#define APDOOM_VERSION_TEXT APDOOM_VERSION " (BETA)"
#define APDOOM_VERSION_FULL_TEXT "APDOOM " APDOOM_VERSION_TEXT


#define AP_CHECK_MAX 64 // Arbitrary number
#define AP_MAX_THING 1024 // Twice more than current max for every level


typedef struct
{
    int doom_type;
    int index;
    int check_sanity;
} ap_thing_info_t;


typedef struct
{
    const char* name;
    int keys[3];
    int use_skull[3];
    int check_count;
    int thing_count;
    ap_thing_info_t thing_infos[AP_MAX_THING];
    int sanity_check_count;

} ap_level_info_t;


typedef struct
{
    int completed;
    int keys[3];
    int check_count;
    int has_map;
    int unlocked;
    int checks[AP_CHECK_MAX];
    int special; // Berzerk or Wings
    int flipped;

} ap_level_state_t;


typedef struct
{
    int type;
    int count;
} ap_inventory_slot_t;


typedef struct
{
    int health;
    int armor_points;
    int armor_type;
    int backpack; // Has backpack or not
    int ready_weapon; // Last weapon held
    int kill_count; // We accumulate globally
    int item_count;
    int secret_count;
    int* powers;
    int* weapon_owned;
    int* ammo;
    int* max_ammo; // Could be deduced by checking backpack
    ap_inventory_slot_t* inventory;

} ap_player_state_t;


typedef struct
{
    ap_level_state_t* level_states;
    ap_player_state_t player_state;
    int ep; // Useful when reloading, to load directly where we left
    int map;
    int difficulty;
    int random_monsters;
    int random_items;
    int two_ways_keydoors;
    int* episodes;
    int victory;
    int flip_levels;
    int check_sanity;
    int reset_level_on_death;
    
} ap_state_t;


typedef struct
{
    const char* ip;
    const char* game;
    const char* player_name;
    const char* passwd;
    void (*message_callback)(const char*);
    void (*give_item_callback)(int doom_type, int ep, int map);
    void (*victory_callback)();
} ap_settings_t;


#define AP_NOTIF_STATE_PENDING 0
#define AP_NOTIF_STATE_DROPPING 1
#define AP_NOTIF_STATE_HIDING 2
#define AP_NOTIF_SIZE 30
#define AP_NOTIF_PADDING 2


typedef struct
{
    char sprite[9];
    int x, y;
    float xf, yf;
    float velx, vely;
    char text[260];
    int t;
    int state;
} ap_notification_icon_t;


extern ap_state_t ap_state;
extern int ap_is_in_game; // Don't give items when in menu (Or when dead on the ground).
extern int ap_episode_count;
extern int ap_map_count;


int apdoom_init(ap_settings_t* settings);
void apdoom_shutdown();
void apdoom_save_state();
void apdoom_check_location(int ep, int map, int index);
int apdoom_is_location_progression(int ep, int map, int index);
void apdoom_check_victory();
void apdoom_update();
const char* apdoom_get_seed();
void apdoom_send_message(const char* msg);
void apdoom_complete_level(int ep, int map);
ap_level_state_t* ap_get_level_state(int ep, int map); // 1-based
ap_level_info_t* ap_get_level_info(int ep, int map); // 1-based
const ap_notification_icon_t* ap_get_notification_icons(int* count);
int ap_get_highest_episode();
int ap_validate_doom_location(int ep, int map, int doom_type, int index);

// Deathlink stuff
void apdoom_on_death();
void apdoom_clear_death();
int apdoom_should_die();

#ifdef __cplusplus
}
#endif


#endif
