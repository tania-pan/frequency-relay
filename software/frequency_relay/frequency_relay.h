// frequency_relay.h

#ifndef __FREQUENCY_RELAY_H_
#define __FREQUENCY_RELAY_H_

#include "types.h"
#include "FreeRTOS/FreeRTOS.h"
#include "FreeRTOS/task.h"
#include "FreeRTOS/queue.h"
#include "FreeRTOS/semphr.h"
#include "freertos/timers.h"

extern freq_data_t freq_data;
extern system_status_t system_status;

extern QueueHandle_t button_q;
extern QueueHandle_t kbd_q;
extern QueueHandle_t freq_data_q;
extern SemaphoreHandle_t peak_ready_sem;
extern SemaphoreHandle_t load_status_mutex;
extern SemaphoreHandle_t system_status_mutex;
extern SemaphoreHandle_t timing_log_mutex;

void init_config(void);

#endif  /* __FREQUENCY_RELAY_H_ */
