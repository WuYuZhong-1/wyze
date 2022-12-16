#include "../wyze/wyze.h"
#include <netdb.h>
#include <iostream>

static wyze::Logger::ptr g_logger = WYZE_LOG_ROOT();

const char* ip = nullptr;
uint16_t port = 0;

void run()
{
    wyze::IPAddress::ptr addr = wyze::IPAddress::Create(ip, port);
    if(!addr) {
        WYZE_LOG_ERROR(g_logger) << "invalid ip:" << ip  << " port:" << port;
        return ;
    }

    wyze::Socket::ptr sock = wyze::Socket::CreateUDP(addr);

    wyze::IOManager::GetThis()->schedule([sock]() {
        wyze::Address::ptr addr;
        WYZE_LOG_INFO(g_logger) << "begin recv:";
        while(true) {
            char buff[1024] = {0};
            int len = sock->recvFrom(buff, sizeof(buff), addr);
            if(len > 0) {
                WYZE_LOG_INFO(g_logger) << std::endl << "recv:" << std::endl << std::string(buff, len)
                        << std::endl << "from addr:" << *addr;
            }
        }
    });

    sleep(1);
    
    while(true) {
        std::string line;
        WYZE_LOG_INFO(g_logger) << std::endl << "input>";
        std::getline(std::cin, line);
        if(!line.empty()) {
            int len = sock->sendTo(line.c_str(), line.size(), addr);
            if(len < 0) {
                int err = sock->getError();
                WYZE_LOG_ERROR(g_logger) << "send error err=" << err
                    << " errstr" << gai_strerror(err) << " len=" << len
                    << " addr=" << *addr
                    << " sock=" << *sock;
            }
            else {
                WYZE_LOG_INFO(g_logger) << std::endl << "send:" << line << " len:" << len;
            }
        }
    }
}

int main(int argc, char** argv)
{
    if(argc < 3) {
        WYZE_LOG_INFO(g_logger) << "useage as[" << argv[0] << "ip port";
    }    
    ip = argv[1];
    port = atoi(argv[2]);

    wyze::IOManager iom(2);
    iom.schedule(&run);
    return 0;
}