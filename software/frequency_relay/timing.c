// timing.c

#include "Freertos/FreeRTOS.h"
#include "Freertos/semphr.h"
#include "Freertos/timers.h"

#include "frequency_relay.h"

timing_log_t timing_log = { {0}, 0, 0, 0, 0 };

// tracks how long it takes from a frequency violation to a load being shed
static void record_timing(TickType_t elapsed_ticks) {
    xSemaphoreTake(timing_log_mutex, portMAX_DELAY);
    // shift recent array
    for (int i = TIMING_LOG_SIZE - 1; i > 0; i--) {
        timing_log.recent[i] = timing_log.recent[i - 1];
    }
    timing_log.recent[0] = elapsed_ticks;

    // update min/max/avg
    timing_log.count++;
    if (timing_log.count == 1) {
        timing_log.minTime = elapsed_ticks;
        timing_log.maxTime = elapsed_ticks;
        timing_log.avgTime = elapsed_ticks;
    } else {
        if (elapsed_ticks < timing_log.minTime) {
            timing_log.minTime = elapsed_ticks;
        }
        if (elapsed_ticks > timing_log.maxTime) {
            timing_log.maxTime = elapsed_ticks;

        // incremental average calculation
        timing_log.avgTime = (timing_log.avgTime * (timing_log.count - 1) + elapsed_ticks) 
                            / timing_log.count;
        }
    }

    xSemaphoreGive(timing_log_mutex);
}