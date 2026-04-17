#include "freertos/FreeRTOS.h"
#include "frequency_relay.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

void TaskFrequencyCalculation(void *pvParameters) {
    float f_old = 0.0f;
    unsigned int N_old = 0;
    int first_reading = 1;  // flag to indicate first reading for ROCF calculation

    while (1) {
        // block until ISR gives semaphore
        xSemaphoreTake(peakReadySem, portMAX_DELAY);

        unsigned int N = latestN;   // read N that ISR stored
        if (N == 0) continue;       // avoid division by zero

        // calculate frequency
        float f_new = 16000.0f / (float)N;

        // for testing task 1: DELETE
        printf("TaskFrequencyCalculation: N=%u, f=%.2fHz\n", N, f_new);

        // calculate ROCF
        float roc = 0.0f;
        if (!first_reading && N_old != 0) {
            float N_avg = (float)(N + N_old) / 2.0f;
            roc = (f_new - f_old) * (16000.0f / N_avg);
        }
            
        // build freqData struct
        freqData_t data = {
            .frequency = f_new,
            .roc = roc,
            .n = N,
            .timestamp = xTaskGetTickCount()
        };

        // send to queue (if queue is full, discard this reading)
        xQueueSend(freqDataQ, &data, 0);

        // save values for next ROCF calculation
        f_old = f_new;
        N_old = N;
        first_reading = 0; // clear first reading flag after initial read

    }
}