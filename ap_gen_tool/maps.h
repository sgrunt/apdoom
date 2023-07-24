#pragma once

#include <cinttypes>
#include <vector>


#define EP_COUNT 4
#define MAP_COUNT 9

#define D2_MAP_COUNT 32


static const char* level_names[EP_COUNT][MAP_COUNT] = {
    {
        "Hangar (E1M1)",
        "Nuclear Plant (E1M2)",
        "Toxin Refinery (E1M3)",
        "Command Control (E1M4)",
        "Phobos Lab (E1M5)",
        "Central Processing (E1M6)",
        "Computer Station (E1M7)",
        "Phobos Anomaly (E1M8)",
        "Military Base (E1M9)"
    },
    {
        "Deimos Anomaly (E2M1)",
        "Containment Area (E2M2)",
        "Refinery (E2M3)",
        "Deimos Lab (E2M4)",
        "Command Center (E2M5)",
        "Halls of the Damned (E2M6)",
        "Spawning Vats (E2M7)",
        "Tower of Babel (E2M8)",
        "Fortress of Mystery (E2M9)"
    },
    {
        "Hell Keep (E3M1)",
        "Slough of Despair (E3M2)",
        "Pandemonium (E3M3)",
        "House of Pain (E3M4)",
        "Unholy Cathedral (E3M5)",
        "Mt. Erebus (E3M6)",
        "Limbo (E3M7)",
        "Dis (E3M8)",
        "Warrens (E3M9)"
    },
    {
        "Hell Beneath (E4M1)",
        "Perfect Hatred (E4M2)",
        "Sever the Wicked (E4M3)",
        "Unruly Evil (E4M4)",
        "They Will Repent (E4M5)",
        "Against Thee Wickedly (E4M6)",
        "And Hell Followed (E4M7)",
        "Unto the Cruel (E4M8)",
        "Fear (E4M9)"
    }
};


static const char* d2_level_names[D2_MAP_COUNT] = {
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


struct map_subsector_t
{
    uint16_t numsegs;
    uint16_t firstseg;
};


struct map_node_t
{
    // Partition line from (x,y) to x+dx,y+dy)
    int16_t x;
    int16_t y;
    int16_t dx;
    int16_t dy;

    // Bounding box for each child,
    // clip against view frustum.
    int16_t bbox[2][4];

    // If NF_SUBSECTOR its a subsector,
    // else it's a node of another subtree.
    uint16_t children[2];
};


struct map_seg_t
{
    uint16_t v1;
    uint16_t v2;
    int16_t angle;
    uint16_t linedef;
    int16_t side;
    int16_t offset;
};


struct seg_t
{
    int sidedef;
    int front_sector;
};


struct sector_t
{
    std::vector<int> vertices;
    std::vector<int> walls;
};


struct node_t
{
    int x;
    int y;
    int dx;
    int dy;
    int bbox[2][4];
    int children[2];
};


struct subsector_t
{
    int sector;
};


struct map_t
{
    std::vector<map_thing_t>        things;
    std::vector<map_linedefs_t>     linedefs;
    std::vector<map_sidedefs_t>     sidedefs;
    std::vector<map_vertex_t>       vertexes;
    std::vector<map_sectors_t>      map_sectors;
    std::vector<map_subsector_t>    map_subsectors;
    std::vector<map_node_t>         map_nodes;
    std::vector<map_seg_t>          map_segs;
    std::vector<seg_t>              segs;
    std::vector<subsector_t>        subsectors;
    std::vector<node_t>             nodes;
    std::vector<sector_t>           sectors;
    int16_t bb[4];
};


struct level_index_t
{
    int ep = 0;
    int map = 0;
    int d2_map = -1;

    bool operator==(const level_index_t& other) const
    {
        return ep == other.ep && map == other.map && d2_map == other.d2_map;
    }

    bool operator!() const
    {
        return ep < 0 && map < 0 && d2_map < 0;
    }
};


extern map_t maps[EP_COUNT][MAP_COUNT];
extern map_t d2_maps[D2_MAP_COUNT];


void init_maps();
int sector_at(int x, int y, map_t* map);
subsector_t* point_in_subsector(int x, int y, map_t* map);
const char* get_level_name(const level_index_t& idx);
map_t* get_map(const level_index_t& idx);
