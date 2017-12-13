#pragma once
#include "cfb.h"

class CGraph {
private:
    CFB     *p_fb;
    int     m_width;
    int     m_height;
public:
    CGraph();
    ~CGraph();
    void line(const CPoint& p1, const CPoint& p2, const u16 color = 1);
    void line(u16 x1, u16 y1, u16 x2, u16 y2, u16 color = 1);
    void circle(const CPoint& center, u16 r, u16 color = 1);
    void clearScreen();

    inline int getWidth() { return m_width; }
    inline int getHeight() { return m_height; }

    int init();
    void deinit();
    void idle();
};


