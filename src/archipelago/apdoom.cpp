#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>
#ifdef _MSC_VER
#include <direct.h>
#endif
#else
#include <sys/types.h>
#endif


#include "apdoom.h"
#include "Archipelago.h"
#include "apdoom_def.h"
#include <json/json.h>
#include <memory.h>
#include <chrono>
#include <thread>
#include <queue>
#include <fstream>


#if defined(_WIN32)
static wchar_t *ConvertMultiByteToWide(const char *str, UINT code_page)
{
    wchar_t *wstr = NULL;
    int wlen = 0;

    wlen = MultiByteToWideChar(code_page, 0, str, -1, NULL, 0);

    if (!wlen)
    {
        errno = EINVAL;
        printf("ConvertMultiByteToWide: Failed to convert path to wide encoding.\n");
        return NULL;
    }

    wstr = (wchar_t*)malloc(sizeof(wchar_t) * wlen);

    if (!wstr)
    {
        printf("ERROR: ConvertMultiByteToWide: Failed to allocate new string.");
        return NULL;
    }

    if (MultiByteToWideChar(code_page, 0, str, -1, wstr, wlen) == 0)
    {
        errno = EINVAL;
        printf("ConvertMultiByteToWide: Failed to convert path to wide encoding.\n");
        free(wstr);
        return NULL;
    }

    return wstr;
}

wchar_t *AP_ConvertUtf8ToWide(const char *str)
{
    return ConvertMultiByteToWide(str, CP_UTF8);
}
#endif



//
// Create a directory
//

void AP_MakeDirectory(const char *path)
{
#ifdef _WIN32
    wchar_t *wdir;

    wdir = AP_ConvertUtf8ToWide(path);

    if (!wdir)
    {
        return;
    }

    _wmkdir(wdir);

    free(wdir);
#else
    mkdir(path, 0755);
#endif
}


FILE *AP_fopen(const char *filename, const char *mode)
{
#ifdef _WIN32
    FILE *file;
    wchar_t *wname = NULL;
    wchar_t *wmode = NULL;

    wname = AP_ConvertUtf8ToWide(filename);

    if (!wname)
    {
        return NULL;
    }

    wmode = AP_ConvertUtf8ToWide(mode);

    if (!wmode)
    {
        free(wname);
        return NULL;
    }

    file = _wfopen(wname, wmode);

    free(wname);
    free(wmode);

    return file;
#else
    return fopen(filename, mode);
#endif
}


int AP_FileExists(const char *filename)
{
    FILE *fstream;

    fstream = AP_fopen(filename, "r");

    if (fstream != NULL)
    {
        fclose(fstream);
        return true;
    }
    else
    {
        // If we can't open because the file is a directory, the 
        // "file" exists at least!

        return errno == EISDIR;
    }
}



ap_level_info_t ap_level_infos[AP_EPISODE_COUNT][AP_LEVEL_COUNT] =
{
    // Episode 0 World Map
    {
        { {false,false,false}, 0 },	// location of level 0 (CJ)
	    { {false,false,true}, 1 },	// location of level 1 (CJ)
	    { {true,true,false}, 2 },	// location of level 2 (CJ)
	    { {true,true,false}, 2 },	// location of level 3 (CJ)
	    { {true,true,false}, 2 },	// location of level 4 (CJ)
	    { {true,true,true}, 3 },	// location of level 5 (CJ)
	    { {true,true,true}, 3 },	// location of level 6 (CJ)
	    { {false,false,false}, 0 },	// location of level 7 (CJ)
	    { {true,true,true}, 3 }	// location of level 8 (CJ)
    },

    // Episode 1 World Map should go here
    {
	    { {true,false,true}, 2 },	// location of level 0 (CJ)
	    { {true,true,true}, 3 },	// location of level 1 (CJ)
	    { {true,false,false}, 1 },	// location of level 2 (CJ)
	    { {true,true,false}, 2 },	// location of level 3 (CJ)
	    { {false,false,false}, 0 },	// location of level 4 (CJ)
	    { {true,true,true}, 3 },	// location of level 5 (CJ)
	    { {true,true,true}, 3 },	// location of level 6 (CJ)
	    { {false,false,false}, 0 },	// location of level 7 (CJ)
	    { {true,true,true}, 3 }	// location of level 8 (CJ)
    },

    // Episode 2 World Map should go here
    {
	    { {false,false,false}, 0 },	// location of level 0 (CJ)
	    { {true,false,false}, 1 },	// location of level 1 (CJ)
	    { {true,false,false}, 1 },	// location of level 2 (CJ)
	    { {true,true,true}, 3 },	// location of level 3 (CJ)
	    { {true,true,false}, 2 },	// location of level 4 (CJ)
	    { {true,false,false}, 1 },	// location of level 5 (CJ)
	    { {true,true,true}, 3 },	// location of level 6 (CJ)
	    { {false,false,false}, 0 },	// location of level 7 (CJ)
	    { {true,false,true}, 2 }	// location of level 8 (CJ)
    }
};


ap_state_t ap_state;
int ap_is_in_game = 0;

static ap_settings_t ap_settings;
static AP_RoomInfo ap_room_info;
static std::queue<int64_t> ap_item_queue; // We queue when we're in the menu.
static bool ap_was_connected = false; // Got connected at least once. That means the state is valid


void f_itemclr();
void f_itemrecv(int64_t item_id, bool notify_player);
void f_locrecv(int64_t loc_id);
void load_state();
void save_state();


int apdoom_init(ap_settings_t* settings)
{
	memset(&ap_state, 0, sizeof(ap_state));
	ap_state.player_state.health = 100;
	ap_state.player_state.ready_weapon = 1;
	ap_state.player_state.weapon_owned[0] = 1; // Fist
	ap_state.player_state.weapon_owned[1] = 1; // Pistol
	ap_state.player_state.ammo[0] = 50; // Clip
	ap_state.player_state.max_ammo[0] = 200;
	ap_state.player_state.max_ammo[1] = 50;
	ap_state.player_state.max_ammo[2] = 300;
	ap_state.player_state.max_ammo[3] = 50;

	ap_settings = *settings;

	AP_NetworkVersion version = {0, 4, 0};
	AP_SetClientVersion(&version);
    AP_Init(ap_settings.ip, ap_settings.game, ap_settings.player_name, ap_settings.passwd);
	AP_SetItemClearCallback(f_itemclr);
	AP_SetItemRecvCallback(f_itemrecv);
	AP_SetLocationCheckedCallback(f_locrecv);
    AP_Start();

	// Block DOOM until connection succeeded or failed
	auto start_time = std::chrono::steady_clock::now();
	while (true)
	{
		switch (AP_GetConnectionStatus())
		{
			case AP_ConnectionStatus::Authenticated:
				printf("AP: Authenticated\n");
				AP_GetRoomInfo(&ap_room_info);

				ap_was_connected = true;

				// Create a directory where saves will go for this AP seed.
				if (!AP_FileExists(("AP_" + ap_room_info.seed_name).c_str()))
					AP_MakeDirectory(("AP_" + ap_room_info.seed_name).c_str());

				load_state();
				return 1;
			case AP_ConnectionStatus::ConnectionRefused:
				printf("AP: Failed to connect\n");
				return 0;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		if (std::chrono::steady_clock::now() - start_time > std::chrono::seconds(10))
		{
			printf("AP: Failed to connect\n");
			return 0;
		}
	}

	return 0;
}


void apdoom_shutdown()
{
	if (ap_was_connected)
		save_state();
}


void apdoom_save_state()
{
	if (ap_was_connected)
		save_state();
}


static void json_get_int(const Json::Value& json, int& out_or_default)
{
	if (json.isInt())
		out_or_default = json.asInt();
}


void load_state()
{
	std::string filename = "AP_" + ap_room_info.seed_name + "/apstate.json";
	std::ifstream f(filename);
	if (!f.is_open())
		return; // Could be no state yet, that's fine
	Json::Value json;
	f >> json;
	f.close();

	// Player state
	json_get_int(json["player"]["health"], ap_state.player_state.health);
	json_get_int(json["player"]["armor_points"], ap_state.player_state.armor_points);
	json_get_int(json["player"]["armor_type"], ap_state.player_state.armor_type);
	json_get_int(json["player"]["backpack"], ap_state.player_state.backpack);
	json_get_int(json["player"]["ready_weapon"], ap_state.player_state.ready_weapon);
	json_get_int(json["player"]["kill_count"], ap_state.player_state.kill_count);
	json_get_int(json["player"]["item_count"], ap_state.player_state.item_count);
	json_get_int(json["player"]["secret_count"], ap_state.player_state.secret_count);
	for (int i = 0; i < AP_NUM_POWERS; ++i)
		json_get_int(json["player"]["powers"][i], ap_state.player_state.powers[i]);
	for (int i = 0; i < AP_NUM_WEAPONS; ++i)
		json_get_int(json["player"]["weapon_owned"][i], ap_state.player_state.weapon_owned[i]);
	for (int i = 0; i < AP_NUM_AMMO; ++i)
		json_get_int(json["player"]["ammo"][i], ap_state.player_state.ammo[i]);
	for (int i = 0; i < AP_NUM_AMMO; ++i)
		json_get_int(json["player"]["max_ammo"][i], ap_state.player_state.max_ammo[i]);

	// Level states
	for (int i = 0; i < AP_EPISODE_COUNT; ++i)
	{
		for (int j = 0; j < AP_LEVEL_COUNT; ++j)
		{
			json_get_int(json["episodes"][i][j]["completed"], ap_state.level_states[i][j].completed);
			json_get_int(json["episodes"][i][j]["keys0"], ap_state.level_states[i][j].keys[0]);
			json_get_int(json["episodes"][i][j]["keys1"], ap_state.level_states[i][j].keys[1]);
			json_get_int(json["episodes"][i][j]["keys2"], ap_state.level_states[i][j].keys[2]);
			json_get_int(json["episodes"][i][j]["check_count"], ap_state.level_states[i][j].check_count);
			json_get_int(json["episodes"][i][j]["has_map"], ap_state.level_states[i][j].has_map);
		}
	}
}


void save_state()
{
	std::string filename = "AP_" + ap_room_info.seed_name + "/apstate.json";
	std::ofstream f(filename);
	if (!f.is_open())
	{
		printf("Failed to save AP state.\n");
		return; // Ok that's bad. we won't save player state
	}

	// Player state
	Json::Value json;
	Json::Value json_player;
	json_player["health"] = ap_state.player_state.health;
	json_player["armor_points"] = ap_state.player_state.armor_points;
	json_player["armor_type"] = ap_state.player_state.armor_type;
	json_player["backpack"] = ap_state.player_state.backpack;
	json_player["ready_weapon"] = ap_state.player_state.ready_weapon;
	json_player["kill_count"] = ap_state.player_state.kill_count;
	json_player["item_count"] = ap_state.player_state.item_count;
	json_player["secret_count"] = ap_state.player_state.secret_count;

	Json::Value json_powers(Json::arrayValue);
	for (int i = 0; i < AP_NUM_POWERS; ++i)
		json_powers.append(ap_state.player_state.powers[i]);
	json_player["powers"] = json_powers;

	Json::Value json_weapon_owned(Json::arrayValue);
	for (int i = 0; i < AP_NUM_WEAPONS; ++i)
		json_weapon_owned.append(ap_state.player_state.weapon_owned[i]);
	json_player["weapon_owned"] = json_weapon_owned;

	Json::Value json_ammo(Json::arrayValue);
	for (int i = 0; i < AP_NUM_AMMO; ++i)
		json_ammo.append(ap_state.player_state.ammo[i]);
	json_player["ammo"] = json_ammo;

	Json::Value json_max_ammo(Json::arrayValue);
	for (int i = 0; i < AP_NUM_AMMO; ++i)
		json_max_ammo.append(ap_state.player_state.max_ammo[i]);
	json_player["max_ammo"] = json_max_ammo;

	json["player"] = json_player;

	// Level states
	Json::Value json_episodes(Json::arrayValue);
	for (int i = 0; i < AP_EPISODE_COUNT; ++i)
	{
		Json::Value json_levels(Json::arrayValue);
		for (int j = 0; j < AP_LEVEL_COUNT; ++j)
		{
			Json::Value json_level;

			json_level["completed"] = ap_state.level_states[i][j].completed;
			json_level["keys0"] = ap_state.level_states[i][j].keys[0];
			json_level["keys1"] = ap_state.level_states[i][j].keys[1];
			json_level["keys2"] = ap_state.level_states[i][j].keys[2];
			json_level["check_count"] = ap_state.level_states[i][j].check_count;
			json_level["has_map"] = ap_state.level_states[i][j].has_map;

			json_levels.append(json_level);
		}
		json_episodes.append(json_levels);
	}
	json["episodes"] = json_episodes;

	f << json;
}


void f_itemclr()
{
	// Not sure what use this would have here.
}


void f_itemrecv(int64_t item_id, bool notify_player /* Unused */)
{
	auto it = item_doom_type_table.find(item_id);
	if (it == item_doom_type_table.end())
		return; // Skip
	ap_item_t item = it->second;

	// Is it a key?
	switch (item.doom_type)
	{
		case 5:
		case 40:
			ap_state.level_states[item.ep - 1][item.map - 1].keys[0] = 1;
			break;
		case 6:
		case 39:
			ap_state.level_states[item.ep - 1][item.map - 1].keys[1] = 1;
			break;
		case 13:
		case 38:
			ap_state.level_states[item.ep - 1][item.map - 1].keys[2] = 1;
			break;
		case 2026:
			ap_state.level_states[item.ep - 1][item.map - 1].has_map = 1;
			break;
	}

	if (!ap_is_in_game)
	{
		ap_item_queue.push(item_id);
		return;
	}

	// Give item to player
	ap_settings.give_item_callback(item.doom_type);
}


void f_locrecv(int64_t loc_id)
{
	// Not much to do here
}


const char* apdoom_get_seed()
{
	return ap_room_info.seed_name.c_str();
}


void apdoom_check_location(int ep, int map, int index)
{
	auto it1 = location_table.find(ep);
	if (it1 == location_table.end()) return;

	auto it2 = it1->second.find(map);
	if (it2 == it1->second.end()) return;

	auto it3 = it2->second.find(index);
	if (it3 == it2->second.end()) return;

	int64_t id = it3->second;

	AP_SendItem(id);
}


void apdoom_victory()
{
	AP_StoryComplete(); // Isn't this already handled by the server rules?
}

void apdoom_update()
{
	while (AP_IsMessagePending())
	{
		auto msg = AP_GetLatestMessage();

		switch (msg->type)
		{
			case AP_MessageType::Plaintext:
			{
				ap_settings.message_callback(msg->text.c_str());
				break;
			}
			case AP_MessageType::ItemSend:
			{
				AP_ItemSendMessage* item_send_msg = (AP_ItemSendMessage*)msg;
				ap_settings.message_callback(msg->text.c_str());
				break;
			}
			case AP_MessageType::ItemRecv:
			{
				AP_ItemRecvMessage* item_recv_msg = (AP_ItemRecvMessage*)msg;
				ap_settings.message_callback(msg->text.c_str());
				break;
			}
			case AP_MessageType::Hint:
			{
				AP_HintMessage* hint_msg = (AP_HintMessage*)msg;
				ap_settings.message_callback(msg->text.c_str());
				break;
			}
			case AP_MessageType::Countdown:
			{
				AP_CountdownMessage* countdown_msg = (AP_CountdownMessage*)msg;
				ap_settings.message_callback(msg->text.c_str());
				break;
			}
		}

		AP_ClearLatestMessage();
	}

	// Check if we're in game, then dequeue the items
	if (ap_is_in_game)
	{
		while (!ap_item_queue.empty())
		{
			auto item_id = ap_item_queue.front();
			ap_item_queue.pop();
			f_itemrecv(item_id, true);
		}
	}
}
