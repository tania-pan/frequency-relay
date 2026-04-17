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
        if (loadStatus[i] == LOAD_ON) {
            shedIndex = i;
            break;
        }
    }

    if (shedIndex >= 0) {
        loadStatus[shedIndex] = LOAD_SHED;
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
        if (loadStatus[i] == LOAD_SHED) {
            reconnectIndex = i;
            break;
        }
    }

    if (reconnectIndex >= 0) {
        // only reconnect if switch is still on
        uint32_t switchState = IORD_ALTERA_AVALON_PIO_DATA(SLIDE_SWITCH_BASE);
        int switchOn = (switchState >> reconnectIndex) & 0x1;

        if (switchOn) {
            loadStatus[reconnectIndex] = LOAD_ON;
            updateLEDs();
        } else {
            // if user flipped switch off while it was shed
            loadStatus[reconnectIndex] = LOAD_OFF;
            updateLEDs();
        }
   
        xSemaphoreGive(loadStatusMutex);
    }

    return reconnectIndex; // return index of reconnected load or -1 if no load to reconnect
}

// check if all loads are reconnected
static int allLoadsReconnected(void) {
    int reconnected = 1;

    xSemaphoreTake(loadStatusMutex, portMAX_DELAY);

    for (int i = 0; i < NUM_LOADS; i++) {
        if (loadStatus[i] == LOAD_SHED) {
            reconnected = 0;
            break;
        }
    }

    xSemaphoreGive(loadStatusMutex);
    return reconnected;
}