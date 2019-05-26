#ifndef __SSD1306__H__
#define __SSD1306__H__
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#define DEVICE_NAME	"lcd_ssd1306"
#define CLASS_NAME	"oled_display" 
#define BUS_NAME	"i2c_1"

#define LCD_WIDTH		128
#define LCD_HEIGHT		64	

#define WRITECOMMAND	0x00 // 
#define WRITEDATA		0x40 //

#define COL_COUNT		128
#define PAGE_COUNT		8

#define SSD1306_SEND_COMMAND(c, d) i2c_smbus_write_byte_data((c), 0x00, (d))

#define SET_LOW_COLUMN			0x00
#define SET_HIGH_COLUMN			0x10
#define COLUMN_ADDR				0x21
#define PAGE_ADDR				0x22
#define SET_START_PAGE			0xB0
#define CHARGE_PUMP				0x8D
#define DISPLAY_OFF				0xAE
#define DISPLAY_ON				0xAF

#define MEMORY_MODE				0x20
#define SET_CONTRAST			0x81
#define SET_SEG_REMAP_1			0xA1
#define SET_ENTIRE_DISPL_ON		0xA4
#define SET_NORMAL_DISPLAY		0xA6
#define SET_INVERT_DISPLAY		0xA7
#define SET_MUX_RATIO			0xA8
#define SET_PAGE_NUM			0xB0
#define COM_SCAN_INC			0xC0
#define COM_SCAN_DEC			0xC8
#define SET_DISPLAY_OFFSET		0xD3
#define SET_DISPLAY_CLOCK_DIV	0xD5
#define SET_PRECHARGE			0xD9
#define SET_COM_PINS			0xDA
#define SET_VCOM_DETECT			0xDB

/* Memory modes */
enum {
	SSD1306_MEM_MODE_HORIZONAL = 0,
	SSD1306_MEM_MODE_VERTICAL,
	SSD1306_MEM_MODE_PAGE,
	SSD1306_MEM_MODE_INVALID
};

struct ssd1306_data {
    struct i2c_client *client;
	struct fb_info *info;
	struct work_struct display_update_ws;
	u32 height;
	u32 width;

	int value;

    int mem_mode;
};

typedef struct ssd1306_data_array
{
	u8      type;
    u8      data[LCD_WIDTH * LCD_HEIGHT / 8];
} ssd1306_data_array;

int ssd1306_UpdateScreen(struct ssd1306_data *drv_data);
int ssd1306_ON(struct ssd1306_data *drv_data);
int ssd1306_OFF(struct ssd1306_data *drv_data);
#endif

