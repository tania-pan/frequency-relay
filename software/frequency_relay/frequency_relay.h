// frequency_relay.h

#ifndef __FREQUENCY_RELAY_H_
#define __FREQUENCY_RELAY_H_

#include "types.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#define BUTTON_Q_LENGTH 10
#define KBD_Q_LENGTH 10
#define FREQDATA_Q_LENGTH 20
#define NUM_LOADS 5
#define TIMING_LOG_SIZE 5

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
