#include "../wyze/wyze.h"

static wyze::Logger::ptr g_logger = WYZE_LOG_ROOT();

int server_main(int argc, char** argv)
{
    wyze::IOManager iom(1);
    iom.addTimer(4000, []() {
        WYZE_LOG_INFO(g_logger) << "onTimer";
        static int count = 0;
        if(++count > 10) {
            exit(1);
        }
    }, true);
    return 0;
}

int main(int argc, char** argv)
{
    return wyze::start_daemon(argc, argv, server_main, argc != 1);
}