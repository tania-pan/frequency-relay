// frequency_relay.c

// project includes
#include "frequency_relay.h"
#include "types.h"
#include "config.h"
#include "isr.h"
#include "vga.h"

// globals variables for FreeRTOS
QueueHandle_t buttonCmdQ;
QueueHandle_t kbdQ;
QueueHandle_t freqDataQ;

SemaphoreHandle_t peakReadySem;
SemaphoreHandle_t loadStatusMutex;
SemaphoreHandle_t systemStatusMutex;
SemaphoreHandle_t timingLogMutex;

loadStatus_t load_status[NUM_LOADS] = {LOAD_OFF, LOAD_OFF, LOAD_OFF, LOAD_OFF, LOAD_OFF};
systemState_t system_state = SYSTEM_NORMAL;
timingLog_t timing_log = {0};

freqData_t freq_data;
system_status_t system_status;

void debug_consumer_task(void *pvParameters) {
	// TODO: remove once tested and working correctly
	int button_rx;
	int kbd_rx;

	printf("Debug task: waiting for ISR triggers");

	while(1) {
		// check semaphore if new resource has appeared
		if (xSemaphoreTake(peakReadySem, 0)) {
//			printf("FAU semaphore accessed\n");
		}
		// check button queue
		if (xQueueReceive(buttonCmdQ, &button_rx, 0) == pdTRUE) {
			printf("Button queue accessed: Value = %u\n", button_rx);
		}
		// check kbd queue
		if (xQueueReceive(kbdQ, &kbd_rx, 0) == pdTRUE) {
			printf("Keyboard queue accessed: Value = %u\n", kbd_rx);
		}

		// yield to scheduler for 50ms
		vTaskDelay(pdMS_TO_TICKS(50));
	}
}

float thresholdFreq;
float thresholdROCF;
volatile unsigned int latestN;

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

	buttonCmdQ = xQueueCreate(BUTTON_Q_LENGTH, sizeof(int));
	kbdQ = xQueueCreate(KBD_Q_LENGTH, sizeof(int));
	freqDataQ = xQueueCreate(FREQDATA_Q_LENGTH, sizeof(freqData_t));

	peakReadySem = xSemaphoreCreateBinary();
	loadStatusMutex = xSemaphoreCreateMutex();
	systemStatusMutex = xSemaphoreCreateMutex();
	timingLogMutex = xSemaphoreCreateMutex();

	// check for any init failures
	if (buttonCmdQ == NULL || kbdQ == NULL || freqDataQ == NULL ||
	peakReadySem == NULL || loadStatusMutex == NULL || systemStatusMutex == NULL ||
	timingLogMutex == NULL) {
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

	// create debug task for testing ISRs
	xTaskCreate(debug_consumer_task, "DebugTask", TASK_STACKSIZE, NULL, 1, NULL);
	
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

	// create tasks
	xTaskCreate(TaskFrequencyCalculation, 
				"FreqCalc", 
				1000, 
				NULL, 
				1, 		// priority
				NULL);

	xTaskCreate(TestFAUTask, "TestFAU", 500, NULL, 3, NULL);

	// start Scheduler
	vTaskStartScheduler();

	// if scheduler returns, not enough FreeRTOS heap memory
	printf("Fatal Error: Insufficient FreeRTOS heap to start scheduler.\n");
	fflush(stdout);
	for (;;);
	return 0;
}
