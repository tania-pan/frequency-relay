// vga.c

#include <stdio.h>

#include "../frequency_relay_bsp/system.h"
#include "../frequency_relay_bsp/drivers/inc/altera_up_avalon_video_character_buffer_with_dma.h"

#include "frequency_relay.h"
#include "task_load_management.h"
#include "vga.h"
#include "types.h"

void vga_display_task(void *pvParameters) {
    // grab char buffer device handle from pv params
    alt_up_char_buffer_dev *char_buffer = (alt_up_char_buffer_dev *)pvParameters;
    
    // ~60Hz (17ms) timing setup
    TickType_t last_wake_ticks = xTaskGetTickCount();
    const TickType_t freq_ticks = pdMS_TO_TICKS(17);

    // local copy variables
    freq_data_t local_freq_data;
    system_status_t local_system_status;
    load_status_t local_load_status[NUM_LOADS];
    timing_log_t local_timing_log;
    
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
        xQueuePeek(freq_data_q, &local_freq_data, 0);
        

        // lock then grab copy of system status to display
        if (xSemaphoreTake(system_status_mutex, portMAX_DELAY)) {
            local_system_status = system_status;
            xSemaphoreGive(system_status_mutex);
        }
        if (xSemaphoreTake(load_status_mutex, portMAX_DELAY)) {
            for (int i = 0; i < NUM_LOADS; i++) {
                local_load_status[i] = load_status[i];
            }
            xSemaphoreGive(load_status_mutex);
        }
        if (xSemaphoreTake(timing_log_mutex, portMAX_DELAY)) {
            local_timing_log = timing_log;
            xSemaphoreGive(timing_log_mutex);
        }

        // -- render VGA elements (current measurements) --
        sprintf(text_buffer, "[%d] TF Threshold: %5.2f Hz    ", 
            (1 - local_system_status.threshold_edit_mode), local_system_status.TF_threshold);
        alt_up_char_buffer_string(char_buffer, text_buffer, 5, 5);
        
        sprintf(text_buffer, "[%d] TROC Threshold: %5.2f Hz/s    ", 
            local_system_status.threshold_edit_mode, local_system_status.TROC_threshold);
        alt_up_char_buffer_string(char_buffer, text_buffer, 5, 7);
        
        sprintf(text_buffer, "Current Frequency: %5.2f Hz    ", ui_freq_data.frequency);
        alt_up_char_buffer_string(char_buffer, text_buffer, 5, 9);

        sprintf(text_buffer, "Current RoC: %5.2f Hz/s    ", ui_freq_data.roc);
        alt_up_char_buffer_string(char_buffer, text_buffer, 5, 11);

        // -- current mode
        const char* state_str;
        switch (local_system_status.system_state) {
            case SYSTEM_NORMAL:
                state_str = "NORMAL     ";
                break;
            case SYSTEM_MAINTENANCE: 
                state_str = "MAINTENANCE"; 
                break;
            case SYSTEM_MANAGING:    
                state_str = "MANAGING   "; 
                break;
            default:                 
                state_str = "UNKNOWN    "; 
                break;
        }
        sprintf(text_buffer, "System State: %s", state_str);
        alt_up_char_buffer_string(char_buffer, text_buffer, 5, 13);

        //  -- load statuses --
        for (int i = 0; i < NUM_LOADS; i++) {
            const char* load_str;
            if (local_load_status[i] == LOAD_ON) load_str = "ON  ";
            else if (local_load_status[i] == LOAD_SHED) load_str = "SHED";
            else load_str = "OFF ";
            
            sprintf(text_buffer, "Load %d: %s", i+1, load_str);
            // space loads out by 2 char units
            alt_up_char_buffer_string(char_buffer, text_buffer, 5, 16 + (i*2));
        }

        // -- timing stats --
        sprintf(text_buffer, "-- Reaction Times (ms) --");
        alt_up_char_buffer_string(char_buffer, text_buffer, 45, 5);

        sprintf(text_buffer, "Recent: %d, %d, %d, %d, %d       ", 
            (int)local_timing_log.recent[0], (int)local_timing_log.recent[1], 
            (int)local_timing_log.recent[2], (int)local_timing_log.recent[3], 
            (int)local_timing_log.recent[4]);
        alt_up_char_buffer_string(char_buffer, text_buffer, 45, 7);

        sprintf(text_buffer, "Min: %d | Max: %d | Avg: %d    ", 
            (int)local_timing_log.min_time, (int)local_timing_log.max_time, (int)local_timing_log.avg_time);
        alt_up_char_buffer_string(char_buffer, text_buffer, 45, 9);

        // -- uptime --
        unsigned int uptime_sec = xTaskGetTickCount() / configTICK_RATE_HZ;
        sprintf(text_buffer, "System Uptime: %u seconds      ", uptime_sec);
        alt_up_char_buffer_string(char_buffer, text_buffer, 45, 12);

        // -- watermark --
        sprintf(text_buffer, "LCFR CONTROL PANEL");
        alt_up_char_buffer_string(char_buffer, text_buffer, 5, 50);
        sprintf(text_buffer, "BY CHRIS M. & TANIA P.");
        alt_up_char_buffer_string(char_buffer, text_buffer, 5, 52);

        vTaskDelayUntil(&last_wake_ticks, freq_ticks);
    }
}
