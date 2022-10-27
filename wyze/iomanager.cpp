#include "iomanager.h"
#include "macro.h"
#include "log.h"

#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>


namespace wyze {

    static Logger::ptr g_logger = WYZE_LOG_NAME("system");

    IOManager::FdContext::EventContext& IOManager::FdContext::getContext(Event event)
    {
        switch(event) {
            case IOManager::Event::READ:
                return read;                    //返回读事件句柄
            case IOManager::Event::WRITE:
                return write;                   //返回写时间句柄
            default:
                WYZE_ASSERT2(false, "getContext");
        }
    }

    void IOManager::FdContext::resetContext(EventContext& ctx)
    {
        ctx.scheduler = nullptr;
        ctx.fiber.reset();
        ctx.cb = nullptr;
    }

    void IOManager::FdContext::triggerEvent(Event event)
    {
        WYZE_ASSERT(events & event);
        events = (Event)(events & ~event);      //将出发的事件取消
        EventContext& ctx = getContext(event);
        if(ctx.cb) {
            ctx.scheduler->schedule(&ctx.cb);   //将对象地址传入，进行交换，避免引用计数
        }
        else {
            ctx.scheduler->schedule(&ctx.fiber);    //TODO::这里感觉不会进入
        }
        ctx.scheduler = nullptr;
    }

    void IOManager::contextResize(size_t size)
    {
        m_fdContexts.resize(size);      //如果size 小于 当前的大小，则后面的被截断，如果大于，后面的则调用默认的构造函数

        for(size_t i = 0 ; i < m_fdContexts.size(); ++i) {
            if(!m_fdContexts[i]) {  //不存在，则创建
                m_fdContexts[i] = new FdContext;
                m_fdContexts[i]->fd = i;
            }
        }
    }

    IOManager::IOManager(size_t threads, bool use_caller, const std::string& name)
        : Scheduler(threads, use_caller, name)
    {
        m_epfd = epoll_create(5000);
        WYZE_ASSERT(m_epfd > 0);

        int rt = pipe(m_tickleFds);
        WYZE_ASSERT(!rt);

        epoll_event ev;
        memset(&ev, 0, sizeof(ev));
        ev.events = EPOLLET | EPOLLIN;
        ev.data.fd = m_tickleFds[0];
        
        rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &ev);
        WYZE_ASSERT(!rt);

        int flag = fcntl(m_tickleFds[0], F_GETFL);
        flag |= O_NONBLOCK;
        rt = fcntl(m_tickleFds[0], F_SETFL, flag);
        WYZE_ASSERT(!rt);

        contextResize(32);
        start();                //开启调度
    }

    IOManager::~IOManager()
    {
        stop();     //停止调度
        close(m_epfd);
        close(m_tickleFds[0]);
        close(m_tickleFds[1]);

        for(size_t i = 0; i < m_fdContexts.size(); ++i) {
            if(m_fdContexts[i]) {
                delete m_fdContexts[i];
            }
        }
    }
    
    // 0 success, -1 error
    int IOManager::addEvent(int fd, Event event, std::function<void()> cb)
    {
        FdContext* fd_ctx = nullptr;

        RWMutexType::ReadLock rlock(m_mutex);   //先加读锁
        if((int)m_fdContexts.size() > fd) { //已经分配好了内存空间
            fd_ctx = m_fdContexts[fd];
            rlock.unlock();
        }
        else {                              //没有分配内存空间
            rlock.unlock();     
            RWMutexType::WriteLock wlock(m_mutex);
            contextResize(fd * 1.5);        //一些子分配 1.5 倍空间，减少 写锁粒度
            fd_ctx = m_fdContexts[fd];
        }

        //对 fdContext 加锁， 避免多线程操作,    处理操作，增加过的事件再增加会报错
        FdContext::MutexTyp::Lock lock(fd_ctx->mutex);
        if(fd_ctx->events & event) {
            WYZE_LOG_ERROR(g_logger) << "addEvent assert fd=" << fd
                << " event=" << event
                << " fd_ctx.event=" << fd_ctx->events;
            WYZE_ASSERT(!(fd_ctx->events & event));
        }

        int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
        epoll_event ev;
        memset(&ev, 0, sizeof(ev));
        ev.events = EPOLLET | fd_ctx->events | event;
        ev.data.ptr = fd_ctx;

        int rt = epoll_ctl(m_epfd, op, fd, &ev);
        if(rt) {
            WYZE_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                << op << ", " << fd << ", " << ev.events << "):" 
                << rt << " (" << errno << ") (" << strerror(errno) << ")";
            return -1;
        }

        ++m_pendingEvent;
        fd_ctx->events = (Event)(fd_ctx->events | event);
        FdContext::EventContext& ev_ctx = fd_ctx->getContext(event);
        WYZE_ASSERT(!ev_ctx.scheduler
                        && !ev_ctx.fiber
                        && !ev_ctx.cb);
        
        // ev_ctx.scheduler = Scheduler::GetThis();     //这里可能会存在 创建当前的线程不进行调度
        ev_ctx.scheduler = static_cast<Scheduler*>(this);
        if(cb) {
            ev_ctx.cb.swap(cb);
        }
        else {      //TODO::这里是因为要挂起自己，
            ev_ctx.fiber = Fiber::GetThis();
            WYZE_ASSERT(ev_ctx.fiber->getState() == Fiber::State::EXEC);
        }
        return 0;
    }

    bool IOManager::delEvent(int fd, Event event)
    {
        FdContext* fd_ctx = nullptr;
        {
            RWMutexType::ReadLock rlock(m_mutex);
            if((int)m_fdContexts.size() <= fd) 
                return false;
            fd_ctx = m_fdContexts[fd];
        }

        FdContext::MutexTyp::Lock lock(fd_ctx->mutex);
        if(!(fd_ctx->events & event))   //不存在该事件则返回
            return false;
        
        Event new_ev = (Event)(fd_ctx->events & ~event);
        int op = new_ev ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
        epoll_event ev;
        memset(&ev, 0, sizeof(ev));
        ev.events = EPOLLET | new_ev;
        ev.data.ptr = fd_ctx;

        int rt = epoll_ctl(m_epfd, op, fd, &ev);
        if(rt) {
            WYZE_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                << op << ", " << fd << ", " << ev.events << "):" 
                << rt << " (" << errno << ") (" << strerror(errno) << ")";
            return false;
        }

        --m_pendingEvent;
        fd_ctx->events = new_ev;
        FdContext::EventContext& ev_ctx = fd_ctx->getContext(event);
        fd_ctx->resetContext(ev_ctx);
        return true;
    }

    bool IOManager::canceEvent(int fd, Event event)
    {
        FdContext* fd_ctx = nullptr;
        {
            RWMutexType::ReadLock rlock(m_mutex);
            if((int)m_fdContexts.size() <= fd)
                return false;
            fd_ctx = m_fdContexts[fd];
        }

        FdContext::MutexTyp::Lock lock(fd_ctx->mutex);
        if(!(fd_ctx->events & event))
            return false;

        Event new_ev = (Event)(fd_ctx->events & ~event);
        int op = new_ev ?  EPOLL_CTL_MOD : EPOLL_CTL_DEL;
        epoll_event ev;
        memset(&ev, 0, sizeof(ev));
        ev.events = EPOLLET | new_ev;
        ev.data.ptr = fd_ctx;

        int rt = epoll_ctl(m_epfd, op, fd, &ev);
        if(rt) {
            WYZE_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                << op << ", " << fd << ", " << ev.events << "):" 
                << rt << " (" << errno << ") (" << strerror(errno) << ")";
            return false;
        }

        fd_ctx->triggerEvent(event);
        --m_pendingEvent;
        return true;
    }

    bool IOManager::canceAll(int fd)
    {
        FdContext* fd_ctx = nullptr;
        {
            RWMutexType::ReadLock rlock(m_mutex);
            if((int)m_fdContexts.size() <= fd)
                return false;
            fd_ctx = m_fdContexts[fd];
        }

        FdContext::MutexTyp::Lock lock(fd_ctx->mutex);
        if(!fd_ctx->events) 
            return false;
        
        //删除
        int rt = epoll_ctl(m_epfd, EPOLL_CTL_DEL, fd, nullptr);
        if(rt) {
            WYZE_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                << EPOLL_CTL_DEL << ", " << fd << "):" 
                << rt << " (" << errno << ") (" << strerror(errno) << ")";
            return false;
        }

        if(fd_ctx->events & Event::READ) {
            fd_ctx->triggerEvent(Event::READ);
            --m_pendingEvent;
        }
        if(fd_ctx->events & Event::WRITE) {
            fd_ctx->triggerEvent(Event::WRITE);
            --m_pendingEvent;
        }

        WYZE_ASSERT(fd_ctx->events == 0);
        return true;
    }

    IOManager* IOManager::GetThis()
    {
        return dynamic_cast<IOManager*>(Scheduler::GetThis());
    }

    void IOManager::tickle()
    {
        if(hasIdleThreads()) 
            return;
        int rt = write(m_tickleFds[1], "T", 1);
        WYZE_ASSERT(rt == 1);
    }

    bool IOManager::stopping(uint64_t& timeout)
    {
        timeout = getNextTimer();
        return timeout == ~0ull
                && m_pendingEvent == 0
                && Scheduler::stopping();
    }

    bool IOManager::stopping()
    {
        uint64_t timeout = 0;
        return stopping(timeout);
    }

    void IOManager::idle()
    {
        static const int MAX_EVENTS = 128;
        epoll_event* evs = new epoll_event[MAX_EVENTS]();
        std::shared_ptr<epoll_event> del_evs(evs, [](epoll_event* ptr) {
            delete[] ptr;
        } );

        while(true) {
            uint64_t next_timeout = 0;
            if(stopping(next_timeout))  {
                // WYZE_LOG_INFO(g_logger) << "name=" << getName() << " idle stopping exit";
                break;
            }

            int rt = 0;
            memset(evs, 0, sizeof(epoll_event) * MAX_EVENTS);
            do {
                static const int MAX_TIMEOUT = 5000;    //TODO::这里不能大于 60 × 60 × 12 在定时器中，会处理时间修改问题
                if(next_timeout != ~0ull) {
                    next_timeout = (int)next_timeout < MAX_TIMEOUT ? next_timeout :  MAX_TIMEOUT;
                }
                else {
                    next_timeout = MAX_TIMEOUT;
                }

                rt = epoll_wait(m_epfd, evs, MAX_EVENTS, (int)next_timeout);  //有事件触发，这里会出现惊群
                if(rt < 0 && errno == EINTR) {
                }   //这里表示重试
                else {
                    break;
                }
            }while(true);

            std::vector<std::function<void()>> cbs;
            listExpiredCb(cbs);                         //这里会出现其他唤醒的线程也会卡住
            if(!cbs.empty()) {
                schedule(cbs.begin(), cbs.end());
                cbs.clear();
            }

            for(int i = 0; i < rt; ++i) {
                epoll_event& ev = evs[i];
                if(ev.data.fd == m_tickleFds[0]) {  //TODO::这里会不会出现地址和fd 相同的情况
                    uint8_t dummy;
                    while(read(m_tickleFds[0], &dummy, sizeof(dummy)) == 1);
                    continue;
                }

                FdContext* fd_ctx = (FdContext*) ev.data.ptr;
                FdContext::MutexTyp::Lock lock(fd_ctx->mutex);

                //获取 该事件由什么 事件类型触发
                if(ev.events & (EPOLLERR | EPOLLHUP)) {
                    ev.events |= (fd_ctx->events & EPOLLIN) | (fd_ctx->events & EPOLLOUT);        //当发生错误，会触发读写事件
                }
                int real_evs = Event::NONE;
                if(ev.events & EPOLLIN) 
                    real_evs |= Event::READ;
                if(ev.events & EPOLLOUT)
                    real_evs |= Event::WRITE;
                if( (fd_ctx->events & real_evs) == Event::NONE)
                    continue;
                
                //去掉触发的类型，保存未触发的事件类型
                int left_evs = (fd_ctx->events & ~real_evs);
                int op = left_evs ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
                ev.events = EPOLLET | left_evs;

                int rt2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &ev);
                if(rt2) {
                    WYZE_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                        << op << "," << fd_ctx->fd << "," << ev.events << "):"
                        << rt2 << " (" << errno << ") (" << strerror(errno) << ")";
                    continue;
                }

                //执行触发的事件
                if(real_evs & Event::READ) {
                    fd_ctx->triggerEvent(Event::READ);
                    --m_pendingEvent;
                }

                if(real_evs & Event::WRITE) {
                    fd_ctx->triggerEvent(Event::WRITE);
                    --m_pendingEvent;
                }
            }

            Fiber::YeildToReady();   //让出该协程
        }
    }


    void IOManager::onTimerInsertdAtFront()
    {
        tickle();
    }
}