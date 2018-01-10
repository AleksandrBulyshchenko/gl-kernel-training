
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/fb.h>

#include <linux/delay.h>
#include <linux/uaccess.h>

#include <linux/platform_device.h>
#include <linux/kthread.h>

#define CMD_ADDR			0

#define SET_LOW_COLUMN			0x00
#define SET_HIGH_COLUMN			0x10
#define COLUMN_ADDR			0x21
#define PAGE_ADDR			0x22
#define SET_START_PAGE			0xB0
#define CHARGE_PUMP			0x8D
#define DISPLAY_OFF			0xAE
#define DISPLAY_ON			0xAF

#define MEMORY_MODE			0x20
#define SET_CONTRAST			0x81
#define SET_NORMAL_DISPLAY		0xA6
#define SET_INVERT_DISPLAY		0xA7
#define COM_SCAN_INC			0xC0
#define COM_SCAN_DEC			0xC8
#define SET_DISPLAY_OFFSET		0xD3
#define SET_DISPLAY_CLOCK_DIV		0xD5
#define SET_PRECHARGE			0xD9
#define SET_COM_PINS			0xDA
#define SET_VCOM_DETECT			0xDB
#define SET_START_LINE			0x40

#define CHECK_STATUS_REG_VAL		0x43

#define MYDEVNAME    "i2c_ssd1306"
#define LCD_WIDTH    128
#define LCD_HEIGHT   64
#define LCD_BPP      8
#define SYNC_RATE_MS 50

struct my_device {
	struct i2c_client  *client;
	struct fb_info     *fb_info;
	struct task_struct *th;
	u8                 *vmem;
	size_t              vmsize;
	atomic_t            dirty;
};

static const struct i2c_device_id i2c_ssd1306_idtable[] = {
	{ MYDEVNAME, 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, i2c_ssd1306_idtable);

static int i2c_ssd1306_init(struct i2c_client *drv_client)
{
	s32 val;

	i2c_smbus_write_byte_data(drv_client, CMD_ADDR, DISPLAY_OFF);
	val = i2c_smbus_read_byte_data(drv_client, CMD_ADDR);

	dev_info(&drv_client->dev, "%s: status reg: %d\n", __func__, val);
	if (val != CHECK_STATUS_REG_VAL)
		return -1;

	i2c_smbus_write_byte_data(drv_client, CMD_ADDR, MEMORY_MODE);
	i2c_smbus_write_byte_data(drv_client, CMD_ADDR, 0);
	/* Set column start / end */
	i2c_smbus_write_byte_data(drv_client, CMD_ADDR, COLUMN_ADDR);
	i2c_smbus_write_byte_data(drv_client, CMD_ADDR, 0);
	i2c_smbus_write_byte_data(drv_client, CMD_ADDR, (LCD_WIDTH - 1));

	/* Set page start / end */
	i2c_smbus_write_byte_data(drv_client, CMD_ADDR, PAGE_ADDR);
	i2c_smbus_write_byte_data(drv_client, CMD_ADDR, 0);
	i2c_smbus_write_byte_data(drv_client, CMD_ADDR, (LCD_HEIGHT / 8 - 1));

	i2c_smbus_write_byte_data(drv_client, CMD_ADDR, COM_SCAN_DEC);

	i2c_smbus_write_byte_data(drv_client, CMD_ADDR, SET_CONTRAST);
	/* Max contrast */
	i2c_smbus_write_byte_data(drv_client, CMD_ADDR, 0xFF);

	/* set segment re-map 0 to 127 */
	i2c_smbus_write_byte_data(drv_client, CMD_ADDR, 0xA1);
	i2c_smbus_write_byte_data(drv_client, CMD_ADDR, SET_NORMAL_DISPLAY);
	/* set multiplex ratio(1 to 64) */
	i2c_smbus_write_byte_data(drv_client, CMD_ADDR, 0xA8);
	i2c_smbus_write_byte_data(drv_client, CMD_ADDR, (LCD_HEIGHT - 1));
	/* 0xA4 => follows RAM content */
	i2c_smbus_write_byte_data(drv_client, CMD_ADDR, 0xA4);
	i2c_smbus_write_byte_data(drv_client, CMD_ADDR, SET_DISPLAY_OFFSET);
	i2c_smbus_write_byte_data(drv_client, CMD_ADDR, 0x00);

	i2c_smbus_write_byte_data(drv_client, CMD_ADDR, SET_DISPLAY_CLOCK_DIV);
	i2c_smbus_write_byte_data(drv_client, CMD_ADDR, 0x80);

	i2c_smbus_write_byte_data(drv_client, CMD_ADDR, SET_PRECHARGE);
	i2c_smbus_write_byte_data(drv_client, CMD_ADDR, 0x22);

	i2c_smbus_write_byte_data(drv_client, CMD_ADDR, SET_COM_PINS);
	i2c_smbus_write_byte_data(drv_client, CMD_ADDR, 0x12);

	i2c_smbus_write_byte_data(drv_client, CMD_ADDR, SET_VCOM_DETECT);
	i2c_smbus_write_byte_data(drv_client, CMD_ADDR, 0x20);


	i2c_smbus_write_byte_data(drv_client, CMD_ADDR, CHARGE_PUMP);
	i2c_smbus_write_byte_data(drv_client, CMD_ADDR, 0x14);

	i2c_smbus_write_byte_data(drv_client, CMD_ADDR, DISPLAY_ON);

	return 0;
}

static inline void set_dirty(struct my_device *md)
{
	atomic_inc(&(md->dirty));
}

static ssize_t i2c_ssd1306_write(struct fb_info *info,
		const char __user *buf, size_t count, loff_t *ppos)
{
	struct i2c_client *drv_client;
	struct my_device *md = info->par;
	int cnt;

	if (!md)
		return -EINVAL;

	drv_client = md->client;
	dev_info(&drv_client->dev, "%s: enter\n", __func__);

	cnt = fb_sys_write(info, buf, count, ppos);

	if (cnt < 0)
		return cnt;

	set_dirty(md);

	return cnt;
}

static int i2c_ssd1306_blank(int blank_mode, struct fb_info *info)
{
	struct i2c_client *drv_client;
	struct my_device *md = info->par;

	if (!md)
		return -EINVAL;

	drv_client = md->client;
	dev_info(&drv_client->dev, "%s: enter\n", __func__);
	return -1;
}

static void i2c_ssd1306_fillrect(struct fb_info *info,
		const struct fb_fillrect *rect)
{
	struct i2c_client *drv_client;
	struct my_device *md = info->par;

	if (!md)
		return;

	drv_client = md->client;
	dev_info(&drv_client->dev, "%s: enter\n", __func__);
}

static void i2c_ssd1306_copyarea(struct fb_info *info,
		const struct fb_copyarea *area)
{
	struct i2c_client *drv_client;
	struct my_device *md = info->par;

	if (!md)
		return;

	drv_client = md->client;
	dev_info(&drv_client->dev, "%s: enter\n", __func__);
}

static void i2c_ssd1306_imageblit(struct fb_info *info,
		const struct fb_image *image)
{
	struct i2c_client *drv_client;
	struct my_device *md = info->par;

	if (!md)
		return;

	drv_client = md->client;
	dev_info(&drv_client->dev, "%s: enter\n", __func__);
}

static void copy_byte_to_bit_data(u8 *dst, u8 *src, int size)
{
	int i, j, k;
	u8 buf;

	for (i = 0; i < LCD_HEIGHT / 8; ++i) {
		for (j = 0; j < LCD_WIDTH; ++j) {
			for (buf = 0, k = 0; k < 8; ++k)
				buf |= src[
				j +
				k * LCD_WIDTH +
				i * LCD_WIDTH * 8] > 0x7f ? 1 << k : 0;
			*dst++ = buf;
		}
	}
}

static int update_display_thread(void *data)
{
	static u8 out[LCD_WIDTH * LCD_HEIGHT / 8 + 1];
	struct my_device *md = (struct my_device *) data;

	pr_info("thread started\n");
	do {
		if (atomic_xchg(&md->dirty, 0)) {
			if (sizeof(out) - 1 != md->vmsize / 8) {
				pr_err("error in code %d != %d\n",
					sizeof(out) - 1, md->vmsize / 8);
				break;
			}
			out[0] = SET_START_LINE + 0;
			copy_byte_to_bit_data(
					&out[1], md->vmem, md->vmsize);
			if (i2c_master_send(md->client, out, sizeof(out)) < 0) {
				pr_err("error send to device\n");
				break;
			}
		}
		msleep(SYNC_RATE_MS);
	} while (!kthread_should_stop());
	pr_info("exit from thread\n");
	return 0;
}

static struct fb_ops fb_ops = {
	.owner          = THIS_MODULE,
	.fb_read        = fb_sys_read,
	.fb_write       = i2c_ssd1306_write,
	.fb_blank       = i2c_ssd1306_blank,
	.fb_fillrect    = i2c_ssd1306_fillrect,
	.fb_copyarea    = i2c_ssd1306_copyarea,
	.fb_imageblit   = i2c_ssd1306_imageblit,
};

static int i2c_ssd1306_probe(struct i2c_client *drv_client,
			 const struct i2c_device_id *id)
{
	struct fb_info   *fb_info;
	struct my_device *md;
	u8               *vmem;
	size_t            vmem_size;
	int               err;

	dev_info(&drv_client->dev, "init I2C driver client address %d\n",
			drv_client->addr);

	if (i2c_ssd1306_init(drv_client)) {
		dev_info(&drv_client->dev, "I2C device not found\n");
		return -1;
	}

	vmem_size = LCD_WIDTH * LCD_HEIGHT * LCD_BPP / 8;
	vmem = vzalloc(vmem_size);
	if (!vmem) {
		dev_info(&drv_client->dev,
				"vzalloc error size: %d\n", vmem_size);
		return -ENOMEM;
	}

	fb_info = framebuffer_alloc(sizeof(struct my_device), &drv_client->dev);
	if (!fb_info) {
		dev_info(&drv_client->dev, "framebuffer_alloc error\n");
		err = -ENOMEM;
		goto error;
	}

	fb_info->fbops = &fb_ops;
	fb_info->screen_base = (u8 __force __iomem *)vmem;
	fb_info->fix.smem_start = __pa(vmem);
	fb_info->fix.smem_len = vmem_size;

	i2c_set_clientdata(drv_client, fb_info->par);

	md = fb_info->par;
	md->fb_info = fb_info;
	md->client = drv_client;
	md->vmsize = vmem_size;
	md->vmem = vmem;

	strncpy(fb_info->fix.id, MYDEVNAME, 16);
	fb_info->fix.type   = FB_TYPE_PACKED_PIXELS;
	fb_info->fix.visual = FB_VISUAL_MONO10;
	fb_info->fix.accel  = FB_ACCEL_NONE;
	fb_info->fix.line_length = LCD_WIDTH;

	fb_info->var.bits_per_pixel = LCD_BPP;
	fb_info->var.xres = LCD_WIDTH;
	fb_info->var.xres_virtual = LCD_WIDTH;
	fb_info->var.yres = LCD_HEIGHT;
	fb_info->var.yres_virtual = LCD_HEIGHT;

	fb_info->var.red.length = 1;
	fb_info->var.red.offset = 0;
	fb_info->var.green.length = 1;
	fb_info->var.green.offset = 0;
	fb_info->var.blue.length = 1;
	fb_info->var.blue.offset = 0;

	err = register_framebuffer(fb_info);
	if (err) {
		dev_err(&drv_client->dev, "Couldn't register the framebuffer\n");
		goto error;
	}

	md->th = kthread_run(update_display_thread, md, "i2c_ssd1306_thread");
	dev_info(&drv_client->dev, "%s: thread %p started\n", __func__, md->th);

	dev_info(&drv_client->dev, "ssd1306 driver successfully loaded\n");
	return 0;

error:
	vfree(vmem);
	return err;
}

static int i2c_ssd1306_remove(struct i2c_client *drv_client)
{
	struct my_device *md = i2c_get_clientdata(drv_client);

	kthread_stop(md->th);
	unregister_framebuffer(md->fb_info);

	dev_info(&drv_client->dev, "ssd1306 driver successfully removed\n");
	return 0;
}

static struct i2c_driver i2c_ssd1306_driver = {
	.driver = {
		.name = MYDEVNAME,
	},

	.probe = i2c_ssd1306_probe,
	.remove = i2c_ssd1306_remove,
	.id_table = i2c_ssd1306_idtable,
};

module_i2c_driver(i2c_ssd1306_driver);

MODULE_AUTHOR("Dmytro Kirtoka <dimk334@gmail.com>");
MODULE_DESCRIPTION("tft lcd b&w screen driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");
