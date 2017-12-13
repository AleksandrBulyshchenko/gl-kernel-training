#include "cgame.h"
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <iostream>

CGame::CGame() : m_score(0), m_target(&m_graph, RADIUS),
    m_center(m_graph.getWidth() / 2, m_graph.getHeight() / 2),
    m_radius(RADIUS),
    m_range_min(0, 0), m_range_max(m_graph.getWidth(), m_graph.getHeight())
{
    p_mpu6050 = CMpu6050::getInstance();
}

CGame::~CGame()
{}

int CGame::init(void)
{
    try {
        p_mpu6050->init();
        m_graph.init();
    }
    catch(const char *e) {
        std::cerr << "Initialization error: " << e << std::endl;
        return -1;
    }
    return 0;
}

void CGame::shift()
{
    mpu6050_values v;
    bool check;
    CPoint old_center(m_center);
    bool needReDraw = false;

    p_mpu6050->getAll();
    set(v, p_mpu6050->coord());

    int dx = - (int)v.accel.x * m_graph.getWidth()  / 360;
    int dy = (int)v.accel.y * m_graph.getHeight() / 360;

    if (dx || dy) {
        m_center.addCheckRange(dx, dy, m_range_min, m_range_max);
        if (old_center != m_center){
            needReDraw = true;
        }
    }

    if ((check = m_target.check(m_center, m_radius)))
    {
        m_score += m_target.val();
        std::cout << "   ===   Good point: " << m_score << std::endl;
        m_target.genNew();
        needReDraw = true;
    }

    if (needReDraw){
        m_graph.clearScreen();
        m_graph.circle(m_center, m_radius);
        m_target.draw();
    }
}

void CGame::idle(void)
{
    m_graph.circle(m_center, m_radius);
    m_target.genNew().draw();

    for(; !kbhit();) {
        shift();
        m_graph.idle();
        usleep(200000);
    }
}

void CGame::done(void)
{
    p_mpu6050->deinit();
    m_graph.deinit();
}

int CGame::kbhit()
{
    struct termios oldt, newt;
    int ch;
    int oldf;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);
    if(ch != EOF)
    {
        ungetc(ch, stdin);
        return 1;
    }

    return 0;
}
