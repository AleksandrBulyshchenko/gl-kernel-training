#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/input.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/err.h>

#include <linux/fb.h>
#include <linux/uaccess.h>

#include "ssd1306_internal.h"

#define SSD1306_DRIVER_ID                           "gl_ssd1306"

struct ssd1306_ctx {
	struct work_struct display_ws;
	struct i2c_client *client;
	struct fb_info *info;
	u8 *vmem;
};

static struct fb_fix_screeninfo ssd1306_fb_fix_screeninfo = {
	.id          = "Solomon SSD1306",
	.type        = FB_TYPE_PACKED_PIXELS,
	.visual      = FB_VISUAL_MONO10,
	.xpanstep    = 0,
	.ypanstep    = 0,
	.ywrapstep   = 0,
	.accel       = FB_ACCEL_NONE,
	.line_length = SSD1306_DISPLAY_WIDTH / 8,
};

static struct fb_var_screeninfo ssd1306_fb_var_screeninfo = {
	.xres = SSD1306_DISPLAY_WIDTH,
	.yres = SSD1306_DISPLAY_HEIGHT,

	.xres_virtual = SSD1306_DISPLAY_WIDTH,
	.yres_virtual = SSD1306_DISPLAY_HEIGHT,

	.red.length   = 1,
	.blue.length  = 1,
	.green.length = 1,

	.bits_per_pixel = 1,
};

static inline int ssd1306_write_cmd_byte(struct ssd1306_ctx *ctx, u32 cmd)
{
	return i2c_smbus_write_byte_data(ctx->client, SSD1306_WRITE_CMD, cmd);
}

static inline int ssd1306_write_buf(struct ssd1306_ctx *ctx, u8 *buf, size_t len)
{
	return i2c_master_send(ctx->client, buf, len);
}

static int ssd1306_display_on(struct ssd1306_ctx *ctx)
{
	ssd1306_write_cmd_byte(ctx, SSD1306_CMD_CHARGE_PUMP_SETTING);
	ssd1306_write_cmd_byte(ctx, SSD1306_CMD_CHARGE_ENABLE);
	ssd1306_write_cmd_byte(ctx, SSD1306_CMD_DISPLAY_ON);
	return 0;
}

static int ssd1306_display_off(struct ssd1306_ctx *ctx)
{
	ssd1306_write_cmd_byte(ctx, SSD1306_CMD_CHARGE_PUMP_SETTING);
	ssd1306_write_cmd_byte(ctx, 0x10); /* ??? */
	ssd1306_write_cmd_byte(ctx, SSD1306_CMD_DISPLAY_OFF);
	return 0;
}

static int ssd1306_blank(int blank_mode, struct fb_info *info)
{
	struct ssd1306_ctx *ctx = info->par;

	if (blank_mode != FB_BLANK_UNBLANK)
		return ssd1306_display_off(ctx);
	else
		return ssd1306_display_on(ctx);
}

static void ssd1306_display_update(struct ssd1306_ctx *ctx)
{
	u8 *vmem = ctx->info->screen_base;
	u8 *buf, *ptr;
	int i, j, k;
	const size_t buf_len = (SSD1306_DISPLAY_WIDTH * SSD1306_DISPLAY_HEIGHT / 8) + 1;

	ptr = buf = kzalloc(buf_len, GFP_KERNEL);
	if (!buf)
		return;

	/* Set command type */
	buf[0] = SSD1306_DATA;
	buf++;

	for (i = 0; i < (SSD1306_DISPLAY_HEIGHT / 8); i++) {
		for (j = 0; j < SSD1306_DISPLAY_WIDTH; j++) {
			u32 pos = i * SSD1306_DISPLAY_WIDTH + j;

			buf[pos] = 0;
			for (k = 0; k < 8; k++) {
				u32 page_length = SSD1306_DISPLAY_WIDTH * i;
				u32 index = page_length + (SSD1306_DISPLAY_WIDTH * k + j) / 8;
				u8 byte = *(vmem + index);
				u8 bit = byte & (1 << (j % 8));

				bit = bit >> (j % 8);
				buf[pos] |= bit << k;
			}
		}
	}

	ssd1306_write_buf(ctx, ptr, buf_len);
	kfree(ptr);
}

static void ssd1306_work_display_update(struct work_struct *work)
{
	struct ssd1306_ctx *ctx = container_of(work, struct ssd1306_ctx, display_ws);

	ssd1306_display_update(ctx);
}

static void ssd1306_schedule_display_update(struct ssd1306_ctx *ctx)
{
	schedule_work(&ctx->display_ws);
}

static void ssd1306_display_init(struct ssd1306_ctx *ctx)
{
	ssd1306_display_off(ctx);

	/* Memory mode */
	ssd1306_write_cmd_byte(ctx, SSD1306_DISPLAY_MEMORY_MODE);
	ssd1306_write_cmd_byte(ctx, SSD1306_DISPLAY_MEMORY_MODE_VAL_HORIZONAL);

	/* Set column start / end */
	ssd1306_write_cmd_byte(ctx, SSD1306_DISPLAY_COLUMN_ADDR);
	ssd1306_write_cmd_byte(ctx, 0);
	ssd1306_write_cmd_byte(ctx, (SSD1306_DISPLAY_WIDTH - 1));

	/* Set page start / end */
	ssd1306_write_cmd_byte(ctx, SSD1306_DISPLAY_PAGE_ADDR);
	ssd1306_write_cmd_byte(ctx, 0);
	ssd1306_write_cmd_byte(ctx, (SSD1306_DISPLAY_HEIGHT / 8 - 1));

	ssd1306_write_cmd_byte(ctx, SSD1306_CMD_SCAN_DEC);

	/* Max contrast */
	ssd1306_write_cmd_byte(ctx, SSD1306_CMD_SET_CONTRAST);
	ssd1306_write_cmd_byte(ctx, 0xFF);

	/* set segment re-map 0 to 127 */
	ssd1306_write_cmd_byte(ctx, 0xA1);
	ssd1306_write_cmd_byte(ctx, SSD1306_CMD_SET_NORMAL_DISPLAY);

	/* set multiplex ratio(1 to 64) */
	ssd1306_write_cmd_byte(ctx, 0xA8);
	ssd1306_write_cmd_byte(ctx, (SSD1306_DISPLAY_HEIGHT - 1));

	/* 0xA4 => follows RAM content */
	ssd1306_write_cmd_byte(ctx, 0xA4);

	ssd1306_write_cmd_byte(ctx, SSD1306_CMD_SET_DISPLAY_OFFSET);
	ssd1306_write_cmd_byte(ctx, 0x00);

	/* set divide ratio */
	ssd1306_write_cmd_byte(ctx, SSD1306_CMD_SET_DISPLAY_CLOCK_DIV);
	ssd1306_write_cmd_byte(ctx, 0x80);

	/* set precharge period in number of ticks from the internal clock */
	ssd1306_write_cmd_byte(ctx, SSD1306_CMD_SET_PRECHARGE);
	ssd1306_write_cmd_byte(ctx, 0x22);

	ssd1306_write_cmd_byte(ctx, SSD1306_CMD_SET_COM_PINS);
	ssd1306_write_cmd_byte(ctx, 0x12);

	ssd1306_write_cmd_byte(ctx, SSD1306_CMD_SET_VCOM_DETECT);
	ssd1306_write_cmd_byte(ctx, 0x20);

	ssd1306_schedule_display_update(ctx);
	ssd1306_display_on(ctx);
}

static void ssd1306_deferred_io(struct fb_info *info, struct list_head *pagelist)
{
	struct ssd1306_ctx *ctx = info->par;

	ssd1306_display_update(ctx);
}

static void ssd1306_fillrect(struct fb_info *info, const struct fb_fillrect *rect)
{
	struct ssd1306_ctx *ctx = info->par;

	sys_fillrect(info, rect);
	ssd1306_schedule_display_update(ctx);
}

static void ssd1306_copyarea(struct fb_info *info, const struct fb_copyarea *area)
{
	struct ssd1306_ctx *ctx = info->par;

	sys_copyarea(info, area);
	ssd1306_schedule_display_update(ctx);
}

static void ssd1306_imageblit(struct fb_info *info, const struct fb_image *image)
{
	struct ssd1306_ctx *ctx = info->par;

	sys_imageblit(info, image);
	ssd1306_schedule_display_update(ctx);
}

static struct fb_ops ssd1306_fb_ops = {
	.owner        = THIS_MODULE,
	.fb_read      = fb_sys_read,
	.fb_write     = fb_sys_write,
	.fb_blank     = ssd1306_blank,
	.fb_fillrect  = ssd1306_fillrect,
	.fb_copyarea  = ssd1306_copyarea,
	.fb_imageblit = ssd1306_imageblit,
};

__maybe_unused static struct fb_deferred_io ssd1306_fb_deferred_io = {
	.delay       = HZ / 10, /* 100 ms */
	.deferred_io = ssd1306_deferred_io,
};

static int ssd1306_probe(struct i2c_client *drv_client, const struct i2c_device_id *id)
{
	struct fb_info *info;
	struct ssd1306_ctx *ctx;
	struct device *dev = &drv_client->dev;
	u32 vmem_size;
	int ret;

	dev_info(dev, "init I2C driver\n");

	info = framebuffer_alloc(sizeof(struct ssd1306_ctx), &drv_client->dev);
	if (!info)
		return -ENOMEM;

	/* Setup context */
	ctx = info->par;
	ctx->info = info;
	ctx->client = drv_client;

	/* Save it for later use */
	i2c_set_clientdata(drv_client, ctx);

	if (!drv_client->adapter) {
		dev_err(dev, "adapter indentification error\n");
		ret = -ENODEV;
		goto release;
	}

	dev_info(dev, "I2C client address [%#x]\n", drv_client->addr);
	if (!i2c_check_functionality(drv_client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_err(dev, "operation not supported: I2C_FUNC_SMBUS_BYTE_DATA\n");
		ret = -ENODEV;
		goto release;
	}

	vmem_size = SSD1306_DISPLAY_WIDTH * SSD1306_DISPLAY_HEIGHT / 8;
	ctx->vmem = (void *)__get_free_pages(GFP_KERNEL | __GFP_ZERO,
					get_order(vmem_size));
	if (!ctx->vmem) {
		ret = -ENOMEM;
		goto release;
	}

	info->fbops   = &ssd1306_fb_ops;
	info->fbdefio = &ssd1306_fb_deferred_io;
	info->fix     = ssd1306_fb_fix_screeninfo;
	info->var     = ssd1306_fb_var_screeninfo;

	info->screen_base = (u8 __force __iomem *)ctx->vmem;
	info->fix.smem_start = __pa(ctx->vmem);
	info->fix.smem_len = vmem_size;

	fb_deferred_io_init(info);

	INIT_WORK(&ctx->display_ws, ssd1306_work_display_update);
	ssd1306_display_init(ctx);

	ret = register_framebuffer(info);
	if (ret)
		goto error;

	dev_info(dev, "loaded\n");
	dev_info(dev, "fb%d: %s device registered, using %d bytes of video memory\n", info->node, info->fix.id, vmem_size);
	return 0;

error:
	devm_kfree(dev, ctx->vmem);

release:
	framebuffer_release(info);
	return ret;
}

static int ssd1306_remove(struct i2c_client *drv_client)
{
	struct ssd1306_ctx *ctx = i2c_get_clientdata(drv_client);

	unregister_framebuffer(ctx->info);
	cancel_work_sync(&ctx->display_ws);
	fb_deferred_io_cleanup(ctx->info);
	__free_pages(__va(ctx->info->fix.smem_start), get_order(ctx->info->fix.smem_len));

	framebuffer_release(ctx->info);

	dev_info(&drv_client->dev, "unregisted\n");
	return 0;
}

static const struct of_device_id ssd1306_match[] = {
	{ .compatible = SSD1306_DRIVER_ID, },
	{ },
};
MODULE_DEVICE_TABLE(of, ssd1306_match);

static const struct i2c_device_id ssd1306_id[] = {
	{ SSD1306_DRIVER_ID, 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, ssd1306_id);

static struct i2c_driver ssd1306_driver = {
	.driver = {
		.name           = SSD1306_DRIVER_ID,
		.of_match_table = ssd1306_match,
	},
	.probe    = ssd1306_probe,
	.remove   = ssd1306_remove,
	.id_table = ssd1306_id,
};
module_i2c_driver(ssd1306_driver);

MODULE_AUTHOR("Yaroslav Syrytsia <me@ys.lc>");
MODULE_DESCRIPTION("ssd1306 I2C driver");
MODULE_VERSION("0.1");
MODULE_LICENSE("GPL");

