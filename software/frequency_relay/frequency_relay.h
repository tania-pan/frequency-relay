#ifndef __FREQUENCY_RELAY_H_
#define __FREQUENCY_RELAY_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#define BUTTON_Q_LENGTH 10
#define KBD_Q_LENGTH 10
#define FREQDATA_Q_LENGTH 20
#define NUM_LOADS 5
#define TIMING_LOG_SIZE 5

typedef struct {
	float frequency;		// in Hz
	float roc; 				// rate of change of frequency in Hz/s
	unsigned int n;			// raw ADC sample count
	TickType_t timestamp;	// FreeRTOS tick count when measured
} freqData_t;

typedef enum {
	LOAD_OFF,		// switch is off
	LOAD_ON,		// switch is on
	LOAD_SHED		// relay has shed this load
} loadStatus_t;

typedef enum {
	SYSTEM_NORMAL,			
	SYSTEM_MAINTENANCE,		
	SYSTEM_MANAGING		
} systemState_t;

typedef struct {
	TickType_t recent[TIMING_LOG_SIZE];	// last 5 measurements
	TickType_t minTime;
	TickType_t maxTime;
	TickType_t avgTime;
int count;								// total measurements taken
} timingLog_t;

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
