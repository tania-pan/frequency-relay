// standard includes
#include <stddef.h>
#include <stdio.h>
#include <string.h>

// scheduler includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include <altera_avalon_pio_regs.h>
#include "system.h" // Needed for hardware base addresses and IRQs

// project includes
#include "frequency_relay.h"

// definition of Task Stacks
#define TASK_STACKSIZE 2048

QueueHandle_t buttonCmdQ;
QueueHandle_t kbdQ;
QueueHandle_t freqDataQ;

SemaphoreHandle_t peakReadSem;
SemaphoreHandle_t loadStatusMutex;
SemaphoreHandle_t systemStatusMutex;
SemaphoreHandle_t timingLogMutex;

freqData_t freq_data;


void init_config(void) {
buttonCmdQ = xQueueCreate(BUTTON_Q_LENGTH, sizeof(int));
kbdQ = xQueueCreate(KBD_Q_LENGTH, sizeof(int));
freqDataQ = xQueueCreate(FREQDATA_Q_LENGTH, sizeof(freqData_t));

peakReadSem = xSemaphoreCreateBinary();
loadStatusMutex = xSemaphoreCreateMutex();
systemStatusMutex = xSemaphoreCreateMutex();
timingLogMutex = xSemaphoreCreateMutex();
}

int main(int argc, char* argv[], char* envp[])
{
	init_config();

	printf("Initialization Complete.\\n");

	// Start Scheduler
	vTaskStartScheduler();

	for (;;);
	return 0;
}
