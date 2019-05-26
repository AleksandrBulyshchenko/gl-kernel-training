#pragma once

#include "cgraph.h"

class CTarget {
private:
    u16     m_dist;
    CPoint  m_center;
    int     m_value;
    CGraph  *p_graph;
public:
    CTarget(CGraph  *g, u16 dist);
    ~CTarget();

    CTarget& genNew();
    void draw();
    void clean();
    inline int val() { return m_value; }
    bool check(const CPoint &center, int radius);
};
