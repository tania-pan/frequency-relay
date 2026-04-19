#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "altera_avalon_pio_regs.h"
#include "system.h"

#include "frequency_relay.h"

static void update_leds(void) {

    volatile int *red_leds = (volatile int *) RED_LEDS_BASE;
    volatile int *green_leds = (volatile int *) GREEN_LEDS_BASE;

    int red_mask = 0;
    int green_mask = 0;

    // wait for mutex before reading shared load status
    xSemaphoreTake(load_status_mutex, portMAX_DELAY);

    for (int i = 0; i < NUM_LOADS; i++) {
        if (load_status[i] == LOAD_ON) {
            red_mask |= (1 << i); // Set bit for red LED
        } else {
            green_mask |= (1 << i); // Set bit for green LED
        }
        // LOAD_OFF
    }

    xSemaphoreGive(load_status_mutex);

    // Update the physical LEDs
    *red_leds = red_mask;
    *green_leds = green_mask;
}

// poll the slide switches and update load status accordingly
static void poll_switches(void) {

    // read switch states
    uint32_t switch_state = IORD_ALTERA_AVALON_PIO_DATA(SLIDE_SWITCH_BASE);

    xSemaphoreTake(load_status_mutex, portMAX_DELAY);

    systemState_t current_state;
    xSemaphoreTake(system_status_mutex, portMAX_DELAY);
    current_state = system_state;    // normal, maintenance, managing
    xSemaphoreGive(system_status_mutex);

    for (int i = 0; i < NUM_LOADS; i++) {
        int switch_on = (switch_state >> i) & 0x1; // check if switch i is on
        
        if (!switch_on) {
            // if switch is off, load must be off
            load_status[i] = LOAD_OFF;
        } else {
            // switch is on
            if (current_state == SYSTEM_MAINTENANCE) {
                // maintenance bypasses relay
                load_status[i] = LOAD_ON;
            } else if (load_status[i] == LOAD_OFF) {
                // user flips switch
                load_status[i] = LOAD_ON;
            }
        }
    }

    xSemaphoreGive(load_status_mutex);
}