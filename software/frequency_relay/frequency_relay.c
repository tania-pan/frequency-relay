// standard includes
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

// scheduler includes
#include "FreeRTOS/portmacro.h"
#include "FreeRTOS/projdefs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "../frequency_relay_bsp/drivers/inc/altera_avalon_pio_regs.h"
#include "../frequency_relay_bsp/drivers/inc/altera_up_avalon_video_character_buffer_with_dma.h"
#include "../frequency_relay_bsp/drivers/inc/altera_up_avalon_video_pixel_buffer_dma.h"
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
SemaphoreHandle_t systemStatusMutex;
SemaphoreHandle_t loadStatusMutex;
SemaphoreHandle_t timingLogMutex;

freqData_t freq_data;
system_status_t system_status;

void vga_display_task(void *pvParameters) {
	// lock (semaphore) -> copy data -> unlock -> send to VGA

	// grab char buffer device handle from pv params
	alt_up_char_buffer_dev *char_buffer = (alt_up_char_buffer_dev *)pvParameters;

	// ~60Hz (17ms) timing setup
	TickType_t xLastWakeTime = xTaskGetTickCount();
	const TickType_t xFrequency = pdMS_TO_TICKS(17); // i DONT THINK FREQ IS ACCURATE


	freqData_t local_freq_data;
	system_status_t local_system_status;
	int kbd_rx;
	char text_buffer[64];

	printf("VGA Task Started\n");
	fflush(stdout);

	while(1) {
		// -- process keyboard inputs --
		while (xQueueReceive(kbdQ, &kbd_rx, 0) == pdTRUE) {
			// -- lock system status to change struct members --
			if (xSemaphoreTake(systemStatusMutex, 0) == pdTRUE) {
				switch (kbd_rx) {
					case PS2_1:
						system_status.threshold_edit_mode = TF;
						break;
					case PS2_2:
						system_status.threshold_edit_mode = TROC;
						break;
					case PS2_UP:
						if (system_status.threshold_edit_mode) {
							system_status.TROC_threshold += 0.5;
						} else {
							system_status.TF_threshold += 0.5;
						}
						break;
					case PS2_DOWN:
						if (system_status.threshold_edit_mode) {
							system_status.TROC_threshold -= 0.5;
						} else {
							system_status.TF_threshold -= 0.5;
						}
						break;
				}
				// -- unlock semaphore --
				xSemaphoreGive(systemStatusMutex);
			}
		}

		// -- gather local copies of data --
		// peak (don't steal from load management)
		if (xQueuePeek(freqDataQ, &local_freq_data, 0) != pdTRUE) {
			local_freq_data.frequency = 0;
			local_freq_data.roc = 0;
		}

		// lock then grab
		if (xSemaphoreTake(systemStatusMutex, portMAX_DELAY)) {
			local_system_status = system_status;
			xSemaphoreGive(systemStatusMutex);
		}
		if (xSemaphoreTake(loadStatusMutex, portMAX_DELAY)) {
			// TODO: local_load_status =
			xSemaphoreGive(loadStatusMutex);
		}
		if (xSemaphoreTake(timingLogMutex, portMAX_DELAY)) {
			// TODO: local_timing_log = timing_log;
			xSemaphoreGive(timingLogMutex);
		}

		// -- render VGA elements --
		sprintf(text_buffer, "TF Threshold: %5.2f Hz    ", local_system_status.TF_threshold);
		alt_up_char_buffer_string(char_buffer, text_buffer, 5, 5);
		
		sprintf(text_buffer, "TROC Threshold: %5.2f Hz/s    ", local_system_status.TROC_threshold);
		alt_up_char_buffer_string(char_buffer, text_buffer, 5, 7);
		
		sprintf(text_buffer, "Current Frequency: %5d Hz    ", local_freq_data.frequency);
		alt_up_char_buffer_string(char_buffer, text_buffer, 5, 9);

		sprintf(text_buffer, "Current RoC: %5d Hz/s    ", local_freq_data.roc);
		alt_up_char_buffer_string(char_buffer, text_buffer, 5, 11);

		// TODO: add load and timing stats here

		vTaskDelayUntil(&xLastWakeTime, xFrequency);
	}
}

void init_config(void) {
	// -- global data --
	freq_data.frequency = 0;
	freq_data.roc = 0;
	freq_data.n = 0;
	freq_data.timestamp = 0;

	system_status.TF_threshold = 50.0;
	system_status.TROC_threshold= 10.0;
	system_status.threshold_edit_mode = TF;
	system_status.system_mode = NORMAL;

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
		fflush(stdout);
		for (;;); // halt on fatal error
	}
	
	// -- vga display --
	alt_up_pixel_buffer_dma_dev *pixel_buf;
    pixel_buf = alt_up_pixel_buffer_dma_open_dev(VIDEO_PIXEL_BUFFER_DMA_NAME);
    if(pixel_buf == NULL){
        printf("Fatal Error: Cannot find pixel buffer device\n");
        fflush(stdout);
        for(;;);
    }
    alt_up_pixel_buffer_dma_clear_screen(pixel_buf, 0);
	// open char buffer "device" -> hardware struct pointer
	alt_up_char_buffer_dev *char_buffer_dev;
	char_buffer_dev = alt_up_char_buffer_open_dev("/dev/video_character_buffer_with_dma");

	if (char_buffer_dev == NULL) {
		printf("Fatal Error: Could not open character buffer device.\n");
		fflush(stdout);
		for(;;);
	}

	alt_up_char_buffer_clear(char_buffer_dev);

	xTaskCreate(
		vga_display_task,
		"VGATask",
		TASK_STACKSIZE,
		(void*)char_buffer_dev,
		3,
		NULL
	);
}

int main(int argc, char* argv[], char* envp[]) {
	printf("System booting...\n");
	fflush(stdout);

	init_config();

	printf("Initialization Complete.\n");
	fflush(stdout);

	// Start Scheduler
	vTaskStartScheduler();

	// if scheduler returns, not enough FreeRTOS heap memory
	printf("Fatal Error: Insufficient FreeRTOS heap to start scheduler.\n");
	fflush(stdout);
	for (;;);
	return 0;
}
