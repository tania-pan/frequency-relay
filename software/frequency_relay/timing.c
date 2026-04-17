#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"

#include "frequency_relay.h"

timingLog_t timingLog = { {0}, 0, 0, 0, 0 };

// tracks how long it takes from a frequency violation to a load being shed
static void recordTiming(TickType_t elapsedTicks) {
    xSemaphoreTake(timingLogMutex, portMAX_DELAY);

    // shift recent array
    for (int i = TIMING_LOG_SIZE - 1; i > 0; i--) {
        timingLog.recent[i] = timingLog.recent[i - 1];
    }
    timingLog.recent[0] = elapsedTicks;

    // update min/max/avg
    timingLog.count++;
    if (timingLog.count == 1) {
        timingLog.minTime = elapsedTicks;
        timingLog.maxTime = elapsedTicks;
        timingLog.avgTime = elapsedTicks;
    } else {
        if (elapsedTicks < timingLog.minTime) {
            timingLog.minTime = elapsedTicks;
        }
        if (elapsedTicks > timingLog.maxTime) {
            timingLog.maxTime = elapsedTicks;

        // incremental average calculation
        timingLog.avgTime = (timingLog.avgTime * (timingLog.count - 1) + elapsedTicks) 
                            / timingLog.count;
        }
    }

    xSemaphoreGive(timingLogMutex);
}