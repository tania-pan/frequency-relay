// task_freq_calc.c

#include <stdio.h>
#include "frequency_relay.h"
#include "FreeRTOS/FreeRTOS.h"

void TaskFrequencyCalculation(void *pvParameters) {
    float f_old = 0.0f;
    unsigned int n_old = 0;
    int first_reading = 1;  // flag to indicate first reading for ROCF calculation

    while (1) {
        // block until ISR gives semaphore
        xSemaphoreTake(peak_ready_sem, portMAX_DELAY);

        unsigned int n = freq_data.n;   // read N that ISR stored
        if (n == 0) continue;       // avoid division by zero

        // calculate frequency
        float f_new = 16000.0f / (float)n;

        // for testing task 1: DELETE
        printf("TaskFrequencyCalculation: N=%u, f=%.2fHz\n", n, f_new);

        // calculate ROCF
        float roc = 0.0f;
        if (!first_reading && n_old != 0) {
            float N_avg = (float)(n + n_old) / 2.0f;
            roc = (f_new - f_old) * (16000.0f / N_avg);
        }
            
        // build freqData struct
        freq_data_t data = {
            .frequency = f_new,
            .roc = roc,
            .n = n,
            .timestamp = xTaskGetTickCount()
        };

        // send to queue (if queue is full, discard this reading)
        xQueueSend(freq_data_q, &data, 0);

        // save values for next ROCF calculation
        f_old = f_new;
        n_old = n;
        first_reading = 0; // clear first reading flag after initial read

    }
}
