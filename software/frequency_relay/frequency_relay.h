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

extern freqData_t freq_data;
extern loadStatus_t load_status[NUM_LOADS];
extern systemState_t system_state;
extern timingLog_t timing_log;

extern float thresholdFreq;
extern float thresholdROCF;
extern volatile unsigned int latestN;

extern QueueHandle_t buttonCmdQ;
extern QueueHandle_t kbdQ;
extern QueueHandle_t freqDataQ;
extern SemaphoreHandle_t peakReadySem;
extern SemaphoreHandle_t loadStatusMutex;
extern SemaphoreHandle_t systemStatusMutex;
extern SemaphoreHandle_t timingLogMutex;

void init_config(void);
void TestFAUTask(void *pvParameters);
void TaskFrequencyCalculation(void *pvParameters);

#endif  /* __FREQUENCY_RELAY_H_ */
