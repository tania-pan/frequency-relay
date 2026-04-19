// task_load_management.c

#include "Freertos/FreeRTOS.h"
#include "altera_avalon_pio_regs.h"

#include "frequency_relay.h"
#include "task_load_management.h"

load_status_t load_status[NUM_LOADS] = {LOAD_OFF, LOAD_OFF, LOAD_OFF, LOAD_OFF, LOAD_OFF};
timing_log_t timing_log = { {0}, 0, 0, 0, 0 };
TimerHandle_t timer_500ms = NULL;
int network_unstable = 0;

void task_load_management(void *pvParameters) {

    freq_data_t rx_data; // received frequency data from queue
    int button_cmd;
    TickType_t violation_time = 0;
    int first_shed_done = 0;

    init_timer(); // initialise timer

    while (1) {
        // receive data from task 1 (if no data after 10ms, loop again to check for button inputs)
        if (xQueueReceive(freq_data_q, &rx_data, pdMS_TO_TICKS(10)) == pdTRUE) {

            xSemaphoreTake(system_status_mutex, portMAX_DELAY);
            system_state_t current_state = system_status.system_state;
            float TF = system_status.TF_threshold;
                float TROC = system_status.TROC_threshold;
            xSemaphoreGive(system_status_mutex);

            if (current_state != SYSTEM_MAINTENANCE) {

                float abs_roc = (rx_data.roc <0 ? -rx_data.roc : rx_data.roc);
                int unstable = (rx_data.frequency < TF) || (abs_roc > TROC);

                if (unstable && !network_unstable) {
                    // frequency just became unstable, start shedding loads
                    network_unstable = 1;
                    violation_time = rx_data.timestamp; // record time of violation
                    first_shed_done = 0; // reset shed flag for this violation

                    xSemaphoreTake(system_status_mutex, portMAX_DELAY);
                    system_status.system_state = SYSTEM_MANAGING;
                    xSemaphoreGive(system_status_mutex);

                    xTimerStart(timer_500ms, 0); // start 500ms recovery timer

                } else if (!unstable && network_unstable) {
                    network_unstable = 0;
                    xTimerReset(timer_500ms, 0); // reset timer
                }

                if (network_unstable && !first_shed_done) {
                    TickType_t current_time = xTaskGetTickCount();
                    if (shed_next_load() >= 0) {
                        first_shed_done = 1;
                        record_timing(current_time - violation_time); 
                    }
                }
            }
        }

        if (xQueueReceive(button_q, &button_cmd, 0) == pdTRUE) {
            if (!(button_cmd & 0x8)) {
                handle_maintenance_toggle();
            }
        }

        poll_switches();
        update_leds();
    }
}

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

// timer callback to manage load shedding and reconnecting based on frequency status
static void timer_500ms_callback(TimerHandle_t xTimer) {

    if (xSemaphoreTake(system_status_mutex, 0)) {
        xSemaphoreGive(system_status_mutex);
        if (system_status.system_state != SYSTEM_MANAGING) {
            return;
        }

        if (network_unstable) {
            // frequency still bad, keep shedding
            shed_next_load();
        } else {
            // frequency good, try reconnecting loads
            int reconnected_index = reconnect_next_load();

            // if nothing left to reconnect, go back to normal mode
            if (reconnected_index < 0 || all_loads_reconnected()) {
                if (xSemaphoreTake(system_status_mutex, 0)) {
                    system_status.system_state = SYSTEM_NORMAL;
                    xSemaphoreGive(system_status_mutex);
                }
                xTimerStop(xTimer, 0);
            }
        }
    }
}

// tracks how long it takes from a frequency violation to a load being shed
static void record_timing(TickType_t elapsed_ticks) {
    xSemaphoreTake(timing_log_mutex, portMAX_DELAY);
    // shift recent array
    for (int i = TIMING_LOG_SIZE - 1; i > 0; i--) {
        timing_log.recent[i] = timing_log.recent[i - 1];
    }
    timing_log.recent[0] = elapsed_ticks;

    // update min/max/avg
    timing_log.count++;
    if (timing_log.count == 1) {
        timing_log.min_time = elapsed_ticks;
        timing_log.max_time = elapsed_ticks;
        timing_log.avg_time = elapsed_ticks;
    } else {
        if (elapsed_ticks < timing_log.min_time) {
            timing_log.min_time = elapsed_ticks;
        }
        if (elapsed_ticks > timing_log.max_time) {
            timing_log.max_time = elapsed_ticks;
        }
        // incremental average calculation
        timing_log.avg_time = (timing_log.avg_time * (timing_log.count - 1) + elapsed_ticks) 
                             / timing_log.count;
    }

    xSemaphoreGive(timing_log_mutex);
}

// sheds lowest priority ON load
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
        int switch_on = (switch_state >> reconnect_index) & 0x1;

        if (switch_on) {
            load_status[reconnect_index] = LOAD_ON;
        } else {
            // if user flipped switch off while it was shed
            load_status[reconnect_index] = LOAD_OFF;
        }
    }

    xSemaphoreGive(load_status_mutex);

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

    xSemaphoreGive(load_status_mutex);
    return reconnected;
}

// handle maintenance mode toggle button
void handle_maintenance_toggle(void) {
    xSemaphoreTake(system_status_mutex, portMAX_DELAY);
    if (system_status.system_state == SYSTEM_MAINTENANCE) {
        system_status.system_state = SYSTEM_NORMAL;
    } else {
        system_status.system_state = SYSTEM_MAINTENANCE;
        xTimerStop(timer_500ms, 0);
        network_unstable = 0;

        // SHED loads return to OFF state in maintenance mode
        xSemaphoreTake(load_status_mutex, portMAX_DELAY);
        for (int i = 0; i < NUM_LOADS; i++) {
            if (load_status[i] == LOAD_SHED) {
                load_status[i] = LOAD_OFF; 
            }
        }
        xSemaphoreGive(load_status_mutex);
    }
    xSemaphoreGive(system_status_mutex);
}

static void update_leds(void) {

    volatile int *red_leds = (volatile int *) RED_LEDS_BASE;
    volatile int *green_leds = (volatile int *) GREEN_LEDS_BASE;

    int red_mask = 0;
    int green_mask = 0;

    // wait for mutex before reading shared load status
    xSemaphoreTake(load_status_mutex, portMAX_DELAY);

    for (int i = 0; i < NUM_LOADS; i++) {
        if (load_status[i] == LOAD_ON) {
            red_mask |= (1 << i); // Set bit for red LED
        } else {
            green_mask |= (1 << i); // Set bit for green LED
        }
        // LOAD_OFF
    }

    xSemaphoreGive(load_status_mutex);

    // Update the physical LEDs
    *red_leds = red_mask;
    *green_leds = green_mask;
}

// poll the slide switches and update load status accordingly
static void poll_switches(void) {

    // read switch states
    uint32_t switch_state = IORD_ALTERA_AVALON_PIO_DATA(SLIDE_SWITCH_BASE);

    xSemaphoreTake(load_status_mutex, portMAX_DELAY);

    system_state_t current_state;
    xSemaphoreTake(system_status_mutex, portMAX_DELAY);
    xSemaphoreGive(system_status_mutex);

    for (int i = 0; i < NUM_LOADS; i++) {
        int switch_on = (switch_state >> i) & 0x1; // check if switch i is on
        
        if (!switch_on) {
            // if switch is off, load must be off
            load_status[i] = LOAD_OFF;
        } else {
            // switch is on
            if (system_status.system_state == SYSTEM_MAINTENANCE) {
                // maintenance bypasses relay
                load_status[i] = LOAD_ON;
            } else if (load_status[i] == LOAD_OFF) {
                // user flips switch
                load_status[i] = LOAD_ON;
            }
        }
    }

    xSemaphoreGive(load_status_mutex);
}

