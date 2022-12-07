#include "hook.h"
#include "log.h"
#include "fiber.h"
#include "iomanager.h"
#include "fdmanager.h"
#include "config.h"
#include "macro.h"

#include <dlfcn.h>
#include <errno.h>

wyze::Logger::ptr g_logger = WYZE_LOG_NAME("system");

namespace wyze {

    static ConfigVar<int>::ptr g_tcp_connect_timeout = 
        Config::Lookup("tcp.connect.timeout", 5000, "tcp connect timeout");

    static thread_local bool t_hook_enable = false;

    #define HOOK_FUN(XX)    \
        XX(sleep)           \
        XX(usleep)          \
        XX(nanosleep)       \
        XX(socket)          \
        XX(connect)         \
        XX(accept)          \
        XX(close)           \
        XX(read)            \
        XX(readv)           \
        XX(recv)            \
        XX(recvfrom)        \
        XX(recvmsg)         \
        XX(write)           \
        XX(writev)          \
        XX(send)            \
        XX(sendto)          \
        XX(sendmsg)         \
        XX(fcntl)           \
        XX(ioctl)           \
        XX(getsockopt)      \
        XX(setsockopt)     


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

    static uint64_t s_connect_timeout = -1;
    struct _HookIniter {
        _HookIniter() {
            hook_init();
            s_connect_timeout = g_tcp_connect_timeout->getValue();

            g_tcp_connect_timeout->addListener([](const int& old_value, const int& new_value) {
                WYZE_LOG_INFO(g_logger) << "tcp connect timeout changed from" 
                                        << old_value << " to " << new_value;
                s_connect_timeout = new_value;
            });
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

    struct TimerCond {      //hook fd 时， 添加定时器，触发时的条件，
        int cancelled = 0;  // 当该变量不存在则不会触发定时器(也就是说数据在定时器来之前触发)
    };

    //模板类，如何 hook 非阻塞fd ，使之让用户 同步使用
    template<typename OriginFun, typename ... Args>
    static ssize_t do_io(int fd, OriginFun fun, const char* hook_fun_name,
                            uint32_t event, int timeout_so, Args&& ... args) 
    {
        if( !t_hook_enable ) 
            return fun(fd, std::forward<Args>(args)...);

        // WYZE_LOG_INFO(g_logger) << "do_io";
        
        FdCtx::ptr ctx = FdMgr::GetInstance()->get(fd);
        if(!ctx) 
            return fun(fd, std::forward<Args>(args)...);
        
        if(ctx->isClose()) {
            errno = EBADF;
            return -1;
        }

        //如果是用户设置了非阻塞，表示用户层使用自己的非阻塞模式
        if(!ctx->isSocket() || ctx->getUserNonblock())
            return fun(fd, std::forward<Args>(args)...);

        ssize_t n;
        uint64_t ms = ctx->getTimeout(timeout_so);
        // WYZE_LOG_INFO(g_logger) << hook_fun_name << "  do_io ms=" << ms;
        do {
            errno = 0;
            n = fun(fd, std::forward<Args>(args)...);
            while( n == -1 && errno == EINTR)
                n = fun(fd, std::forward<Args>(args)...);
            
            if( n == -1 && errno == EAGAIN ) {  //如果是 -1 且 errno 提示重试，则进入阻塞

                IOManager* iom = IOManager::GetThis();
                int rt = iom->addEvent(fd, (IOManager::Event)(event));
                if(rt) {
                    WYZE_LOG_ERROR(g_logger) << hook_fun_name << " addEvent("
                                            << fd << ", " << event << ")";
                    return -1;
                }
                else {

                    std::shared_ptr<TimerCond> tcnd(new TimerCond);
                    std::weak_ptr<TimerCond> wtcnd(tcnd);
                    Timer::ptr timer;

                    if(ms != (uint64_t)0 ) {    //在挂起之前检测有没有定时

                        timer = iom->addConditionTimer(ms, [wtcnd, fd, iom, event]() {
                            auto t = wtcnd.lock();
                            if( !t || t->cancelled) //当指针不存在，获取指针里面设置为取消，直接返回
                                return ;
                            t->cancelled = ETIMEDOUT;
                            iom->canceEvent(fd, (IOManager::Event)(event)); //唤醒协程
                        }, wtcnd);
                    }

                    Fiber::YeildToHold();       //挂起
                                                //这里开始，表示定时器，获取添加的fd事件触发
                    if(timer)
                        timer->cancel();

                    if(tcnd->cancelled) {       //如果该值不为0 表示超时触发
                        errno = tcnd->cancelled;
                        iom->delEvent(fd, (IOManager::Event)(event));   //删除事件
                        return -1;
                    }  
                }
            }
            else 
                break;
            
        } while(true);

        // WYZE_LOG_INFO(g_logger) << hook_fun_name << " end  do_io "
        //         << "  n=" << n  << "  errno=" << errno;
        return n;
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

    int nanosleep(const struct timespec *req, struct timespec* rem)
    {
        if(!wyze::t_hook_enable)
            return nanosleep(req, rem);

        int timeout_ms = req->tv_sec * 1000 + req->tv_nsec / 1000 / 1000;
        wyze::Fiber::ptr fiber = wyze::Fiber::GetThis();
        wyze::IOManager* iom = wyze::IOManager::GetThis();
        iom->addTimer(timeout_ms, std::bind((void(wyze::Scheduler::*)(wyze::Fiber::ptr, int))
                                                &wyze::IOManager::schedule, iom, fiber, -1));
        wyze::Fiber::YeildToHold();
        return 0;
    }

    int socket(int domain, int type, int protocol)
    {
        int fd = socket_f(domain, type, protocol);

        if( (!wyze::t_hook_enable) || (fd == -1) ) 
            return fd;

        wyze::FdMgr::GetInstance()->get(fd, true);
        return fd; 
    }

    int connect_with_tiemout(int fd, const struct sockaddr* addr, socklen_t addrlen, uint64_t timeout_ms)
    {
        if( !wyze::t_hook_enable )
            return connect_f(fd, addr, addrlen);

        //  WYZE_LOG_INFO(g_logger) << "connect_with_tiemout";
        
        wyze::FdCtx::ptr ctx = wyze::FdMgr::GetInstance()->get(fd);
        if( !ctx || ctx->isClose() ) {
            errno = EBADF;
            return -1;
        }

        if( !ctx->isSocket() || ctx->getUserNonblock() )    //用户使用非阻塞，外部会处理
            return connect_f(fd, addr, addrlen);

        int n = connect_f(fd, addr, addrlen);
        if(n == 0)          //返回 表示错误
            return 0;
        else if( n != -1 || errno != EINPROGRESS)
            return n;

        //这里处理 返回 -1 且错误码为 EINPROGRESS 的情况
        wyze::IOManager* iom = wyze::IOManager::GetThis();

        int rt = iom->addEvent(fd, wyze::IOManager::WRITE);     //检测可写表示真正的连接成功
        if(rt) {
            WYZE_LOG_ERROR(g_logger) << "connect addEvent(" << fd << " , WRITE) error";
        }
        else {
            //处理定时器
            wyze::Timer::ptr timer;
            std::shared_ptr<wyze::TimerCond> tcnd(new wyze::TimerCond);
            std::weak_ptr<wyze::TimerCond> wtcnd(tcnd);

            if(timeout_ms != (uint64_t)-1) {
                timer = iom->addConditionTimer(timeout_ms, [wtcnd, fd, iom](){
                    auto t = wtcnd.lock();
                    if(!t || t->cancelled)
                        return;
                    t->cancelled = ETIMEDOUT;
                    iom->canceEvent(fd, wyze::IOManager::WRITE);
                }, wtcnd);
            }

            wyze::Fiber::YeildToHold();
                                            //这里表示触发
            if(timer)
                timer->cancel();
            
            if(tcnd->cancelled) {           //超时唤醒，直接返回
                errno = tcnd->cancelled;
                iom->delEvent(fd, wyze::IOManager::WRITE);
                return -1;
            }
        }
        
        //读事件触发
        int error = 0;
        socklen_t len = sizeof(int);
        if( -1 == getsockopt_f(fd, SOL_SOCKET, SO_ERROR, &error, &len) ) 
            return -1;

        if(!error)
            return 0;
        else {
            errno = error;
            return -1;
        }
    }

    int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
    {
        return connect_with_tiemout(sockfd, addr, addrlen, wyze::s_connect_timeout);
    }


    int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
    {
        int fd = wyze::do_io(sockfd, accept_f, "accept", wyze::IOManager::READ 
                                ,SO_RCVTIMEO, addr, addrlen);
        if(fd >= 0) 
            wyze::FdMgr::GetInstance()->get(fd, true);
        
        return fd;
    }

    int close(int fd)
    {
        if( wyze::t_hook_enable ) { //如果 时能hook ，则做收尾处理

            wyze::FdCtx::ptr ctx = wyze::FdMgr::GetInstance()->get(fd);
            if(ctx) {   
                auto iom = wyze::IOManager::GetThis();
                if(iom)         
                    iom->canceAll(fd);      //取消事件
                
                wyze::FdMgr::GetInstance()->del(fd);
            }
        }

        return close_f(fd);
    }

    //read
    ssize_t read(int fd, void *buf, size_t count)
    {
        return wyze::do_io(fd, read_f, "read", wyze::IOManager::READ
                                , SO_RCVTIMEO, buf, count);
    }

    ssize_t readv(int fd, const struct iovec *iov, int iovcnt)
    {
        return wyze::do_io(fd, readv_f, "readv", wyze::IOManager::READ
                                , SO_RCVTIMEO, iov, iovcnt);
    }

    ssize_t recv(int sockfd, void *buf, size_t len, int flags)
    {
        return wyze::do_io(sockfd, recv_f, "recv", wyze::IOManager::READ
                                , SO_RCVTIMEO, buf, len, flags);
    }

    ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen)
    {
        return wyze::do_io(sockfd, recvfrom_f, "recvfrom", wyze::IOManager::READ
                                , SO_RCVTIMEO, buf, len,  flags, src_addr, addrlen);
    }

    ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags)
    {
        return wyze::do_io(sockfd, recvmsg_f, "recvmsg", wyze::IOManager::READ
                                , SO_RCVTIMEO, msg, flags);
    }

    // write
    ssize_t write(int fd, const void *buf, size_t count)
    {
        return wyze::do_io(fd, write_f, "write", wyze::IOManager::WRITE
                                , SO_SNDTIMEO, buf, count);
    }

    ssize_t writev(int fd, const struct iovec *iov, int iovcnt)
    {
        return wyze::do_io(fd, writev_f, "writev", wyze::IOManager::WRITE
                                , SO_SNDTIMEO, iov, iovcnt);
    }

    ssize_t send(int sockfd, const void *buf, size_t len, int flags)
    {
        return wyze::do_io(sockfd, send_f, "send", wyze::IOManager::WRITE
                                , SO_SNDTIMEO, buf, len, flags);
    }

    ssize_t sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen)
    {
        return wyze::do_io(sockfd, sendto_f, "sendto", wyze::IOManager::WRITE
                                , SO_SNDTIMEO, buf, len, flags, dest_addr, addrlen);
    }

    ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags)
    {
        return wyze::do_io(sockfd, sendmsg_f, "sendmsg", wyze::IOManager::WRITE
                                , SO_SNDTIMEO, msg, flags);
    }

    // TODO::这里的设置操作，读写操作和设置操作不再同一个线程可能会出现错误
    int fcntl(int fd, int cmd, ... /*arg*/)
    {
        va_list var;
        va_start(var, cmd);

        switch (cmd) {
            case F_SETFL:
                {
                    int arg = va_arg(var, int);
                    va_end(var);
                    if(wyze::t_hook_enable) {

                        wyze::FdCtx::ptr ctx = wyze::FdMgr::GetInstance()->get(fd);
                        if(!ctx || ctx->isClose() || !ctx->isSocket())
                            return fcntl_f(fd, cmd, arg);

                        ctx->setUserNonblock(arg & O_NONBLOCK);
                        if(ctx->getSysNonblock()) {
                            arg |= O_NONBLOCK;
                        }
                        else {
                            arg &= ~O_NONBLOCK;
                        }
                    }

                    return fcntl_f(fd, cmd, arg);
                }
                break;
            case F_GETFL:
                {
                    va_end(var);
                    int arg = fcntl_f(fd, cmd);

                    if( wyze::t_hook_enable ) {
                        wyze::FdCtx::ptr ctx = wyze::FdMgr::GetInstance()->get(fd);
                        if(!ctx || ctx->isClose() || !ctx->isSocket())
                            return arg;
                        
                        if(ctx->getUserNonblock())
                            arg |= O_NONBLOCK;
                        else 
                            arg &= ~O_NONBLOCK;
                    }
                    return arg;
                }
                break;
            case F_DUPFD:
            case F_DUPFD_CLOEXEC:
            case F_SETFD:
            case F_SETOWN:
            case F_SETSIG:
            case F_SETLEASE:
            case F_NOTIFY:
            case F_SETPIPE_SZ:
            case F_ADD_SEALS:
                {
                    int arg = va_arg(var,int);
                    va_end(var);
                    return fcntl_f(fd, cmd, arg);
                }
                break;
            case F_GETFD:
            case F_GETOWN:
            case F_GETLEASE:
            case F_GETPIPE_SZ:
            case F_GET_SEALS:
                {
                    va_end(var);
                    return fcntl_f(fd,cmd);
                }
                break;
            case F_SETLK:
            case F_SETLKW:
            case F_GETLK:
            case F_OFD_SETLK:
            case F_OFD_SETLKW:
            case F_OFD_GETLK:
                {
                    struct flock* arg = va_arg(var, struct flock*);
                    return fcntl_f(fd, cmd, arg);
                }
                break;
            case F_GETOWN_EX:
            case F_SETOWN_EX:
                {
                    struct f_owner_ex* arg = va_arg(var, struct f_owner_ex*);
                    return fcntl_f(fd, cmd, arg);
                }
                break;
            case F_GET_RW_HINT:
            case F_SET_RW_HINT:
            case F_GET_FILE_RW_HINT:
            case F_SET_FILE_RW_HINT:
                {
                    uint64_t* arg = va_arg(var, uint64_t*);
                    return fcntl_f(fd, cmd, arg);
                }
        default:
            WYZE_ASSERT2(false,"cmd=" + std::to_string(cmd));
        }

        return 0;
    }

    int ioctl(int fd, unsigned long request, ...)
    {
        va_list va;
        va_start(va, request);
        void *arg = va_arg(va, void*);
        va_end(va);

        if(FIONBIO == request && wyze::t_hook_enable) {

            bool user_nonblock = !!(*(int*)arg);            //非零表示允许非阻塞， 零表示禁止非阻塞

            wyze::FdCtx::ptr ctx = wyze::FdMgr::GetInstance()->get(fd);
            if(ctx && !ctx->isClose() && ctx->isSocket() ) {
                
                ctx->setUserNonblock(user_nonblock);    //设置用户是否阻塞
                int nonblock = 1;                       //不管用户设置阻塞模式，系统内部都是非阻塞
                return  ioctl_f(fd, request, &nonblock);
            }
        }
        return ioctl_f(fd, request, arg);
    }

    int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen)
    {
        return getsockopt_f(sockfd, level, optname, optval, optlen);
    }

    int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen)
    {
        if(wyze::t_hook_enable) {

            if( level == SOL_SOCKET
                && (optname == SO_RCVTIMEO || optname == SO_SNDTIMEO)) {
                    
                    wyze::FdCtx::ptr ctx = wyze::FdMgr::GetInstance()->get(sockfd);
                    if(ctx) {
                        
                        const timeval* v = (const timeval*)optval;
                        ctx->setTimeout(optname, v->tv_sec * 1000 + v->tv_usec / 1000);
                    }
                return 0;
            }
        }

        return setsockopt_f(sockfd, level, optname, optval, optlen);
    }
}