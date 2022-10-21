#ifndef _WYZE_TIMER_H_
#define _WYZE_TIMER_H_

#include <memory>
#include <functional>
#include <vector>
#include <set>

#include "thread.h"

namespace wyze {

    class TimerManager;
    class Timer : public std::enable_shared_from_this<Timer> {
        friend class TimerManager;      //生命TimerManager 便于 TimerManager 直接访问 Timer 内部私有对象
    public:
        using ptr = std::shared_ptr<Timer>;
        bool cancel();      //取消定时器
        bool refresh();     //刷新定时器
        bool reset(uint64_t ms, bool from_now = true); //从新设置定时器

    private:
        Timer(uint64_t ms, std::function<void()> cb
                ,bool recurring, TimerManager* manager);    //创建新的定时器
        Timer(uint64_t next);                               //这个在查找哪些定时器超时使用，外部不会用

    private:
        uint64_t m_ms = 0;                      //执行周期
        std::function<void()> m_cb = nullptr;   //超时执行的任务
        bool m_recurring = false;               //是否循环定时
        TimerManager* m_manager = nullptr;      //管理该定时器的对象
        uint64_t m_next = 0;                    //精确的执行时间

        struct Comparator {                     //std::set 集合进行比较
            bool operator()(const Timer::ptr& lhs, const Timer::ptr& rhs) const;
        };
    };

    class TimerManager {
        friend class Timer;
    public:
        using RWMutexType = RWMutex;
        TimerManager();
        virtual ~TimerManager();    //接口类，需要继承

        Timer::ptr addTimer(uint64_t ms, std::function<void()> cb
                                , bool recurring = false);
        Timer::ptr addConditionTimer(uint64_t ms, std::function<void()> cb
                                , std::weak_ptr<void> weak_cond, bool recurring = false);
        uint64_t getNextTimer();    //获取当前时间距离下一次唤醒的时间段
        void listExpiredCb(std::vector<std::function<void()>>& cbs);    //唤醒后获取那些超时的任务
        bool hasTimer();    //是否有定时器任务

    protected:
        virtual void onTimerInsertdAtFront() = 0;   //添加一个定时器，如果该定时器在 set 集合中为开始，表示需要重新设置阻塞时间
        void addTimer(Timer::ptr val, RWMutexType::WriteLock& wlock);   //向set集合插入timer

    private:
        bool detectClockRollover(uint64_t now_ms);  //检测时间是否被修改

    private:
        RWMutexType m_mutex;                                //操作集合时加锁
        std::set<Timer::ptr, Timer::Comparator> m_timers;   //存放定时器的集合
        bool m_tickled = false;                             //是否唤醒
        uint64_t m_previousTime = 0;                        //上一次的时间
    };
}


#endif