#ifndef _WYZE_FDMANAGER_H_
#define _WYZE_FDMANAGER_H_

#include <memory>
#include <vector>
#include "thread.h"
#include "singleton.h"

namespace wyze {

    class FdCtx : public std::enable_shared_from_this<FdCtx> {
    public:
        using ptr = std::shared_ptr<FdCtx>;
        FdCtx(int fd);
        ~FdCtx(){}

        void init();
        bool isInit() const { return m_isInit; }
        bool isSocket() const { return m_isSocket; } 
        bool isClose() const { return m_isClosed; }

        void setUserNonblock(bool v) { m_userNonblock = v; }
        bool getUserNonblock() const { return m_userNonblock; }
        void setSysNonblock(bool v) { m_sysNonblock = v; }
        bool getSysNonblock() const { return m_sysNonblock; }

        void setTimeout(int type, uint64_t v);
        uint64_t getTimeout(int type) const ;
    private:   
        bool m_isInit: 1;           //是否初始化过
        bool m_isSocket: 1;         //文件描述符是否socket
        bool m_sysNonblock: 1;      //是否设置为 nonblick(非阻塞)
        bool m_userNonblock: 1;     //用户是否使用 非阻塞，用户设置，那么内部就不是用 hook函数
        bool m_isClosed: 1;
        int m_fd;
        uint64_t m_recvTimeout;
        uint64_t m_sendTimeout;
    };


    class FdManager {
    public:
        using RWMutexType = RWMutex;
        FdManager();

        FdCtx::ptr get(int fd, bool auto_create = false);
        void del(int fd);

    private:
        RWMutexType m_mutex;
        std::vector<FdCtx::ptr> m_datas;
    };

    using FdMgr = Single<FdManager>;
}

#endif // !_FDMANAGER_H_