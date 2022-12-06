#ifndef _WYZE_TPCSERVER_H_
#define _WYZE_TPCSERVER_H_

#include <memory>
#include <vector>
#include "noncopyable.h"
#include "iomanager.h"
#include "socket.h"
#include "address.h"


namespace wyze {

class TcpServer: public std::enable_shared_from_this<TcpServer>, Noncopyable {
public:
    using ptr = std::shared_ptr<TcpServer>;
    TcpServer(IOManager* worker = IOManager::GetThis(), 
                IOManager* acceptWorker = IOManager::GetThis());
    virtual ~TcpServer();

    virtual bool bind(Address::ptr addr);
    virtual bool bind(const std::vector<Address::ptr>& addrs,
                        std::vector<Address::ptr>& fails);
    virtual bool start();
    virtual void stop();

    uint64_t getRecvTimeout() const { return m_recvTimeout; }
    std::string getName() const { return m_name; }
    void setRecvTimeout(uint64_t v) { m_recvTimeout = v; }
    void setName(const std::string v) { m_name = v; }
    bool isStop() const { return m_isStop; }

protected:
    virtual void handleClient(Socket::ptr client);      //接收到一个socket 则创建一个携程，并携带该socket类
    virtual void startAccept(Socket::ptr sock);         //要accept 的socket

private:
    std::vector<Socket::ptr> m_socks;       //要监听的套接字
    IOManager* m_worker;
    IOManager* m_acceptWorker;
    uint64_t m_recvTimeout;
    std::string m_name;
    bool m_isStop;
};

}


#endif //_WYZE_TPCSERVER_H_