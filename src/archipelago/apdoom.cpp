#include "apdoom.h"
#include "Archipelago.h"
#include "apdoom_def.h"
#include <memory.h>
#include <chrono>
#include <thread>


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
static ap_settings_t ap_settings;
static AP_RoomInfo ap_room_info;


void f_itemclr();
void f_itemrecv(int64_t item_id, bool notify_player);
void f_locrecv(int64_t loc_id);


int apdoom_init(ap_settings_t* settings)
{
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

	AP_GetRoomInfo(&ap_room_info);

	return 0;
}


void f_itemclr()
{
	// Not sure what use this would have here.
}


void f_itemrecv(int64_t item_id, bool notify_player)
{
	auto it = item_doom_type_table.find(item_id);
	if (it == item_doom_type_table.end())
		return; // Skip

	// Give item to player
	ap_settings.give_item_callback(it->second);
}


void f_locrecv(int64_t loc_id)
{
	// Not much to do here
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
}
