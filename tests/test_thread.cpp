#include "../wyze/wyze.h"
#include <unistd.h>

wyze::Logger::ptr g_logger = WYZE_LOG_ROOT();
int g_count  = 0;
wyze::Mutex g_mutex;

void fun1() 
{
    WYZE_LOG_INFO(g_logger) << "name:" << wyze::Thread::GetName()
                            << " this.name: " << wyze::Thread::GetThis()->getName()
                            << " id: " << wyze::GetThreadId()
                            << " this.id: " << wyze::Thread::GetThis()->getId();

    for(int i = 0; i < 10000000; ++i) {
        wyze::Mutex::Lock lock(g_mutex);
        ++g_count;
    }

}

void thread_test() 
{
    std::vector<wyze::Thread::ptr> ths;
    for(int i = 0; i < 3; ++i) {
        wyze::Thread::ptr thr(new wyze::Thread(&fun1, "name_" + std::to_string(i)));
        ths.push_back(thr);
    }

    for(size_t i = 0; i < ths.size(); ++i) {
        ths[i]->join();
    }

    WYZE_LOG_INFO(g_logger) << "g_count = " << g_count;
}

void fun2() {
    while(true) {
        WYZE_LOG_INFO(g_logger) << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
    }
}

void fun3() {
    while(true) {
        WYZE_LOG_INFO(g_logger) << "========================================";
    }
}

void log_test() 
{
    wyze::Thread::ptr thr1(new wyze::Thread(&fun2, "name_" + std::to_string(1)));
    
    wyze::Thread::ptr thr2(new wyze::Thread(&fun3, "name_" + std::to_string(2)));
        
    thr1->join();
    thr2->join();
}

int main(int argc, char** argv) 
{
    thread_test();
    log_test();

    return 0;
}