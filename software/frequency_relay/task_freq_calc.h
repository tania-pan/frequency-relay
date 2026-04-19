// task_freq_calc.h

#ifndef __TASK_FREQ_CALC_H_
#define __TASK_FREQ_CALC_H_

#include "FreeRTOS/FreeRTOS.h"
#include "FreeRTOS/task.h"
#include "FreeRTOS/queue.h"
#include "FreeRTOS/semphr.h"
#include "frequency_relay.h"
#include "types.h"

void task_frequency_calculation(void *pvParameters);

#endif  /* __TASK_FREQ_CALC_H_ */