// vga.h

#ifndef __VGA_H_
#define __VGA_H_

#include "types.h"

#define PS2_1 0x16
#define PS2_2 0x1E
#define PS2_UP 0x75
#define PS2_DOWN 0x72
#define PS2_BREAK 0xF0

enum Threshold {TF, TROC};

vga_display_task(void *pvParameters);

#endif  /* __VGA_H_ */


