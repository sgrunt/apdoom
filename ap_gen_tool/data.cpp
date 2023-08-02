#include "data.h"
#include "maps.h"

#include <onut/Files.h>
#include <onut/Json.h>
#include <onut/Log.h>
#include <json/json.h>


std::map<std::string, game_t> games;


void init_data()
{
    auto game_json_files = onut::findAllFiles("./games/", "json", false);
    for (const auto& game_json_file : game_json_files)
    {
        Json::Value game_json;
        if (!onut::loadJson(game_json, game_json_file)) continue;

        game_t game;

        game.name = game_json["name"].asString();
        game.world = game_json["world"].asString();
        game.codename = game_json["codename"].asString();
        game.classname = game_json["classname"].asString();
        game.wad_name = game_json["wad"].asString();
        game.item_ids = game_json["item_ids"].asInt64();
        game.loc_ids = game_json["loc_ids"].asInt64();

        const auto& episodes_json = game_json["map_names"];
        if (!episodes_json.empty())
        {
            if (episodes_json[0].isArray()) // Episodic
            {
                game.episodic = true;
                game.ep_count = (int)episodes_json.size();
                game.map_count = (int)episodes_json[0].size();
                game.metas.resize(game.ep_count * game.map_count);
                for (int ep = 0; ep < game.ep_count; ++ep)
                {
                    for (int map = 0; map < game.map_count; ++map)
                    {
                        game.metas[ep * game.map_count + map].name = episodes_json[ep][map].asString();
                    }
                }
            }
            else // Non-Episodic
            {
                game.episodic = false;
                game.ep_count = 1;
                game.map_count = (int)episodes_json.size();
                game.metas.resize(game.map_count);
                for (int i = 0; i < game.map_count; ++i)
                {
                    game.metas[i].name = episodes_json[i].asString();
                }
            }
        }

        const auto& doom_types_ids = game_json["location_doom_types"].getMemberNames();
        for (const auto& doom_types_id : doom_types_ids)
        {
            game.location_doom_types[std::stoi(doom_types_id)] = game_json["location_doom_types"][doom_types_id].asString();
        }

        const auto& progressions_json = game_json["progressions"];
        for (const auto& progression_json : progressions_json)
        {
            ap_item_def_t item;
            item.doom_type = progression_json["doom_type"].asInt();
            item.name = progression_json["name"].asString();
            item.group = progression_json["group"].asString();
            game.progressions.push_back(item);
        }

        const auto& fillers_json = game_json["fillers"];
        for (const auto& filler_json : fillers_json)
        {
            ap_item_def_t item;
            item.doom_type = filler_json["doom_type"].asInt();
            item.name = filler_json["name"].asString();
            item.group = filler_json["group"].asString();
            game.fillers.push_back(item);
        }

        const auto& unique_progressions_json = game_json["unique_progressions"];
        for (const auto& progression_json : unique_progressions_json)
        {
            ap_item_def_t item;
            item.doom_type = progression_json["doom_type"].asInt();
            item.name = progression_json["name"].asString();
            item.group = progression_json["group"].asString();
            game.unique_progressions.push_back(item);
        }

        const auto& unique_fillers_json = game_json["unique_fillers"];
        for (const auto& filler_json : unique_fillers_json)
        {
            ap_item_def_t item;
            item.doom_type = filler_json["doom_type"].asInt();
            item.name = filler_json["name"].asString();
            item.group = filler_json["group"].asString();
            game.unique_fillers.push_back(item);
        }

        const auto& keys_json = game_json["keys"];
        for (const auto& key_json : keys_json)
        {
            ap_key_def_t item;
            item.item.doom_type = key_json["doom_type"].asInt();
            item.item.name = key_json["name"].asString();
            item.item.group = key_json["group"].asString();
            item.key = key_json["key"].asInt();
            item.use_skull = key_json["use_skull"].asBool();
            game.keys.push_back(item);
        }

        const auto& loc_remap_json = game_json["loc_remap"];
        const auto& loc_remap_names = game_json["loc_remap"].getMemberNames();
        for (const auto& loc_name : loc_remap_names)
        {
            game.loc_remap[loc_name] = loc_remap_json[loc_name].asInt64();
        }

        const auto& item_remap_json = game_json["item_remap"];
        const auto& item_remap_names = game_json["item_remap"].getMemberNames();
        for (const auto& item_name : item_remap_names)
        {
            game.item_remap[item_name] = item_remap_json[item_name].asInt64();
        }

        init_maps(game);

        games[game.name] = game;
    }
}


game_t* get_game(const level_index_t& idx)
{
    auto it = games.find(idx.game_name);
    if (it == games.end()) return nullptr;
    return &it->second;
}


meta_t* get_meta(const level_index_t& idx, active_source_t source)
{
    auto game = get_game(idx);
    if (!game) return nullptr;
    auto k = idx.ep * game->map_count + idx.map;
    if (source == active_source_t::current) return &game->metas[k];
    if (source == active_source_t::target) return &game->metas[k];
    return nullptr;
}

map_state_t* get_state(const level_index_t& idx, active_source_t source)
{
    auto game = get_game(idx);
    if (!game) return nullptr;
    auto k = idx.ep * game->map_count + idx.map;
    if (source == active_source_t::current) return &game->metas[k].state;
    if (source == active_source_t::target) return &game->metas[k].state;
    return nullptr;
}

map_t* get_map(const level_index_t& idx)
{
    auto game = get_game(idx);
    if (!game) return nullptr;
    auto k = idx.ep * game->map_count + idx.map;
    return &game->metas[k].map;
}

const std::string& get_level_name(const level_index_t& idx)
{
    auto game = get_game(idx);
    static std::string err_str = "ERROR";
    if (!game) return err_str;
    auto k = idx.ep * game->map_count + idx.map;
    return game->metas[k].name;
}
