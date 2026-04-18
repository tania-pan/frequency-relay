// frequency_relay.h

#ifndef __FREQUENCY_RELAY_H_
#define __FREQUENCY_RELAY_H_

// standard includes
// #include <stddef.h>
// #include <stdio.h>
// #include <string.h>

// // scheduler includes
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "freertos/queue.h"
// #include "freertos/semphr.h"
// #include "FreeRTOS/projdefs.h"
// #include "freertos/portmacro.h"
// #include "freertos/projdefs.h"

// #include "../frequency_relay_bsp/system.h"
// #include "../frequency_relay_bsp/drivers/inc/altera_up_avalon_ps2_regs.h"
// #include "../frequency_relay_bsp/drivers/inc/altera_avalon_pio_regs.h"
// #include "../frequency_relay_bsp/drivers/inc/altera_up_avalon_video_character_buffer_with_dma.h"
// #include "../frequency_relay_bsp/drivers/inc/altera_up_avalon_video_pixel_buffer_dma.h"

#include "types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

extern freq_data_t freq_data;
extern load_status_t load_status[NUM_LOADS];
extern timing_log_t timing_log;

extern QueueHandle_t button_q;
extern QueueHandle_t kbd_q;
extern QueueHandle_t freq_data_q;
extern SemaphoreHandle_t peak_ready_sem;
extern SemaphoreHandle_t load_status_mutex;
extern SemaphoreHandle_t system_status_mutex;
extern SemaphoreHandle_t timing_log_mutex;

void init_config(void);
void TestFAUTask(void *pvParameters);
void TaskFrequencyCalculation(void *pvParameters);

#endif  /* __FREQUENCY_RELAY_H_ */
