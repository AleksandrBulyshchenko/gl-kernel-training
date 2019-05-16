#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/input.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/err.h>

#include <linux/fb.h>
#include <linux/uaccess.h>

#include "ssd1306.h"

MODULE_AUTHOR("Oleksii Kogutenko <oleksii.kogutenko@gmail.com>");
MODULE_DESCRIPTION("ssd1306 I2C");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");

/* --------------------------------------------------------------- */
static struct device *dev;
/*
 *  struct fb_fix_screeninfo {
 *  	char id[16];			// identification string eg "TT Builtin" 
 *  	unsigned long smem_start;	// Start of frame buffer mem 
 *										// (physical address) 
 *		__u32 smem_len;			// Length of frame buffer mem 
 *		__u32 type;				// see FB_TYPE_*
 *		__u32 type_aux;			// Interleave for interleaved Planes 
 *		__u32 visual;			// see FB_VISUAL_*
 *		__u16 xpanstep;			// zero if no hardware panning
 *		__u16 ypanstep;			// zero if no hardware panning
 *		__u16 ywrapstep;		// zero if no hardware ywrap
 *		__u32 line_length;		// length of a line in bytes
 *		unsigned long mmio_start;	// Start of Memory Mapped I/O
 *									// (physical address)
 *		_u32 mmio_len;			// Length of Memory Mapped I/O
 *		_u32 accel;				// Indicate to driver which
 *								//  specific chip/card we have
 *		__u16 capabilities;		// see FB_CAP_*
 *		__u16 reserved[2];		// Reserved for future compatibility
 *	};
 * */
static struct fb_fix_screeninfo ssd1307fb_fix = {
	.id     	= "Solomon SSD1307",
	.type       = FB_TYPE_PACKED_PIXELS,
	.visual     = FB_VISUAL_MONO10,
	.xpanstep   = 0,
	.ypanstep   = 0,
	.ywrapstep  = 0,
	.accel      = FB_ACCEL_NONE,
};

static struct fb_var_screeninfo ssd1307fb_var = {
	.bits_per_pixel = 1,
};
/* --------------------------------------------------------------- */
ssd1306_data_array dataArray;
ssd1306_data_array dataArray_lcd;
static u8 ssd1306_first_start;

/* SSD1306 data buffer */
u8 *ssd1306_Buffer = &dataArray.data[0];
static u8 *lcd_vmem;
/* --------------------------------------------------------------- */
static const struct of_device_id ssd1306_match[] = {
	{ .compatible = "DAndy,lcd_ssd1306", },
	{ },
};
MODULE_DEVICE_TABLE(of, ssd1306_match);
/* --------------------------------------------------------------- */
static const struct i2c_device_id ssd1306_id[] = {
	{ "lcd_ssd1306", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ssd1306_id);
/* --------------------------------------------------------------- */
static struct i2c_driver ssd1306_driver = {
	.driver = {
		.name	= DEVICE_NAME,
		.of_match_table = ssd1306_match,
	},
	.probe		= ssd1306_probe,
	.remove 	= ssd1306_remove,
	.id_table	= ssd1306_id,
};
/* --------------------------------------------------------------- */
static struct fb_ops ssd1307fb_ops = {
	.owner          = THIS_MODULE,
	.fb_read        = fb_sys_read,
	.fb_write       = ssd1307fb_write,
	.fb_blank       = ssd1307fb_blank,
	.fb_fillrect    = ssd1307fb_fillrect,
	.fb_copyarea    = ssd1307fb_copyarea,
	.fb_imageblit   = ssd1307fb_imageblit,
};
/* --------------------------------------------------------------- */
u8 ssd1306_init_cmd[] = {
	/* Init LCD */ 
	WRITECOMMAND, DISPLAY_OFF,

	WRITECOMMAND, MEMORY_MODE, 
	WRITECOMMAND, SSD1306_MEM_MODE_PAGE,
	/* Set column start / end */
	WRITECOMMAND, COLUMN_ADDR,
	WRITECOMMAND, 0,
	WRITECOMMAND, (LCD_WIDTH - 1),
	/* Set page start / end */
	WRITECOMMAND, PAGE_ADDR,
	WRITECOMMAND, 0,
	WRITECOMMAND, (LCD_HEIGHT / 8 - 1),

	WRITECOMMAND, COM_SCAN_DEC,

	WRITECOMMAND, SET_CONTRAST,
	WRITECOMMAND, 0xFF,	/* Max contrast */

	WRITECOMMAND, SET_SEG_REMAP_1, /* set segment re-map 0 to 127 */
	WRITECOMMAND, SET_NORMAL_DISPLAY,
	WRITECOMMAND, SET_MUX_RATIO, /* set multiplex ratio(1 to 64) */
	WRITECOMMAND, (LCD_HEIGHT - 1),
	WRITECOMMAND, SET_ENTIRE_DISPL_ON, /* 0xA4 => follows RAM content */
	WRITECOMMAND, SET_DISPLAY_OFFSET,
	WRITECOMMAND, 0x00,  /* no offset */
	WRITECOMMAND, SET_DISPLAY_CLOCK_DIV,
	WRITECOMMAND, 0x80, /* set divide ratio */
	WRITECOMMAND, SET_PRECHARGE,
	WRITECOMMAND, 0x22,
	WRITECOMMAND, SET_COM_PINS,
	WRITECOMMAND, 0x12,
	WRITECOMMAND, SET_VCOM_DETECT,
	WRITECOMMAND, 0x20 /* 0x20, 0.77x Vcc */
};

u8 ssd1306_data_display_on[] = {
	WRITECOMMAND, CHARGE_PUMP,
	WRITECOMMAND, 0x14, /*Enable Charge Pump*/
	WRITECOMMAND, DISPLAY_ON
};

u8 ssd1306_data_display_off[] = {
	WRITECOMMAND, CHARGE_PUMP,
	WRITECOMMAND, 0x10, /*Disable Charge Pump*/
	WRITECOMMAND, DISPLAY_OFF
};

/* ******************************************************************
 *  Init sequence taken from the Adafruit SSD1306 Arduino library 
 * *****************************************************************/
static void ssd1306_init_lcd(struct i2c_client *drv_client)
{
    dev_info(dev, "ssd1306: Device init \n");
    /* Init LCD */    
	SSD1306_SEND_COMMAND(drv_client, DISPLAY_OFF);

	SSD1306_SEND_COMMAND(drv_client, MEMORY_MODE);
	SSD1306_SEND_COMMAND(drv_client, SSD1306_MEM_MODE_PAGE);
	/* Set column start / end */
	SSD1306_SEND_COMMAND(drv_client, COLUMN_ADDR);
	SSD1306_SEND_COMMAND(drv_client, 0);
	SSD1306_SEND_COMMAND(drv_client, (LCD_WIDTH - 1));

	/* Set page start / end */
	SSD1306_SEND_COMMAND(drv_client, PAGE_ADDR);
	SSD1306_SEND_COMMAND(drv_client, 0);
	SSD1306_SEND_COMMAND(drv_client, (LCD_HEIGHT / 8 - 1));

	SSD1306_SEND_COMMAND(drv_client, COM_SCAN_DEC);

	SSD1306_SEND_COMMAND(drv_client, SET_CONTRAST);
	SSD1306_SEND_COMMAND(drv_client, 0xFF); /* Max contrast */

	SSD1306_SEND_COMMAND(drv_client, 0xA1); /* set segment re-map 0 to 127 */
	SSD1306_SEND_COMMAND(drv_client, SET_NORMAL_DISPLAY);
	SSD1306_SEND_COMMAND(drv_client, 0xA8); /* set multiplex ratio(1 to 64) */
	SSD1306_SEND_COMMAND(drv_client, (LCD_HEIGHT - 1));
	SSD1306_SEND_COMMAND(drv_client, 0xA4); /* 0xA4 => follows RAM content */
	SSD1306_SEND_COMMAND(drv_client, SET_DISPLAY_OFFSET);
	SSD1306_SEND_COMMAND(drv_client, 0x00); /* no offset */

	SSD1306_SEND_COMMAND(drv_client, SET_DISPLAY_CLOCK_DIV);
	SSD1306_SEND_COMMAND(drv_client, 0x80); /* set divide ratio */

	SSD1306_SEND_COMMAND(drv_client, SET_PRECHARGE);
	SSD1306_SEND_COMMAND(drv_client, 0x22);

	SSD1306_SEND_COMMAND(drv_client, SET_COM_PINS);
	SSD1306_SEND_COMMAND(drv_client, 0x12);

	SSD1306_SEND_COMMAND(drv_client, SET_VCOM_DETECT);
	SSD1306_SEND_COMMAND(drv_client, 0x20); /* 0x20, 0.77x Vcc */

	SSD1306_SEND_COMMAND(drv_client, 0x8D);
	SSD1306_SEND_COMMAND(drv_client, 0x14);
	SSD1306_SEND_COMMAND(drv_client, 0xAF);
}
/* ******************************************************************
 *
 * *****************************************************************/
int ssd1306_UpdateScreen_old(struct ssd1306_data *drv_data)
{

    struct i2c_client *drv_client;
	int ret;
	u64 jiff, diff;
	jiff = get_jiffies_64();

    drv_client = drv_data->client;
    
    dataArray.type = 0x40;
	ret = i2c_master_send(drv_client, (u8 *) &dataArray, sizeof(ssd1306_data_array));
	if (unlikely(ret < 0))
	{
		printk(KERN_ERR "i2c_master_send() has returned ERROR %d\n", ret);
		return ret;
	}
	
	diff = get_jiffies_64() - jiff;
	pr_info ("diff: %lld\n", diff); ///diff: 6

    return 0;
}
/* ******************************************************************
 *
 * *****************************************************************/
int ssd1306_sendDataArea(struct i2c_client *drv_client, u8 *data, int size, int pos) 
{
	u8 send_data[COL_COUNT + 1];
	u8 page, col;
	int send_size, i = 0;
	int ret;

	pr_info("[%s] size:%d pos:%d\n", __func__, size, pos);
	while (i < size) {
		page = (pos + i) / COL_COUNT;
		col  = (pos + i) % COL_COUNT;
		SSD1306_SEND_COMMAND(drv_client, SET_PAGE_NUM    | page);
		SSD1306_SEND_COMMAND(drv_client, SET_LOW_COLUMN  | (col & 0x0F));
		SSD1306_SEND_COMMAND(drv_client, SET_HIGH_COLUMN | ((col >> 4) & 0x0F));
		send_size = COL_COUNT - col;
		if (send_size > size)
			send_size = size;
		*send_data = WRITEDATA;
		memcpy(send_data + 1, data + pos + i, send_size);
		pr_info("[%s] page:%d, col:%d, send_size:%d, i:%d\n", __func__, page, col, send_size, i);
		ret = i2c_master_send(drv_client, send_data, send_size + 1);
		if (unlikely(ret < 0))
		{
			pr_err("i2c_master_send() has returned ERROR %d\n", ret);
			return ret;
		}
		i += send_size;
	}
	return 0;
}
/* ******************************************************************
 *
 * *****************************************************************/
int ssd1306_UpdateScreen(struct ssd1306_data *drv_data)
{

    struct i2c_client *drv_client;
	int ret;
	u64 jiff, diff;
	int cur_pos = 0, size = 0, msize = LCD_WIDTH * LCD_HEIGHT / 8;
	int start_pos = 0;

	jiff = get_jiffies_64();

    drv_client = drv_data->client;
    
	if (0 == ssd1306_first_start) {
		ssd1306_sendDataArea(drv_client, dataArray.data, sizeof(dataArray.data), 0);
		memcpy(&dataArray_lcd, &dataArray, sizeof(dataArray));
		ssd1306_first_start++;
	} else {
		while (cur_pos < msize) {
			while (cur_pos < msize && dataArray.data[cur_pos] == dataArray_lcd.data[cur_pos]) 
				cur_pos++;
			start_pos = cur_pos;
			while (cur_pos < msize && dataArray.data[cur_pos] != dataArray_lcd.data[cur_pos]) {
				dataArray_lcd.data[cur_pos] = dataArray.data[cur_pos];
				cur_pos++;
			}
			size = cur_pos - start_pos;

			//pr_info("[%s]==>[cur_pos:%d, start_pos:%d, size:%d]\n", __func__, cur_pos, start_pos, size);

			if (size > 0 && start_pos < msize) {
				ret = ssd1306_sendDataArea(drv_client, dataArray.data, size, start_pos);
				if (unlikely(ret < 0))
					return ret;
			}
		}
		
		//ssd1306_sendDataArea(drv_client, dataArray.data, sizeof(dataArray.data), 0);
		//memcpy(&dataArray_lcd, &dataArray, sizeof(dataArray));
	}

	diff = get_jiffies_64() - jiff;
	pr_info ("diff: %lld\n", diff); ///diff: 6

	return 0;
}
/* ******************************************************************
 *
 * *****************************************************************/
int ssd1306_ON(struct ssd1306_data *drv_data)
{
	struct i2c_client *drv_client;

	drv_client = drv_data->client;

	i2c_smbus_write_byte_data(drv_client, 0x00, 0x8D);
	i2c_smbus_write_byte_data(drv_client, 0x00, 0x14);
	i2c_smbus_write_byte_data(drv_client, 0x00, 0xAF);
	return 0;
}
/* ******************************************************************
 *
 * *****************************************************************/
int ssd1306_OFF(struct ssd1306_data *drv_data)
{
	struct i2c_client *drv_client;

	drv_client = drv_data->client;

	i2c_smbus_write_byte_data(drv_client, 0x00, 0x8D);
	i2c_smbus_write_byte_data(drv_client, 0x00, 0x10);
	i2c_smbus_write_byte_data(drv_client, 0x00, 0xAE);
	return 0;
}
/* ******************************************************************
 *
 * *****************************************************************/
static void ssd1307fb_update_display(struct ssd1306_data *lcd)
{
	u8 *vmem = lcd->info->screen_base;
	int i, j, k;

	for (i = 0; i < (lcd->height / 8); i++) {
		for (j = 0; j < lcd->width; j++) {
			u32 array_idx = i * lcd->width + j;
			ssd1306_Buffer[array_idx] = 0;
			for (k = 0; k < 8; k++) {
				u32 page_length = lcd->width * i;
				u32 index = page_length + (lcd->width * k + j) / 8;
				u8 byte = *(vmem + index);
				u8 bit = byte & (1 << (j % 8));
				bit = bit >> (j % 8);
				ssd1306_Buffer[array_idx] |= bit << k;
			}
		}
	}

	ssd1306_UpdateScreen(lcd);
}
/* ******************************************************************
 *
 * *****************************************************************/
static void update_display_work(struct work_struct *work)
{
	struct ssd1306_data *lcd =
		container_of(work, struct ssd1306_data, display_update_ws);
	ssd1307fb_update_display(lcd);
}
/* ******************************************************************
 *
 * *****************************************************************/
static ssize_t ssd1307fb_write(struct fb_info *info, const char __user *buf,
		size_t count, loff_t *ppos)
{
    struct ssd1306_data *lcd = info->par;
	unsigned long total_size;
	unsigned long p = *ppos;
	u8 __iomem *dst;

	total_size = info->fix.smem_len;

	if (p > total_size)
		return -EINVAL;

	if (count + p > total_size)
		count = total_size - p;

	if (!count)
		return -EINVAL;

	dst = (void __force *) (info->screen_base + p);

	if (copy_from_user(dst, buf, count))
		return -EFAULT;

	schedule_work(&lcd->display_update_ws);

	*ppos += count;

	return count;
}
/* ******************************************************************
 *
 * *****************************************************************/
static int ssd1307fb_blank(int blank_mode, struct fb_info *info)
{
    struct ssd1306_data *lcd = info->par;

    if (blank_mode != FB_BLANK_UNBLANK)
    	return ssd1306_OFF(lcd->client);
    else
    	return ssd1306_ON(lcd->client);
}
/* ******************************************************************
 *
 * *****************************************************************/
static void ssd1307fb_fillrect(struct fb_info *info, const struct fb_fillrect *rect)
{
    struct ssd1306_data *lcd = info->par;
	sys_fillrect(info, rect);
	schedule_work(&lcd->display_update_ws);
}
/* ******************************************************************
 *
 * *****************************************************************/
static void ssd1307fb_copyarea(struct fb_info *info, const struct fb_copyarea *area)
{
    struct ssd1306_data *lcd = info->par;
	sys_copyarea(info, area);
	schedule_work(&lcd->display_update_ws);
}
/* ******************************************************************
 *
 * *****************************************************************/
static void ssd1307fb_imageblit(struct fb_info *info, const struct fb_image *image)
{
    struct ssd1306_data *lcd = info->par;
	sys_imageblit(info, image);
	schedule_work(&lcd->display_update_ws);
}
/* ******************************************************************
 *
 * *****************************************************************/
static int ssd1306_probe(struct i2c_client *drv_client,
			const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter;
	struct fb_info *info;
	struct ssd1306_data *lcd;
	u32 vmem_size;
	u8 *vmem;
	int ret;

	ssd1306_first_start = 0;
	dev = &drv_client->dev;

	dev_info(dev, "init I2C driver\n");

	info = framebuffer_alloc(sizeof(struct ssd1306_data), &drv_client->dev);

	if (!info)
		return -ENOMEM;

	lcd = info->par;
	lcd->info = info;
	lcd->client = drv_client;

	i2c_set_clientdata(drv_client, lcd);

	adapter = drv_client->adapter;

	if (!adapter)
	{
		dev_err(dev, "adapter indentification error\n");
		return -ENODEV;
	}

	dev_info(dev, "I2C client address %d \n", drv_client->addr);

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
			dev_err(dev, "operation not supported\n");
			return -ENODEV;
	}

	lcd->info = info;

	lcd->width  = LCD_WIDTH;
	lcd->height = LCD_HEIGHT;

	vmem_size = lcd->width * lcd->height / 8;
	
    lcd_vmem = devm_kzalloc(&drv_client->dev, vmem_size, GFP_KERNEL);
	
    if (!lcd_vmem) {
		dev_err(dev, "Couldn't allocate graphical memory.\n");
		return -ENOMEM;        
	}
	
	vmem = lcd_vmem;//&ssd1306_Buffer[0];

	info->fbops     = &ssd1307fb_ops;
	info->fix       = ssd1307fb_fix;
	info->fix.line_length = lcd->width / 8;

	info->var = ssd1307fb_var;
	info->var.xres = lcd->width;
	info->var.xres_virtual = lcd->width;
	info->var.yres = lcd->height;
	info->var.yres_virtual = lcd->height;

	info->var.red.length = 1;
	info->var.red.offset = 0;
	info->var.green.length = 1;
	info->var.green.offset = 0;
	info->var.blue.length = 1;
	info->var.blue.offset = 0;

	info->screen_base = (u8 __force __iomem *)vmem;
	info->fix.smem_start = __pa(vmem);
	info->fix.smem_len = vmem_size;

	i2c_set_clientdata(drv_client, info);

	ssd1306_init_lcd(drv_client);
	INIT_WORK(&lcd->display_update_ws, update_display_work);

	ret = register_framebuffer(info);
	if (ret) {
		dev_err(dev, "Couldn't register the framebuffer\n");
		return ret;
		//goto panel_init_error;
	}    

	dev_info(dev, "ssd1306 driver successfully loaded\n");
	dev_info(dev, "DAndy fb%d: %s device registered, using %d bytes of video memory\n", info->node, info->fix.id, vmem_size);

	return 0;
}
/* ******************************************************************
 *
 * *****************************************************************/
static int ssd1306_remove(struct i2c_client *drv_client)
{
	struct fb_info *info = i2c_get_clientdata(drv_client);
    struct ssd1306_data *lcd = info->par;

	cancel_work_sync(&lcd->display_update_ws);

	unregister_framebuffer(info);

	dev_info(dev, "Goodbye, world!\n");
	return 0;
}
/* ******************************************************************
 *
 * *****************************************************************/
/**
 * module_i2c_driver() - Helper macro for registering a modular I2C driver
 * @__i2c_driver: i2c_driver struct
 *
 * Helper macro for I2C drivers which do not do anything special in module
 * init/exit. This eliminates a lot of boilerplate. Each module may only
 * use this macro once, and calling it replaces module_init() and module_exit()
 */
module_i2c_driver(ssd1306_driver);

