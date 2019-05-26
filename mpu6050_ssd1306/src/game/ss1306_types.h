#pragma once

#define SSD1306_WIDTH   128
#define SSD1306_HEIGHT  64

typedef unsigned char	u8;
typedef unsigned short	u16;
typedef signed short	s16;

typedef enum {
        SSD1306_COLOR_BLACK = 0x00,   /*!< Black color, no pixel */
        SSD1306_COLOR_WHITE = 0x01	  /*!< Pixel is set. Color depends on LCD */
} ssd1306_COLOR_t;

#ifndef PAGE_SHIFT
    #define PAGE_SHIFT 12
#endif
#ifndef PAGE_SIZE
    #define PAGE_SIZE (1UL << PAGE_SHIFT)
#endif
#ifndef PAGE_MASK
    #define PAGE_MASK (~(PAGE_SIZE - 1))
#endif

