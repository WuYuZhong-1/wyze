#include "tcpserver.h"
#include "log.h"
#include "config.h"
#include "macro.h"

namespace wyze {

static Logger::ptr g_logger = WYZE_LOG_NAME("system");

static ConfigVar<uint64_t>::ptr g_tcp_server_read_timeout =
    Config::Lookup("tcp_server.read_timeout", (uint64_t)(60 * 1000 * 2),
                    "tcp server read timeout");

TcpServer::TcpServer(IOManager* worker, IOManager* acceptWorker )
    : m_worker(worker)
    , m_acceptWorker(acceptWorker)
    , m_recvTimeout(g_tcp_server_read_timeout->getValue())
    , m_name("wyze/1.0.0")
    , m_isStop(true)
{
}

TcpServer::~TcpServer()
{
    for(auto& i : m_socks)
        i->close();
    m_socks.clear();
}

bool TcpServer::bind(Address::ptr addr)
{
    std::vector<Address::ptr> addrs;
    std::vector<Address::ptr> fails;
    addrs.push_back(addr);

    return bind(addrs,fails);
}

bool TcpServer::bind(const std::vector<Address::ptr>& addrs,
                        std::vector<Address::ptr>& fails)
{
    //这里的理念是一个绑定错误，全部都不绑定，返回错误的绑定addr
    for(auto& addr : addrs) {
        Socket::ptr sock = Socket::CreateTCP(addr);
        if(!sock->bind(addr)) {
            WYZE_LOG_ERROR(g_logger) << "bind fail errno=" << errno
                << " errstr=" << strerror(errno) 
                << " addr=[" << *addr << "]";
            fails.push_back(addr);
            continue;
        }

        if(!sock->listen()) {
            WYZE_LOG_ERROR(g_logger) << "listen fail errno=" << errno
                << " errstr=" << strerror(errno)
                << " addr=[" << *addr << "]";
                fails.push_back(addr);
                continue;
        }
        m_socks.push_back(sock);
    }

    if(!fails.empty()) {
        m_socks.clear();
        return false;
    }

    for(auto& i : m_socks) {
        WYZE_LOG_INFO(g_logger) << "server bind success: " << *i;
    }
    
    return true;
}

bool TcpServer::start()
{
    if(!m_isStop)
        return true;

    m_isStop = false;
    for(auto& sock : m_socks) {
        m_acceptWorker->schedule(std::bind(&TcpServer::startAccept,
                                    shared_from_this(), sock));
    }
    return true;
}

void TcpServer::stop()
{
    //这里采用的是一个携程人物来停止服务工作
    m_isStop = true;
    auto self = shared_from_this();
    m_acceptWorker->schedule([this, self]() {
        for(auto& sock : m_socks) {
            sock->cancelAll();
            sock->close();
        }
        m_socks.clear();
    });
}

//接收到一个socket 则创建一个携程，并携带该socket类
void TcpServer::handleClient(Socket::ptr client)
{
    WYZE_LOG_INFO(g_logger) << "handleClient: " << *client;
}

//要accept 的socket
void TcpServer::startAccept(Socket::ptr sock)
{
    while(!m_isStop) {
        Socket::ptr client = sock->accept();
        if(WYZE_UNLICKLY(!client))
            continue;       //accept 里面打印了，这里就不需要打印
        
        client->setRecvTimeout(m_recvTimeout);
        m_worker->schedule(std::bind(&TcpServer::handleClient,
                shared_from_this(), client));
    }
}   

}