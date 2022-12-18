#ifndef _WYZE_IOMANAGER_H_
#define _WYZE_IOMANAGER_H_

#include "scheduler.h"
#include "timer.h"
#include <vector>

namespace wyze {

    class IOManager: public Scheduler, public TimerManager{
    public:
        using ptr = std::shared_ptr<IOManager>;
        using RWMutexType = RWMutex;

        enum Event {
            NONE = 0,
            READ = 0X1,  //EPOLLIN
            WRITE = 0X4, //EPOLLOUT
        };
    private:
        struct FdContext {
            using MutexTyp = Mutex;
            struct EventContext {
                Scheduler* scheduler = nullptr;     //事件执行的scheduler
                Fiber::ptr fiber;                   //事件协程
                std::function<void()> cb = nullptr; //时间的回调函数
            };

            EventContext& getContext(Event event);
            void resetContext(EventContext& ctx);
            void triggerEvent(Event event);

            EventContext read;              //读事件
            EventContext write;             //写事件
            int fd = 0;                     //事件关联的句柄
            Event events = Event::NONE;     //已经注册的事件
            MutexTyp mutex;
        };

        void contextResize(size_t size);

    public:
        IOManager(size_t threads = 1, 
            bool use_caller = true, const std::string& name = "UNKONW");
        ~IOManager();
        
        // 0 success, -1 error      该函数只支持单事件的增加
        int addEvent(int fd, Event event, std::function<void()> cb = nullptr);
        bool delEvent(int fd, Event event);
        bool canceEvent(int fd, Event event);
        bool canceAll(int fd);
        static IOManager* GetThis();
    
    protected:
        void tickle() override;
        bool stopping() override;
        void idle() override;
        void onTimerInsertdAtFront() override;   //添加一个定时器，如果该定时器在 set 集合中为开始，表示需要重新设置阻塞时间

        bool stopping(uint64_t& timeout);

    private:
        int m_epfd = 0;             //epoll fd
        int m_tickleFds[2];         //唤醒 epoll 主塞的 pipe  fd

        std::atomic<size_t> m_pendingEvent = {0};   //添加的时间，增加时间会增加，删除和触发会取消
        RWMutexType m_mutex;                        //对 m_fdContexts 对象操作会进行 加锁
        std::vector<FdContext *> m_fdContexts;      // 保存fd 事件句柄
    };

}

#endif // _WYZE_IOMANAGER_H_