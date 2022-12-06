#include "fdmanager.h"
#include "hook.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "macro.h"

namespace wyze {

    FdCtx::FdCtx(int fd)
        :m_isInit(false)
        ,m_isSocket(false)
        ,m_sysNonblock(false)
        ,m_userNonblock(false)
        ,m_isClosed(false)
        ,m_fd(fd)
        ,m_recvTimeout(-1)
        ,m_sendTimeout(-1)
    {
        init();
    }

    void FdCtx::init()
    {
        if(m_isInit)
            return;

        m_recvTimeout = m_sendTimeout = -1;
        m_isClosed = false;
        m_userNonblock = false;

        struct stat fd_stat = {0};
        if(-1 == fstat(m_fd, &fd_stat)) {
            return;
        }
        
        m_isSocket = S_ISSOCK(fd_stat.st_mode);
        if(m_isSocket) {
            int flag = fcntl_f(m_fd, F_GETFL);
            if( !(flag & O_NONBLOCK))
                fcntl_f(m_fd, F_SETFL, flag | O_NONBLOCK);
            m_sysNonblock  = true;
        }
        else {
            m_sysNonblock = false;
        }

        m_isInit = false;
    }

    void FdCtx::setTimeout(int type, uint64_t v)
    {
        switch(type) {
        case SO_RCVTIMEO:
            m_recvTimeout = v;
            break;
        case SO_SNDTIMEO:
            m_sendTimeout = v;
            break;
        default:
            WYZE_ASSERT2(false, "setTimeout");
        }
    }

    uint64_t FdCtx::getTimeout(int type) const 
    {
        switch(type) {
        case SO_RCVTIMEO:
            return m_recvTimeout;
        case SO_SNDTIMEO:
            return m_sendTimeout;
        default:
            WYZE_ASSERT2(false, "getTimeout");
            return 0;
        }
    }

    FdManager::FdManager()
    {
        m_datas.resize(1024);
    }

    FdCtx::ptr FdManager::get(int fd, bool auto_create)
    {
        if(auto_create) {
            RWMutexType::WriteLock wlock(m_mutex);
            if( ((int) m_datas.size() <= fd)
                || ((int)m_datas.size() > fd && m_datas[fd] == nullptr) ) {
                FdCtx::ptr ctx(new FdCtx(fd));
                m_datas[fd] = ctx;
                return ctx;
            }
            else {
                return m_datas[fd];
            }     
        }
        else {
            RWMutexType::ReadLock rlock(m_mutex);
            if((int) m_datas.size() <= fd)
                return nullptr;
            return m_datas[fd];
        } 
    }

    void FdManager::del(int fd)
    {
        RWMutexType::WriteLock wlock(m_mutex);
        if( (int)m_datas.size() <= fd)  {
            return;
        }
        m_datas[fd].reset();
    }

}