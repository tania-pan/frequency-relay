// isr.c

// #include "isr.h"
// #include "frequency_relay.h"

#include <io.h>

#include "../frequency_relay_bsp/system.h"
#include "../frequency_relay_bsp/drivers/inc/altera_avalon_pio_regs.h"
#include "../frequency_relay_bsp/drivers/inc/altera_up_avalon_ps2_regs.h"

#include "isr.h"
#include "frequency_relay.h"

void fau_isr(void* context, alt_u32 id) {
	// placeholder for return status of if a higher priority task
	// was blocked from a resource liberated in this ISR
	BaseType_t xHigherPriorityTask = pdFALSE;

	// read 32 bit N counter from FAU
	freq_data.n = IORD_ALTERA_AVALON_PIO_DATA(FREQUENCY_ANALYSER_BASE);

	// give the semaphore (aka signal to the task that data is available)
	// also if there is a higher priority task waiting on that resource
	// set xHigherPriorityTask high
	xSemaphoreGiveFromISR(peak_ready_sem, &xHigherPriorityTask);

	// if there is a higher priority task, switch to it before resuming
	portEND_SWITCHING_ISR(xHigherPriorityTask);

}

void button_isr(void* context, alt_u32 id) {
	// placeholder for return status of if a higher priority task
	// was blocked from a resource liberated in this ISR
	BaseType_t xHigherPriorityTask = pdFALSE;

	int button_input = IORD_ALTERA_AVALON_PIO_DATA(PUSH_BUTTON_BASE);

	// clear the hardware interrupt
	IOWR_ALTERA_AVALON_PIO_EDGE_CAP(PUSH_BUTTON_BASE, 0xF);

	// add data to mailbox, if higher task blocked cause queue
	// make xHigherPriorityTask = pdTRUE
	xQueueSendFromISR(button_q, &button_input, &xHigherPriorityTask);

	// if there is a higher priority task, switch to it before resuming
	portEND_SWITCHING_ISR(xHigherPriorityTask);
}

void kbd_isr(void* context, alt_u32 id) {
	(void)context;
	(void)id;
	BaseType_t xHigherPriorityTask = pdFALSE;

	// read the full 32bit register
	uint32_t ps2_reg = IORD_ALT_UP_PS2_PORT_DATA_REG(PS2_BASE);

	// bit 15 is RVALID (if data valid)
	if (ps2_reg & 0x8000) {
		// extract only the 8bit key code (0-7)
		int key_code = ps2_reg & 0xFF;

		xQueueSendFromISR(kbd_q, &key_code, &xHigherPriorityTask);
	}

	portEND_SWITCHING_ISR(xHigherPriorityTask);
}
