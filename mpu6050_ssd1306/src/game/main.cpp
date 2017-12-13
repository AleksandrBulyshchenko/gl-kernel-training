#include "cgame.h"

int main()
{
    CGame g;

    if (g.init() >=0) {
        g.idle();
        g.done();
    }
}
