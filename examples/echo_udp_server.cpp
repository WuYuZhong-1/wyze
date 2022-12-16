#include "../wyze/wyze.h"

static wyze::Logger::ptr g_logger = WYZE_LOG_ROOT();

void run()
{
    wyze::IPAddress::ptr addr = wyze::IPAddress::Create("0.0.0.0", 12345);
    wyze::Socket::ptr sock = wyze::Socket::CreateUDP(addr);
    if(sock->bind(addr)) {
        WYZE_LOG_INFO(g_logger) << "udp bind: " << *addr;
    }
    else {
        WYZE_LOG_ERROR(g_logger) << "udp bind: " << *addr << " fail";
        return ;
    }

    while(true) {
        char buff[1024];
        wyze::Address::ptr from;
        int len = sock->recvFrom(buff, sizeof(buff), from);
        if(len > 0) {
            buff[len] = '\0';
            WYZE_LOG_INFO(g_logger) << std::endl << "recv: " << buff << std::endl
                << "from: " << *from;
            len = sock->sendTo(buff, len, from);
            if(len < 0) {
                WYZE_LOG_INFO(g_logger) << std::endl << "send: " << buff << std::endl
                    << "to: " << *from << "error=" << len;
            }
        }
    }
}

int main(int argc, char** argv)
{
    wyze::IOManager iom(1);
    iom.schedule(&run);
    return 0;
}