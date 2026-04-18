// vga.c
// #include "frequency_relay.h"

// #include "vga.h"
// #include "types.h"

#include <stdio.h>

#include "../frequency_relay_bsp/system.h"
#include "../frequency_relay_bsp/drivers/inc/altera_up_avalon_video_character_buffer_with_dma.h"

#include "frequency_relay.h"
#include "vga.h"
#include "types.h"

void vga_display_task(void *pvParameters) {
    // grab char buffer device handle from pv params
    alt_up_char_buffer_dev *char_buffer = (alt_up_char_buffer_dev *)pvParameters;

    // ~60Hz (17ms) timing setup
    TickType_t last_wake_ticks = xTaskGetTickCount();
    const TickType_t freq_ticks = pdMS_TO_TICKS(17); // i DONT THINK FREQ IS ACCURATE

    freq_data_t local_freq_data;
    system_status_t local_system_status;
    int kbd_rx;
    int ignore_next_key = 0; // track releases
    char text_buffer[64];

    printf("VGA Task Started\n");
    fflush(stdout);

    while(1) {
        // -- process keyboard inputs --
        while (xQueueReceive(kbd_q, &kbd_rx, 0) == pdTRUE) {

            // check for if its a release code
            if (kbd_rx == PS2_BREAK) {
                ignore_next_key = 1;
                continue;
            }
            // break sequence means code is repeated after PS2_BREAK
            if (ignore_next_key) {
                ignore_next_key = 0;
                continue;
            }

            // -- lock system status to change struct members --
            if (xSemaphoreTake(system_status_mutex, 0) == pdTRUE) {
                switch (kbd_rx) {
                    case PS2_1:
                        system_status.threshold_edit_mode = TF;
                        break;
                    case PS2_2:
                        system_status.threshold_edit_mode = TROC;
                        break;
                    case PS2_UP:
                        if (system_status.threshold_edit_mode) {
                            system_status.TROC_threshold += 0.5;
                        } else {
                            system_status.TF_threshold += 0.5;
                        }
                        break;
                    case PS2_DOWN:
                        if (system_status.threshold_edit_mode) {
                            system_status.TROC_threshold -= 0.5;
                        } else {
                            system_status.TF_threshold -= 0.5;
                        }
                        break;
                }
                // -- unlock semaphore --
                xSemaphoreGive(system_status_mutex);
            }
        }

        // -- gather local copies of data for display --
        // peak (don't steal from load management)
            if (xQueuePeek(freq_data_q, &local_freq_data, 0) != pdTRUE) {
                local_freq_data.frequency = 0;
                local_freq_data.roc = 0;
            }

            // lock then grab
            if (xSemaphoreTake(system_status_mutex, portMAX_DELAY)) {
                local_system_status = system_status;
                xSemaphoreGive(system_status_mutex);
            }
            if (xSemaphoreTake(load_status_mutex, portMAX_DELAY)) {
                // TODO: local_load_status =
                xSemaphoreGive(load_status_mutex);
            }
            if (xSemaphoreTake(timing_log_mutex, portMAX_DELAY)) {
                // TODO: local_timing_log = timing_log;
                xSemaphoreGive(timing_log_mutex);
            }

            // -- render VGA elements --
            sprintf(text_buffer, "[%d] TF Threshold: %5.2f Hz    ", 
                (1 - system_status.threshold_edit_mode), local_system_status.TF_threshold);
            alt_up_char_buffer_string(char_buffer, text_buffer, 5, 5);
            
            sprintf(text_buffer, "[%d] TROC Threshold: %5.2f Hz/s    ", 
                system_status.threshold_edit_mode, local_system_status.TROC_threshold);
            alt_up_char_buffer_string(char_buffer, text_buffer, 5, 7);
            
            sprintf(text_buffer, "Current Frequency: %f Hz    ", local_freq_data.frequency);
            alt_up_char_buffer_string(char_buffer, text_buffer, 5, 9);

            sprintf(text_buffer, "Current RoC: %f Hz/s    ", local_freq_data.roc);
            alt_up_char_buffer_string(char_buffer, text_buffer, 5, 11);

            // TODO: add load and timing stats here

            vTaskDelayUntil(&last_wake_ticks, freq_ticks);
        }
    }
