#include "../wyze/wyze.h"

static wyze::Logger::ptr g_logger = WYZE_LOG_ROOT();

void  test_fiber() 
{
    static int s_count = 10;

    while (--s_count){
        WYZE_LOG_INFO(g_logger) << "s_count 0 = " << s_count;
        // sleep(1);
        wyze::Fiber::YeildToReady();
    }
    
}

void  test_fiber1() 
{
    static int s_count = 10;

    while (--s_count){
        WYZE_LOG_INFO(g_logger) << "s_count 1 = " << s_count;
        // sleep(1);
        wyze::Fiber::YeildToReady();
    }
    
}

void test_fiber2()
{
    static int s_count = 10;
    WYZE_LOG_INFO(g_logger) << "test in fiber s_coutn = " << s_count;

    // sleep(1);
    if(--s_count) {
        wyze::Scheduler::GetThis()->schedule(&test_fiber2, wyze::GetThreadId());
    }
}

void test_user_caller()
{
    wyze::Scheduler sc;
    WYZE_LOG_INFO(g_logger) << "start";
    sc.start();
    WYZE_LOG_INFO(g_logger) << "scheduler";
    sc.schedule(&test_fiber);
    WYZE_LOG_INFO(g_logger) << "stop";
    sc.stop();
}

void test_threads()
{
    wyze::Scheduler sc(3, true, "test");
    WYZE_LOG_INFO(g_logger) << "start";
    sc.start();
    WYZE_LOG_INFO(g_logger) << "scheduler";
    sc.schedule(&test_fiber);
    sc.schedule(&test_fiber1);
    sc.schedule(&test_fiber2);
    WYZE_LOG_INFO(g_logger) << "stop";
    sc.stop();
}


int main(int argc, char** argv) 
{
    WYZE_LOG_INFO(g_logger) << "main";
    // test_user_caller();
    test_threads();
    WYZE_LOG_INFO(g_logger) << "over";
    
    return 0;
}