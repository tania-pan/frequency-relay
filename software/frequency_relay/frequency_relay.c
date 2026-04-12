// standard includes
#include <stddef.h>
#include <stdio.h>
#include <string.h>

// scheduler includes
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "../frequency_relay_bsp/drivers/inc/altera_avalon_pio_regs.h"
#include "../frequency_relay_bsp/system.h"

// project includes
#include "frequency_relay.h"

// definition of task stacks
#define TASK_STACKSIZE 2048

// globals variables for FreeRTOS
QueueHandle_t buttonCmdQ;
QueueHandle_t kbdQ;
QueueHandle_t freqDataQ;

SemaphoreHandle_t peakReadSem;
SemaphoreHandle_t loadStatusMutex;
SemaphoreHandle_t systemStatusMutex;
SemaphoreHandle_t timingLogMutex;

freqData_t freq_data;

void fau_isr(void* context) {
	// placeholder for return status of if a higher priority task
	// was blocked from a resource liberated in this ISR
	BaseType_t xHigherPriorityTask;

	// read 32 bit N counter from FAU
	freq_data.n = IORD_ALTERA_AVALON_PIO_DATA(FREQUENCY_ANALYSER_BASE);

	// clear the hardware interrupt
	IOWR_ALTERA_AVALON_PIO_EDGE_CAP(FREQUENCY_ANALYSER_BASE, 0x0);

	// give the semaphore (aka signal to the task that data is available)
	// also if there is a higher priority task waiting on that resource
	// set xHigherPriorityTask high
	xSemaphoreGiveFromISR(peakReadSem, &xHigherPriorityTask);

	// if there is a higher priority task, switch to it before resuming
	portYIELD_FROM_ISR(xHigherPriorityTask);
}

void debug_consumer_task(void *pvParameters) {
	// TODO: remove once tested and working correctly
	int button_rx;
	int kbd_rx;

	printf("Debug task: waiting for ISR triggers");

	while(1) {
		// check semaphore if new resource has appeared
		if (xSemaphoreTake(peakReadSem, 0)) {
			printf("FAU semaphore accessed");
		}
		// check button queue
		if (xQueueReceive(buttonCmdQ, &button_rx, 0) == pdTRUE) {
			printf("Button queue accessed: Value = %u", button_rx);
		}
		// check kbd queue
		if (xQueueReceive(kbdQ, &kbd_rx, 0) == pdTRUE) {
			printf("Keyboard queue accessed: Value = %u", kbd_rx);
		}
	}

	// yield to scheduler for 50ms 
	vTaskDelay(pdMS_TO_TICKS(50));
}

void init_config(void) {
	// init global data
	freq_data.frequency = 0;
	freq_data.roc = 0;
	freq_data.n = 0;
	freq_data.timestamp = 0;

	buttonCmdQ = xQueueCreate(BUTTON_Q_LENGTH, sizeof(int));
	kbdQ = xQueueCreate(KBD_Q_LENGTH, sizeof(int));
	freqDataQ = xQueueCreate(FREQDATA_Q_LENGTH, sizeof(freqData_t));

	peakReadSem = xSemaphoreCreateBinary();
	loadStatusMutex = xSemaphoreCreateMutex();
	systemStatusMutex = xSemaphoreCreateMutex();
	timingLogMutex = xSemaphoreCreateMutex();

	// check for any init failures
	if (buttonCmdQ == NULL || kbdQ == NULL || freqDataQ == NULL ||
	peakReadSem == NULL || loadStatusMutex == NULL || systemStatusMutex == NULL ||
	timingLogMutex == NULL) {
		printf("Fatal Error: Failed to create FreeRTOS primitives.\n");
		for (;;); // Halt on fatal error
	}
}

int main(int argc, char* argv[], char* envp[]) {
	init_config();

	printf("Initialization Complete.\n");

	// Start Scheduler
	vTaskStartScheduler();

	// if scheduler returns, not enough FreeRTOS heap memory
	printf("Fatal Error: Insufficient FreeRTOS heap to start scheduler.\n");
	for (;;);
	return 0;
}
