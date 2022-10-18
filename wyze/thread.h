#ifndef _WYZE_THREAD_H_
#define _WYZE_THREAD_H_

#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>
#include <atomic>
#include <memory>
#include <functional>

namespace wyze {

    class Semaphore {
    public:
        Semaphore(uint32_t count = 0);
        ~Semaphore();

        void wait();
        void notify();
    private:
        Semaphore(const Semaphore&) = delete;
        Semaphore(const Semaphore&&) = delete;
        Semaphore& operator=(const Semaphore&) = delete;
    private:
        sem_t m_semaphore;
    };


    template<class T>
    class ScopeLockImpl {
    public:
        ScopeLockImpl(T& mutex) 
            : m_mutex(mutex) {
            m_mutex.lock();
            m_locked = true;
        }

        ~ScopeLockImpl() {
            unlock();
        }

        void lock() {
            if(!m_locked) {
                m_mutex.lock();
                m_locked = true;
            }
        }

        void unlock() {
            if(m_locked) {
                m_mutex.unlock();
                m_locked = false;
            }
        }
    private:
        ScopeLockImpl(const ScopeLockImpl&)  = delete;
        ScopeLockImpl(const ScopeLockImpl&&) = delete;
        ScopeLockImpl& operator=(const ScopeLockImpl&) = delete;

    private:
        T& m_mutex;
        bool m_locked;
    };


    template<class T>
    class ReadScopeLockImpl {
    public:
        ReadScopeLockImpl(T& mutex) 
            : m_mutex(mutex) {
            m_mutex.rdlock();
            m_locked = true;
        }

        ~ReadScopeLockImpl() {
            unlock();
        }

        void lock() {
            if(!m_locked) {
                m_mutex.rdlock();
                m_locked = true;
            }
        }

        void unlock() {
            if(m_locked) {
                m_mutex.unlock();
                m_locked = false;
            }
        }
    private:
        ReadScopeLockImpl(const ReadScopeLockImpl&)  = delete;
        ReadScopeLockImpl(const ReadScopeLockImpl&&) = delete;
        ReadScopeLockImpl& operator=(const ReadScopeLockImpl&) = delete;

    private:
        T& m_mutex;
        bool m_locked;
    };

    template<class T>
    class WriteScopeLockImpl {
    public:
        WriteScopeLockImpl(T& mutex) 
            : m_mutex(mutex) {
            m_mutex.wrlock();
            m_locked = true;
        }

        ~WriteScopeLockImpl() {
            unlock();
        }

        void lock() {
            if(!m_locked) {
                m_mutex.wrlock();
                m_locked = true;
            }
        }

        void unlock() {
            if(m_locked) {
                m_mutex.unlock();
                m_locked = false;
            }
        }
    private:
        WriteScopeLockImpl(const WriteScopeLockImpl&)  = delete;
        WriteScopeLockImpl(const WriteScopeLockImpl&&) = delete;
        WriteScopeLockImpl& operator=(const WriteScopeLockImpl&) = delete;

    private:
        T& m_mutex;
        bool m_locked;
    };

    class Mutex {
    public:
        using Lock = ScopeLockImpl<Mutex>;

        Mutex();
        ~Mutex();
        void lock();
        void unlock();

    private:
        Mutex(const Mutex&)  = delete;
        Mutex(const Mutex&&) = delete;
        Mutex& operator=(const Mutex&) = delete;
    
    private:
        pthread_mutex_t m_mutex;
    };

    class RWMutex {
    public:
        using ReadLock = ReadScopeLockImpl<RWMutex>;
        using WriteLock = WriteScopeLockImpl<RWMutex>;

        RWMutex();
        ~RWMutex();
        void rdlock();
        void wrlock();
        void unlock();
    private:
        RWMutex(const RWMutex&)  = delete;
        RWMutex(const RWMutex&&) = delete;
        RWMutex& operator=(const RWMutex&) = delete;

    private:
        pthread_rwlock_t m_lock;
    };

    class SpinLock {
    public:
        using Lock = ScopeLockImpl<SpinLock>;
        SpinLock();
        ~SpinLock();
        void lock();
        void unlock();

    private:
        SpinLock(const SpinLock&) = delete;
        SpinLock(const SpinLock&&) = delete;
        SpinLock& operator=(const SpinLock&) = delete;
    
    private:
        pthread_spinlock_t m_mutex;
    };

    //原子操作自选锁
    class CASLock {
    public:
        using Lock = ScopeLockImpl<CASLock>;

        CASLock() {
            m_mutex.clear();
        }
        ~CASLock(){}
        void lock() {
            //设置成功，就会返回fase
            while(std::atomic_flag_test_and_set_explicit(&m_mutex, std::memory_order_acquire));
        }

        void unlock() {
            std::atomic_flag_test_and_set_explicit(&m_mutex, std::memory_order_release);
        }
    private:
        CASLock(const CASLock&) = delete;
        CASLock(const CASLock&&) = delete;
        CASLock& operator=(const CASLock&) = delete;
    private:
        volatile std::atomic_flag m_mutex;
    };


    class Thread {
    public:
        using ptr = std::shared_ptr<Thread>;
        Thread(std::function<void()> cb, const std::string name);
        ~Thread();

        pid_t getId() const { return m_id; }
        const std::string& getName() const { return m_name; };
        void join();
        //TODO::有一个bug，当该类析构后，不应该能拿到该对象
        static Thread* GetThis();
        static const std::string& GetName() ;
        static void SetName(const std::string& name);
    private:
        Thread(const Thread&) = delete;
        Thread(const Thread&&) = delete;
        Thread& operator=(const Thread&) = delete;

        static void* run(void* arg);

    private:
        pid_t m_id;
        pthread_t m_thread = 0;
        std::function<void()> m_cb;
        std::string m_name;
        Semaphore m_semaphore;
    };

}


#endif // !_WYZE_THREAD_H_






