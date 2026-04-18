// load_logic.c

#include "Freertos/FreeRTOS.h"
#include "Freertos/semphr.h"
#include "Freertos/timers.h"

#include "altera_avalon_pio_regs.h"
#include "system.h"

#include "frequency_relay.h"

loadStatus_t load_status[NUM_LOADS] = {LOAD_OFF, LOAD_OFF, LOAD_OFF, LOAD_OFF, LOAD_OFF};
systemState_t system_state = SYSTEM_NORMAL;
TimerHandle_t timer_500ms = NULL;

static void timer_500ms_callback(TimerHandle_t timer_500ms);

void init_timer(void) {
    timer_500ms = xTimerCreate(
        "Timer500ms",       // name for debugging
        pdMS_TO_TICKS(500), // convert 500ms to ticks
        pdTRUE,             // auto-reload
        (void *)0,          // timer ID (not used)
        timer_500ms_callback  // callback function
    );

    if (timer_500ms == NULL) {
        // handle error
        printf("Failed to create 500ms timer\n");
    }
}

// sheds ON load with lowest priority 
static int shed_next_load(void) {
    xSemaphoreTake(load_status_mutex, portMAX_DELAY);

    int shed_index = -1;
    for (int i = 0; i < NUM_LOADS; i++) {
        // find lowest priority ON load
        if (load_status[i] == LOAD_ON) {
            shed_index = i;
            break;
        }
    }

    if (shed_index >= 0) {
        load_status[shed_index] = LOAD_SHED;
        update_leds();
    }
    xSemaphoreGive(load_status_mutex);
    return shed_index; // return index of shed load or -1 if no load to shed
}

// reconnects highest priority SHED load
static int reconnect_next_load(void) {
    int reconnect_index = -1;

    xSemaphoreTake(load_status_mutex, portMAX_DELAY);

    for (int i = NUM_LOADS - 1; i >= 0; i--) {
        // find highest priority SHED load
        if (load_status[i] == LOAD_SHED) {
            reconnect_index = i;
            break;
        }
    }

    if (reconnect_index >= 0) {
        // only reconnect if switch is still on
        uint32_t switch_state = IORD_ALTERA_AVALON_PIO_DATA(SLIDE_SWITCH_BASE);
        int switchOn = (switch_state >> reconnect_index) & 0x1;

        if (switchOn) {
            load_status[reconnect_index] = LOAD_ON;
            update_leds();
        } else {
            // if user flipped switch off while it was shed
            load_status[reconnect_index] = LOAD_OFF;
            update_leds();
        }
   
        xSemaphoreGive(load_status_mutex);
    }

    return reconnect_index; // return index of reconnected load or -1 if no load to reconnect
}

// check if all loads are reconnected
static int all_loads_reconnected(void) {
    int reconnected = 1;

    xSemaphoreTake(load_status_mutex, portMAX_DELAY);

    for (int i = 0; i < NUM_LOADS; i++) {
        if (load_status[i] == LOAD_SHED) {
            reconnected = 0;
            break;
        }
    }

    xSemaphoreGive(loadStatusMutex);
    return reconnected;
}

static void timer_500ms_callback(TimerHandle_t xTimer) {

    if (xSemaphoreTake(system_status_mutex, 0)) {
        systemState_t current_state = system_state;
        xSemaphoreGive(system_status_mutex);
        if (current_state != SYSTEM_MANAGING) {
            return;
        }

        if (network_unstable) {
            // frequency still bad, keep shedding
            shed_next_load();
        } else {
            // frequency good, try reconnecting loads
            int reconnectedIndex = reconnect_next_load();

            // if nothing left to reconnect, go back to normal mode
            if (reconnectedIndex < 0 || all_loads_reconnected()) {
                if (xSemaphoreTake(system_status_mutex, 0)) {
                    system_state = SYSTEM_NORMAL;
                    xSemaphoreGive(system_status_mutex);
                }
                xTimerStop(xTimer, 0);
            }
        }
        update_leds();
    }
}