#include <memory>
#include <set>
#include "../wyze/wyze.h"



class Test_weak_ptr: public std::enable_shared_from_this<Test_weak_ptr> {
public:
    using ptr = std::shared_ptr<Test_weak_ptr>;
    using wptr = std::weak_ptr<Test_weak_ptr>;

    Test_weak_ptr(int a) ;
    int getValue() const { return m_a;}
    static Test_weak_ptr::ptr GetThis() ;
private:
    int m_a = 0;
};

static thread_local Test_weak_ptr* t_addr = nullptr; 


Test_weak_ptr::Test_weak_ptr(int a) 
    : m_a(a) {
        t_addr = this;
}

Test_weak_ptr::ptr Test_weak_ptr::GetThis() 
{
    return (t_addr ? t_addr->shared_from_this() : nullptr);
}

void test() 
{
    Test_weak_ptr::ptr test(new Test_weak_ptr(2));
    std::cout << "count = " << test.use_count() << std::endl;
    Test_weak_ptr::ptr test2 = Test_weak_ptr::GetThis();
    std::cout << "count = " << test.use_count() << std::endl;
    Test_weak_ptr::wptr wtest = Test_weak_ptr::GetThis();
    std::cout << "count = " << test.use_count() << std::endl;
}

static wyze::Logger::ptr g_logger = WYZE_LOG_ROOT();

wyze::Timer::ptr s_timer = nullptr;
void test_timer()
{
    wyze::IOManager iom(2);
    wyze::Timer::ptr timer = iom.addTimer(1000, [](){
        static int count = 0;
        WYZE_LOG_INFO(g_logger) << "timer1 count = " << count++;
    }, true);

    s_timer = iom.addTimer(2000, [timer](){
        static int count = 0;

        WYZE_LOG_INFO(g_logger) << "timer2 count = " << count;
        if(count == 4) 
            timer->reset(2000);
        if(count == 8)
            timer->cancel();
        ++count;

        if(count == 12)
            s_timer->cancel();
    }, true);
}



int main(int argc, char** argv)
{
    // test();
    // std::set<int> s;
    // s.insert(1);
    test_timer();
    return 0;
}