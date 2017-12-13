#pragma once

#include "cgraph.h"
#include "cmpu6050.h"
#include "ctarget.h"

#define RADIUS  5

class CGame
{
private:
    int kbhit();

    CGraph      m_graph;
    CMpu6050    *p_mpu6050;

    int         m_score;
    CTarget     m_target;
    CPoint      m_center;
    int         m_radius;
    CPoint      m_range_min, m_range_max;
public:
    CGame();
    ~CGame();
    int init(void);
    void idle(void);
    void done(void);

    void shift();
};
