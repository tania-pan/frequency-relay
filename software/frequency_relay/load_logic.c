#include "freertos/FreeRTOS.h"
#include "semphr.h"

#include "altera_avalon_pio_regs.h"
#include "system.h"

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

// reconnects highest priority SHED load
static int reconnectNextLoad(void) {
    int reconnectIndex = -1;

    xSemaphoreTake(loadStatusMutex, portMAX_DELAY);

    for (int i = NUM_LOADS - 1; i >= 0; i--) {
        // find highest priority SHED load
        if (load_status[i] == LOAD_SHED) {
            reconnectIndex = i;
            break;
        }
    }

    if (reconnectIndex >= 0) {
        // only reconnect if switch is still on
        uint32_t switchState = IORD_ALTERA_AVALON_PIO_DATA(SLIDE_SWITCH_BASE);
        int switchOn = (switchState >> reconnectIndex) & 0x1;

        if (switchOn) {
            load_status[reconnectIndex] = LOAD_ON;
            updateLEDs();
        } else {
            // if user flipped switch off while it was shed
            load_status[reconnectIndex] = LOAD_OFF;
            updateLEDs();
        }
   
        xSemaphoreGive(loadStatusMutex);
    }

    return reconnectIndex; // return index of reconnected load or -1 if no load to reconnect
}