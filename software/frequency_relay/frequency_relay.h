// frequency_relay.h

#ifndef __FREQUENCY_RELAY_H_
#define __FREQUENCY_RELAY_H_

#include "types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

extern freq_data_t freq_data;
extern load_status_t load_status[NUM_LOADS];
extern timing_log_t timing_log;
extern system_status_t system_status;

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
