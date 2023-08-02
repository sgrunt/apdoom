#include "maps.h"

#include <onut/onut.h>
#include <onut/Dialogs.h>
#include <onut/Point.h>

#include "earcut.hpp"

#include <stdio.h>
#include <algorithm>

#include "data.h"


// For earcut to work
namespace mapbox {
namespace util {
template <>
struct nth<0, Point> {
    inline static auto get(const Point &t) {
        return t.x;
    };
};
template <>
struct nth<1, Point> {
    inline static auto get(const Point &t) {
        return t.y;
    };
};
} // namespace util
} // namespace mapbox


#define	NF_SUBSECTOR_VANILLA	0x8000
#define	NF_SUBSECTOR	0x80000000 // [crispy] extended nodes
#define	NO_INDEX	((unsigned short)-1) // [crispy] extended nodes


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


struct patch_header_t
{
    uint16_t width;
    uint16_t height;
    int16_t leftoffset;
    int16_t topoffset;
};


struct post_t
{
    uint8_t topdelta;
    uint8_t length;
    uint8_t unused;
};


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


int FixedMul(int a, int b)
{
    return ((int64_t) a * (int64_t) b) >> 16;
}


int point_on_side(int x, int y, node_t* node)
{
    int	dx;
    int	dy;
    int	left;
    int	right;
	
    if (!node->dx)
    {
        if (x <= node->x)
            return node->dy > 0;
	
        return node->dy < 0;
    }
    if (!node->dy)
    {
        if (y <= node->y)
            return node->dx < 0;
	
        return node->dx > 0;
    }
	
    dx = (x - node->x);
    dy = (y - node->y);
	
    // Try to quickly decide by looking at sign bits.
    if ( (node->dy ^ node->dx ^ dx ^ dy)&0x80000000 )
    {
        if  ( (node->dy ^ dx) & 0x80000000 )
        {
            // (left is negative)
            return 1;
        }
        return 0;
    }

    left = FixedMul(node->dy >> 16, dx);
    right = FixedMul(dy, node->dx >> 16);
	
    if (right < left)
    {
        // front side
        return 0;
    }

    // back side
    return 1;			
}


subsector_t* point_in_subsector(int x, int y, map_t* map)
{
    node_t*	node;
    int side;
    int nodenum;

    // single subsector is a special case
    if (map->nodes.empty())
        return map->subsectors.data();
		
    nodenum = (int)map->nodes.size() - 1;

    while (!(nodenum & NF_SUBSECTOR))
    {
        node = &map->nodes[nodenum];
        side = point_on_side(x, y, node);
        nodenum = node->children[side];
    }
	
    return &map->subsectors[nodenum & ~NF_SUBSECTOR];
}


struct wall_t
{
    int v1, v2;
    int sector;
};


static void create_wall(std::vector<wall_t>& map_walls, map_t* map, int16_t linedef_idx, int16_t sidedef_idx)
{
    const auto &linedef = map->linedefs[linedef_idx];
    const auto &sidedef = map->sidedefs[sidedef_idx];
    const auto &sectordef = map->sectors[sidedef.sector];

    auto &sector = map->sectors[sidedef.sector];

    wall_t wall;
    wall.v1 = linedef.start_vertex;
    wall.v2 = linedef.end_vertex;
    
    // When moving a sector we will need to refer to the sector behind to invalidate it
    if (linedef.front_sidedef != sidedef_idx)
    {
        // Invert direction of wall if we are behind the linedef
        std::swap(wall.v1, wall.v2);
    }

    wall.sector = sidedef.sector;

    sector.walls.push_back((int)map_walls.size());
    map_walls.push_back(wall);
}


static void triangulate_sector(const std::vector<wall_t>& map_walls, map_t* map, int sectori)
{
    auto& sector = map->sectors[sectori];

    std::vector<int> wall_idx_remaining(sector.walls.size());
    for (int i = 0, len = (int)sector.walls.size(); i < len; ++i)
        wall_idx_remaining[i] = i;

    // Build loops
    std::vector<std::vector<int>> loops;
    while (wall_idx_remaining.size() >= 3)
    {
        // Pick first line, and try to build a loop
        std::vector<int> loop = { wall_idx_remaining[0] };
        wall_idx_remaining.erase(wall_idx_remaining.begin());
        while (true)
        {
            auto previous = loop[(int)loop.size() - 1];
            bool found = false;
            for (int i = 0, len = (int)wall_idx_remaining.size(); i < len; ++i)
            {
                auto idx = wall_idx_remaining[i];
                const auto &wall = map_walls[sector.walls[idx]];
                if (map_walls[sector.walls[previous]].v1 == wall.v2)
                {
                    loop.push_back(idx);
                    wall_idx_remaining.erase(wall_idx_remaining.begin() + i);
                    found = true;
                    break;
                }
            }
            if (!found) break;
        }

        if (loop.size() >= 3)
        {
            loops.push_back(loop);
        }
    }

    // Calculate biggest loop, and put it in front. This will be our outside loop. This is what earcut expects.
    double biggest_area = 0;
    int biggest_loop = 0;
    for (int i = 0, len = (int)loops.size(); i < len; ++i)
    {
        const auto &loop = loops[i];
        auto first_wall = loop[0];
        const auto &vertex = map->vertexes[map_walls[sector.walls[first_wall]].v1];
        double mins[2] = { (double)vertex.x, (double)vertex.y };
        double maxs[2] = { (double)vertex.x, (double)vertex.y };
        for (auto wall_idx : loop)
        {
            const auto &vert = map->vertexes[map_walls[sector.walls[wall_idx]].v1];
            mins[0] = std::min(mins[0], (double)vert.x);
            mins[1] = std::min(mins[1], (double)vert.y);
            maxs[0] = std::max(maxs[0], (double)vert.x);
            maxs[1] = std::max(maxs[1], (double)vert.y);
        }
        auto area = (maxs[0] - mins[0]) * (maxs[1] - mins[1]);
        if (area > biggest_area)
        {
            biggest_area = area;
            biggest_loop = i;
        }
    }
    if (biggest_loop != 0)
        std::swap(loops[0], loops[biggest_loop]);

    // Construct points that earcut will understand
    std::vector<std::vector<Point>> point_loops(loops.size());
    for (int j = 0, len = (int)loops.size(); j < len; ++j)
    {
        const auto &loop = loops[j];
        auto &point_loop = point_loops[j];
        point_loop.resize(loop.size());
        for (int i = 0, len = (int)loop.size(); i < len; ++i)
        {
            const auto &vertex = map->vertexes[map_walls[sector.walls[loop[i]]].v1];
            auto &point = point_loop[i];
            point.x = (int)vertex.x;
            point.y = (int)vertex.y;
        }
    }

    // Triangulate
    auto indices = mapbox::earcut<int>(point_loops);

    // Flatten loop so we can know which indice match which loop
    std::vector<int> flatten_loops;
    for (int j = 0, len = (int)loops.size(); j < len; ++j)
    {
        const auto &loop = loops[j];
        flatten_loops.insert(flatten_loops.end(), loop.begin(), loop.end());
    }

    // Translate this into indices to vertexes from mapdef
    sector.vertices.resize(indices.size());
    for (int i = 0, len = (int)indices.size(); i < len; ++i)
    {
        sector.vertices[i] = map_walls[sector.walls[flatten_loops[indices[i]]]].v1;
    }
}


std::vector<uint8_t> load_lump(const std::vector<map_directory_t>& directory, const char* lump_name, FILE* f)
{
    for (const auto& dir_entry : directory)
    {
        if (strncmp(dir_entry.name, lump_name, 8) == 0)
        {
            std::vector<uint8_t> ret;
            ret.resize(dir_entry.size);
            fseek(f, dir_entry.offset, SEEK_SET);
            fread(ret.data(), 1, dir_entry.size, f);
            return ret;
        }
    }
    return {};
}


OTextureRef load_sprite(const std::vector<map_directory_t>& directory, const char* lump_name, FILE* f, const uint8_t* pal)
{
    auto raw_data = load_lump(directory, lump_name, f);
    if (raw_data.empty()) return nullptr;

    patch_header_t header;
    memcpy(&header, raw_data.data(), sizeof(patch_header_t));
    uint32_t* columnofs = new uint32_t[header.width * sizeof(uint32_t)];
    memcpy(columnofs, raw_data.data() + sizeof(patch_header_t), header.width * sizeof(uint32_t));

    std::vector<uint8_t> img_data;
    img_data.resize(header.width * header.height * 4);

    for (int x = 0; x < header.width; ++x)
    {
        int offset = columnofs[x];
        while (raw_data[offset] != 0xFF)
        {
            post_t post;
            memcpy(&post, &raw_data[offset], sizeof(post_t));
            offset += 3;
            for (int j = 0; j < post.length; ++j, ++offset)
            {
                int y = post.topdelta + j;
                int idx = raw_data[offset] * 3;
                int k = y * header.width * 4 + x * 4;
                img_data[k + 0] = pal[idx + 0];
                img_data[k + 1] = pal[idx + 1];
                img_data[k + 2] = pal[idx + 2];
                img_data[k + 3] = 255;
            }
            ++offset;
        }
    }

    delete[] columnofs;
    return OTexture::createFromData(img_data.data(), {header.width, header.height}, false);
}


void init_wad(const char* filename, game_t& game)
{
    // Load DOOM.WAD
    FILE* f = fopen(filename, "rb");
    if (!f)
    {
        onut::showMessageBox("Error", std::string("Cannot open file: ") + filename);
        exit(1); // Hard kill
    }
    
    // Read header
    map_header_t header;
    fread(&header, sizeof(header), 1, f);
    if (strncmp(header.identification, "PWAD", 4) != 0 && strncmp(header.identification, "IWAD", 4) != 0)
    {
        onut::showMessageBox("Error", std::string("Invalid IWAD or PWAD: ") + filename);
        exit(1); // Hard kill
    }
    
    // Read directory
    std::vector<map_directory_t> directory(header.num_lumps);
    fseek(f, header.directory_offset, SEEK_SET);
    fread(directory.data(), sizeof(map_directory_t), header.num_lumps, f);

    // loop directory and find levels, then load them all. YOLO
    for (int i = 0, len = (int)directory.size(); i < len; ++i)
    {
        const auto &dir_entry = directory[i];
        {
            map_t* map = nullptr;

            // DOOM 1 style
            if (game.episodic)
                if (strlen(dir_entry.name) == 4 && dir_entry.name[0] == 'E' && dir_entry.name[2] == 'M')
                {
                    // That's a level!
                    auto ep = dir_entry.name[1] - '0';
                    auto lvl = dir_entry.name[3] - '0';
                    if (!(ep < 1 || ep > game.ep_count || lvl < 1 || lvl > game.map_count))
                    {
                        map = &game.metas[(ep - 1) * game.map_count + (lvl - 1)].map;
                    }
                }

            // DOOM 2 style
            if (!game.episodic)
                if (strlen(dir_entry.name) == 5 && strncmp(dir_entry.name, "MAP", 3) == 0)
                {
                    auto lvl = std::atoi(dir_entry.name + 3) - 1;
                    map = &game.metas[lvl].map;
                }

            if (!map) continue;

            ++i;
            for (; i < len; ++i)
            {
                const auto &dir_entry = directory[i];
                try_load_lump("THINGS", f, dir_entry, map->things);
                try_load_lump("LINEDEFS", f, dir_entry, map->linedefs);
                try_load_lump("SIDEDEFS", f, dir_entry, map->sidedefs);
                try_load_lump("VERTEXES", f, dir_entry, map->vertexes);
                try_load_lump("SECTORS", f, dir_entry, map->map_sectors);
                try_load_lump("SSECTORS", f, dir_entry, map->map_subsectors);
                try_load_lump("NODES", f, dir_entry, map->map_nodes);
                try_load_lump("SEGS", f, dir_entry, map->map_segs);
                if (strncmp(dir_entry.name, "BLOCKMAP", 8) == 0)
                {
                    break;
                }
            }

            map->sectors.resize(map->map_sectors.size());
            map->subsectors.resize(map->map_subsectors.size());
            map->nodes.resize(map->map_nodes.size());
            for (int j = 0, lenj = (int)map->map_nodes.size(); j < lenj; ++j)
            {
                map->nodes[j].x = (int16_t)map->map_nodes[j].x << 16;
                map->nodes[j].y = (int16_t)map->map_nodes[j].y << 16;
                map->nodes[j].dx = (int16_t)map->map_nodes[j].dx << 16;
                map->nodes[j].dy = (int16_t)map->map_nodes[j].dy << 16;
                for (int jj = 0; jj < 2; ++jj)
                {
                    map->nodes[j].children[jj] = (uint16_t)(int16_t)map->map_nodes[j].children[jj];
                    if (map->nodes[j].children[jj] == NO_INDEX)
                        map->nodes[j].children[jj] = -1;
                    else if (map->nodes[j].children[jj] & NF_SUBSECTOR_VANILLA)
                    {
                        map->nodes[j].children[jj] &= ~NF_SUBSECTOR_VANILLA;
                        if (map->nodes[j].children[jj] >= (int)map->map_subsectors.size())
                            map->nodes[j].children[jj] = 0;
                        map->nodes[j].children[jj] |= NF_SUBSECTOR;
                    }
                    for (int k = 0; k < 4; ++k)
                        map->nodes[j].bbox[jj][k] = (int16_t)map->map_nodes[j].bbox[jj][k] << 16;
                }
            }

            map->segs.resize(map->map_segs.size());
            for (int j = 0, lenj = (int)map->map_segs.size(); j < lenj; ++j)
            {
                const auto& map_seg = map->map_segs[j];
                auto& seg = map->segs[j];
                int side = map_seg.side;
                seg.sidedef = (&(map->linedefs[map_seg.linedef].front_sidedef))[side];
                seg.front_sector = map->sidedefs[seg.sidedef].sector;
            }

            // Assign sector to subsector
            for (int j = 0, lenj = (int)map->map_subsectors.size(); j < lenj; ++j)
            {
                const auto& seg = map->segs[map->map_subsectors[j].firstseg];
                //const auto& map_sidedef = map->sidedefs[seg.sidedef];
                map->subsectors[j].sector = seg.front_sector;
            }

            map->bb[0] = map->vertexes[0].x;
            map->bb[1] = map->vertexes[0].y;
            map->bb[2] = map->vertexes[0].x;
            map->bb[3] = map->vertexes[0].y;
            for (int v = 1, vlen = (int)map->vertexes.size(); v < vlen; ++v)
            {
                map->bb[0] = std::min(map->bb[0], map->vertexes[v].x);
                map->bb[1] = std::min(map->bb[1], map->vertexes[v].y);
                map->bb[2] = std::max(map->bb[2], map->vertexes[v].x);
                map->bb[3] = std::max(map->bb[3], map->vertexes[v].y);
            }

            // Create "walls" used in triangulation step
            std::vector<wall_t> map_walls;
            for (int j = 0; j < (int)map->linedefs.size(); ++j)
            {
                const auto &linedef = map->linedefs[j];

                if (linedef.front_sidedef != -1)
                    create_wall(map_walls, map, j, linedef.front_sidedef);
                if (linedef.back_sidedef != -1)
                    create_wall(map_walls, map, j, linedef.back_sidedef);
            }

            // Triangulate
            for (int j = 0; j < (int)map->sectors.size(); ++j)
            {
                triangulate_sector(map_walls, map, j);
            }
        }
    }

    // Load palette
    auto pal = load_lump(directory, "PLAYPAL", f);

    // Load sprites for item requirements
    for (auto& item_requirement : game.item_requirements)
    {
        if (item_requirement.sprite != "")
        {
            item_requirement.icon = load_sprite(directory, item_requirement.sprite.c_str(), f, pal.data());
        }
    }

    // close file
    fclose(f);
}


void init_maps(game_t& game)
{
    init_wad(game.wad_name.c_str(), game);
}


int sector_at(int x, int y, map_t* map)
{
    x = (int)((int16_t)x << 16);
    y = (int)((int16_t)y << 16);

    auto subsector = point_in_subsector(x, y, map);
    return subsector->sector;
}
