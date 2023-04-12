#include "apdoom.h"
#include <memory.h>


ap_state_t ap_state;


void apdoom_init(void)
{
    memset(&ap_state, 0, sizeof(ap_state));
}
