#include "../wyze/wyze.h"

#include <netinet/in.h>
#include <iostream>
#include <sys/epoll.h>
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <unistd.h>
#include <fcntl.h>
 
#define WORKER_THREAD 4
#define PORT  8989
//创建socket，并返回fd
int createSocket() {
    int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd < 0) {
        std::cout << "create socket error" << std::endl;
        return 0;
    }
 
    sockaddr_in sockAddr{};
    sockAddr.sin_port = htons(PORT);
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_addr.s_addr = htons(INADDR_ANY);
 
    if (bind(fd, (sockaddr *) &sockAddr, sizeof(sockAddr)) < 0) {
        std::cout << "bind socket error, port:" << PORT << std::endl;
        return 0;
    }
 
    if (listen(fd, 100) < 0) {
        std::cout << "listen port error" << std::endl;
        return 0;
    }
    return fd;
}
 
void Worker1(int socketFd, int k) {
    std::cout << " Worker" << k << " run " << std::endl;
    while (true) {
        int tfd = 0;
        sockaddr_in cli_addr{};
        socklen_t length = sizeof(cli_addr);
        std::cout << "worker" << k << " in " << std::endl;
        tfd = accept(socketFd, (sockaddr *) &cli_addr, &length);
        if (tfd <= 0) {
            std::cout << "accept error" << std::endl;
            return;
        } else {
            std::cout << "worker" << k << " accept " << std::endl;
        }
    }
}
 

#define EVENT_NUM 20

void Worker2(int socketFd, int k) {
    std::cout << " Worker" << k << " run " << std::endl;
 
    int eFd = epoll_create(1);
    if (eFd < 0) {
        std::cout << "create epoll fail";
        return;
    }
    epoll_event epev_{};
    // epev_.events = EPOLLIN;
    epev_.events = EPOLLIN | EPOLLEXCLUSIVE;         //加上互斥
    epev_.data.fd = socketFd;
    epoll_ctl(eFd, EPOLL_CTL_ADD, socketFd, &epev_);
    epoll_event events[EVENT_NUM];
 
    while (true) {
        int eNum = epoll_wait(eFd, events, EVENT_NUM, -1);
        if (eNum == -1) {
            std::cout << "epoll error";
            return;
        }
        std::cout << "worker" << k << " in " << std::endl;
        //一定要加上这句,防止事件被瞬间处理,导致看不到结果
        std::this_thread::sleep_for((std::chrono::seconds (1)));
        std::cout << "worker" << k << " in " << std::endl;
        for (int i = 0; i < eNum; ++i) {
            if (events[i].data.fd == socketFd) {
                int tfd = 0;
                sockaddr_in cli_addr{};
                socklen_t length = sizeof(cli_addr);
                tfd = accept(socketFd, (sockaddr *) &cli_addr, &length);
                if (tfd <= 0) {
                    std::cout << "accept error" << std::endl;
                } else {
                    std::cout << "worker" << k << " accept " << std::endl;
                }
            } else {
                //处理正常的socket读写事件,这里可以忽略,不是这次关注的点
            }
        }
    }
}

 
#define PORT1  9898

//创建socket，并返回fd
int createSocket2() {
    int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd == -1) {
        std::cout << "create socket error" << std::endl;
        return 0;
    }
 
    int on = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, (const void *) &on, sizeof(on)) < 0) {
        std::cout << "set opt error, ret:" << std::endl;
    }
 
    sockaddr_in sockAddr{};
    sockAddr.sin_port = htons(PORT1);
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_addr.s_addr = htons(INADDR_ANY);
 
    if (bind(fd, (sockaddr *) &sockAddr, sizeof(sockAddr)) < 0) {
        std::cout << "bind socket error, port:" << PORT1 << std::endl;
        return 0;
    }
 
    if (listen(fd, 100) < 0) {
        std::cout << "listen port error" << std::endl;
        return 0;
    }
    return fd;
}
 
void Worker3(int k) {
    std::cout << " Worker" << k << " run " << std::endl;
 
    int socketFd = createSocket2();
    int eFd = epoll_create(1);
    if (eFd == -1) {
        std::cout << "create epoll fail" << std::endl;
        return;
    }
 
    epoll_event epev_{};
    epev_.events = EPOLLIN;
    epev_.data.fd = socketFd;
    epoll_ctl(eFd, EPOLL_CTL_ADD, socketFd, &epev_);
    epoll_event events[EVENT_NUM];
 
    while (true) {
        int eNum = epoll_wait(eFd, events, EVENT_NUM, -1);
        if (eNum == -1) {
            std::cout << "epoll error" << std::endl;
            return;
        }
        std::this_thread::sleep_for((std::chrono::seconds(1)));
        std::cout << "worker" << k << " in " << std::endl;
        for (int i = 0; i < eNum; ++i) {
            if (events[i].data.fd == socketFd) {
                int tfd = 0;
                sockaddr_in cli_addr{};
                socklen_t length = sizeof(cli_addr);
                tfd = accept(socketFd, (sockaddr *) &cli_addr, &length);
                if (tfd <= 0) {
                    std::cout << "accept error" << std::endl;
                } else {
                    std::cout << "worker" << k << " accept " << std::endl;
                }
            } else {
                //处理正常的socket读写事件
            }
        }
    }
}
 
void test_epoll() 
{
    std::mutex mutex;
    std::unique_lock<std::mutex> lck(mutex);
    std::condition_variable cv;
    
    int fd = createSocket();
    (void)fd;
    //第一种,多个线程不使用多路复用,accept同一个socket
    for (int i = 0; i < WORKER_THREAD; ++i) {
        // std::thread th(&Worker1, fd, i + 1);
        // std::thread th(&Worker2, fd, i + 1);
        std::thread th(&Worker3, i + 1);            //这种方式的性能会更好
        th.detach();
    }
 
    cv.wait(lck);
}



//测试IO调度

static wyze::Logger::ptr g_logger = WYZE_LOG_ROOT();

void test_iomanager()
{
    WYZE_LOG_INFO(g_logger) << "EPOLLIN=" << EPOLLIN
                            << " EPOLLOUT=" << EPOLLOUT;
    wyze::IOManager iom(2, false);

    int pfd[2] = {0};
    int rt = pipe(pfd);
    WYZE_ASSERT(!rt);

    int rfd = pfd[0];
    rt = iom.addEvent(pfd[0], wyze::IOManager::Event::READ, [rfd](){
        
        int flag = fcntl(rfd, F_GETFL);
        int rt = fcntl(rfd, F_SETFL, flag | O_NONBLOCK);
        WYZE_ASSERT(!rt);
        
        int count = 10;
        while(count > 0) {
            char buf[128] = {0};
            int len = read(rfd, buf, sizeof(buf));
            if(len == 0) {
                WYZE_LOG_ERROR(g_logger) << "len == 0";
                break;
            } 
            else if(len == -1) {
                if(errno == EAGAIN) {
                    wyze::Fiber::YeildToReady();
                }
                else {
                    WYZE_ASSERT(false);
                }
            }
            else {
                WYZE_LOG_INFO(g_logger) << buf << std::endl;
                --count;
            }
        }
    });

    WYZE_LOG_INFO(g_logger) << "------1---------- rt = " << rt;
    int wfd = pfd[1];
    int count = 1;
    while(count <= 10) {
        std::string buf = "pipe write count " + std::to_string(count++);
        write(wfd, buf.c_str(), buf.size());
        WYZE_LOG_DEBUG(g_logger) << buf;
        sleep(1);
    }
    WYZE_LOG_INFO(g_logger) << "------2----------";
}

int main() {
    test_iomanager();
    return 0;
}