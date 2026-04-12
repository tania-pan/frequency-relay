// standard includes
#include <stddef.h>
#include <stdio.h>
#include <string.h>

// scheduler includes
#include "freertos/FreeRTOS.h"
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

void vga_display_task(void *pvParameters) {
	return;
}

void init_config(void) {
	// -- global data --
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
		for (;;); // halt on fatal error
	}
	
	// -- vga display --
	// open char buffer "device" -> hardware struct pointer
	alt_up_char_buffer_dev *char_buffer_dev;
	char_buffer_dev = alt_up_char_buffer_open_dev("video_character_buffer_with_dma_avalon_char_buffer_slave");

	if (char_buffer_dev == NULL) {
		printf("Fatal Error: Could not open character buffer device.\n");
		for(;;);
	}

	xTaskCreate(
		"VGATask",
		TASK_STACKSIZE,
		(void*)char_buffer_dev,
		3, 
		NULL
	);
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
