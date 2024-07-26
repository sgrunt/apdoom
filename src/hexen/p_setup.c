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

#include <math.h>
#include <stdlib.h>
#include "h2def.h"
#include "i_system.h"
#include "m_argv.h"
#include "m_bbox.h"
#include "m_misc.h"
#include "i_swap.h"
#include "s_sound.h"
#include "p_local.h"

#include "aphexen_c_def.h"
#include "apdoom.h"

// MACROS ------------------------------------------------------------------

#define MAPINFO_SCRIPT_NAME "MAPINFO"
#define MCMD_SKY1 1
#define MCMD_SKY2 2
#define MCMD_LIGHTNING 3
#define MCMD_FADETABLE 4
#define MCMD_DOUBLESKY 5
#define MCMD_CLUSTER 6
#define MCMD_WARPTRANS 7
#define MCMD_NEXT 8
#define MCMD_CDTRACK 9
#define MCMD_CD_STARTTRACK 10
#define MCMD_CD_END1TRACK 11
#define MCMD_CD_END2TRACK 12
#define MCMD_CD_END3TRACK 13
#define MCMD_CD_INTERTRACK 14
#define MCMD_CD_TITLETRACK 15

#define UNKNOWN_MAP_NAME "DEVELOPMENT MAP"
#define DEFAULT_SKY_NAME "SKY1"
#define DEFAULT_SONG_LUMP "DEFSONG"
#define DEFAULT_FADE_TABLE "COLORMAP"

// TYPES -------------------------------------------------------------------

typedef struct mapInfo_s mapInfo_t;
struct mapInfo_s
{
    short cluster;
    short warpTrans;
    short nextMap;
    short cdTrack;
    char name[32];
    short sky1Texture;
    short sky2Texture;
    fixed_t sky1ScrollDelta;
    fixed_t sky2ScrollDelta;
    boolean doubleSky;
    boolean lightning;
    int fadetable;
    char songLump[10];
};

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

void P_SpawnMapThing(mapthing_t * mthing, int index);

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

static int QualifyMap(int map);

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

int MapCount;
mapthing_t deathmatchstarts[MAXDEATHMATCHSTARTS], *deathmatch_p;
mapthing_t playerstarts[MAX_PLAYER_STARTS][MAXPLAYERS];
int numvertexes;
vertex_t *vertexes;
int numsegs;
seg_t *segs;
int numsectors;
sector_t *sectors;
int numsubsectors;
subsector_t *subsectors;
int numnodes;
node_t *nodes;
int numlines;
line_t *lines;
int numsides;
side_t *sides;
short *blockmaplump;            // offsets in blockmap are from here
short *blockmap;
int bmapwidth, bmapheight;      // in mapblocks
fixed_t bmaporgx, bmaporgy;     // origin of block map
mobj_t **blocklinks;            // for thing chains
byte *rejectmatrix;             // for fast sight rejection

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static mapInfo_t MapInfo[99];
static const char *MapCmdNames[] = {
    "SKY1",
    "SKY2",
    "DOUBLESKY",
    "LIGHTNING",
    "FADETABLE",
    "CLUSTER",
    "WARPTRANS",
    "NEXT",
    "CDTRACK",
    "CD_START_TRACK",
    "CD_END1_TRACK",
    "CD_END2_TRACK",
    "CD_END3_TRACK",
    "CD_INTERMISSION_TRACK",
    "CD_TITLE_TRACK",
    NULL
};
static int MapCmdIDs[] = {
    MCMD_SKY1,
    MCMD_SKY2,
    MCMD_DOUBLESKY,
    MCMD_LIGHTNING,
    MCMD_FADETABLE,
    MCMD_CLUSTER,
    MCMD_WARPTRANS,
    MCMD_NEXT,
    MCMD_CDTRACK,
    MCMD_CD_STARTTRACK,
    MCMD_CD_END1TRACK,
    MCMD_CD_END2TRACK,
    MCMD_CD_END3TRACK,
    MCMD_CD_INTERTRACK,
    MCMD_CD_TITLETRACK
};

static int cd_NonLevelTracks[6];        // Non-level specific song cd track numbers 

// CODE --------------------------------------------------------------------

unsigned long long hash_seed(unsigned char *str)
{
    unsigned long long hash = 5381;
    int c;

    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

/*
=================
=
= P_LoadVertexes
=
=================
*/

void P_LoadVertexes(int lump)
{
    byte *data;
    int i;
    mapvertex_t *ml;
    vertex_t *li;

    numvertexes = W_LumpLength(lump) / sizeof(mapvertex_t);
    vertexes = Z_Malloc(numvertexes * sizeof(vertex_t), PU_LEVEL, 0);
    data = W_CacheLumpNum(lump, PU_STATIC);

    ml = (mapvertex_t *) data;
    li = vertexes;
    for (i = 0; i < numvertexes; i++, li++, ml++)
    {
        li->x = SHORT(ml->x) << FRACBITS;
        li->y = SHORT(ml->y) << FRACBITS;
    }

    W_ReleaseLumpNum(lump);
}


/*
=================
=
= P_LoadSegs
=
=================
*/

void P_LoadSegs(int lump)
{
    byte *data;
    int i;
    mapseg_t *ml;
    seg_t *li;
    line_t *ldef;
    int linedef, side;

    numsegs = W_LumpLength(lump) / sizeof(mapseg_t);
    segs = Z_Malloc(numsegs * sizeof(seg_t), PU_LEVEL, 0);
    memset(segs, 0, numsegs * sizeof(seg_t));
    data = W_CacheLumpNum(lump, PU_STATIC);

    ml = (mapseg_t *) data;
    li = segs;
    for (i = 0; i < numsegs; i++, li++, ml++)
    {
        li->v1 = &vertexes[SHORT(ml->v1)];
        li->v2 = &vertexes[SHORT(ml->v2)];

        li->angle = (SHORT(ml->angle)) << 16;
        li->offset = (SHORT(ml->offset)) << 16;
        linedef = SHORT(ml->linedef);
        ldef = &lines[linedef];
        li->linedef = ldef;
        side = SHORT(ml->side);
        li->sidedef = &sides[ldef->sidenum[side]];
        li->frontsector = sides[ldef->sidenum[side]].sector;
        if (ldef->flags & ML_TWOSIDED)
            li->backsector = sides[ldef->sidenum[side ^ 1]].sector;
        else
            li->backsector = 0;
    }

    W_ReleaseLumpNum(lump);
}

// [crispy] fix long wall wobble

static void P_SegLengths (void)
{
    int i;

    for (i = 0; i < numsegs; i++)
    {
        seg_t *const li = &segs[i];
        int64_t dx, dy;

        dx = li->v2->x - li->v1->x;
        dy = li->v2->y - li->v1->y;

        li->length = (uint32_t)(sqrt((double)dx*dx + (double)dy*dy)/2);
    }
}

/*
=================
=
= P_LoadSubsectors
=
=================
*/

void P_LoadSubsectors(int lump)
{
    byte *data;
    int i;
    mapsubsector_t *ms;
    subsector_t *ss;

    numsubsectors = W_LumpLength(lump) / sizeof(mapsubsector_t);
    subsectors = Z_Malloc(numsubsectors * sizeof(subsector_t), PU_LEVEL, 0);
    data = W_CacheLumpNum(lump, PU_STATIC);

    ms = (mapsubsector_t *) data;
    memset(subsectors, 0, numsubsectors * sizeof(subsector_t));
    ss = subsectors;
    for (i = 0; i < numsubsectors; i++, ss++, ms++)
    {
        ss->numlines = SHORT(ms->numsegs);
        ss->firstline = SHORT(ms->firstseg);
    }

    W_ReleaseLumpNum(lump);
}


/*
=================
=
= P_LoadSectors
=
=================
*/

void P_LoadSectors(int lump)
{
    byte *data;
    int i;
    mapsector_t *ms;
    sector_t *ss;

    numsectors = W_LumpLength(lump) / sizeof(mapsector_t);
    sectors = Z_Malloc(numsectors * sizeof(sector_t), PU_LEVEL, 0);
    memset(sectors, 0, numsectors * sizeof(sector_t));
    data = W_CacheLumpNum(lump, PU_STATIC);

    ms = (mapsector_t *) data;
    ss = sectors;

    for (i = 0; i < numsectors; i++, ss++, ms++)
    {
        ss->floorheight = SHORT(ms->floorheight) << FRACBITS;
        ss->ceilingheight = SHORT(ms->ceilingheight) << FRACBITS;
        ss->floorpic = R_FlatNumForName(ms->floorpic);
        ss->ceilingpic = R_FlatNumForName(ms->ceilingpic);
        ss->lightlevel = SHORT(ms->lightlevel);
        ss->special = SHORT(ms->special);
        ss->tag = SHORT(ms->tag);
        ss->thinglist = NULL;
        ss->seqType = SEQTYPE_STONE;    // default seqType

        // [crispy] WiggleFix: [kb] for R_FixWiggle()
        ss->cachedheight = 0;

        // [AM] Sector interpolation.  Even if we're
        //      not running uncapped, the renderer still
        //      uses this data.
        ss->oldfloorheight = ss->floorheight;
        ss->interpfloorheight = ss->floorheight;
        ss->oldceilingheight = ss->ceilingheight;
        ss->interpceilingheight = ss->ceilingheight;
        // [crispy] inhibit sector interpolation during the 0th gametic
        ss->oldgametic = -1;
    }
    W_ReleaseLumpNum(lump);
}


/*
=================
=
= P_LoadNodes
=
=================
*/

void P_LoadNodes(int lump)
{
    byte *data;
    int i, j, k;
    mapnode_t *mn;
    node_t *no;

    numnodes = W_LumpLength(lump) / sizeof(mapnode_t);
    nodes = Z_Malloc(numnodes * sizeof(node_t), PU_LEVEL, 0);
    data = W_CacheLumpNum(lump, PU_STATIC);

    mn = (mapnode_t *) data;
    no = nodes;
    for (i = 0; i < numnodes; i++, no++, mn++)
    {
        no->x = SHORT(mn->x) << FRACBITS;
        no->y = SHORT(mn->y) << FRACBITS;
        no->dx = SHORT(mn->dx) << FRACBITS;
        no->dy = SHORT(mn->dy) << FRACBITS;
        for (j = 0; j < 2; j++)
        {
            no->children[j] = SHORT(mn->children[j]);
            for (k = 0; k < 4; k++)
                no->bbox[j][k] = SHORT(mn->bbox[j][k]) << FRACBITS;
        }
    }
    W_ReleaseLumpNum(lump);
}

typedef enum
{
    rmc_melee,
    rmc_ranged,
    rmc_flying,
    rmc_boss,

    NUM_RMC
} random_monster_cat_t;


typedef struct
{
    int doom_type;
    int hit_points;
    int radius;
    int height;
    int frequency[5]; // Per-episode counts.. This is used for random balanced
    random_monster_cat_t category;
    int dont_shuffle;
} random_monster_def_t;


static random_monster_def_t random_monster_defs[] =
{
    { 107, 200, 20*FRACUNIT, 64*FRACUNIT, {36, 67, 59, 56, 126}, rmc_melee }, // Centaur
    { 121, 90, 32*FRACUNIT, 70*FRACUNIT, {9, 27, 1, 19, 4}, rmc_melee }, // Stalker
    { 10030, 175, 25*FRACUNIT, 68*FRACUNIT, {200, 194, 177, 166, 74}, rmc_melee }, // Ettin

    { 115, 250, 20*FRACUNIT, 64*FRACUNIT, {0, 1, 42, 35, 28}, rmc_ranged }, // Slaughtaur
    { 31, 250, 32*FRACUNIT, 64*FRACUNIT, {46, 30, 62, 45, 53}, rmc_ranged }, // Green Chaos Serpent
    { 8080, 250, 32*FRACUNIT, 64*FRACUNIT, {14, 59, 0, 8, 42}, rmc_ranged }, // Brown Chaos Serpent
    { 120, 90, 32*FRACUNIT, 70*FRACUNIT, {0, 7, 5, 10, 40}, rmc_ranged }, // Stalker Boss
    { 8020, 120, 22*FRACUNIT, 75*FRACUNIT, {34, 0, 0, 0, 7}, rmc_ranged }, // Wendigo

    { 34, 150, 20*FRACUNIT, 55*FRACUNIT, {0, 0, 0, 0, 30}, rmc_flying }, // Reiver
    { 10011, 150, 20*FRACUNIT, 55*FRACUNIT, {0, 0, 0, 0, 60}, rmc_flying, 1 }, // Reiver (buried)
    { 114, 150, 22*FRACUNIT, 65*FRACUNIT, {0, 0, 226, 79, 144}, rmc_flying }, // Dark Bishop
    { 10060, 150, 20*FRACUNIT, 68*FRACUNIT, {128, 157, 90, 81, 40}, rmc_flying }, // Afrit

    { 254, 640, 20*FRACUNIT, 65*FRACUNIT, {0, 1, 0, 0, 0}, rmc_boss, 1 }, // Death Wyvern
    { 10080, 5000, 40*FRACUNIT, 110*FRACUNIT, {0, 0, 1, 1, 0}, rmc_boss, 1 }, // Heresiarch
    { 10100, 800, 16*FRACUNIT, 64*FRACUNIT, {0, 0, 0, 0, 1}, rmc_boss, 1 }, // Zedek
    { 10101, 800, 16*FRACUNIT, 64*FRACUNIT, {0, 0, 0, 0, 1}, rmc_boss, 1 }, // Traductus
    { 10102, 800, 16*FRACUNIT, 64*FRACUNIT, {0, 0, 0, 0, 1}, rmc_boss, 1 }, // Melenkir
    { 10200, 5000, 65*FRACUNIT, 115*FRACUNIT, {0, 0, 0, 0, 1}, rmc_boss, 1 } // Korax
};
static const int monster_def_count = sizeof(random_monster_defs) / sizeof(random_monster_def_t);


typedef struct
{
    int index;
    random_monster_def_t* og_monster;
    random_monster_def_t* monster;
    fixed_t fit_radius;
    fixed_t fit_height;
} monster_spawn_def_t;

extern fixed_t tmbbox[4];
extern int tmflags;
extern fixed_t tmx;
extern fixed_t tmy;
extern fixed_t tmdropoffz;
fixed_t tmradius;
int tmtype;

boolean PIT_CheckLine_NoFlags(line_t * ld)
{
    if (tmbbox[BOXRIGHT] <= ld->bbox[BOXLEFT]
        || tmbbox[BOXLEFT] >= ld->bbox[BOXRIGHT]
        || tmbbox[BOXTOP] <= ld->bbox[BOXBOTTOM]
        || tmbbox[BOXBOTTOM] >= ld->bbox[BOXTOP])
    {
        return (true);
    }
    if (P_BoxOnLineSide(tmbbox, ld) != -1)
    {
        return (true);
    }

// a line has been hit
/*
=
= The moving thing's destination position will cross the given line.
= If this should not be allowed, return false.
= If the line is special, keep track of it to process later if the move
=       is proven ok.  NOTE: specials are NOT sorted by order, so two special lines
=       that are only 8 pixels apart could be crossed in either order.
*/

    if (!ld->backsector)
    {                           // One sided line
        return (false);
    }
    if (ld->flags & ML_BLOCKING)
    {                       // Explicitly blocking everything
        return (false);
    }
    if (ld->flags & ML_BLOCKMONSTERS)
    {                       // Block monsters only
        return (false);
    }

    // Check if the back sector can step up/down
    float floor = ld->frontsector->floorheight;
    float back_floor = ld->backsector->floorheight;
    
    if (tmradius <= 20 || abs(floor - back_floor) <= 20 * FRACUNIT) {
        P_LineOpening(ld);          // set openrange, opentop, openbottom
        // adjust floor / ceiling heights
        if (opentop < tmceilingz)
        {
            tmceilingz = opentop;
            ceilingline = ld;
        }
        if (openbottom > tmfloorz)
        {
            tmfloorz = openbottom;
        }
        if (lowfloor < tmdropoffz)
        {
            tmdropoffz = lowfloor;
        }
        return (true);
    }
    return false;
}

boolean P_BlockLinesIterator_NoPolyobj(int x, int y, boolean(*func) (line_t *))
{
    int offset;
    short *list;
    line_t *ld;

    int i;
    polyblock_t *polyLink;
    seg_t **tempSeg;

    if (x < 0 || y < 0 || x >= bmapwidth || y >= bmapheight)
        return true;

    offset = y * bmapwidth + x;
    offset = *(blockmap + offset);

    for (list = blockmaplump + offset; *list != -1; list++)
    {
        ld = &lines[*list];
        if (ld->validcount == validcount)
            continue;           // line has already been checked
        ld->validcount = validcount;

        if (!func(ld))
            return false;
    }

    return true;                // everything was checked
}


boolean check_position(fixed_t x, fixed_t y, fixed_t radius)
{
    int			xl;
    int			xh;
    int			yl;
    int			yh;
    int			bx;
    int			by;
    subsector_t*	newsubsec;
	
    tmx = x;
    tmy = y;
    tmradius = radius;
	
    tmbbox[BOXTOP] = y + radius;
    tmbbox[BOXBOTTOM] = y - radius;
    tmbbox[BOXRIGHT] = x + radius;
    tmbbox[BOXLEFT] = x - radius;

    newsubsec = R_PointInSubsector(x, y);
    validcount++;
    
    // check lines
    xl = (tmbbox[BOXLEFT] - bmaporgx)>>MAPBLOCKSHIFT;
    xh = (tmbbox[BOXRIGHT] - bmaporgx)>>MAPBLOCKSHIFT;
    yl = (tmbbox[BOXBOTTOM] - bmaporgy)>>MAPBLOCKSHIFT;
    yh = (tmbbox[BOXTOP] - bmaporgy)>>MAPBLOCKSHIFT;

    for (bx=xl ; bx<=xh ; bx++)
        for (by=yl ; by<=yh ; by++)
            if (!P_BlockLinesIterator_NoPolyobj(bx, by, PIT_CheckLine_NoFlags))
                return false;

    return true;
}


void get_fit_dimensions(fixed_t x, fixed_t y, fixed_t* fit_radius, fixed_t* fit_height)
{
    static const fixed_t radius_checks[] = {
        128*FRACUNIT,
        64*FRACUNIT,
        48*FRACUNIT,
        40*FRACUNIT,
        31*FRACUNIT,
        30*FRACUNIT,
        24*FRACUNIT,
        20*FRACUNIT,
        16*FRACUNIT
    };

    static const fixed_t height_checks[] = {
        110*FRACUNIT,
        100*FRACUNIT,
        64*FRACUNIT,
        56*FRACUNIT
    };

    subsector_t* ss = R_PointInSubsector(x, y);

    for (int i = 0, len = sizeof(height_checks) / sizeof(fixed_t); i < len; ++i)
    {
        fixed_t sector_height = ss->sector->ceilingheight - ss->sector->floorheight;
        if (sector_height >= height_checks[i] || i == len - 1)
        {
            *fit_height = height_checks[i];
            break;
        }
    }

    for (int i = 0, len = sizeof(radius_checks) / sizeof(fixed_t); i < len; ++i)
    {
        if (check_position(x, y, radius_checks[i]) || i == len - 1)
        {
            *fit_radius = radius_checks[i];
            break;
        }
    }
}

// These thing types are referenced in conditionals in their respective maps.
boolean dont_shuffle_for_map(int doom_type) {
    switch (gamemap) {
	case 11:
	    return doom_type == MT_DEMON || doom_type == MT_ETTIN;
	case 23:
	    return doom_type == MT_DEMON;
	case 25:
	    return doom_type == MT_DEMON || doom_type == MT_ETTIN;
	case 35:
	    return doom_type == MT_WRAITH;
	case 40:
	    return doom_type == MT_CENTAUR || doom_type == MT_ETTIN;
    }
    return false;
}

//==========================================================================
//
// P_LoadThings
//
//==========================================================================

void P_LoadThings(int lump)
{
    byte *data;
    int i, j;
    mapthing_t spawnthing;
    mapthing_t spawnthing_player1_start;
    mapthing_t *mt;
    int numthings;
    int bit;
    int playerCount;
    int deathSpotsCount;
    boolean foundplayer1;

    foundplayer1 = false;
    data = W_CacheLumpNum(lump, PU_STATIC);
    numthings = W_LumpLength(lump) / sizeof(mapthing_t);

    // Generate unique random seed from ap seed + level
    const char* ap_seed = apdoom_get_seed();
    unsigned long long seed = hash_seed(ap_seed);
    seed += gameepisode * 9 + gamemap;
    srand(seed);

    int things_type_remap[1024] = {0};

    mt = (mapthing_t *)data;
    for (i = 0; i < numthings; i++, mt++)
    {
        things_type_remap[i] = mt->type;
    }

    int do_random_monsters = ap_state.random_monsters;
    if (do_random_monsters > 0)
    {
        // Make sure at the right difficulty level
        if (gameskill == sk_baby)
            bit = 1;
        else if (gameskill == sk_nightmare)
            bit = 4;
        else
            bit = 1<<(gameskill-1);

        random_monster_def_t* monsters[1024] = {0};
        int monster_count = 0;
        monster_spawn_def_t spawns[1024] = {0};
        int spawn_count = 0;

        // Collect spawn points
        mt = (mapthing_t *)data;
        for (i = 0; i < numthings; i++, mt++)
        {
            if (!(mt->options & bit))
                continue;
            if (mt->tid)
	        continue;

            for (int j = 0; j < monster_def_count; ++j)
            {
                if (random_monster_defs[j].dont_shuffle)
                    continue;

		if (dont_shuffle_for_map(random_monster_defs[j].doom_type))
		    continue;

                if (random_monster_defs[j].doom_type == mt->type)
                {
                    tmtype = mt->type;
                    get_fit_dimensions(mt->x * FRACUNIT, mt->y * FRACUNIT, &spawns[spawn_count].fit_radius, &spawns[spawn_count].fit_height);
                    spawns[spawn_count].og_monster = &random_monster_defs[j];
                    spawns[spawn_count++].index = i;
                    break;
                }
            }
        }

        if (ap_state.random_monsters == 1) // Shuffle
        {
            // Collect monsters
            for (int i = 0; i < spawn_count; ++i)
            {
                monster_spawn_def_t* spawn = &spawns[i];
                monsters[monster_count++] = spawn->og_monster;
            }
        }
        else if (ap_state.random_monsters == 2) // Random balanced
        {
            int ratios[NUM_RMC] = {0};
            random_monster_def_t* defs_by_rmc[NUM_RMC][20];
            int defs_by_rmc_count[NUM_RMC] = {0};
            int rmc_ratios[NUM_RMC] = {0};
	    int hub = P_GetMapCluster(gamemap) - 1;
            for (int i = 0; i < monster_def_count; ++i)
            {
                random_monster_def_t* monster = &random_monster_defs[i];
                defs_by_rmc[monster->category][defs_by_rmc_count[monster->category]++] = monster;
                rmc_ratios[monster->category] += monster->frequency[hub];
            }

            int total = 0;
            for (int i = 0; i < spawn_count; ++i)
            {
                ratios[spawns[i].og_monster->category]++;
                total++;
            }

            while (monster_count < spawn_count)
            {
                int rnd = rand() % total;
                for (int i = 0; i < NUM_RMC; ++i)
                {
                    if (rnd < ratios[i])
                    {
                        rnd = rand() % rmc_ratios[i];
                        for (int j = 0; j < defs_by_rmc_count[i]; ++j)
                        {
                            if (rnd < defs_by_rmc[i][j]->frequency[hub])
                            {
                                monsters[monster_count++] = defs_by_rmc[i][j];
                                break;
                            }
                            rnd -= defs_by_rmc[i][j]->frequency[hub];
                        }
                        break;
                    }
                    rnd -= ratios[i];
                }
            }
        }
        else if (ap_state.random_monsters == 3) // Random chaotic
        {
            int total = 0;
	    int hub = P_GetMapCluster(gamemap) - 1;
            for (int i = 0; i < monster_def_count; ++i)
            {
                random_monster_def_t* monster = &random_monster_defs[i];
                if (monster->dont_shuffle) continue;
		if (dont_shuffle_for_map(monster->doom_type))
		    continue;
                total += monster->frequency[hub];
            }

            while (monster_count < spawn_count)
            {
                int rnd = rand() % total;
                for (int i = 0; i < monster_def_count; ++i)
                {
                    random_monster_def_t* monster = &random_monster_defs[i];
                    if (monster->dont_shuffle) continue;
		    if (dont_shuffle_for_map(monster->doom_type))
		        continue;
                    if (rnd < monster->frequency[hub])
                    {
                        monsters[monster_count++] = monster;
                        break;
                    }
                    rnd -= monster->frequency[hub];
                }
            }
        }

        // Randomly pick them until empty, and place them in different spots
        for (i = 0; i < spawn_count; i++)
        {
            int idx = rand() % monster_count;
            spawns[i].monster = monsters[idx];
            monsters[idx] = monsters[monster_count - 1];
            monster_count--;
        }

        // Go through again, and make sure they fit
        for (i = 0; i < spawn_count; i++)
        {
            monster_spawn_def_t* spawn1 = &spawns[i];
            if (spawn1->monster->height > spawn1->fit_height ||
                spawn1->monster->radius > spawn1->fit_radius)
            {
                // He doesn't fit here, find another monster randomly that would fit here, then swap
                int tries = 1000;
                while (tries--)
                {
                    int j = rand() % spawn_count;
                    if (j == i) continue;
                    monster_spawn_def_t* spawn2 = &spawns[j];
                    if (spawn1->monster->height <= spawn2->fit_height &&
                        spawn1->monster->radius <= spawn2->fit_radius &&
                        spawn2->monster->height <= spawn1->fit_height &&
                        spawn2->monster->radius <= spawn1->fit_radius)
                    {
                        random_monster_def_t* tmp = spawn1->monster;
                        spawn1->monster = spawn2->monster;
                        spawn2->monster = tmp;
                        break;
                    }
                }
            }
        }

        // Do the final remapping
        for (i = 0; i < spawn_count; i++)
        {
            monster_spawn_def_t* spawn = &spawns[i];
            things_type_remap[spawn->index] = spawn->monster->doom_type;
        }
    }

    if (ap_state.random_items > 0)
    {
        // Make sure at the right difficulty level
        if (gameskill == sk_baby)
            bit = 1;
        else if (gameskill == sk_nightmare)
            bit = 4;
        else
            bit = 1<<(gameskill-1);

        if (ap_state.random_items == 1) // Shuffle
        {
            int items[1024];
            int item_count = 0;
            int indices[1024];
            int index_count = 0;

            // Collect all items
            mt = (mapthing_t *)data;
            for (i = 0; i < numthings; i++, mt++)
            {
                if (mt->options & 16)
                    continue; // Multiplayer item
                if (!(mt->options & bit))
                    continue;

                switch (mt->type)
                {
                    case 122: // Blue Mana
                    case 124: // Green Mana
                    case 8004: // Combined Mana
                    case 81: // Crystal Vial
                    case 82: // Quartz Flask
                    case 10110: // Disc of Repulsion
		    case 8000: // Flechette
                    {
                        items[item_count++] = mt->type;
                        indices[index_count++] = i;
                        break;
                    }
                }
            }

            // Randomly pick them until empty, and place them in different spots
            mt = (mapthing_t *)data;
            for (i = 0; i < index_count; i++)
            {
                int idx = rand() % item_count;
                things_type_remap[indices[i]] = items[idx];
                items[idx] = items[item_count - 1];
                item_count--;
            }
        }
        else if (ap_state.random_items == 2) // Random balanced
        {
            int ratios[2] = {0, 0};
            int total = 0;

            // Make sure at the right difficulty level
            if (gameskill == sk_baby)
                bit = 1;
            else if (gameskill == sk_nightmare)
                bit = 4;
            else
                bit = 1<<(gameskill-1);

            // Calculate ratios
            mt = (mapthing_t *)data;
            for (i = 0; i < numthings; i++, mt++)
            {
                if (mt->options & 16)
                    continue; // Multiplayer item

                switch (mt->type)
                {
                    // Tiny
                    case 81: // Crystal Vial
                    case 122: // Blue Mana
                    case 124: // Green Mana
                        ratios[0]++;
                        total++;
                        break;

                    // Big
                    case 8004: // Combined Mana
                    case 82: // Quartz Flask
                    case 10110: // Disc of Repulsion
		    case 8000: // Flechette
                        ratios[1]++;
                        total++;
                        break;
                }
            }

            // Randomly pick items based on ratio
            mt = (mapthing_t *)data;
            for (i = 0; i < numthings; i++, mt++)
            {
                switch (mt->type)
                {
                    case 122: // Blue Mana
                    case 124: // Green Mana
                    case 8004: // Combined Mana
                    case 81: // Crystal Vial
                    case 82: // Quartz Flask
                    case 10110: // Disc of Repulsion
		    case 8000: // Flechette
                    {
                        int rnd = rand() % total;
                        if (rnd < ratios[0])
                        {
                            switch (rand()%3)
                            {
                                case 0: things_type_remap[i] = 81; break; // Crystal Vial
                                case 1: things_type_remap[i] = 122; break; // Blue Mana
                                case 2: things_type_remap[i] = 124; break; // Green Mana
                            }
                        }
                        else
                        {
                            switch (rand()%4)
                            {
                                case 0: things_type_remap[i] = 8004; break; // combined Mana
                                case 1: things_type_remap[i] = 82; break; // Quartz Flask
                                case 2: things_type_remap[i] = 10110; break; // Disc of Repulsion
                                case 3: things_type_remap[i] = 8000; break; // Flechette
                            }
                        }
                        break;
                    }
                }
            }
        }
    }

    mt = (mapthing_t *) data;
    for (i = 0; i < numthings; i++, mt++)
    {
        spawnthing.tid = SHORT(mt->tid);
        spawnthing.x = SHORT(mt->x);
        spawnthing.y = SHORT(mt->y);
        spawnthing.height = SHORT(mt->height);
        spawnthing.angle = SHORT(mt->angle);
        spawnthing.type = SHORT(things_type_remap[i]);
        spawnthing.options = SHORT(mt->options);

        spawnthing.special = mt->special;
        spawnthing.arg1 = mt->arg1;
        spawnthing.arg2 = mt->arg2;
        spawnthing.arg3 = mt->arg3;
        spawnthing.arg4 = mt->arg4;
        spawnthing.arg5 = mt->arg5;

        // Replace AP locations with AP item
        if (is_hexen_type_ap_location(spawnthing.type))
        {
            // Validate that the location index matches what we have in our data. If it doesn't then the WAD is not the same, we can't continue
            int ret = ap_validate_doom_location(ap_make_level_index(gameepisode, gamemap), spawnthing.type, i);
            if (ret == -1)
            {
                I_Error("WAD file doesn't match the one used to generate the logic.\nTo make sure it works as intended, get HEXEN.WAD from the steam releases.");
            }
            else if (ret == 0)
            {
                continue; // Skip it
            }
            else if (ret == 1)
            {
                if (apdoom_is_location_progression(ap_make_level_index(gameepisode, gamemap), i))
                    spawnthing.type = 20001;
                else
                    spawnthing.type = 20000;
                int skip = 0;
                ap_level_state_t* level_state = ap_get_level_state(ap_make_level_index(gameepisode, gamemap));
                for (j = 0; j < level_state->check_count; ++j)
                {
                    if (level_state->checks[j] == i)
                    {
                        skip = 1;
                        break;
                    }
                }
                if (skip)
                    continue;
            }
        }

        // [AP] On player start 1, put level select teleport "HUB"
        if (spawnthing.type == 1 && !foundplayer1) {
            spawnthing_player1_start = spawnthing;
	    foundplayer1 = true;
	}

        P_SpawnMapThing(&spawnthing, i);
    }

    // [AP] Spawn level select teleport "HUB"
    if (gamemap == 1 || gamemap == 2 || gamemap == 6 || gamemap == 13 || gamemap == 22 || gamemap == 27 || gamemap == 35 || gamemap == 40) {
        spawnthing_player1_start.type = 20002;
	// put the Bright Crucible hub tele in an accessible spot
	if (gamemap == 6) {
	    spawnthing_player1_start.x = 352;
	    spawnthing_player1_start.y = -1152;
	    spawnthing_player1_start.height = -344;
	}
        P_SpawnMapThing(&spawnthing_player1_start, i);
    }

    P_CreateTIDList();
    P_InitCreatureCorpseQueue(false);   // false = do NOT scan for corpses
    W_ReleaseLumpNum(lump);

    if (!deathmatch)
    {                           // Don't need to check deathmatch spots
        return;
    }
    playerCount = 0;
    for (i = 0; i < maxplayers; i++)
    {
        playerCount += playeringame[i];
    }
    deathSpotsCount = deathmatch_p - deathmatchstarts;
    if (deathSpotsCount < playerCount)
    {
        I_Error("P_LoadThings: Player count (%d) exceeds deathmatch "
                "spots (%d)", playerCount, deathSpotsCount);
    }
}

/*
=================
=
= P_LoadLineDefs
=
=================
*/

void P_LoadLineDefs(int lump)
{
    byte *data;
    int i;
    maplinedef_t *mld;
    line_t *ld;
    vertex_t *v1, *v2;

    numlines = W_LumpLength(lump) / sizeof(maplinedef_t);
    lines = Z_Malloc(numlines * sizeof(line_t), PU_LEVEL, 0);
    memset(lines, 0, numlines * sizeof(line_t));
    data = W_CacheLumpNum(lump, PU_STATIC);

    mld = (maplinedef_t *) data;
    ld = lines;
    for (i = 0; i < numlines; i++, mld++, ld++)
    {
        ld->flags = SHORT(mld->flags);

	// [ap] hack - make Winnowing Hall exit repeatable
	if (gamemap == 1 && i == 1550) {
	    ld->flags = ld->flags | ML_REPEAT_SPECIAL;
	}

        // Old line special info ...
        //ld->special = SHORT(mld->special);
        //ld->tag = SHORT(mld->tag);

        // New line special info ...
        ld->special = mld->special;
        ld->arg1 = mld->arg1;
        ld->arg2 = mld->arg2;
        ld->arg3 = mld->arg3;
        ld->arg4 = mld->arg4;
        ld->arg5 = mld->arg5;

        v1 = ld->v1 = &vertexes[SHORT(mld->v1)];
        v2 = ld->v2 = &vertexes[SHORT(mld->v2)];
        ld->dx = v2->x - v1->x;
        ld->dy = v2->y - v1->y;
        if (!ld->dx)
            ld->slopetype = ST_VERTICAL;
        else if (!ld->dy)
            ld->slopetype = ST_HORIZONTAL;
        else
        {
            if (FixedDiv(ld->dy, ld->dx) > 0)
                ld->slopetype = ST_POSITIVE;
            else
                ld->slopetype = ST_NEGATIVE;
        }

        if (v1->x < v2->x)
        {
            ld->bbox[BOXLEFT] = v1->x;
            ld->bbox[BOXRIGHT] = v2->x;
        }
        else
        {
            ld->bbox[BOXLEFT] = v2->x;
            ld->bbox[BOXRIGHT] = v1->x;
        }
        if (v1->y < v2->y)
        {
            ld->bbox[BOXBOTTOM] = v1->y;
            ld->bbox[BOXTOP] = v2->y;
        }
        else
        {
            ld->bbox[BOXBOTTOM] = v2->y;
            ld->bbox[BOXTOP] = v1->y;
        }
        ld->sidenum[0] = SHORT(mld->sidenum[0]);
        ld->sidenum[1] = SHORT(mld->sidenum[1]);
        if (ld->sidenum[0] != -1)
            ld->frontsector = sides[ld->sidenum[0]].sector;
        else
            ld->frontsector = 0;
        if (ld->sidenum[1] != -1)
            ld->backsector = sides[ld->sidenum[1]].sector;
        else
            ld->backsector = 0;

	ld->index = i;

	// [ap] Dark Crucible accessibility changes
	if (gamemap == 40) {
	    if (i >= 785 && i <= 791) { // front door - keep it open
	        ld->arg3 = 0;
	    }

	    if (i >= 512 && i <= 518) { // middle door - make it openable in case Korax dies early
	        ld->special = 11;
		ld->flags |= 0x1000; // push action
		ld->arg1 = 14;
		ld->arg2 = 16;
		ld->arg3 = 0;
	    }
	    if ((i == 143) || (i == 145)) { // first half side doors - make them openable
	        ld->special = 11;
		ld->arg1 = 10;
		ld->arg2 = 16;
		ld->arg3 = 0;
	    }
	    if ((i == 139) || (i == 149)) { // first half side doors - make them openable
	        ld->special = 11;
		ld->arg1 = 12;
		ld->arg2 = 16;
		ld->arg3 = 0;
	    }
	    if ((i == 138) || (i == 148)) { // second half side doors - make them openable
	        ld->special = 11;
		ld->arg1 = 13;
		ld->arg2 = 16;
		ld->arg3 = 0;
	    }
	    if ((i == 142) || (i == 144)) { // second half side doors - make them openable
	        ld->special = 11;
		ld->arg1 = 11;
		ld->arg2 = 16;
		ld->arg3 = 0;
	    }
	    if ((i == 138) || (i == 148)) { // second half side doors - make them openable
	        ld->special = 11;
		ld->arg1 = 13;
		ld->arg2 = 16;
		ld->arg3 = 0;
	    }
	}
    }

    W_ReleaseLumpNum(lump);
}


/*
=================
=
= P_LoadSideDefs
=
=================
*/

void P_LoadSideDefs(int lump)
{
    byte *data;
    int i;
    mapsidedef_t *msd;
    side_t *sd;

    numsides = W_LumpLength(lump) / sizeof(mapsidedef_t);
    sides = Z_Malloc(numsides * sizeof(side_t), PU_LEVEL, 0);
    memset(sides, 0, numsides * sizeof(side_t));
    data = W_CacheLumpNum(lump, PU_STATIC);

    msd = (mapsidedef_t *) data;
    sd = sides;

    for (i = 0; i < numsides; i++, msd++, sd++)
    {
        sd->textureoffset = SHORT(msd->textureoffset) << FRACBITS;
        sd->rowoffset = SHORT(msd->rowoffset) << FRACBITS;
        sd->toptexture = R_TextureNumForName(msd->toptexture);
        sd->bottomtexture = R_TextureNumForName(msd->bottomtexture);
        sd->midtexture = R_TextureNumForName(msd->midtexture);
        sd->sector = &sectors[SHORT(msd->sector)];
    }
    W_ReleaseLumpNum(lump);
}

/*
=================
=
= P_LoadBlockMap
=
=================
*/

void P_LoadBlockMap(int lump)
{
    int i, count;
    int lumplen;

    lumplen = W_LumpLength(lump);

    blockmaplump = Z_Malloc(lumplen, PU_LEVEL, NULL);
    W_ReadLump(lump, blockmaplump);
    blockmap = blockmaplump + 4;

    // Swap all short integers to native byte ordering:

    count = lumplen / 2;

    for (i = 0; i < count; i++)
        blockmaplump[i] = SHORT(blockmaplump[i]);

    bmaporgx = blockmaplump[0] << FRACBITS;
    bmaporgy = blockmaplump[1] << FRACBITS;
    bmapwidth = blockmaplump[2];
    bmapheight = blockmaplump[3];

    // clear out mobj chains

    count = sizeof(*blocklinks) * bmapwidth * bmapheight;
    blocklinks = Z_Malloc(count, PU_LEVEL, 0);
    memset(blocklinks, 0, count);
}




/*
=================
=
= P_GroupLines
=
= Builds sector line lists and subsector sector numbers
= Finds block bounding boxes for sectors
=================
*/

void P_GroupLines(void)
{
    line_t **linebuffer;
    int i, j, total;
    line_t *li;
    sector_t *sector;
    subsector_t *ss;
    seg_t *seg;
    fixed_t bbox[4];
    int block;

// look up sector number for each subsector
    ss = subsectors;
    for (i = 0; i < numsubsectors; i++, ss++)
    {
        seg = &segs[ss->firstline];
        ss->sector = seg->sidedef->sector;
    }

// count number of lines in each sector
    li = lines;
    total = 0;
    for (i = 0; i < numlines; i++, li++)
    {
        total++;
        li->frontsector->linecount++;
        if (li->backsector && li->backsector != li->frontsector)
        {
            li->backsector->linecount++;
            total++;
        }
    }

// build line tables for each sector
    linebuffer = Z_Malloc(total * sizeof(line_t *), PU_LEVEL, 0);
    sector = sectors;
    for (i = 0; i < numsectors; i++, sector++)
    {
        M_ClearBox(bbox);
        sector->lines = linebuffer;
        li = lines;
        for (j = 0; j < numlines; j++, li++)
        {
            if (li->frontsector == sector || li->backsector == sector)
            {
                *linebuffer++ = li;
                M_AddToBox(bbox, li->v1->x, li->v1->y);
                M_AddToBox(bbox, li->v2->x, li->v2->y);
            }
        }
        if (linebuffer - sector->lines != sector->linecount)
            I_Error("P_GroupLines: miscounted");

        // set the degenmobj_t to the middle of the bounding box
        sector->soundorg.x = (bbox[BOXRIGHT] + bbox[BOXLEFT]) / 2;
        sector->soundorg.y = (bbox[BOXTOP] + bbox[BOXBOTTOM]) / 2;

        // adjust bounding box to map blocks
        block = (bbox[BOXTOP] - bmaporgy + MAXRADIUS) >> MAPBLOCKSHIFT;
        block = block >= bmapheight ? bmapheight - 1 : block;
        sector->blockbox[BOXTOP] = block;

        block = (bbox[BOXBOTTOM] - bmaporgy - MAXRADIUS) >> MAPBLOCKSHIFT;
        block = block < 0 ? 0 : block;
        sector->blockbox[BOXBOTTOM] = block;

        block = (bbox[BOXRIGHT] - bmaporgx + MAXRADIUS) >> MAPBLOCKSHIFT;
        block = block >= bmapwidth ? bmapwidth - 1 : block;
        sector->blockbox[BOXRIGHT] = block;

        block = (bbox[BOXLEFT] - bmaporgx - MAXRADIUS) >> MAPBLOCKSHIFT;
        block = block < 0 ? 0 : block;
        sector->blockbox[BOXLEFT] = block;
    }

}

//=============================================================================


/*
=================
=
= P_SetupLevel
=
=================
*/

extern int leveltimesinceload;
void P_SetupLevel(int episode, int map, int playermask, skill_t skill)
{
    int i;
    int parm;
    char lumpname[9];
    int lumpnum;
    int acslumpnum;
    mobj_t *mobj;

    for (i = 0; i < maxplayers; i++)
    {
        players[i].killcount = players[i].secretcount
            = players[i].itemcount = 0;
    }
    players[consoleplayer].viewz = 1;   // will be set by player think

    // Waiting-for-level-load song; not played if playing music from CD
    // (the seek time will be so long it will just make loading take
    // longer)
    if (!cdmusic)
    {
        S_StartSongName("chess", true);
    }

    Z_FreeTags(PU_LEVEL, PU_PURGELEVEL - 1);

    P_InitThinkers();
    leveltime = 0;
    leveltimesinceload = 0;
    oldleveltime = 0;  // [crispy] Track if game is running

    M_snprintf(lumpname, sizeof(lumpname), "MAP%02d", map);
    lumpnum = W_GetNumForName(lumpname);
    //
    // Begin processing map lumps
    // Note: most of this ordering is important
    //
    P_LoadBlockMap(lumpnum + ML_BLOCKMAP);
    P_LoadVertexes(lumpnum + ML_VERTEXES);
    P_LoadSectors(lumpnum + ML_SECTORS);
    P_LoadSideDefs(lumpnum + ML_SIDEDEFS);
    P_LoadLineDefs(lumpnum + ML_LINEDEFS);
    P_LoadSubsectors(lumpnum + ML_SSECTORS);
    P_LoadNodes(lumpnum + ML_NODES);
    P_LoadSegs(lumpnum + ML_SEGS);
    rejectmatrix = W_CacheLumpNum(lumpnum + ML_REJECT, PU_LEVEL);
    P_GroupLines();

    // [crispy] fix long wall wobble
    P_SegLengths();

    bodyqueslot = 0;
    po_NumPolyobjs = 0;
    deathmatch_p = deathmatchstarts;
    P_LoadThings(lumpnum + ML_THINGS);
    PO_Init(lumpnum + ML_THINGS);       // Initialize the polyobjs
    // [ap] hack: load patched scripts to prevent logic softlocks
    M_snprintf(lumpname, sizeof(lumpname), "MAP%02dACS", map);
    acslumpnum = W_CheckNumForName(lumpname);
    if (acslumpnum != -1)
        P_LoadACScripts(acslumpnum);
    else
        P_LoadACScripts(lumpnum + ML_BEHAVIOR);     // ACS object code
    //
    // End of map lump processing
    //

    // If deathmatch, randomly spawn the active players
    TimerGame = 0;
    if (deathmatch)
    {
        for (i = 0; i < maxplayers; i++)
        {
            if (playeringame[i])
            {                   // must give a player spot before deathmatchspawn
                mobj = P_SpawnMobj(playerstarts[0][i].x << 16,
                                   playerstarts[0][i].y << 16, 0,
                                   MT_PLAYER_FIGHTER);
                players[i].mo = mobj;
                G_DeathMatchSpawnPlayer(i);
                P_RemoveMobj(mobj);
            }
        }

        //!
        // @arg <n>
        // @category net
        // @vanilla
        //
        // For multiplayer games: exit each level after n minutes.
        //

        parm = M_CheckParmWithArgs("-timer", 1);
        if (parm)
        {
            TimerGame = atoi(myargv[parm + 1]) * 35 * 60;
        }
    }

// set up world state
    P_SpawnSpecials();

// build subsector connect matrix
//      P_ConnectSubsectors ();

// Load colormap and set the fullbright flag
    i = P_GetMapFadeTable(gamemap);
    W_ReadLump(i, colormaps);
    if (i == W_GetNumForName("COLORMAP"))
    {
        LevelUseFullBright = true;
    }
    else
    {                           // Probably fog ... don't use fullbright sprites
        LevelUseFullBright = false;
    }

// preload graphics
    if (precache)
        R_PrecacheLevel();

    // Check if the level is a lightning level
    P_InitLightning();

    S_StopAllSound();
    SN_StopAllSequences();
    S_StartSong(gamemap, true);

//printf ("free memory: 0x%x\n", Z_FreeMemory());

}

//==========================================================================
//
// InitMapInfo
//
//==========================================================================

static void InitMapInfo(void)
{
    int map;
    int mapMax;
    int mcmdValue;
    mapInfo_t *info;
    char songMulch[10];
    const char *default_sky_name = DEFAULT_SKY_NAME;

    mapMax = 1;

    if (gamemode == shareware)
    {
	default_sky_name = "SKY2";
    }

    // Put defaults into MapInfo[0]
    info = MapInfo;
    info->cluster = 0;
    info->warpTrans = 0;
    info->nextMap = 1;          // Always go to map 1 if not specified
    info->cdTrack = 1;
    info->sky1Texture = R_TextureNumForName(default_sky_name);
    info->sky2Texture = info->sky1Texture;
    info->sky1ScrollDelta = 0;
    info->sky2ScrollDelta = 0;
    info->doubleSky = false;
    info->lightning = false;
    info->fadetable = W_GetNumForName(DEFAULT_FADE_TABLE);
    M_StringCopy(info->name, UNKNOWN_MAP_NAME, sizeof(info->name));

//    M_StringCopy(info->songLump, DEFAULT_SONG_LUMP, sizeof(info->songLump));
    SC_Open(MAPINFO_SCRIPT_NAME);
    while (SC_GetString())
    {
        if (SC_Compare("MAP") == false)
        {
            SC_ScriptError(NULL);
        }
        SC_MustGetNumber();
        if (sc_Number < 1 || sc_Number > 99)
        {                       // 
            SC_ScriptError(NULL);
        }
        map = sc_Number;

        info = &MapInfo[map];

        // Save song lump name
        M_StringCopy(songMulch, info->songLump, sizeof(songMulch));

        // Copy defaults to current map definition
        memcpy(info, &MapInfo[0], sizeof(*info));

        // Restore song lump name
        M_StringCopy(info->songLump, songMulch, sizeof(info->songLump));

        // The warp translation defaults to the map number
        info->warpTrans = map;

        // Map name must follow the number
        SC_MustGetString();
        M_StringCopy(info->name, sc_String, sizeof(info->name));

        // Process optional tokens
        while (SC_GetString())
        {
            if (SC_Compare("MAP"))
            {                   // Start next map definition
                SC_UnGet();
                break;
            }
            mcmdValue = MapCmdIDs[SC_MustMatchString(MapCmdNames)];
            switch (mcmdValue)
            {
                case MCMD_CLUSTER:
                    SC_MustGetNumber();
                    info->cluster = sc_Number;
                    break;
                case MCMD_WARPTRANS:
                    SC_MustGetNumber();
                    info->warpTrans = sc_Number;
                    break;
                case MCMD_NEXT:
                    SC_MustGetNumber();
                    info->nextMap = sc_Number;
                    break;
                case MCMD_CDTRACK:
                    SC_MustGetNumber();
                    info->cdTrack = sc_Number;
                    break;
                case MCMD_SKY1:
                    SC_MustGetString();
                    info->sky1Texture = R_TextureNumForName(sc_String);
                    SC_MustGetNumber();
                    info->sky1ScrollDelta = sc_Number << 8;
                    break;
                case MCMD_SKY2:
                    SC_MustGetString();
                    info->sky2Texture = R_TextureNumForName(sc_String);
                    SC_MustGetNumber();
                    info->sky2ScrollDelta = sc_Number << 8;
                    break;
                case MCMD_DOUBLESKY:
                    info->doubleSky = true;
                    break;
                case MCMD_LIGHTNING:
                    info->lightning = true;
                    break;
                case MCMD_FADETABLE:
                    SC_MustGetString();
                    info->fadetable = W_GetNumForName(sc_String);
                    break;
                case MCMD_CD_STARTTRACK:
                case MCMD_CD_END1TRACK:
                case MCMD_CD_END2TRACK:
                case MCMD_CD_END3TRACK:
                case MCMD_CD_INTERTRACK:
                case MCMD_CD_TITLETRACK:
                    SC_MustGetNumber();
                    cd_NonLevelTracks[mcmdValue - MCMD_CD_STARTTRACK] =
                        sc_Number;
                    break;
            }
        }
        mapMax = map > mapMax ? map : mapMax;
    }
    SC_Close();
    MapCount = mapMax;
}

//==========================================================================
//
// P_GetMapCluster
//
//==========================================================================

int P_GetMapCluster(int map)
{
    return MapInfo[QualifyMap(map)].cluster;
}

//==========================================================================
//
// P_GetMapCDTrack
//
//==========================================================================

int P_GetMapCDTrack(int map)
{
    return MapInfo[QualifyMap(map)].cdTrack;
}

//==========================================================================
//
// P_GetMapWarpTrans
//
//==========================================================================

int P_GetMapWarpTrans(int map)
{
    return MapInfo[QualifyMap(map)].warpTrans;
}

//==========================================================================
//
// P_GetMapNextMap
//
//==========================================================================

int P_GetMapNextMap(int map)
{
    return MapInfo[QualifyMap(map)].nextMap;
}

//==========================================================================
//
// P_TranslateMap
//
// Returns the actual map number given a warp map number.
//
//==========================================================================

int P_TranslateMap(int map)
{
    int i;

    for (i = 1; i < 99; i++)    // Make this a macro
    {
        if (MapInfo[i].warpTrans == map)
        {
            return i;
        }
    }
    // Not found
    return -1;
}

//==========================================================================
//
// P_GetMapSky1Texture
//
//==========================================================================

int P_GetMapSky1Texture(int map)
{
    return MapInfo[QualifyMap(map)].sky1Texture;
}

//==========================================================================
//
// P_GetMapSky2Texture
//
//==========================================================================

int P_GetMapSky2Texture(int map)
{
    return MapInfo[QualifyMap(map)].sky2Texture;
}

//==========================================================================
//
// P_GetMapName
//
//==========================================================================

char *P_GetMapName(int map)
{
    return MapInfo[QualifyMap(map)].name;
}

//==========================================================================
//
// P_GetMapSky1ScrollDelta
//
//==========================================================================

fixed_t P_GetMapSky1ScrollDelta(int map)
{
    return MapInfo[QualifyMap(map)].sky1ScrollDelta;
}

//==========================================================================
//
// P_GetMapSky2ScrollDelta
//
//==========================================================================

fixed_t P_GetMapSky2ScrollDelta(int map)
{
    return MapInfo[QualifyMap(map)].sky2ScrollDelta;
}

//==========================================================================
//
// P_GetMapDoubleSky
//
//==========================================================================

boolean P_GetMapDoubleSky(int map)
{
    return MapInfo[QualifyMap(map)].doubleSky;
}

//==========================================================================
//
// P_GetMapLightning
//
//==========================================================================

boolean P_GetMapLightning(int map)
{
    return MapInfo[QualifyMap(map)].lightning;
}

//==========================================================================
//
// P_GetMapFadeTable
//
//==========================================================================

boolean P_GetMapFadeTable(int map)
{
    return MapInfo[QualifyMap(map)].fadetable;
}

//==========================================================================
//
// P_GetMapSongLump
//
//==========================================================================

char *P_GetMapSongLump(int map)
{
    if (!strcasecmp(MapInfo[QualifyMap(map)].songLump, DEFAULT_SONG_LUMP))
    {
        return NULL;
    }
    else
    {
        return MapInfo[QualifyMap(map)].songLump;
    }
}

//==========================================================================
//
// P_PutMapSongLump
//
//==========================================================================

void P_PutMapSongLump(int map, char *lumpName)
{
    if (map < 1 || map > MapCount)
    {
        return;
    }
    M_StringCopy(MapInfo[map].songLump, lumpName,
                 sizeof(MapInfo[map].songLump));
}

//==========================================================================
//
// P_GetCDStartTrack
//
//==========================================================================

int P_GetCDStartTrack(void)
{
    return cd_NonLevelTracks[MCMD_CD_STARTTRACK - MCMD_CD_STARTTRACK];
}

//==========================================================================
//
// P_GetCDEnd1Track
//
//==========================================================================

int P_GetCDEnd1Track(void)
{
    return cd_NonLevelTracks[MCMD_CD_END1TRACK - MCMD_CD_STARTTRACK];
}

//==========================================================================
//
// P_GetCDEnd2Track
//
//==========================================================================

int P_GetCDEnd2Track(void)
{
    return cd_NonLevelTracks[MCMD_CD_END2TRACK - MCMD_CD_STARTTRACK];
}

//==========================================================================
//
// P_GetCDEnd3Track
//
//==========================================================================

int P_GetCDEnd3Track(void)
{
    return cd_NonLevelTracks[MCMD_CD_END3TRACK - MCMD_CD_STARTTRACK];
}

//==========================================================================
//
// P_GetCDIntermissionTrack
//
//==========================================================================

int P_GetCDIntermissionTrack(void)
{
    return cd_NonLevelTracks[MCMD_CD_INTERTRACK - MCMD_CD_STARTTRACK];
}

//==========================================================================
//
// P_GetCDTitleTrack
//
//==========================================================================

int P_GetCDTitleTrack(void)
{
    return cd_NonLevelTracks[MCMD_CD_TITLETRACK - MCMD_CD_STARTTRACK];
}

//==========================================================================
//
// QualifyMap
//
//==========================================================================

static int QualifyMap(int map)
{
    return (map < 1 || map > MapCount) ? 0 : map;
}

//==========================================================================
//
// P_Init
//
//==========================================================================

void P_Init(void)
{
    InitMapInfo();
    P_InitSwitchList();
    P_InitFTAnims();            // Init flat and texture animations
    P_InitTerrainTypes();
    P_InitLava();
    R_InitSprites(sprnames);
}


// Special early initializer needed to start sound before R_Init()
void InitMapMusicInfo(void)
{
    int i;

    for (i = 0; i < 99; i++)
    {
        M_StringCopy(MapInfo[i].songLump, DEFAULT_SONG_LUMP,
                     sizeof(MapInfo[i].songLump));
    }
    MapCount = 98;
}

/*
void My_Debug(void)
{
	int i;

	printf("My debug stuff ----------------------\n");
	printf("gamemap=%d\n",gamemap);
	for (i=0; i<10; i++)
	{
		printf("i=%d  songlump=%s\n",i,MapInfo[i].songLump);
	}
}
*/
