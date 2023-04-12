#ifndef _APDOOM_
#define _APDOOM_


#ifdef __cplusplus
extern "C"
{
#endif

typedef struct
{
    int completed;
    int keys[3];

} ap_level_state_t;


typedef struct
{
    ap_level_state_t level_states[3][8];
    
} ap_state_t;


extern ap_state_t ap_state;


void apdoom_init(void);

#ifdef __cplusplus
}
#endif


#endif
