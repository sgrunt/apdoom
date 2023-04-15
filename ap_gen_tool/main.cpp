#include <stdio.h>
#include <cinttypes>
#include <string.h>
#include <vector>
#include <string>
#include <map>
#include <set>


struct map_header_t
{
    char identification[4];
    int32_t num_lumps;
    int32_t directory_offset;
};


struct map_directory_t
{
    int32_t offset;
    int32_t size;
    char name[8];
};


struct map_thing_t
{
    int16_t x;
    int16_t y;
    int16_t direction;
    int16_t type;
    int16_t flags;
};


struct map_linedefs_t
{
    int16_t start_vertex;
    int16_t end_vertex;
    int16_t flags;
    int16_t special_type;
    int16_t sector_tag;
    int16_t front_sidedef;
    int16_t back_sidedef;
};


struct map_sidedefs_t
{
    int16_t x_offset;
    int16_t y_offset;
    char    upper_texture[8];
    char    lower_texture[8];
    char    middle_texture[8];
    int16_t sector;
};


struct map_vertex_t
{
    int16_t x;
    int16_t y;
};


struct map_sectors_t
{
    int16_t floor_height;
    int16_t ceiling_height;
    char    floor_texture[8];
    char    ceiling_texture[8];
    int16_t light_level;
    int16_t type;
    int16_t tag;
};


enum item_classification_t
{
    FILLER          = 0b0000, // aka trash, as in filler items like ammo, currency etc,
    PROGRESSION     = 0b0001, // Item that is logically relevant
    USEFUL          = 0b0010, // Item that is generally quite useful, but not required for anything logical
    TRAP            = 0b0100, // detrimental or entirely useless (nothing) item
    SKIP_BALANCING  = 0b1000,
    PROGRESSION_SKIP_BALANCING = 0b1001
};


struct ap_item_t
{
    uint64_t id;
    std::string name;
    int ep;
    int lvl;
    int doom_type;
    int count;
    bool is_key;
    item_classification_t classification;
};


struct ap_location_t
{
    uint64_t id;
    std::string name;
    int ep;
    int lvl;
    int doom_thing_index;
    int doom_type;
    std::vector<uint64_t> required_items;
};


struct level_t
{
    char name[9];
    int ep;
    int lvl;
    std::vector<map_thing_t>    things;
    std::vector<map_linedefs_t> linedefs;
    std::vector<map_sidedefs_t> sidedefs;
    std::vector<map_vertex_t>   vertexes;
    std::vector<map_sectors_t>  sectors;
};


uint64_t item_next_id = 350000;
uint64_t location_next_id = 351000;

int total_item_count = 0;
std::vector<ap_item_t> ap_items;
std::vector<ap_location_t> ap_locations;
std::map<std::string, std::set<std::string>> item_name_groups;


const char* level_names[3][9] = {
    {
        "Hangar",
        "Nuclear Plant",
        "Toxin Refinery",
        "Command Control",
        "Phobos Lab",
        "Central Processing",
        "Computer Station",
        "Phobos Anomaly",
        "Military Base"
    },
    {
        "Deimos Anomaly",
        "Containment Area",
        "Refinery",
        "Deimos Lab",
        "Command Center",
        "Halls of the Damned",
        "Spawning Vats",
        "Tower of Babel",
        "Fortress of Mystery"
    },
    {
        "Hell Keep",
        "Slough of Despair",
        "Pandemonium",
        "House of Pain",
        "Unholy Cathedral",
        "Mt. Erebus",
        "Limbo",
        "Dis",
        "Warrens"
    }
};

void add_loc(const std::string& name, const map_thing_t& thing, const level_t* level, int index)
{
    int count = 0;
    for (const auto& loc : ap_locations)
    {
        if (loc.name == name) count++;
    }

    ap_location_t loc;
    loc.id = location_next_id++;
    loc.name = name;
    if (count > 0)
        loc.name += " " + std::to_string(count + 1);
    loc.ep = level->ep;
    loc.lvl = level->lvl;
    loc.doom_thing_index = index;
    loc.doom_type = thing.type; // Index can be a risky one. We could replace the item by it's type if it's unique enough
    ap_locations.push_back(loc);
}

void add_unique(const std::string& name, const map_thing_t& thing, const level_t* level, int index, bool is_key, item_classification_t classification, const std::string& group_name)
{
    ap_item_t item;
    item.id = item_next_id++;
    item.name = name;
    item.ep = level->ep;
    item.lvl = level->lvl;
    item.doom_type = thing.type;
    item.count = 1;
    total_item_count++;
    item.is_key = is_key;
    item.classification = classification;
    ap_items.push_back(item);
    if (!group_name.empty())
    {
        item_name_groups[group_name].insert(name);
    }
    add_loc(name, thing, level, index);
}

void add_item(const std::string& name, int doom_type, int count, item_classification_t classification, const std::string& group_name)
{
    ap_item_t item;
    item.id = item_next_id++;
    item.name = name;
    item.ep = -1;
    item.lvl = -1;
    item.doom_type = doom_type;
    item.count = count;
    total_item_count += count;
    item.is_key = false;
    item.classification = classification;
    if (!group_name.empty())
    {
        item_name_groups[group_name].insert(name);
    }
    ap_items.push_back(item);
}


template<typename T>
static bool try_load_lump(const char *lump_name, 
                          FILE *f, 
                          const map_directory_t &dir_entry, 
                          std::vector<T> &elements)
{
    if (strncmp(dir_entry.name, lump_name, 8) == 0)
    {
        auto count = dir_entry.size / sizeof(T);
        elements.resize(count);
        fseek(f, dir_entry.offset, SEEK_SET);
        fread(elements.data(), sizeof(T), count, f);
        return true;
    }
    return false;
}


int main(int argc, char** argv)
{
    printf("AP Gen Tool\n");

    if (argc != 3) // Minimum effort validation
    {
        printf("Usage: ap_gen_tool.exe DOOM.WAD output_dir (i.e.: C:\\Archipelago\\worlds\\ultimate_doom)\n");
        return 1;
    }

    std::string out_dir = argv[2] + std::string("\\");

    FILE* f = fopen(argv[1], "rb");
    if (!f)
    {
        printf("Cannot open file: %s\n", argv[1]);
        return 1;
    }

    // Read header
    map_header_t header;
    fread(&header, sizeof(header), 1, f);
    if (strncmp(header.identification, "PWAD", 4) != 0 && strncmp(header.identification, "IWAD", 4) != 0)
    {
        printf("Invalid IWAD or PWAD\n");
        return 1;
    }
    
    // Read directory
    std::vector<map_directory_t> directory(header.num_lumps);
    fseek(f, header.directory_offset, SEEK_SET);
    fread(directory.data(), sizeof(map_directory_t), header.num_lumps, f);

    // loop directory and find levels, then load them all. YOLO
    std::vector<level_t*> levels;
    for (int i = 0, len = (int)directory.size(); i < len; ++i)
    {
        const auto &dir_entry = directory[i];
        if (strlen(dir_entry.name) == 4 && dir_entry.name[0] == 'E' && dir_entry.name[2] == 'M')
        {
            // That's a level!
            level_t* level = new level_t();
            level->ep = dir_entry.name[1] - '0';
            level->lvl = dir_entry.name[3] - '0';
            if (level->ep < 1 || level->ep > 3 || level->lvl < 1 || level->lvl > 9)
            {
                // ok.. not a level
                delete level;
                continue;
            }

            strncpy(level->name, dir_entry.name, 8);

            ++i;
            for (; i < len; ++i)
            {
                const auto &dir_entry = directory[i];
                try_load_lump("THINGS", f, dir_entry, level->things);
                try_load_lump("LINEDEFS", f, dir_entry, level->linedefs);
                try_load_lump("SIDEDEFS", f, dir_entry, level->sidedefs);
                try_load_lump("VERTEXES", f, dir_entry, level->vertexes);
                try_load_lump("SECTORS", f, dir_entry, level->sectors);
                if (strncmp(dir_entry.name, "BLOCKMAP", 8) == 0)
                {
                    break;
                }
            }

            levels.push_back(level);
        }
    }

    // close file
    fclose(f);


    int armor_count = 0;
    int megaarmor_count = 0;
    int berserk_count = 0;
    int invulnerability_count = 0;
    int partial_invisibility_count = 0;
    int supercharge_count = 0;

    for (auto level : levels)
    {
        std::string lvl_prefix = level_names[level->ep - 1][level->lvl - 1] + std::string(" - "); //"E" + std::to_string(level->ep) + "M" + std::to_string(level->lvl) + " ";
        int i = 0;

        for (const auto& thing : level->things)
        {
            if (thing.flags & 0x0010) continue; // Thing is not in single player
            switch (thing.type)
            {
                // Uniques
                case 5: add_unique(lvl_prefix + "Blue keycard", thing, level, i, true, PROGRESSION, "Keys"); break;
                case 40: add_unique(lvl_prefix + "Blue skull key", thing, level, i, true, PROGRESSION, "Keys"); break;
                case 13: add_unique(lvl_prefix + "Red keycard", thing, level, i, true, PROGRESSION, "Keys"); break;
                case 38: add_unique(lvl_prefix + "Red skull key", thing, level, i, true, PROGRESSION, "Keys"); break;
                case 6: add_unique(lvl_prefix + "Yellow keycard", thing, level, i, true, PROGRESSION, "Keys"); break;
                case 39: add_unique(lvl_prefix + "Yellow skull key", thing, level, i, true, PROGRESSION, "Keys"); break;
                case 2026: add_unique(lvl_prefix + "Map", thing, level, i, false, USEFUL, ""); break;

                // Locations
                case 2018: add_loc(lvl_prefix + "Armor", thing, level, i); ++armor_count; break;
                case 8: add_loc(lvl_prefix + "Backpack", thing, level, i); break;
                case 2019: add_loc(lvl_prefix + "Mega Armor", thing, level, i); ++megaarmor_count; break;
                case 2023: add_loc(lvl_prefix + "Berserk", thing, level, i); ++berserk_count; break;
                case 2022: add_loc(lvl_prefix + "Invulnerability", thing, level, i); ++invulnerability_count; break;
                case 2024: add_loc(lvl_prefix + "Partial invisibility", thing, level, i); ++partial_invisibility_count; break;
                case 2013: add_loc(lvl_prefix + "Supercharge", thing, level, i); ++supercharge_count; break;
                case 2006: add_loc(lvl_prefix + "BFG9000", thing, level, i); break;
                case 2002: add_loc(lvl_prefix + "Chaingun", thing, level, i); break;
                case 2005: add_loc(lvl_prefix + "Chainsaw", thing, level, i); break;
                case 2004: add_loc(lvl_prefix + "Plasma gun", thing, level, i); break;
                case 2003: add_loc(lvl_prefix + "Rocket launcher", thing, level, i); break;
                case 2001: add_loc(lvl_prefix + "Shotgun", thing, level, i); break;
            }
            ++i;
        }
    }

    add_item("Armor", 2018, armor_count, FILLER, "Powerups");
    add_item("Mega Armor", 2019, megaarmor_count, FILLER, "Powerups");
    add_item("Berserk", 2023, berserk_count, FILLER, "Powerups");
    add_item("Invulnerability", 2022, invulnerability_count, FILLER, "Powerups");
    add_item("Partial invisibility", 2024, partial_invisibility_count, FILLER, "Powerups");
    add_item("Supercharge", 2013, supercharge_count, FILLER, "Powerups");

    // Make backpack progression item (Idea, gives more than one, with less increase each time)
    add_item("Backpack", 8, 1, PROGRESSION, "");

    // Guns. Fixed count. More chance to receive lower tier. Receiving twice the same weapon, gives you ammo
    add_item("Shotgun", 2001, 3, PROGRESSION, "Weapons");
    add_item("Shotgun", 2001, 7, USEFUL, "Weapons");
    add_item("Rocket launcher", 2003, 1, PROGRESSION, "Weapons");
    add_item("Rocket launcher", 2003, 2, USEFUL, "Weapons");
    add_item("Plasma gun", 2004, 2, USEFUL, "Weapons");
    add_item("Chainsaw", 2005, 5, USEFUL, "Weapons");
    add_item("Chaingun", 2002, 2, PROGRESSION, "Weapons");
    add_item("Chaingun", 2002, 2, USEFUL, "Weapons");
    add_item("BFG9000", 2006, 1, USEFUL, "Weapons");

    // Junk items
    add_item("Medikit", 2012, 30, FILLER, "");
    add_item("Box of bullets", 2048, 20, FILLER, "Ammos");
    add_item("Box of rockets", 2046, 20, FILLER, "Ammos");
    add_item("Box of shotgun shells", 2049, 20, FILLER, "Ammos");
    add_item("Energy cell pack", 17, 10, FILLER, "Ammos");

    printf("%i locations\n%i items\n", (int)ap_locations.size(), total_item_count);

    //--- Generate the python files

    // Items
    {
        FILE* fout = fopen((out_dir + "Items.py").c_str(), "w");
        fprintf(fout, "# This file is auto generated. More info: https://github.com/Daivuk/apdoom\n\n");
        fprintf(fout, "from BaseClasses import ItemClassification\n\
from typing import TypedDict, Dict, Set \n\
\n\
\n\
class ItemDict(TypedDict, total=False): \n\
    classification: ItemClassification \n\
    count: int \n\
    name: str \n\
    doom_type: int # Unique numerical id used to spawn the item. \n\
    episode: int # Relevant if that item targets a specific level, like keycard or map reveal pickup. \n\
    map: int \n\
\n\
\n\
");

        fprintf(fout, "item_table: Dict[int, ItemDict] = {\n");
        for (const auto& item : ap_items)
        {
            fprintf(fout, "    %llu: {", item.id);
            switch (item.classification)
            {
                case FILLER: fprintf(fout, "'classification': ItemClassification.filler"); break;
                case PROGRESSION: fprintf(fout, "'classification': ItemClassification.progression"); break;
                case USEFUL: fprintf(fout, "'classification': ItemClassification.useful"); break;
                case TRAP: fprintf(fout, "'classification': ItemClassification.trap"); break;
                case SKIP_BALANCING: fprintf(fout, "'classification': ItemClassification.skip_balancing"); break;
                case PROGRESSION_SKIP_BALANCING: fprintf(fout, "'classification': ItemClassification.progression_skip_balancing"); break;
            }
            fprintf(fout, ",\n             'count': %i", item.count);
            fprintf(fout, ",\n             'name': '%s'", item.name.c_str());
            fprintf(fout, ",\n             'doom_type': %i", item.doom_type);
            fprintf(fout, ",\n             'episode': %i", item.ep);
            fprintf(fout, ",\n             'map': %i", item.lvl);
            fprintf(fout, "},\n");
        }
        fprintf(fout, "}\n\n\n");

        // item_name_groups
        fprintf(fout, "item_name_groups: Dict[str, Set[str]] = {\n");
        for (const auto& kv : item_name_groups)
        {
            fprintf(fout, "    '%s': {", kv.first.c_str());
            for (const auto& item_name : kv.second)
            {
                fprintf(fout, "'%s', ", item_name.c_str());
            }
            fprintf(fout, "},\n");
        }
        fprintf(fout, "}\n");

        fclose(fout);
    }
    
    // Locations
    {
        FILE* fout = fopen((out_dir + "Locations.py").c_str(), "w");

        fprintf(fout, "# This file is auto generated. More info: https://github.com/Daivuk/apdoom\n\n");
        fprintf(fout, "from typing import Dict, TypedDict, List, Set \n\
\n\
\n\
class LocationDict(TypedDict, total=False): \n\
    name: str \n\
    episode: int \n\
    map: int \n\
    index: int # Thing index as it is stored in the wad file. \n\
    doom_type: int # In case index end up unreliable, we can use doom type. Maps have often only one of each important things. \n\
    required_items: list \n\
\n\
\n\
");

        fprintf(fout, "location_table: Dict[int, LocationDict] = {\n");
        for (const auto& loc : ap_locations)
        {
            fprintf(fout, "    %llu: {", loc.id);
            fprintf(fout, "'name': '%s'", loc.name.c_str());
            fprintf(fout, ",\n             'episode': %i", loc.ep);
            fprintf(fout, ",\n             'map': %i", loc.lvl);
            fprintf(fout, ",\n             'index': %i", loc.doom_thing_index);
            fprintf(fout, ",\n             'doom_type': %i", loc.doom_type);
            fprintf(fout, ",\n             'required_items': [");
            for (auto item_id : loc.required_items)
            {
                fprintf(fout, "%llu,", item_id);
            }
            fprintf(fout, "]");
            fprintf(fout, "},\n");
        }
        fprintf(fout, "}\n\n\n");

        fprintf(fout, "location_name_groups: Dict[str, Set[str]] = {\n");
        std::map<std::string, std::set<std::string>> location_name_groups;
        for (const auto& loc : ap_locations)
        {
            location_name_groups[level_names[loc.ep - 1][loc.lvl - 1]].insert(loc.name);
        }
        for (const auto& kv : location_name_groups)
        {
            fprintf(fout, "    '%s': {", kv.first.c_str());
            for (const auto& v : kv.second)
            {
                fprintf(fout, "\n        '%s',", v.c_str());
            }
            fprintf(fout, "\n    },\n");
        }
        fprintf(fout, "}\n");

        fclose(fout);
    }

    // Events
    {
        FILE* fout = fopen((out_dir + "Events.py").c_str(), "w");

        fprintf(fout, "# This file is auto generated. More info: https://github.com/Daivuk/apdoom\n\n");
        fprintf(fout, "from typing import List\n\n\n");

        fprintf(fout, "events: List[str] = [");
        for (auto level : levels)
        {
            fprintf(fout, "\n    '%s Complete',", level_names[level->ep - 1][level->lvl - 1]);
        }
        fprintf(fout, "\n]\n");

        fclose(fout);
    }

    // Clean up
    for (auto level : levels) delete level; // We don't need to
    return 0;
}
