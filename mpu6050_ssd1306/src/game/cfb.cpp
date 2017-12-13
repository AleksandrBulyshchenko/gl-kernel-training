#include "cfb.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>
#include "ssd1306_ioctl.h"

const char *CFB::FB_DEVICE = "/dev/fb0";
const char *CFB::SSD_DEVICE = "/dev/ssd1306_x";
unsigned char *CFB::p_fb_mem = 0;
//unsigned char *CFB::p_fb_screenMem = 0;
int  CFB::m_fb = -1;
#ifdef DEV_SSD
int  CFB::m_ssd1306 = -1;
#endif
struct fb_var_screeninfo CFB::m_fb_var;
struct fb_var_screeninfo CFB::m_fb_screen;
struct fb_fix_screeninfo CFB::m_fb_fix;

CFB::CFB() : m_width(SSD1306_WIDTH), m_height(SSD1306_HEIGHT), m_needUpdate(false)
{
}

CFB::~CFB()
{
}

int CFB::drawPixel(u16 x, u16 y, ssd1306_COLOR_t color)
{
    if ( x > SSD1306_WIDTH || y > SSD1306_HEIGHT ) {
        return -1;
    }

    /* Set color */
    if (color == SSD1306_COLOR_WHITE) {
        m_ssd1306_Buffer[x/8 + (y * (SSD1306_WIDTH / 8))] |= (1 << (x % 8));
    }
    else {
        m_ssd1306_Buffer[x/8 + (y * (SSD1306_WIDTH / 8))] &= ~(1 << (x % 8));
    }

    m_needUpdate = true;

    return 0;

}

void CFB::setPoint(const u16 x, const u16 y, const u16 color)
{
    drawPixel(x, y, (ssd1306_COLOR_t)color);
}

int CFB::ioctl_update()
{
#ifdef DEV_SSD
    if (-1 == ioctl(m_fb, FB_UPDATESCREEN)) {
        fprintf(stderr, "Ioctl FB_UPDATESCREEN error.\n");
        return -1;
    }
#endif
    return 0;
}

void CFB::cleanScreen()
{
    memset(m_ssd1306_Buffer, 0, sizeof(m_ssd1306_Buffer));
    m_needUpdate = true;
}

void CFB::updateScreen(void)
{
    if (!m_needUpdate) return;

    memcpy(m_lcd.fbp, m_ssd1306_Buffer, m_lcd.screensize);
    ioctl_update();
}

int CFB::fb_init(const char *device, const char *ssd_dev) {

    int fb_mem_offset;

    // get current settings (which we have to restore)
    if (-1 == (m_fb = open(device, O_RDWR))) {
        fprintf(stderr, "Open %s error\n", device);
        throw "Cannot open device for fb...";
        return 0;
    }
#ifdef DEV_SSD
/*    if (-1 == (m_ssd1306 = open(ssd_dev, O_RDWR))) {
        fprintf(stderr, "Open %s error\n", ssd_dev);
        throw "Cannot open device for ssd1306...";
        return 0;
    }
    */
#endif
    if (-1 == ioctl(m_fb, FBIOGET_VSCREENINFO, &m_fb_var)) {
        fprintf(stderr, "Ioctl FBIOGET_VSCREENINFO error.\n");
        return 0;
    }
    if (-1 == ioctl(m_fb, FBIOGET_FSCREENINFO, &m_fb_fix)) {
        fprintf(stderr, "Ioctl FBIOGET_FSCREENINFO error.\n");
        return 0;
    }
    if (m_fb_fix.type != FB_TYPE_PACKED_PIXELS) {
        fprintf(stderr, "Can handle only packed pixel frame buffers.\n");
        goto err;
    }

    fb_mem_offset = (unsigned long)(m_fb_fix.smem_start) & (~PAGE_MASK);
    //extern void *mmap (void *__addr, size_t __len, int __prot,  int __flags, int __fd, __off_t __offset) __THROW;


    fprintf(stdout, "fb_mem_offset: %d\nm_fb_fix.smem_len:%d\n", fb_mem_offset, m_fb_fix.smem_len);

    p_fb_mem = (unsigned char*)mmap(NULL, m_fb_fix.smem_len + fb_mem_offset, PROT_READ | PROT_WRITE, MAP_SHARED, m_fb, 0);
    if (-1L == (long)p_fb_mem) {
        fprintf(stderr, "Mmap error.\n");
        goto err;
    }
    fprintf(stdout, "Mmap:: %p \n", p_fb_mem);
    // move viewport to upper left corner
    if (m_fb_var.xoffset != 0 || m_fb_var.yoffset != 0) {
        m_fb_var.xoffset = 0;
        m_fb_var.yoffset = 0;
        if (-1 == ioctl(m_fb, FBIOPAN_DISPLAY, &m_fb_var)) {
            fprintf(stderr, "Ioctl FBIOPAN_DISPLAY error.\n");
            munmap(p_fb_mem, m_fb_fix.smem_len);
            goto err;
        }
    }

    m_lcd.width         = m_fb_var.xres;
    m_lcd.height		= m_fb_var.yres;
    m_lcd.screensize	= m_fb_var.xres * m_fb_var.yres * m_fb_var.bits_per_pixel / 8;
    m_lcd.fbp			= p_fb_mem + fb_mem_offset;

    printf("lcd.width %d\n", m_lcd.width);
    printf("lcd.height %d\n", m_lcd.height);

    return 1;

err:
    if (-1 == ioctl(m_fb, FBIOPUT_VSCREENINFO, &m_fb_var))
        fprintf(stderr, "Ioctl FBIOPUT_VSCREENINFO error.\n");
    if (-1 == ioctl(m_fb, FBIOGET_FSCREENINFO, &m_fb_fix))
        fprintf(stderr, "Ioctl FBIOGET_FSCREENINFO.\n");
    return 0;
}

void CFB::fb_cleanup(void) {

    if (m_fb < 0) return;

    if (-1 == ioctl(m_fb, FBIOPUT_VSCREENINFO, &m_fb_var))
        fprintf(stderr, "Ioctl FBIOPUT_VSCREENINFO error.\n");
    if (-1 == ioctl(m_fb, FBIOGET_FSCREENINFO, &m_fb_fix))
        fprintf(stderr, "Ioctl FBIOGET_FSCREENINFO.\n");
    munmap(p_fb_mem, m_fb_fix.smem_len);
    close(m_fb);
#ifdef DEV_SSD
    //close(m_ssd1306);
#endif
}

int CFB::init()
{
    return fb_init(FB_DEVICE, SSD_DEVICE);
}

void CFB::deinit()
{
    fb_cleanup();
}
