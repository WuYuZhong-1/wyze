#include "../wyze/hook.h"
#include "../wyze/wyze.h"
#include <arpa/inet.h>


wyze::Logger::ptr g_logger = WYZE_LOG_ROOT();


void test_sleep()
{
    wyze::IOManager iom(3);
    iom.schedule([](){
        int count = 20;
        while(count--) {
            sleep(2);
            WYZE_LOG_INFO(g_logger) << "sleep 2";
        }
    });

    iom.schedule([](){
        int count = 20;
        while(count--) {
            usleep(3 * 1000 * 1000);
            WYZE_LOG_INFO(g_logger) << "usleep 3 * 1000 * 1000";
        }
    });

    iom.schedule([]{
        int count = 20;
        while(count--) {
            const struct timespec ts = { 3, 1000 * 1000 * 1000};
            nanosleep(&ts,nullptr);
            WYZE_LOG_INFO(g_logger) << "sleep 4 seconds";
        }
    });

    WYZE_LOG_INFO(g_logger) << "test sleep";
}


void test_socket()
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port= htons(80);
    inet_pton(AF_INET, "110.242.68.66", &addr.sin_addr.s_addr);

    WYZE_LOG_INFO(g_logger) << "begin connect";
    int rt = connect(sock, (const sockaddr*)&addr, sizeof(addr));
    WYZE_LOG_INFO(g_logger) << "connect rt=" << rt << " errno=" << errno;
    if(rt)
        return;

    const char data[] = "GET / HTTP/1.0\r\n\r\n";
    rt = send(sock, data, sizeof(data), 0);
    WYZE_LOG_INFO(g_logger) << "send rt=" << rt << " errno=" << errno;
    if(rt <= 0) 
        return;

    std::string buff;
    buff.resize(4096);
    rt  = recv(sock, &buff[0], buff.size(), 0);
    WYZE_LOG_INFO(g_logger) << "recv rt=" << rt << " errno=" << errno;
    if(rt <= 0)
        return;
    buff.resize(rt);
    WYZE_LOG_INFO(g_logger) << buff;
}

int main(int argc, char** argv)
{
    // test_sleep();
    wyze::IOManager iom(1);
    iom.schedule(&test_socket);
    return 0;
}