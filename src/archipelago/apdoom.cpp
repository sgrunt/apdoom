#include "apdoom.h"
#include <memory.h>


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


void apdoom_init(void)
{
    memset(&ap_state, 0, sizeof(ap_state));
}
