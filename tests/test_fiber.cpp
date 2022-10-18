#include <iostream>
#include <ucontext.h>
#include <thread>
#include "../wyze/wyze.h"

bool flag = false;

void test1()
{

    ucontext_t uc;
    getcontext(&uc);
    if(flag) {
        std::cout << "flag is true" << std::endl;
        return;
    }
    flag = true;
    std::cout << "flag is false" << std::endl;
    setcontext(&uc);
    std::cout << "test1 end" << std::endl;
}


void test2_fun()
{
    for(size_t i = 0; i < 10; i++) {
        std::cout << "i = " << i << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void test2()
{
    ucontext_t context;
    getcontext(&context);   //获取当前上下文
    context.uc_link = nullptr; //设置后继上下文，
    context.uc_stack.ss_sp = malloc(1024*1024); //设置当前上下文的栈地址
    context.uc_stack.ss_size = 1024*1024;   //设置栈大小
    makecontext(&context, &test2_fun, 0);   //修改当前上下文指向test2_fun， 参数为 0
    setcontext(&context);   //切换当前上下文
    std::cout << "finished" << std::endl;   //这里不会运行
}

ucontext_t _main;

void test3_fun() 
{
    test2_fun();
    // setcontext(&_main);
}

void test3()
{
    ucontext_t child;
    getcontext(&child);     //获取当前上下文
    child.uc_link = &_main; // 设置后继上下文
    // child.uc_link = nullptr;
    child.uc_stack.ss_sp = malloc(1024 * 1024);
    child.uc_stack.ss_size = 1024* 1024;
    makecontext(&child, &test3_fun,0);
    swapcontext(&_main,&child);
    std::cout << "finished" << std::endl;   //这里不会运行
    if(child.uc_stack.ss_sp)
        free(child.uc_stack.ss_sp);
}


static wyze::Logger::ptr g_logger = WYZE_LOG_NAME("system");

void test_fiber_fun()
{
    int count = 0;
    for(; count < 10; count++) {
        WYZE_LOG_INFO(g_logger) << "test_fiber count = " << count ;
        wyze::Fiber::YeildToReady();
    }

}

void test_fiber()
{
    // wyze::Thread::SetName("main");
    wyze::Fiber::GetThis();
    wyze::Fiber::ptr fiber(new wyze::Fiber(&test_fiber_fun, 1024 * 1024));
    wyze::Fiber::ptr fiber2(new wyze::Fiber(&test_fiber_fun, 1024 * 1024));

    for(int i = 0; i < 12; ++i) {
        WYZE_LOG_INFO(g_logger) << "main fiber i " << i ;
        fiber->swapIn();
        fiber2->swapIn();
    }
    WYZE_LOG_INFO(g_logger) << "test_fiber  end" ;
}

void test_thread_fiber()
{
    std::vector<wyze::Thread::ptr> ths;

    for(int i = 0; i < 3; ++i) {
        ths.push_back(wyze::Thread::ptr(
                new wyze::Thread(&test_fiber, "test_" + std::to_string(i))
                ));
    }

    for(size_t i = 0; i < ths.size() ; ++i) {
        ths[i]->join();
    }
}


int main(int argc, char** argv)
{
    std::cout << "main start\n";
    // test1();
    // test2();
    // test3();
    // test_fiber();
    test_thread_fiber();
    std::cout << "main endl\n"; //这里也不会在执行
    return 0;
}