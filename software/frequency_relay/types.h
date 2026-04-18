// types.h

#ifndef __TYPES_H_
#define __TYPES_H_

// #include "../frequency_relay_bsp/HAL/inc/alt_types.h"
#include <stdint.h>
#include "config.h"
#include "FreeRTOS/FreeRTOS.h"
// #include "FreeRTOS/portmacro.h"

enum Threshold {TF, TROC};

typedef struct {
	float frequency;		// in Hz
	float roc; 				// rate of change of frequency in Hz/s
	unsigned int n;			// raw ADC sample count
	TickType_t timestamp;	// FreeRTOS tick count when measured
} freq_data_t;

typedef enum {
	LOAD_OFF,		// switch is off
	LOAD_ON,		// switch is on
	LOAD_SHED		// relay has shed this load
} load_status_t;

typedef enum {
	SYSTEM_NORMAL,			
	SYSTEM_MAINTENANCE,		
	SYSTEM_MANAGING		
} system_state_t;

typedef struct {
	TickType_t recent[TIMING_LOG_SIZE];	// last 5 measurements
	TickType_t min_time;
	TickType_t max_time;
	TickType_t avg_time;
	int count;								// total measurements taken
} timing_log_t;

typedef struct {
	float TF_threshold;
	float TROC_threshold;
	enum Threshold threshold_edit_mode;
	system_state_t system_state;
} system_status_t;

#endif  /* __TYPES_H_ */
