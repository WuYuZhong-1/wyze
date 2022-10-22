#ifndef _WYZE_FIBER_H_
#define _WYZE_FIBER_H_

#include <memory>
#include <ucontext.h>
#include <functional>
#include <stdint.h>

namespace wyze {

class Scheduler;

class Fiber : public std::enable_shared_from_this<Fiber> {
friend class Scheduler;
public:
    using ptr = std::shared_ptr<Fiber>;

    enum State {
        INIT,
        HOLD,
        READY,
        EXEC,
        TERM,
        EXCEPT
    };

    Fiber(std::function<void()> f, size_t stack_size = 0, bool use_caller = false);
    ~Fiber();

    //重置携程函数，并且重置状态为 INIT
    void reset(std::function<void()> cb);
    //从调度协程切换到当前协程
    void swapIn();
    // //从当前协程切换到调度协程
    // void swapOut();
    //从主协程切换到当前协程
    void call();
    //从当前协程返回主协程
    void back();

    uint64_t getId() const { return m_id; }
    State getState() const { return m_state;}

    //设置当前协程
    static void SetThis(Fiber* f);
    //返回当前协程
    static Fiber::ptr GetThis();
    //当前协程切换到后台，并设置状态为READY
    static void YeildToReady();
    // 当前协程切换到后台，并设置状态为HOLD
    static void YeildToHold();
    static uint64_t TotalFibers();
    static void MainFunc();
    static uint64_t GetFiberId();

private:
    Fiber();        //在没有协程时，线程获取自己的协程所使用
    //从当前协程切换到调度协程
    void swapOut();


private:
    uint64_t m_id = 0;
    uint32_t m_stacksize = 0;
    void* m_stack = nullptr;
    State m_state = State::INIT;
    ucontext_t m_context;
    std::function<void()> m_cb;
    bool m_useCaller = false;

};


}

#endif // !_WYZE_FIBER_H_