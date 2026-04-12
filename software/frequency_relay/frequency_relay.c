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
#include "../frequency_relay_bsp/HAL/inc/alt_types.h"

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

void fau_isr(void* context, alt_u32 id) {
	// placeholder for return status of if a higher priority task
	// was blocked from a resource liberated in this ISR
	BaseType_t xHigherPriorityTask = pdFALSE;

	// read 32 bit N counter from FAU
	freq_data.n = IORD_ALTERA_AVALON_PIO_DATA(FREQUENCY_ANALYSER_BASE);

	// give the semaphore (aka signal to the task that data is available)
	// also if there is a higher priority task waiting on that resource
	// set xHigherPriorityTask high
	xSemaphoreGiveFromISR(peakReadSem, &xHigherPriorityTask);

	// if there is a higher priority task, switch to it before resuming
	portEND_SWITCHING_ISR(xHigherPriorityTask);

}

void button_isr(void* context, alt_u32 id) {
	// placeholder for return status of if a higher priority task
	// was blocked from a resource liberated in this ISR
	BaseType_t xHigherPriorityTask = pdFALSE;

	int button_input = IORD_ALTERA_AVALON_PIO_DATA(PUSH_BUTTON_BASE);

	// clear the hardware interrupt
	IOWR_ALTERA_AVALON_PIO_EDGE_CAP(PUSH_BUTTON_BASE, 0x0);

	// add data to mailbox, if higher task blocked cause queue
	// make xHigherPriorityTask = pdTRUE
	xQueueSendFromISR(buttonCmdQ, &button_input, &xHigherPriorityTask);

	// if there is a higher priority task, switch to it before resuming
	portEND_SWITCHING_ISR(xHigherPriorityTask);
}

void kbd_isr(void* context, alt_u32 id) {
	// placeholder for return status of if a higher priority task
	// was blocked from a resource liberated in this ISR
	BaseType_t xHigherPriorityTask = pdFALSE;

	// read 32 bit N counter from FAU
	int kbd_input = IORD_ALTERA_AVALON_PIO_DATA(PS2_BASE);

	// add data to mailbox, if higher task blocked cause queue
	// make xHigherPriorityTask = pdTRUE
	xQueueSendFromISR(kbdQ, &kbd_input, &xHigherPriorityTask);

	// if there is a higher priority task, switch to it before resuming
	portEND_SWITCHING_ISR(xHigherPriorityTask);
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

		// yield to scheduler for 50ms
		vTaskDelay(pdMS_TO_TICKS(50));
	}
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

	// link the FAU IRQ to our routine
	alt_irq_register(
		FREQUENCY_ANALYSER_IRQ,
		NULL,
		fau_isr
	);

	// link the button IRQ to our routine
	alt_irq_register(
		PUSH_BUTTON_IRQ,
		NULL,
		button_isr
	);

	// link the keyboard IRQ to our routine
	alt_irq_register(
		PS2_IRQ,
		NULL,
		kbd_isr
	);

	// create debug task for testing ISRs
	xTaskCreate(debug_consumer_task, "DebugTask", TASK_STACKSIZE, NULL, 1, NULL);
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
