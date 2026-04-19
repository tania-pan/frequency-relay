// task_load_management.c

#include "Freertos/FreeRTOS.h"
#include "altera_avalon_pio_regs.h"

#include "frequency_relay.h"
#include "types.h"

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
            handle_maintenance_toggle();
        }

        poll_switches();
        update_leds();
    }
}

