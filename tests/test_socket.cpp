#include "../wyze/wyze.h"


wyze::Logger::ptr g_logger = WYZE_LOG_ROOT();


void test_shared_ptr()
{
    std::shared_ptr<std::string> p1;
    std::shared_ptr<std::string> p2 ;
    p2 = p1;
    WYZE_LOG_INFO(g_logger) << p2.use_count();
}

void test_socket()
{
    wyze::IPAddress::ptr addr = wyze::Address::LookAnyIPAddress("www.baidu.com", AF_INET, SOCK_STREAM);
    if(addr) {
        addr->setPort(80);
        WYZE_LOG_INFO(g_logger) << "get address: " << addr->toString();
    }
    else {
        WYZE_LOG_ERROR(g_logger) << "get address fail";
    }
    
    wyze::Socket::ptr sock = wyze::Socket::CreateTCP(addr);
    if(!sock->connect(addr)) {
        WYZE_LOG_ERROR(g_logger) << "connect " << addr->toString() << " fail";
        return ;
    }
    else {
        WYZE_LOG_INFO(g_logger) << "connect " << addr->toString() << " connected";
    }

    const char buff[] = "GET / HTTP/1.0\r\n\r\n\r\n";
    int rt = sock->send(buff, sizeof(buff));
    if(rt <= 0) {
        WYZE_LOG_INFO(g_logger) << "send fail rt=" << rt;
        return ;
    }

    std::string buffs;
    buffs.resize(4096);     //string 字符床申请内存
    rt = sock->recv(&buffs[0], buffs.size());
    if(rt <= 0) {
        WYZE_LOG_ERROR(g_logger) << "recv fail rt=" << rt;
        return ;
    }

    buffs.resize(rt);       //字符床的截取
    WYZE_LOG_INFO(g_logger) << "\r\n" << buffs;

}

int main(int argc, char** argv)
{
    // test_shared_ptr();
    wyze::IOManager iom;
    iom.schedule(&test_socket);
    return 0;
}