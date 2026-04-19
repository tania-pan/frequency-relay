// task_load_management.h

#ifndef __TASK_LOAD_MANAGEMENT_H_
#define __TASK_LOAD_MANAGEMENT_H_

#include "FreeRTOS/FreeRTOS.h"
#include "FreeRTOS/task.h"
#include "FreeRTOS/queue.h"
#include "FreeRTOS/semphr.h"
#include "freertos/timers.h"
#include "frequency_relay.h"
#include "types.h"

#define NUM_LOADS 5

void task_load_management(void *pvParameters);
void init_timer(void);
static void timer_500ms_callback(TimerHandle_t xTimer);
static void record_timing(TickType_t elapsed_ticks);

static int shed_next_load(void);
static int reconnect_next_load(void);
static int all_loads_reconnected(void);

void handle_maintenance_toggle(void);
static void update_leds(void);
static void poll_switches(void);

extern TimerHandle_t timer_500ms;
extern timing_log_t timing_log;
extern load_status_t load_status[NUM_LOADS];

extern int network_unstable;


#endif  /* __TASK_LOAD_MANAGEMENT_H_ */
