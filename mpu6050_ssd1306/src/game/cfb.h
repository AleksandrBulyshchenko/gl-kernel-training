#pragma once
#include "ss1306_types.h"
#include "csingle.h"
#include <linux/fb.h>
#include "cpoint.h"

#define DEV_SSD

/**
 * @brief The CPoint class
 */
class CLCD {
public:
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;
    int			width;
    int			height;
    long int	screensize;
    u8			*fbp;
};
/**
 * @brief The CPoint class
 */
class CFB : public CSingleTone<CFB> {
    friend class CSingleTone<CFB>;
private:
    static const char * FB_DEVICE;
    static const char * SSD_DEVICE;
    static int     m_fb;
#ifdef DEV_SSD
    static int     m_ssd1306;
#endif
    static struct fb_var_screeninfo    m_fb_var;
    static struct fb_var_screeninfo    m_fb_screen;
    static struct fb_fix_screeninfo    m_fb_fix;
    static unsigned char               *p_fb_mem;
    //static unsigned char               *p_fb_screenMem;

    int		m_width;
	int		m_height;
    CLCD    m_lcd;
    u8      m_ssd1306_Buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];

    bool    m_needUpdate;
    CFB();
    ~CFB();

    int fb_init(const char *device, const char* ssd_dev);
    void fb_cleanup(void);
    int ioctl_update();
public:
    inline int getWidt() { return m_width; }
    inline int getHeight() { return m_height; }
    int drawPixel(u16 x, u16 y, ssd1306_COLOR_t color);
    void setPoint(const u16 x, const u16 y, const u16 color = 1);
    void cleanScreen();
    void updateScreen(void);
    int init();
    void deinit();
};

