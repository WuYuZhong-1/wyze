#include "../wyze/wyze.h"

int main(int argc, char** argv)
{
    wyze::Uri::ptr uri = wyze::Uri::Create("www.sylar.top");
    // wyze::Uri::ptr uri = wyze::Uri::Create("http://39.100.72.123:82");              //这种方法是不能实现

    if(!uri)
        std::cout << "uri is nullptr";
    std::cout << uri->toString() << std::endl;
    auto addr = uri->createAddress();
    if(!addr)
        std::cout << "addr is nullptr";
    std::cout << *addr << std::endl;

    return 0;
}