#include "thread.h"
#include <stdexcept>
#include "log.h"
#include "macro.h"
#include <iostream>

namespace wyze {

    // static thread_local Thread* t_thread = nullptr;
    static thread_local std::string t_thread_name = "UNKNOW";
    static Logger::ptr g_logger = WYZE_LOG_NAME("system");

    Semaphore::Semaphore(uint32_t count)
    {
        if(sem_init(&m_semaphore, 0, count)) {
            throw std::logic_error("sem_init");
        }
    }

    Semaphore::~Semaphore()
    {
        sem_destroy(&m_semaphore);
    }

    void Semaphore::wait()
    {
        if(sem_wait(&m_semaphore)) {
            throw std::logic_error("sem_wait");
        }
    }

    void Semaphore::notify()
    {
        if(sem_post(&m_semaphore)) {
            throw std::logic_error("sem_wait");
        }
    }

    Mutex::Mutex()
    {
        if(pthread_mutex_init(&m_mutex, nullptr)) {
            throw std::logic_error("pthread_mutex_init");
        }
    }

    Mutex::~Mutex()
    {
        pthread_mutex_destroy(&m_mutex);
    }

    void Mutex::lock()
    {
        if(pthread_mutex_lock(&m_mutex)) {
            throw std::logic_error("pthread_mutex_lock");
        }
    }

    void Mutex::unlock()
    {
        if(pthread_mutex_unlock(&m_mutex)) {
            throw std::logic_error("pthread_mutex_unlock");
        }
    }



    RWMutex::RWMutex()
    {
        if(pthread_rwlock_init(&m_lock,nullptr)) {
            throw std::logic_error("pthread_rwlock_init");
        }
    }

    RWMutex::~RWMutex()
    {
        pthread_rwlock_destroy(&m_lock);
    }

    void RWMutex::rdlock()
    {
        if(pthread_rwlock_rdlock(&m_lock)) {
            throw std::logic_error("pthread_rwlock_rdlock");
        }
    }

    void RWMutex::wrlock()
    {
        if(pthread_rwlock_wrlock(&m_lock)) {
            throw std::logic_error("pthread_rwlock_wrlock");
        }
    }

    void RWMutex::unlock()
    {
        if(pthread_rwlock_unlock(&m_lock)) {
            throw std::logic_error("pthread_rwlock_unlock");
        }
    }


    SpinLock::SpinLock()
    {
        if(pthread_spin_init(&m_mutex, 0)) {
            throw std::logic_error("pthread_spin_init");
        }
    }

    SpinLock::~SpinLock()
    {
        pthread_spin_destroy(&m_mutex);
    }

    void SpinLock::lock()
    {
        if(pthread_spin_lock(&m_mutex)) {
            throw std::logic_error("pthread_spin_lock");
        }
    }

    void SpinLock::unlock()
    {
        if(pthread_spin_unlock(&m_mutex)) {
            throw std::logic_error("pthread_spin_unlock");
        }
    }


    Thread::Thread(std::function<void()> cb, const std::string name)
        :m_cb(cb), m_name(name)
    {
        if(m_name.empty()) {
            m_name = "UNKNOW";
        }

        int rt = pthread_create(&m_thread, nullptr, &Thread::run, this);
        if(rt) {
            WYZE_LOG_ERROR(g_logger) << "pthread_create thread fail, rt=" << rt
                << " name=" << name;
            throw std::logic_error("pthread_create error");
        }

        //此函数的作用是避免子线程还没有运行，
        m_semaphore.wait();
    }

    Thread::~Thread()
    {
        if(m_thread) {
            pthread_detach(m_thread);
        }
    }

    void Thread::join()
    {
        if(m_thread) {
            int rt = pthread_join(m_thread, nullptr);
            if(rt) {
                WYZE_LOG_ERROR(g_logger) << "pthread_join thread fail, rt=" << rt
                    << " name=" << m_name;
                    throw std::logic_error("pthread_join error");
            }
            m_thread = 0;

        }
    }


    Thread* Thread::GetThis()
    {
        WYZE_ASSERT2(false, "GetThis is error");
        // return t_thread;
        return nullptr;
    }

    const std::string& Thread::GetName()
    {
        return t_thread_name;
    }

    void Thread::SetName(const std::string& name)
    {
        // if(t_thread) {
        //     t_thread->m_name = name;
        // }
        t_thread_name = name;
    }

    void* Thread::run(void* arg) 
    {
 
        Thread* thread = (Thread*)arg;
        // t_thread = thread;
        t_thread_name = thread->m_name;
        thread->m_id = wyze::GetThreadId();
        pthread_setname_np(pthread_self(), thread->m_name.substr(0,15).c_str());

        std::function<void()> cb;
        cb.swap(thread->m_cb);
        thread->m_semaphore.notify();

        cb();
        return 0;
    }

}