#include "common.h"
#include "fl_ecu.h"
#include "fr_ecu.h"
#include "rl_ecu.h"
#include "rr_ecu.h"

int main(void)
{
    // Inits
#ifdef ECU_FR
    fr_init();
#elif defined(ECU_FL)
    fl_init();
#elif defined(ECU_RR)
    rr_init();
#elif defined(ECU_RL)
    rl_init();
#elif defined(ECU_REAR)
    rear_init();
#else
    #error "No ECU defined!"
#endif

    // Main loop
    while(1)
    {
#ifdef ECU_FR
        fr_handler();
#elif defined(ECU_FL)
        fl_handler();
#elif defined(ECU_RR)
        rr_handler();
#elif defined(ECU_RL)
        rl_handler();
#elif defined(ECU_REAR)
        rear_handler();
#endif
    }
    
    return 0;
}
