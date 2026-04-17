#include "altera_avalon_pio_regs.h"
#include "system.h"
#include "frequency_relay.h"
#include "FreeRTOS.h"

static void updateLEDs(void) {

    volatile int *redLEDs = (volatile int *) RED_LEDS_BASE;
    volatile int *greenLEDs = (volatile int *) GREEN_LEDS_BASE;

    int redMask = 0;
    int greenMask = 0;

    // wait for mutex before reading shared load status
    xSemaphoreTake(loadStatusMutex, portMAX_DELAY);

    for (int = 0; i < NUM_LOADS; i++) {
        if (loadStates[i] == LOAD_ON) {
            redMask |= (1 << i); // Set bit for red LED
        } else {
            greenMask |= (1 << i); // Set bit for green LED
        }
        // LOAD_OFF
    }

    xSemaphoreGive(loadStatusMutex);

    // Update the physical LEDs
    *redLEDs = redMask;
    *greenLEDs = greenMask;
}

// poll the slide switches and update load status accordingly
static void pollSwitches(void) {

    // read switch states
    uint32_t switchState = IORD_ALTERA_AVALON_PIO_DATA(SLIDE_SWITCH_BASE);

    xSemaphoreTake(loadStatusMutex, portMAX_DELAY);

    systemState_t currentState;
    xSemaphoreTake(systemStatusMutex, portMAX_DELAY);
    currentState = system_state;    // normal, maintenance, managing
    xSemaphoreGive(systemStatusMutex);

    for (int i = 0; i < NUM_LOADS; i++) {
        int switchOn = (switchState >> i) & 0x1; // check if switch i is on
        
        if (!switchOn) {
            // if switch is off, load must be off
            load_status[i] = LOAD_OFF;
        } else {
            // switch is on
            if (currentState == SYSTEM_MAINTENANCE) {
                // maintenance bypasses relay
                load_status[i] = LOAD_ON;
            } else if (load_status[i] == LOAD_OFF) {
                // user flips switch
                load_status[i] = LOAD_ON;
            }
        }
    }

    xSemaphoreGive(loadStatusMutex);
}