#include "fiber.h"
#include <atomic>
#include <stdlib.h>
#include "config.h"
#include "macro.h"
#include "scheduler.h"

namespace wyze {

static Logger::ptr g_logger = WYZE_LOG_NAME("system");

static std::atomic<uint64_t> s_fiber_id(0);     //TODO::协程id，当不停运行时，这个id可能会重复
static std::atomic<uint64_t> s_fiber_count {0}; //总的协程数

static thread_local Fiber* t_fiber = nullptr;   //当前运行的协程
static thread_local Fiber::ptr t_threadFiber = nullptr;     //main 协程对象


static ConfigVar<uint64_t>::ptr g_fiber_stack_size = 
            Config::Lookup<uint64_t>("fiber.stack_size", 16 * 1024, "fiber stack size");

class MallocStackAllocator {
public:
    static void* Alloc(size_t size) {
        return malloc(size);
    } 

    static void Dealloc(void* p, size_t size) {
        return free(p);
    }
};

using StackAllocator = MallocStackAllocator;

Fiber::Fiber()
{
    m_state = State::EXEC;  
    SetThis(this);

    if(getcontext(&m_context)){
        WYZE_ASSERT2(false, "getcontext");
    }

    m_id = s_fiber_id++;
    ++s_fiber_count;
    // WYZE_LOG_DEBUG(g_logger) << "Fiber::Fiber id = " << m_id;
}  

Fiber::Fiber(std::function<void()> cb, size_t stack_size, bool use_caller)
    : m_id(s_fiber_id++)
    , m_cb(cb)
    , m_useCaller(use_caller)
{
    ++s_fiber_count;
    m_stacksize = stack_size > (16 * 1024) ? stack_size : g_fiber_stack_size->getValue();
    m_stack = StackAllocator::Alloc(m_stacksize);

    if(getcontext(&m_context)) {
        WYZE_ASSERT2(false, "getcontet");
    }

    m_context.uc_link = nullptr;
    m_context.uc_stack.ss_size = m_stacksize;
    m_context.uc_stack.ss_sp = m_stack;

    makecontext(&m_context, &Fiber::MainFunc, 0);
    // WYZE_LOG_DEBUG(g_logger) << "Fiber::Fiber id = " << m_id;
}

Fiber::~Fiber()
{
    --s_fiber_count;
    if(m_stack) {
        //子协程
        WYZE_ASSERT( m_state == State::INIT 
                    || m_state == State::TERM
                    || m_state == State::EXCEPT );
        StackAllocator::Dealloc(m_stack, m_stacksize);
    }
    else {
        //main 协程
        WYZE_ASSERT(!m_cb);
        WYZE_ASSERT(m_state == State::EXEC);

        Fiber* cur = t_fiber;
        if(cur == this) {
            SetThis(nullptr);
        }
    }

    // WYZE_LOG_DEBUG(g_logger) << "Fiber::~Fiber id=" << m_id;
}

//重置携程函数，并且重置状态为 INIT
void Fiber::reset(std::function<void()> cb)
{
    WYZE_ASSERT(m_stack);
    WYZE_ASSERT( m_state == State::INIT
                    || m_state == State::TERM
                    || m_state == State::EXCEPT);
    m_cb = cb;
    if(getcontext(&m_context)) {
        WYZE_ASSERT2(false, "getcontext");
    }

    m_context.uc_link = nullptr;
    m_context.uc_stack.ss_size = m_stacksize;
    m_context.uc_stack.ss_sp = m_stack;

    makecontext(&m_context,&Fiber::MainFunc, 0);
    m_state = State::INIT;
}

void Fiber::call() 
{
    WYZE_ASSERT(m_state != State::EXEC);
    WYZE_ASSERT2(t_threadFiber != nullptr, "use Fiber::GetThis befor use Fiber::call");
    if(m_state == State::TERM || m_state == State::EXCEPT)
        return;

    SetThis(this);
    m_state = State::EXEC;
    if(swapcontext(&t_threadFiber->m_context, &m_context) ) {
        WYZE_ASSERT2(false, "swapcontext");
    }
}

void Fiber::back() 
{
    WYZE_ASSERT2(t_threadFiber != nullptr, "Fiber::call Fiber::back is pair");
    SetThis(t_threadFiber.get());

    if(m_state == State::EXEC)
        m_state = State::READY;

    if(swapcontext(&m_context, &t_threadFiber->m_context)) {
        WYZE_ASSERT2(false, "swapcontext");
    }
}

//切换到当前携程执行
void Fiber::swapIn()
{
    WYZE_ASSERT(m_state != State::EXEC);
    WYZE_ASSERT2(Scheduler::GetMainFiber() != nullptr, "make sure Scheduler init befor use Fiber::swapIn");
    if(m_state == State::TERM || m_state == State::EXCEPT)
        return;

    SetThis(this);
    
    m_state = State::EXEC;

    if(swapcontext(&Scheduler::GetMainFiber()->m_context, &m_context)) {
        WYZE_ASSERT2(false, "swapcontext");
    }
}

//切换到后台执行
void Fiber::swapOut()
{
    WYZE_ASSERT2(Scheduler::GetMainFiber() != nullptr, "Fiber::swapIn Fiber::swapOut is pair");
    SetThis(t_threadFiber.get());

    if(swapcontext(&m_context, &Scheduler::GetMainFiber()->m_context)){
        WYZE_ASSERT2(false,"swapcontext");
    }

}

//设置当前协程
void Fiber::SetThis(Fiber* f)
{
    t_fiber = f;
}

//返回当前协程
Fiber::ptr Fiber::GetThis()
{
    if(t_fiber) {
        return t_fiber->shared_from_this();
    }

    Fiber::ptr main_fiber(new Fiber);
    WYZE_ASSERT(t_fiber == main_fiber.get());
    t_threadFiber = main_fiber;
    return t_fiber->shared_from_this();
}

//当前协程切换到后台，并设置状态为READY
void Fiber::YeildToReady()
{
    Fiber::ptr cur = GetThis();
    cur->m_state = State::READY;
    cur->swapOut();
}

// 当前协程切换到后台，并设置状态为HOLD
void Fiber::YeildToHold()
{
    Fiber::ptr cur = GetThis();
    cur->m_state = State::HOLD;
    cur->swapOut();
}

uint64_t Fiber::TotalFibers()
{
    return s_fiber_count;
}

uint64_t Fiber::GetFiberId()
{
    if(t_fiber) {
        return t_fiber->getId();
    }
    return 0;
}

void Fiber::MainFunc()
{
    Fiber::ptr cur = GetThis();
    WYZE_ASSERT(cur);

    try{
        cur->m_cb();
        cur->m_cb = nullptr;
        cur->m_state = State::TERM;
    }
    catch(std::exception& ex) {
        cur->m_state = State::EXCEPT;
        WYZE_LOG_ERROR(g_logger) << "Fiber execption: " << ex.what()
            << " fiber_id=" << cur->getId()
            << std::endl
            << BacktraceToString();
    }
    catch(...) {
        cur->m_state = State::EXCEPT;
        WYZE_LOG_ERROR(g_logger) << "Fiber execption: "
            << " fiber_id=" << cur->getId()
            << std::endl
            << BacktraceToString();
    }

    auto raw_ptr = cur.get();
    cur.reset();
    if(!raw_ptr->m_useCaller) {
        raw_ptr->swapOut();
    }
    else {
        raw_ptr->back();
    }

    WYZE_ASSERT2(false,"never reach");
}

}