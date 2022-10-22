#include "../wyze/hook.h"
#include "../wyze/wyze.h"


wyze::Logger::ptr g_logger = WYZE_LOG_ROOT();


void test_sleep()
{
    wyze::IOManager iom;
    iom.schedule([](){
        sleep(2);
        WYZE_LOG_INFO(g_logger) << "sleep 2";
    });

    iom.schedule([](){
        usleep(3 * 1000 * 1000);
        WYZE_LOG_INFO(g_logger) << "usleep 3 * 1000 * 1000";
    });

    WYZE_LOG_INFO(g_logger) << "test sleep";
}


int main(int argc, char** argv)
{
    test_sleep();
    return 0;
}