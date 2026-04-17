#include "freertos/FreeRTOS.h"

#include "semphr.h"

#include "frequency_relay.h"

// sheds ON load with lowest priority 
static int shedNextLoad(void) {
    xSemaphoreTake(loadStatusMutex, portMAX_DELAY);

    int shedIndex = -1;
    for (int i = 0; i < NUM_LOADS; i++) {
        // find lowest priority ON load
        if (load_status[i] == LOAD_ON) {
            shedIndex = i;
            break;
        }
    }

    if (shedIndex >= 0) {
        load_status[shedIndex] = LOAD_SHED;
        updateLEDs();
    }
    xSemaphoreGive(loadStatusMutex);
    return shedIndex; // return index of shed load or -1 if no load to shed
}