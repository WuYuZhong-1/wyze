#ifndef _WYZE_SCHEDULER_H_
#define _WYZE_SCHEDULER_H_

#include <memory>
#include <functional>
#include <vector>
#include <list>
#include <atomic>
#include "thread.h"
#include "fiber.h"

namespace wyze {

    class Scheduler {
    public:
        using ptr = std::shared_ptr<Scheduler>;
        using MutexType = Mutex;

        Scheduler(size_t threads = 1, bool use_caller = true, const std::string& name = "UNKNOW");
        virtual ~Scheduler();

        const std::string& getName() const { return m_name;};  //获取调度器的名称
        static Scheduler* GetThis();                    //获取调度器对象
        static Fiber* GetMainFiber();                   //获取 调度器创建的主协程对象

        void start();                                   //开始调度
        void stop();                                    //停止调度

        template <class FiberOrCb>
        void schedule(FiberOrCb fc, int thread = -1) {
            bool need_tickle = false;
            {
                MutexType::Lock lock(m_mutex);
                need_tickle = scheduleNolock(fc, thread);
            }
            if(need_tickle){
                tickle();
            }
        }

        template<class InputIterator>
        void schedule(InputIterator begin, InputIterator end) {
            bool need_tickle = false;
            {
                MutexType::Lock lock(m_mutex);
                while(begin != end) {
                    need_tickle = scheduleNolock(&*begin, -1) || need_tickle;
                    ++begin;
                }
                if(need_tickle) {
                    tickle();
                }
            }
        }

    protected:
        virtual void tickle();      //唤醒调度器
        void run();                 //调度核心
        virtual bool stopping();    //是否停止
        virtual void idle();        //无协程对象执行，则执行空闲
        void setThis();             //使当前线程保存 调度器对象
        bool hasIdleThreads() const { return m_idleThreadCount > 0; }   //是否有空闲线程

    private:

        template<class FiberOrCb>
        bool scheduleNolock(FiberOrCb fc, int thread) {
            bool need_tickle = m_fibers.empty();
            FiberAndThread ft(fc, thread);
            if(ft.fiber || ft.cb) {
                m_fibers.push_back(ft);
            }

            return need_tickle;   //返回是否唤醒
        }

        struct FiberAndThread {
            Fiber::ptr fiber;           //当调度器传入的协程对象
            std::function<void()> cb;   //当调度器传入的是函数，在内部会生成一个协程对象
            int thread;                 //指定 哪个线程运行该对象

            FiberAndThread(Fiber::ptr f, int thr)
                : fiber(f), thread(thr) { }
            FiberAndThread(Fiber::ptr* f, int thr)      //避免只能指针计数累加，内部运行完毕会释放该对象
                : thread(thr) { fiber.swap(*f); }   
            FiberAndThread(std::function<void()> f, int thr) 
                : cb(f), thread(thr) { }
            FiberAndThread(std::function<void()> *f, int thr)   //这里感觉没有必要
                : thread(thr) { cb.swap(*f); }
            FiberAndThread()                        //默认构造函数，在容器中会用到
                : thread(-1) { }
            void rest() {
                fiber = nullptr;
                cb = nullptr;
                thread = -1;
            }
        };

    private:
        MutexType m_mutex;      //锁多个线程会同时去获取执行对象
        std::vector<Thread::ptr> m_threads; //线程池
        std::list<FiberAndThread> m_fibers; //协程对象池
        Fiber::ptr m_rootFiber;     //当想要创建Scheduler 对象的线程也进行调度时，该对象会被创建
        int m_rootThread = 0;       //rootThread 线程id
        std::string m_name;         //调度器的名称，调度器创建的线程名 等于调度器名+ 序号
    protected:
        std::vector<int> m_threadIds;     //每个线程对应的线程id
        size_t m_threadCount = 0;       //需要创建的线程数
        std::atomic<size_t> m_activeThreadCount = {0};
        std::atomic<size_t> m_idleThreadCount = {0};
        bool m_stopping = true;     //是否停止
        bool m_autoStop = false;    //自动停止
               
    };


}

#endif // !_WYZE_SCHEDULER_H_