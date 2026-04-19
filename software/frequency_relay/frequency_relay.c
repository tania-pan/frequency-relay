// frequency_relay.c

#include <stdio.h>
#include <io.h>

#include "../frequency_relay_bsp/system.h"
#include "../frequency_relay_bsp/drivers/inc/altera_avalon_pio_regs.h"
#include "../frequency_relay_bsp/drivers/inc/altera_up_avalon_ps2_regs.h"
#include "../frequency_relay_bsp/drivers/inc/altera_up_avalon_video_character_buffer_with_dma.h"
#include "../frequency_relay_bsp/drivers/inc/altera_up_avalon_video_pixel_buffer_dma.h"

#include "frequency_relay.h"
#include "config.h"
#include "isr.h"
#include "vga.h"
#include "task_freq_calc.h"
#include "task_load_management.h"

// globals variables for FreeRTOS
QueueHandle_t button_q;
QueueHandle_t kbd_q;
QueueHandle_t freq_data_q;

SemaphoreHandle_t peak_ready_sem;
SemaphoreHandle_t load_status_mutex;
SemaphoreHandle_t system_status_mutex;
SemaphoreHandle_t timing_log_mutex;

freq_data_t freq_data;
system_status_t system_status;

void init_config(void) {
	// -- global data --
	freq_data.frequency = 0;
	freq_data.roc = 0;
	freq_data.n = 0;
	freq_data.timestamp = 0;

	system_status.TF_threshold = 50.0;
	system_status.TROC_threshold= 10.0;
	system_status.threshold_edit_mode = TF;
	system_status.system_state = SYSTEM_NORMAL;

	button_q = xQueueCreate(BUTTON_Q_LENGTH, sizeof(int));
	kbd_q = xQueueCreate(KBD_Q_LENGTH, sizeof(int));
	freq_data_q = xQueueCreate(FREQDATA_Q_LENGTH, sizeof(freq_data_t));

	peak_ready_sem = xSemaphoreCreateBinary();
	load_status_mutex = xSemaphoreCreateMutex();
	system_status_mutex = xSemaphoreCreateMutex();
	timing_log_mutex = xSemaphoreCreateMutex();

	// check for any init failures
	if (button_q == NULL || kbd_q == NULL || freq_data_q == NULL ||
	peak_ready_sem == NULL || load_status_mutex == NULL || system_status_mutex == NULL ||
	timing_log_mutex == NULL) {
		printf("Fatal Error: Failed to create FreeRTOS primitives.\n");
		fflush(stdout);
		for (;;); // halt on fatal error
	}

	// enable interrupts for button PIO
	IOWR_ALTERA_AVALON_PIO_IRQ_MASK(PUSH_BUTTON_BASE, 0xF);
	// clear any pending buttons
	IOWR_ALTERA_AVALON_PIO_EDGE_CAP(PUSH_BUTTON_BASE, 0xF);

	// enable read interrupts for PS2 KBD
	IOWR_ALT_UP_PS2_PORT_CTRL_REG(PS2_BASE, 1);

	// link the IRQs to our routines
	alt_irq_register(FREQUENCY_ANALYSER_IRQ, NULL, fau_isr);
	alt_irq_register(PUSH_BUTTON_IRQ, NULL, button_isr);
	alt_irq_register(PS2_IRQ, NULL, kbd_isr);

	// -- vga display --
	// open pixel buffer
	alt_up_pixel_buffer_dma_dev *pixel_buf;
    pixel_buf = alt_up_pixel_buffer_dma_open_dev(VIDEO_PIXEL_BUFFER_DMA_NAME);
    if(pixel_buf == NULL){
        printf("Fatal Error: Cannot find pixel buffer device\n");
        fflush(stdout);
        for(;;);
    }
    alt_up_pixel_buffer_dma_clear_screen(pixel_buf, 0);
    alt_up_pixel_buffer_dma_clear_screen(pixel_buf, 1);

	// open char buffer
	alt_up_char_buffer_dev *char_buffer_dev;
	char_buffer_dev = alt_up_char_buffer_open_dev("/dev/video_character_buffer_with_dma");

	if (char_buffer_dev == NULL) {
		printf("Fatal Error: Could not open character buffer device.\n");
		fflush(stdout);
		for(;;);
	}

	alt_up_char_buffer_clear(char_buffer_dev);

	// create tasks
	xTaskCreate(
		task_frequency_calculation, 
		"FreqCalc", 
		TASK_STACKSIZE, 
		NULL, 
		3,
		NULL
	);

	xTaskCreate(
		task_load_management,
		"LoadMgmt", 
		TASK_STACKSIZE, 
		NULL, 
		2,
		NULL
	);

	xTaskCreate(
		vga_display_task,
		"VGATask",
		TASK_STACKSIZE,
		(void*)char_buffer_dev,
		1,
		NULL
	);
}

int main(int argc, char* argv[], char* envp[]) {
	printf("System booting...\n");
	fflush(stdout);

	init_config();
	printf("Initialization Complete.\n");
	fflush(stdout);

	// start Scheduler
	vTaskStartScheduler();

	// if scheduler returns, not enough FreeRTOS heap memory
	printf("Fatal Error: Insufficient FreeRTOS heap to start scheduler.\n");
	fflush(stdout);
	for (;;);
	return 0;
}
