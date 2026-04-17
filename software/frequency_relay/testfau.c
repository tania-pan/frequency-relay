// testfau.c

#include "frequency_relay.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

void TestFAUTask(void *pvParameters) {
    // These N values represent ~50Hz (N = 320) dropping to ~47Hz (N = 340)
    unsigned int testNValues[] = {320, 320, 320, 320, 330, 340, 350, 340, 330, 320};
    int idx = 0;
    int numValues = 10;

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(20)); // simulate one peak every 20ms (50Hz)
        
        latestN = testNValues[idx % numValues];
        idx++;
        
        // Simulate what the ISR does
        xSemaphoreGive(peakReadySem);
    }
}