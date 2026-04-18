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
} freqData_t;

typedef enum {
	LOAD_OFF,		// switch is off
	LOAD_ON,		// switch is on
	LOAD_SHED		// relay has shed this load
} loadStatus_t;

typedef enum {
	SYSTEM_NORMAL,			
	SYSTEM_MAINTENANCE,		
	SYSTEM_MANAGING		
} systemState_t;

typedef struct {
	TickType_t recent[TIMING_LOG_SIZE];	// last 5 measurements
	TickType_t minTime;
	TickType_t maxTime;
	TickType_t avgTime;
	int count;								// total measurements taken
} timingLog_t;

typedef struct {
	float TF_threshold;
	float TROC_threshold;
	enum Threshold threshold_edit_mode;
	systemState_t system_state;
} system_status_t;

#endif  /* __TYPES_H_ */
