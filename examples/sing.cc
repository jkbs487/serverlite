#include "slite/EventLoop.h"
#include "slite/Logger.h"

int main()
{
    slite::EventLoop loop;
    loop.runAfter(1, []{ fprintf(stderr, "."); });
    loop.runAfter(2, []{ fprintf(stderr, "."); });
    loop.runAfter(3, []{ fprintf(stderr, "."); });
    loop.runAfter(4, []{ fprintf(stderr, "."); });
    loop.runAfter(4.5, []{ fprintf(stderr, "If I."); });
    loop.runAfter(5, []{ fprintf(stderr, "die young. "); });
    loop.runAfter(6, []{ fprintf(stderr, "bury me in."); });
    loop.runAfter(7, []{ fprintf(stderr, "satin. ");});
    loop.runAfter(8, []{ fprintf(stderr, "lay me down ");});
    loop.runAfter(9, []{ fprintf(stderr, "on a. ");});
    loop.runAfter(10, [&]{ fprintf(stderr, "bed of roses."); });
    loop.loop();
}