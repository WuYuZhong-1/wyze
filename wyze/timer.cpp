#include "timer.h"
#include "util.h"

namespace wyze {

    //set 集合会排序，会根据 < 操作符 进行排序，
    bool Timer::Comparator::operator()(const Timer::ptr& lhs, const Timer::ptr& rhs) const
    {
        if(!lhs && !rhs)
            return false;
        if(!lhs)
            return true;
        if(!rhs)
            return false;
        if(lhs->m_next < rhs->m_next)
            return true;
        if(rhs->m_next < lhs->m_next)
            return false;
        return lhs.get() < rhs.get();
    }

    //取消定时器
    bool Timer::cancel()
    {
        TimerManager::RWMutexType::WriteLock wlock(m_manager->m_mutex);
        if(m_cb) {      //这里的判断是因为，外部持有该对象，可能多次操作
            m_cb = nullptr;
            auto it = m_manager->m_timers.find(shared_from_this());
            m_manager->m_timers.erase(it);
            return true;
        }
        return false;
    }

    //刷新定时器
    bool Timer::refresh()
    {
        TimerManager::RWMutexType::WriteLock wlock(m_manager->m_mutex);
        if(!m_cb)
            return false;
        auto it = m_manager->m_timers.find(shared_from_this());
        if(it == m_manager->m_timers.end())
            return false;
        
        m_manager->m_timers.erase(it);
        m_next = GetCurrentMS() + m_ms;
        m_manager->m_timers.insert(shared_from_this());  //这里也要考虑到当前定时器是否为第一个
        // m_manager->addTimer(shared_from_this(), wlock);
        return true;
    }

    //从新设置定时器
    bool Timer::reset(uint64_t ms, bool from_now)
    {
        if(ms == m_ms && !from_now)
            return true;
        
        TimerManager::RWMutexType::WriteLock wlock(m_manager->m_mutex);
        if(!m_cb)
            return false;
        
        auto it = m_manager->m_timers.find(shared_from_this());
        if(it == m_manager->m_timers.end())
            return false;
        m_manager->m_timers.erase(it);
        uint64_t start  = 0;
        if(from_now) 
            start = GetCurrentMS();
        else 
            start = m_next - m_ms;
        m_ms = ms;
        m_next = start + m_ms;
        m_manager->addTimer(shared_from_this(), wlock);
        return true;
    }

    //创建新的定时器
    Timer::Timer(uint64_t ms, std::function<void()> cb
                    ,bool recurring, TimerManager* manager)
        : m_ms(ms), m_cb(cb), m_recurring(recurring), m_manager(manager)
    {
        m_next = GetCurrentMS() + m_ms;
    }

    Timer::Timer(uint64_t next) : m_next(next) { }

    TimerManager::TimerManager()
    {
        m_previousTime = GetCurrentMS();
    }

    TimerManager::~TimerManager() { }

    Timer::ptr TimerManager::addTimer(uint64_t ms, std::function<void()> cb
                            , bool recurring)
    {
        Timer::ptr timer(new Timer(ms, cb, recurring, this));
        RWMutexType::WriteLock wlock(m_mutex);
        addTimer(timer, wlock);
        return timer;
    }

    static void OnTimer(std::weak_ptr<void> weak_cond, std::function<void()> cb)
    {
        std::shared_ptr<void> tmp = weak_cond.lock();   //获取地址
        if(tmp) {
            cb();
        }
    }
    
    Timer::ptr TimerManager::addConditionTimer(uint64_t ms, std::function<void()> cb
                            , std::weak_ptr<void> weak_cond, bool recurring)
    {
        return addTimer(ms, std::bind(&OnTimer, weak_cond, cb), recurring);
    }

    //获取当前时间距离下一次唤醒的时间段
    uint64_t TimerManager::getNextTimer()
    {
        RWMutexType::ReadLock rlock(m_mutex);
        m_tickled = false;
        if(m_timers.empty())
            return ~0ull;
        
        const Timer::ptr next = *m_timers.begin();
        uint64_t now_ms = GetCurrentMS();
        if(now_ms >= next->m_next)
            return 0;
        else 
            return next->m_next - now_ms;
    } 

    //唤醒后获取那些超时的任务
    void TimerManager::listExpiredCb(std::vector<std::function<void()>>& cbs)
    {
        {
            RWMutexType::ReadLock rlock(m_mutex);
            if(m_timers.empty())
                return;
        }

        RWMutexType::WriteLock wlock(m_mutex);
        uint64_t now_ms = GetCurrentMS();
        std::vector<Timer::ptr> expired;

        bool rollover = detectClockRollover(now_ms);    //检测当前时间 和上一次的时间
        if(!rollover && ((*m_timers.begin())->m_next > now_ms)) // 没有发生时间修改，且没有定时任务处理
            return;
        
        Timer::ptr now_timer(new Timer(now_ms));
        auto it = rollover ? m_timers.end() : m_timers.lower_bound(now_timer);
        while( it != m_timers.end() && (*it)->m_next == now_ms)
            ++it;
        
        //保存过期的定时器
        expired.insert(expired.begin(), m_timers.begin(), it);
        m_timers.erase(m_timers.begin(), it);
        cbs.reserve(expired.size());

        for(auto& timer: expired) {     //取出任务，且判断是否循环
            cbs.push_back(timer->m_cb);
            if(timer->m_recurring) {
                timer->m_next = now_ms + timer->m_ms;
                m_timers.insert(timer);
            }
            else {
                timer->m_cb = nullptr;
            }
        }
    }

    //是否有定时器任务
    bool TimerManager::hasTimer()
    {
        RWMutexType::ReadLock rlock(m_mutex);
        return !m_timers.empty();
    }    

    //向set集合插入timer
    void TimerManager::addTimer(Timer::ptr val, RWMutexType::WriteLock& wlock)
    {
        auto it = m_timers.insert(val).first;   // insert 返回 pair first 为 位置，second 为是否成功
        bool at_front = (it == m_timers.begin()) && !m_tickled; // m_tickled 表示是否要唤醒
        if(at_front)
            m_tickled = true;
        wlock.unlock();

        if(at_front)
            onTimerInsertdAtFront();
    }

    //检测时间是否被修改
    bool TimerManager::detectClockRollover(uint64_t now_ms)
    {
        bool rollover = false;
        if(now_ms < m_previousTime &&
            now_ms < (m_previousTime - 60 * 60 * 12)) {      //TODO::这里存在问题，如果时间差为 12 小时，表示时间发生改动
            rollover = true;                                    //在做 阻塞时时间不能大于 12 小时
        }
        m_previousTime = now_ms;
        return rollover;
    }

}