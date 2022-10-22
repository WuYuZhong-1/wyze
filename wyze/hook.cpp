#include "hook.h"
#include "log.h"
#include "fiber.h"
#include "iomanager.h"

#include <dlfcn.h>

wyze::Logger::ptr g_logger = WYZE_LOG_NAME("system");

namespace wyze {

    static thread_local bool t_hook_enable = false;

    #define HOOK_FUN(XX)    \
        XX(sleep)           \
        XX(usleep)      


    // RTLD_DEFAULT表示按默认的顺序搜索共享库中符号symbol第一次出现的地址
    // RTLD_NEXT表示在当前库以后按默认的顺序搜索共享库中符号symbol第一次出现的地址 
    void hook_init() 
    {
        static bool is_inited = false;
        if(is_inited)
            return;
    #define XX(name) name##_f = (name##_fun)dlsym(RTLD_NEXT, #name);
        HOOK_FUN(XX);
    #undef XX 
    }


    struct _HookIniter {
        _HookIniter() {
            hook_init();
        }
    };

    static _HookIniter s_hook_initer;

    //使能钩子函数
    bool is_hook_enable()
    {
        return t_hook_enable;
    }

    //设置是否使用钩子函数
    void set_hook_enable(bool flag)
    {
        t_hook_enable = flag;
    }
}

extern "C" {
    
    //定义函数指针
    #define XX(name) name##_fun name##_f = nullptr;
        HOOK_FUN(XX);
    #undef XX

    unsigned int sleep(unsigned int seconds)
    {
        if( !wyze::t_hook_enable ) 
            return sleep_f(seconds);
        
        wyze::Fiber::ptr fiber = wyze::Fiber::GetThis();
        wyze::IOManager* iom = wyze::IOManager::GetThis();
        //创建一个定时器，定时到，将当前协程加入协程调度中
        iom->addTimer(seconds * 1000, std::bind((void(wyze::Scheduler::*)(wyze::Fiber::ptr, int thread)) //这里表示的是一个函数类型
                                                &wyze::IOManager::schedule, iom, fiber, -1));
        wyze::Fiber::YeildToHold();
        return 0;
    }

    int usleep(useconds_t usec)
    {
        if( !wyze::t_hook_enable ) 
            return usleep_f(usec);

        wyze::Fiber::ptr fiber = wyze::Fiber::GetThis();
        wyze::IOManager* iom = wyze::IOManager::GetThis();
        iom->addTimer(usec / 1000, std::bind( (void(wyze::Scheduler::*)(wyze::Fiber::ptr, int thread))
                                                &wyze::IOManager::schedule, iom, fiber, -1));
        wyze::Fiber::YeildToHold();
        return 0;
    }
}