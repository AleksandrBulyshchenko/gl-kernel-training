#pragma once

#include <linux/ioctl.h>

/* ioctl()'s for the ssd1306 values read */

/* Get the entropy count. */
#define FB_UPDATESCREEN	0x7777 //_IO( 'W', 0x01 )
