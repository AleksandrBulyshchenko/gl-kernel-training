#pragma once

typedef unsigned char	u8;
typedef unsigned short	u16;

#define SSD1306_WIDTH	128
#define SSD1306_HEIGHT	64

typedef struct{
	u16		X;
	u16		Y;
}_Point;

typedef enum {
		SSD1306_COLOR_BLACK = 0x00,   /*!< Black color, no pixel */
		SSD1306_COLOR_WHITE = 0x01	  /*!< Pixel is set. Color depends on LCD */
} ssd1306_COLOR_t;

typedef struct {
	struct fb_var_screeninfo vinfo;
	struct fb_fix_screeninfo finfo;
	int			width;
	int			height;
	long int	screensize;
	u8			*fbp;
} lcd_type;

extern lcd_type lcd;

int ssd1306_DrawPixel(u16 x, u16 y, ssd1306_COLOR_t color);
void Graphic_setPoint(const u16 X, const u16 Y);
void Graphic_drawLine(_Point p1, _Point p2);
void Graphic_drawLine_(u16 x1, u16 y1, u16 x2, u16 y2);
void Graphic_drawCircle(_Point center, u16 r);
void Graphic_ClearScreen(void);
void Graphic_UpdateScreen(void);
#ifndef PAGE_SHIFT
	#define PAGE_SHIFT 12
#endif
#ifndef PAGE_SIZE
	#define PAGE_SIZE (1UL << PAGE_SHIFT)
#endif
#ifndef PAGE_MASK
	#define PAGE_MASK (~(PAGE_SIZE - 1))
#endif

int fb_init(char *device);
void fb_cleanup(void);

