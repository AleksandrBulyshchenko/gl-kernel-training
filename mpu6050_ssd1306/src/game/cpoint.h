#pragma once
#include "ss1306_types.h"
#include <string>
#include <sstream>
/**
 * @brief The CPoint class
 */
class CPoint {
public:
    u16		x;
    u16		y;
    CPoint(const CPoint& a) : x(a.x), y(a.y) {}
    CPoint(u16 _x = 0, u16 _y = 0) : x(_x), y(_y) {}
    CPoint& add(s16 a) { x+=a; y+=a; return *this; }
    CPoint& add(s16 dx, s16 dy) { x+=dx; y+=dy; return *this; }
    void addCheckRange(const s16 a, const CPoint& min, const CPoint& max) {
        if (x+a >= min.x && x+a <=max.x) x+=a;
        if (y+a >= min.y && y+a <=max.y) y+=a;
    }
    void addCheckRange(const s16 dx, const s16 dy, const CPoint& min, const CPoint& max) {
        if (x+dx >= min.x && x+dx <=max.x) x+=dx;
        if (y+dy >= min.y && y+dy <=max.y) y+=dy;
    }
    bool check(const CPoint& min, const CPoint& max) { return (x>=min.x) && (x<=max.x) && (y>=min.y) && (y<=max.y); }
    bool check(const CPoint& center, int radius) { return (x>=center.x-radius) && (x<=center.x+radius) && (y>=center.y-radius) && (y<=center.y+radius); }
    std::string toString() const { std::ostringstream os; os << "[" << x << "," << y << "]"; return os.str(); }

    bool operator==(const CPoint& p) { return x==p.x && y==p.y; }
    bool operator!=(const CPoint& p) { return x!=p.x || y!=p.y; }
    CPoint& operator=(const CPoint& p) { x=p.x; y=p.y; return *this; }
};
