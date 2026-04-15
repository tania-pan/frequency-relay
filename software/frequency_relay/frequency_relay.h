#ifndef __FREQUENCY_RELAY_H_
#define __FREQUENCY_RELAY_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#define BUTTON_Q_LENGTH 10
#define KBD_Q_LENGTH 10
#define FREQDATA_Q_LENGTH 20

#define PS2_1 0x16
#define PS2_2 0x1E
#define PS2_UP 0x75
#define PS2_DOWN 0x72

enum Threshold {TF, TROC};
enum SystemMode {NORMAL, MAINTENANCE};

typedef struct {
	// TODO: scale up for efficient decimals
	int frequency;
	int roc; 		// rate of change of frequency
	int n;			// sample count
	int timestamp;	// in ms
} freqData_t;

typedef struct {
	float TF_threshold;
	float TROC_threshold;
	enum Threshold threshold_edit_mode;
	enum SystemMode system_mode;
} system_status_t;

extern freqData_t freq_data;

extern QueueHandle_t buttonCmdQ;
extern QueueHandle_t kbdQ;
extern QueueHandle_t freqDataQ;
extern SemaphoreHandle_t peakReadSem;
extern SemaphoreHandle_t loadStatusMutex;
extern SemaphoreHandle_t systemStatusMutex;
extern SemaphoreHandle_t timingLogMutex;

// ISR functions
void fau_isr(void* context, alt_u32 id);
void button_isr(void* context, alt_u32 id);
void kbd_isr(void* context, alt_u32 id);

void init_config(void);

#endif  /* __SYSTEM_H_ */
