#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <linux/kd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <string.h>

#include <math.h>
#include <unistd.h>
#include "cgraph.h"

#include <string.h>
#include <stdio.h>
#include <iostream>

#define DBG(x) x

CGraph::CGraph()
{
    p_fb     = CFB::getInstance();
    m_width  = p_fb->getWidt();
    m_height = p_fb->getHeight();
}

CGraph::~CGraph()
{}

int CGraph::init()
{
    int ret_val = p_fb->init();
    p_fb->cleanScreen();
    return ret_val;
}

void CGraph::deinit()
{
    p_fb->deinit();
}

void CGraph::idle()
{
    p_fb->updateScreen();
}

void CGraph::clearScreen()
{
    DBG(std::cout << __func__ << std::endl; );
    p_fb->cleanScreen();
}

void CGraph::line(const CPoint& p1, const CPoint& p2, const u16 color)
{
	int dx, dy, inx, iny, e;
    u16 x1 = p1.x, x2 = p2.x;
    u16 y1 = p1.y, y2 = p2.y;

	dx = x2 - x1;
	dy = y2 - y1;
	inx = dx > 0 ? 1 : -1;
	iny = dy > 0 ? 1 : -1;

	dx = (dx > 0) ? dx : -dx;
	dy = (dy > 0) ? dy : -dy;


	if(dx >= dy) {
		dy <<= 1;
		e = dy - dx;
		dx <<= 1;
		while (x1 != x2){
            p_fb->setPoint(x1, y1, color);
			if(e >= 0){
				y1 += iny;
				e-= dx;
			}
			e += dy;
			x1 += inx;
		}
	}
	else{
		dx <<= 1;
		e = dx - dy;
		dy <<= 1;
		while (y1 != y2){
            p_fb->setPoint(x1, y1, color);
			if(e >= 0){
				x1 += inx;
				e -= dy;
			}
			e += dx;
			y1 += iny;
		}	}
    p_fb->setPoint(x1, y1);
}
// ---------------------------------------------------------------------------
void CGraph::line(u16 x1, u16 y1, u16 x2, u16 y2, u16 color)
{
    line(CPoint(x1, y1), CPoint(x2, y2), color);
}
// ---------------------------------------------------------------------------
void CGraph::circle(const CPoint &center, u16 r, u16 color)
{
    DBG(std::cout << __func__ << " " << center.toString() << ", r:" << r << " c:" << color << std::endl; );
    if (!r) return;

	int  draw_x0, draw_y0;			//draw points
	int  draw_x1, draw_y1;
	int  draw_x2, draw_y5;
	int  draw_x3, draw_y3;
	int  draw_x4, draw_y4;
	int  draw_x6, draw_y6;
	int  draw_x7, draw_y7;
	int  draw_x5, draw_y2;
	int  xx, yy;					// circle control variable
	int  di;						// decide variable
	
	if( 0 == r)
		return;

	// calculate 8 special point(0��45��90��135��180��225��270degree) display them 
    draw_x0 = draw_x1 = center.x;
    draw_y0 = draw_y1 = center.y + r;
    if(draw_y0<m_height)
        p_fb->setPoint(draw_x0, draw_y0, color);	// 90degree
    draw_x2 = draw_x3 = center.x;
    draw_y2 = draw_y3 = center.y - r;
    if(draw_y2>0)
        p_fb->setPoint(draw_x2,draw_y2, color);// 270degree
    draw_x4 = draw_x6 = center.x + r;
    draw_y4 = draw_y6 = center.y;
    if(draw_x4<m_height)
        p_fb->setPoint(draw_x4, draw_y4, color);// 0degree
    draw_x5 = draw_x7 = center.x - r;
    draw_y5 = draw_y7 = center.y;
    if(draw_x5>0)
        p_fb->setPoint(draw_x5, draw_y5, color);	// 180degree
	if(1==r)   // if the radius is 1, finished.
		return;
	//using Bresenham 
	di = 3 - 2*r;
	xx = 0;
	yy = r;
	while(xx<yy){
		if(di<0){
			di += 4*xx + 6;
		}else{
			di += 4*(xx - yy) + 10;
			yy--;
			draw_y0--;
			draw_y1--;
			draw_y2++;
			draw_y3++;
			draw_x4--;
			draw_x5++;
			draw_x6--;
			draw_x7++;
		}
		xx++;
		draw_x0++;
		draw_x1--;
		draw_x2++;
		draw_x3--;
		draw_y4++;
		draw_y5++;
		draw_y6--;
		draw_y7--;
		//judge current point in the avaible range
        if( (draw_x0<m_width)&&(draw_y0>0) ){
            p_fb->setPoint(draw_x0, draw_y0, color);
		}	
        if( (draw_x1>0)&&(draw_y1>0) ){
            p_fb->setPoint(draw_x1, draw_y1, color);
		}
        if( (draw_x2<m_width)&&(draw_y2<m_height) ){
            p_fb->setPoint(draw_x2, draw_y2, color);
		}
        if( (draw_x3>0)&&(draw_y3<m_height) ){
            p_fb->setPoint(draw_x3, draw_y3, color);
		}
        if( (draw_x4<m_width)&&(draw_y4>0) ){
            p_fb->setPoint(draw_x4, draw_y4, color);
		}
        if( (draw_x5>0)&&(draw_y5>0) ){
            p_fb->setPoint(draw_x5, draw_y5, color);
		}
        if( (draw_x6<m_width)&&(draw_y6<m_height) ){
            p_fb->setPoint(draw_x6, draw_y6, color);
		}
        if( (draw_x7>0)&&(draw_y7<m_height) ){
            p_fb->setPoint(draw_x7, draw_y7, color);
		}
	}
}


