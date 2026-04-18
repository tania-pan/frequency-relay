// isr.h

#ifndef __ISR_H_
#define __ISR_H_

#include "../frequency_relay_bsp/HAL/inc/alt_types.h"
#include "types.h"

void fau_isr(void* context, alt_u32 id);
void button_isr(void* context, alt_u32 id);
void kbd_isr(void* context, alt_u32 id);

#endif  /* __ISR_H_ */
