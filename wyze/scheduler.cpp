#include "scheduler.h"
#include "log.h"
#include "macro.h"
#include "hook.h"
#include <unistd.h>

namespace wyze {

    static Logger::ptr g_logger = WYZE_LOG_NAME("system");
    static thread_local Scheduler* t_scheduler = nullptr;   //每个线程都会保存调度器对象
    static thread_local Fiber* t_fiber = nullptr;       //每个线程都会有住协程，切换回到 run 方法中运行

    Scheduler::Scheduler(size_t threads, 
        bool use_caller, const std::string& name)
        :m_name(name)
    {
        WYZE_ASSERT(threads > 0)

        if(use_caller) {    //自己所在的线程也需要执行调度
            
            //创建 主协程对象
            Fiber::GetThis();
            --threads;
            WYZE_ASSERT(GetThis() == nullptr); //在该线程中不存在调度对象的创建
            t_scheduler = this;

            m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
            Thread::SetName(m_name);    //设置创建协程对象的线程所在的线程名
            
            t_fiber = m_rootFiber.get();    //设置run协程对象，以后进行切换
            m_rootThread = GetThreadId();   //设置当前的线程id
            m_threadIds.push_back(m_rootThread);    //将该线程id放入线程id队列，以后指定协程所在的线程做准备
        }
        else {
            m_rootThread = -1;
        }
        m_threadCount = threads;    //在run中会创建线程数
    }

    Scheduler::~Scheduler()
    {
        WYZE_ASSERT(m_stopping);
        if(GetThis() == this) {
            t_scheduler = nullptr;
        }
    }

    //获取调度器对象
    Scheduler* Scheduler::GetThis()
    {
        return t_scheduler;
    }   

    //获取 调度器创建的主协程对象
    Fiber* Scheduler::GetMainFiber()
    {
        return t_fiber;
    }                   

    //开始调度
    void Scheduler::start()
    {
        MutexType::Lock lock(m_mutex);
        if(!m_stopping) 
            return;
        m_stopping = false;
        WYZE_ASSERT(m_threads.empty());

        m_threads.resize(m_threadCount);    //创建对应的线程智能指针对象
        for(size_t i = 0; i < m_threadCount; ++i) {
            m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this)
                                    , m_name + "_" + std::to_string(i)));
            m_threadIds.push_back(m_threads[i]->getId());   //这里能获取到是因为 Semaphore 起到了作用
        }
    }

    void Scheduler::stop()
    {
        m_autoStop = true;
        if(m_rootFiber
            && m_threadCount == 0
            && (m_rootFiber->getState() == Fiber::State::TERM
                || m_rootFiber->getState() == Fiber::State::INIT)) {
            WYZE_LOG_INFO(g_logger) << "STOPPEND";
            m_stopping = true;

            if(stopping())
                return;
        }

        if(m_rootThread != -1) {
            WYZE_ASSERT(GetThis() == this); //在创建调度对象的线程结束调度
        }
        else {
            WYZE_ASSERT(GetThis() != this);
        }

        m_stopping = true;
        for(size_t i = 0; i < m_threadCount; ++i) {
            tickle();
        }

        if(m_rootFiber) 
            tickle();

        if(m_rootFiber) {
            if(!stopping()) {   //不具备停止的条件
                m_rootFiber->call();
            }
        }

        std::vector<Thread::ptr> thrs;
        {
            MutexType::Lock lock(m_mutex);
            thrs.swap(m_threads);
        }

        for(auto& i : thrs) {
            i->join();
        }

    }

    //唤醒调度器
    void Scheduler::tickle()
    {
        WYZE_LOG_INFO(g_logger) << "tickle";
    }  

    //调度核心
    void Scheduler::run()
    {
        // WYZE_LOG_INFO(g_logger) << "run";
        set_hook_enable(true);
        setThis();      //每个线程都保存调度器对象
        if(GetThreadId() != m_rootThread) {     //如果不是创建 调度器的线程，则创建主协程
            t_fiber = Fiber::GetThis().get();
        }

        Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));    //空闲协程，没有人物做则运行该协程
        Fiber::ptr cb_fiber;    //执行函数协程

        FiberAndThread ft;
        while(true) {
            ft.rest();
            bool tickle_me = false; //唤醒别的线程
            bool is_active = false; //是否活跃
            {   //从调度任务中，取出任务
                MutexType::Lock lock(m_mutex);
                auto it = m_fibers.begin();
                while(it != m_fibers.end()) {
                    if(it->thread != -1 && it->thread != GetThreadId()) {
                        ++it;
                        tickle_me = true;
                        continue;
                    }

                    WYZE_ASSERT(it->fiber || it->cb);
                    if(it->fiber && it->fiber->getState() == Fiber::EXEC) {
                        WYZE_LOG_INFO(g_logger) << "don't known fiber="<< it->fiber->getId();
                        ++it;
                        continue;
                    }

                    ft = *it;
                    m_fibers.erase(it);
                    ++m_activeThreadCount;
                    is_active = true;
                    break;
                }
            }
       

            if(tickle_me) 
                tickle();

            //执行获取到的任务
            if(ft.fiber && (ft.fiber->getState() != Fiber::State::TERM 
                            && ft.fiber->getState() != Fiber::State::EXCEPT)) {
                ft.fiber->swapIn();
                --m_activeThreadCount;

                if(ft.fiber->getState() == Fiber::State::READY) {
                    schedule(ft.fiber);
                }
                // else if(ft.fiber->getState() != Fiber::State::TERM
                //         && ft.fiber->getState() != Fiber::State::EXCEPT) {
                //     ft.fiber->m_state = Fiber::HOLD;
                //     WYZE_LOG_DEBUG(g_logger) << "fiber swapOut in Fiber::State::HOLD";
                // }
                ft.rest(); 
                // usleep(5000);
            }
            else if(ft.cb) {

                // if(cb_fiber) {                  //TODO::这样子做相当于所有的函数任务都在使用 一个固定的协程
                //     cb_fiber->reset(ft.cb);     //Fiber 对象重置
                // }
                // else {
                //     cb_fiber.reset(new Fiber(ft.cb));   //创建 Fiber对象
                // }
                
                cb_fiber.reset(new Fiber(ft.cb));   //创建 Fiber对象,如果不重新创建，那么就会以一直看到同一个协程id出现，但实际上有两个对象
                ft.rest();
                cb_fiber->swapIn();
                --m_activeThreadCount;

                if(cb_fiber->getState() == Fiber::State::READY) {
                    schedule(cb_fiber);
                    // cb_fiber.reset();
                }
                // else if( cb_fiber->getState() == Fiber::State::EXCEPT
                //         || cb_fiber->getState() == Fiber::State::TERM) {
                //     // cb_fiber->reset(nullptr);
                // }
                // else {
                //     cb_fiber->m_state = Fiber::State::HOLD;
                //     // cb_fiber.reset();
                // }
                // usleep(5000);
            }
            else {
                if(is_active) {
                    --m_activeThreadCount;
                    continue;
                }

                if(idle_fiber->getState() == Fiber::State::TERM) {
                    // WYZE_LOG_INFO(g_logger) << "idle fiber term";
                    break;
                }

                ++m_idleThreadCount;
                idle_fiber->swapIn();
                --m_idleThreadCount;
                // if(idle_fiber->getState() != Fiber::State::TERM
                //     && idle_fiber->getState() != Fiber::State::EXCEPT ) {
                //     idle_fiber->m_state = Fiber::State::HOLD;   //这里便是让出了处理
                // }
                // usleep(1000);
            }
            
        }

    }   

    //是否停止
    bool Scheduler::stopping()
    {
        MutexType::Lock lock(m_mutex);
        return m_autoStop &&                //自动停止
                m_stopping &&               //停止
                m_fibers.empty() &&        //没有协程可以执行
                m_activeThreadCount == 0;   //没有活跃的线程
    }

    //无协程对象执行，则执行空闲
    void Scheduler::idle()
    {
        while(!stopping()) {
            // WYZE_LOG_INFO(g_logger) << "idle";
            Fiber::YeildToHold();
        }
    }    

    //使当前线程保存 调度器对象  
    void Scheduler::setThis()
    {
        t_scheduler = this;
    }          
  

}