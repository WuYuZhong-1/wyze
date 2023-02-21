#include "../wyze/wyze.h"
#include <unistd.h>

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


int test_time() {
    int time1 = time(nullptr);
    uint64_t time2 = time(nullptr);

    std::cout << "time1=" << time1 << std::endl;
    std::cout << "time2=" << time2 << std::endl;
    return 0;
}

int test_detect_thread(int argc, char** argv)
{
    wyze::set_detect_interval(5);
    while(1) {
        sleep(5);
        std::cout << "wake ..." << wyze::GetThreadId() << std::endl;
        wyze::update_thread_time("hello ........................... world");
    }
    return 0;
}

int main(int argc, char** argv)
{
    // return wyze::start_daemon(argc, argv, server_main, argc != 1);
    // return test_time();
    return wyze::start_daemon(argc, argv, test_detect_thread, argc != 1);
}