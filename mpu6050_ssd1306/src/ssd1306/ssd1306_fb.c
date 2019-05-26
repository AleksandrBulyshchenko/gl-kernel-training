#include "ssd1306_fb.h"
#include "ssd1306.h"
#include <linux/uaccess.h>
#include "ssd1306_ioctl.h"

#define DBG(x)

static ssize_t ssd1306fb_write(struct fb_info *info, const char __user *buf,
		size_t count, loff_t *ppos);
static int ssd1306fb_blank(int blank_mode, struct fb_info *info);
static void ssd1306fb_fillrect(struct fb_info *info, const struct fb_fillrect *rect);
static void ssd1306fb_copyarea(struct fb_info *info, const struct fb_copyarea *area);
static void ssd1306fb_imageblit(struct fb_info *info, const struct fb_image *image);
static int ssd1306fb_cursor(struct fb_info *info, struct fb_cursor *cursor);
static int ssd1306fb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg);


struct fb_ops ssd1306fb_ops;
struct fb_fix_screeninfo ssd1306fb_fix; 
struct fb_var_screeninfo ssd1306fb_var;
/* --------------------------------------------------------------- */
struct fb_ops ssd1306fb_ops = {
	.owner          = THIS_MODULE,
	.fb_read        = fb_sys_read,
	.fb_write       = ssd1306fb_write,
	.fb_blank       = ssd1306fb_blank,
	.fb_fillrect    = ssd1306fb_fillrect,
	.fb_copyarea    = ssd1306fb_copyarea,
	.fb_imageblit   = ssd1306fb_imageblit,
    .fb_ioctl       = ssd1306fb_ioctl,
    .fb_cursor      = ssd1306fb_cursor,
};
/* --------------------------------------------------------------- */
struct fb_fix_screeninfo ssd1306fb_fix = {
	.id     	= "Solomon SSD1306",
	.type       = FB_TYPE_PACKED_PIXELS,
	.visual     = FB_VISUAL_MONO10,
	.xpanstep   = 0,
	.ypanstep   = 0,
	.ywrapstep  = 0,
	.accel      = FB_ACCEL_NONE,
};
/* --------------------------------------------------------------- */
struct fb_var_screeninfo ssd1306fb_var = {
	.bits_per_pixel = 1,
};
/* ******************************************************************
 *
 * *****************************************************************/
static ssize_t ssd1306fb_write(struct fb_info *info, const char __user *buf,
		size_t count, loff_t *ppos)
{
    struct ssd1306_data *lcd = info->par;
	unsigned long total_size;
	unsigned long p = *ppos;
	u8 __iomem *dst;

    DBG(pr_info("[%s] i:%p b:%p, c:%d\n", __func__, info, buf, count);)

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
static int ssd1306fb_blank(int blank_mode, struct fb_info *info)
{
    struct ssd1306_data *lcd = info->par;
    DBG(pr_info("[%s]\n", __func__);)

    if (blank_mode != FB_BLANK_UNBLANK)
    	return ssd1306_OFF(lcd);
    else
    	return ssd1306_ON(lcd);
}
/* ******************************************************************
 *
 * *****************************************************************/
static void ssd1306fb_fillrect(struct fb_info *info, const struct fb_fillrect *rect)
{
    struct ssd1306_data *lcd = info->par;
    DBG(pr_info("[%s]\n", __func__);)
    sys_fillrect(info, rect);
	schedule_work(&lcd->display_update_ws);
}
/* ******************************************************************
 *
 * *****************************************************************/
static void ssd1306fb_copyarea(struct fb_info *info, const struct fb_copyarea *area)
{
    struct ssd1306_data *lcd = info->par;
    DBG(pr_info("[%s]\n", __func__);)
    sys_copyarea(info, area);
	schedule_work(&lcd->display_update_ws);
}
/* ******************************************************************
 *
 * *****************************************************************/
static void ssd1306fb_imageblit(struct fb_info *info, const struct fb_image *image)
{
    struct ssd1306_data *lcd = info->par;
    DBG(pr_info("[%s] i:%p lcd:%p [%d, %d, %d, %d, 0x%p]\n", __func__, info, lcd, image->dx, image->dy, image->width, image->height, image->data);)
    sys_imageblit(info, image);
	schedule_work(&lcd->display_update_ws);
}
/* ******************************************************************
 *
 * *****************************************************************/
static int ssd1306fb_cursor(struct fb_info *info, struct fb_cursor *cursor)
{
    DBG(pr_info("[%s] %p cursor_enabled:%d]\n", __func__, info, cursor->enable);)
    return 0;
}
/* ******************************************************************
 *
 * *****************************************************************/
static int ssd1306fb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg)
{
    int retval = 0;
    struct ssd1306_data *lcd;

    lcd = info->par;
    DBG(pr_info("[%s]:: %p, %x, %lx %p\n", __func__, info, cmd, arg, lcd));

    switch(cmd) {
        case FB_UPDATESCREEN:
            DBG(pr_info("[%s]:: FB_UPDATESCREEN %p\n", __func__, lcd);)
            if (lcd){
                schedule_work(&lcd->display_update_ws);
            }
            break;
        default:
            DBG(pr_err("[%s]:: wrong IOCTL: %d %lx\n", __func__, cmd, arg);)
            return -EINVAL;
    }
    return retval;
}
