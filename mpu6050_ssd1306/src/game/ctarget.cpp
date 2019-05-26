#include "ctarget.h"

#include <time.h>
#include <iostream>

#define DBG(x) x

CTarget::CTarget(CGraph *g, u16 dist) : m_dist(dist), m_value(0), p_graph(g)
{}

CTarget::~CTarget()
{}

CTarget &CTarget::genNew()
{
    //p_graph->clearScreen();
    //clean();

    m_value = 2 + clock() % 4;
    m_center.x = m_dist + clock() % (p_graph->getWidth()  - 2 * m_dist);
    m_center.y = m_dist + clock() % (p_graph->getHeight() - 2 * m_dist);

    DBG(std::cout << "NewTarget:" << m_center.toString() << " r:" << m_value << std::endl; );

    //draw();

    return *this;
}

void CTarget::draw()
{
    p_graph->circle(m_center, m_value, 1);
}

void CTarget::clean()
{
    p_graph->circle(m_center, m_value, 0);
}

bool CTarget::check(const CPoint& center, int radius)
{
    return  CPoint(center).add(-radius, -radius).check(m_center, m_value) ||
            CPoint(center).add(-radius, +radius).check(m_center, m_value) ||
            CPoint(center).add(+radius, -radius).check(m_center, m_value) ||
            CPoint(center).add(+radius, +radius).check(m_center, m_value);
}
