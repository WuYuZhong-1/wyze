#include "socket.h"
#include "log.h"
#include "macro.h"
#include "hook.h"
#include "iomanager.h"
#include "fdmanager.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/tcp.h>
#include <string.h>


namespace wyze {

static Logger::ptr  g_logger = WYZE_LOG_NAME("system");

Socket::Socket(int family, int type, int protocol) 
    : m_sock(-1)
    , m_family(family)
    , m_type(type)
    , m_protocol(protocol)
    , m_isConnected(false)
{

}

Socket::~Socket()
{
    this->close();
}


Socket::ptr Socket::CreateTCP(Address::ptr address)
{
    Socket::ptr sock(new Socket(address->getFamily(), Type::TCP, 0));
    return sock;
    // return (new Socket(address->getFamily(), Type::TCP, 0));
}

Socket::ptr Socket::CreateUDP(Address::ptr address)
{
    Socket::ptr sock(new Socket(address->getFamily(), Type::UDP, 0));
    return sock;
}

Socket::ptr Socket::CreateTCPSocket()
{
    Socket::ptr sock(new Socket(Family::IPv4, Type::TCP, 0));
    return sock;
}

Socket::ptr Socket::CreateUDPSocket()
{
    Socket::ptr sock(new Socket(Family::IPv4, Type::UDP, 0));
    return sock;
}

Socket::ptr Socket::CreateTCPSocket6()
{
    Socket::ptr sock(new Socket(Family::IPv6, Type::TCP, 0));
    return sock;
}

Socket::ptr Socket::CreateUDPSocket6()
{
    Socket::ptr sock(new Socket(Family::IPv6, Type::UDP, 0));
    return sock;
}

Socket::ptr Socket::CreateUnixTCPSocket()
{
    Socket::ptr sock(new Socket(Family::UNIX, Type::TCP, 0));
    return sock;
}

Socket::ptr Socket::CreateUnixUDPSocket()
{
    Socket::ptr sock(new Socket(Family::UNIX, Type::UDP, 0));
    return sock;
}

int64_t Socket::getSendTimeout()
{
    if(!isVaild()) 
        return -1;
    
    FdCtx::ptr ctx = FdMgr::GetInstance()->get(m_sock);
    if(ctx) {
        return ctx->getTimeout(SO_SNDTIMEO);
    }

    return -1;
}

void Socket::setSendTimeout(int64_t v)
{
    if(!isVaild())
        return;
    struct timeval tv{int(v / 1000),int(v % 1000 * 1000)};
    setOption(SOL_SOCKET, SO_SNDTIMEO, tv);
}

int64_t Socket::getRecvTimeout()
{
    if(!isVaild()) 
        return -1;
    
    FdCtx::ptr ctx = FdMgr::GetInstance()->get(m_sock);
    if(ctx) {
        return ctx->getTimeout(SO_RCVTIMEO);
    }
    return -1;
}

void Socket::setRecvTimeout(int64_t v)
{
    if(!isVaild())
        return ;
    struct timeval tv{ int(v / 1000), int(v % 1000 * 1000)};
    setOption(SOL_SOCKET, SO_RCVTIMEO, tv);
}

bool Socket::getOption(int level, int option, void* result, size_t* len)
{
    int rt = getsockopt(m_sock, level, option, result, (socklen_t*)len);
    if(rt) {
        WYZE_LOG_ERROR(g_logger) << "getOption sock=" << m_sock
            << " level=" << level << " option=" << option 
            << " errno=" << errno << " errstr=" << strerror(errno);
        return false;
    }
    return true;
}

bool Socket::setOption(int level, int option, const  void* value, size_t len)
{
    int rt = setsockopt(m_sock, level, option, value, len);
    if(rt) {
        WYZE_LOG_ERROR(g_logger) << "setOption sock=" << m_sock
            << " level=" << level << " option=" << option 
            << " errno=" << errno << " errstr=" << strerror(errno);
        return false;
    }
    return true;
}

Socket::ptr Socket::accept()
{
    Socket::ptr sock(new Socket(m_family, m_type, m_protocol));
    int new_sock = ::accept(m_sock, nullptr, nullptr);
    if(WYZE_UNLICKLY(new_sock == -1)) {
        WYZE_LOG_ERROR(g_logger) << "accept(" << m_sock << ") errno="
            << errno << " errstr" << strerror(errno);
        return nullptr;
    }

    if(sock->init(new_sock)) 
        return sock;
    return nullptr;
}

bool Socket::init(int sock)
{
    FdCtx::ptr ctx = FdMgr::GetInstance()->get(sock, true);
    if(ctx && ctx->isSocket() && !ctx->isClose()) {
        m_sock = sock;
        m_isConnected = true;
        initSock();
        getLocalAddress();
        getRemoteAddress();
        return true;
    }
    return false;
}

void Socket::initSock()
{
    int val = 1;
    setOption(SOL_SOCKET, SO_REUSEADDR, val);
    if( m_type == SOCK_STREAM) {
        setOption(IPPROTO_TCP, TCP_NODELAY, val);
    }
}

void Socket::newSock()
{
    m_sock = ::socket(m_family, m_type, m_protocol);
    if(WYZE_LICKLY(m_sock != -1)) {
        initSock();
    }
    else {
        WYZE_LOG_ERROR(g_logger) << "socket(" << m_family
            << ", " << m_type << ", " << m_protocol << ") errno=" 
            << errno << " errstr=" << strerror(errno);
    }
}

bool Socket::bind(Address::ptr addr)
{
    if(!isVaild()) {
        newSock();
        if(WYZE_UNLICKLY(!isVaild()))
            return false;
    }

    if(WYZE_UNLICKLY( addr->getFamily() != m_family)) {
        WYZE_LOG_ERROR(g_logger) << "bind sock.family("
            << m_family << ") addr.family(" << addr->getFamily()
            << ") not equal, addr=" << addr->toString();
        return false;
    }

    if(::bind(m_sock, addr->getAddr(), addr->getAddrLen())) {
        WYZE_LOG_ERROR(g_logger) << "bind error errno=" << errno 
            << " errstr=" << strerror(errno);
        return false;
    }

    getLocalAddress();
    return true;
}

bool Socket::connect(const Address::ptr addr, int64_t timeout_ms)
{
    if(!isVaild()) {
        newSock();
        if(WYZE_UNLICKLY(!isVaild()))
            return false;
    }

    if(WYZE_UNLICKLY( addr->getFamily() != m_family)) {
        WYZE_LOG_ERROR(g_logger) << "connect sock.family("
            << m_family << ") addr.family(" << addr->getFamily()
            << ") not equal, addr=" << addr->toString();
        return false;
    }

    if(timeout_ms == (int64_t)-1) {
        if(::connect(m_sock, addr->getAddr(), addr->getAddrLen())) {
            WYZE_LOG_ERROR(g_logger) << "sock=" << m_sock << "connect(" << addr->toString()
                << ") error errno=" << errno << "errstr=" << strerror(errno);
            this->close();
            return false;
        }
    }
    else {
        if(::connect_with_tiemout(m_sock, addr->getAddr(), addr->getAddrLen(), (uint64_t)timeout_ms)) {
            WYZE_LOG_ERROR(g_logger) << "sock=" << m_sock << "connect(" << addr->toString()
                << ") timeout="  << timeout_ms << "error errno=" 
                << errno << "errstr=" << strerror(errno);
            this->close();
            return false;
        }
    }

    m_isConnected = true;
    getRemoteAddress();
    getLocalAddress();
    return true;
}

bool Socket::listen(int backlog)
{
    if(WYZE_UNLICKLY(!isVaild())) {
        WYZE_LOG_ERROR(g_logger) << "listen error sock=-1";
        return false;
    }

    if(::listen(m_sock, backlog)) {
        WYZE_LOG_ERROR(g_logger) << "listen error errno=" << errno 
            << " errstr=" << strerror(errno);
        return false;
    }
    return true;
}

bool Socket::close()
{
    if(m_sock == -1)
        return true;
    
    m_isConnected = false;
    //TODO::这里如果添加事件，colse 小心，事件返回
    ::close(m_sock);
    m_sock = -1;
    m_localAddress.reset();
    m_remoteAddress.reset();

    return true;
}

int Socket::send(const void* buffer, size_t length, int flags)
{
    if(WYZE_UNLICKLY(!isConnected()))
        return -1;
    return ::send(m_sock, buffer, length, flags);
}

int Socket::send(const iovec* buffers, size_t length, int flags)
{
    if(WYZE_UNLICKLY(!isConnected()))
        return -1;

    //writev 没有 flags
    msghdr msg;
    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = (iovec*)buffers;
    msg.msg_iovlen = length;
    return ::sendmsg(m_sock, &msg, flags);
}

int Socket::sendTo(const void* buffer, size_t length, const Address::ptr to, int flags)
{
    //这里在第一次会预判错误
    if(WYZE_UNLICKLY(!isVaild())) {
        newSock();
        if(WYZE_UNLICKLY(!isVaild())) 
            return -1;
    } 

    if(!to) {
       WYZE_LOG_ERROR(g_logger) << "sendTo error, arg --> to is nullptr";
        return -1; 
    }

    if(to->getFamily() != m_family) {
        WYZE_LOG_ERROR(g_logger) << "sendTo error, sock.family=" 
            << m_family << " to.family=" << to->getFamily();
        return -1;
    }

    return ::sendto(m_sock, buffer, length, flags, to->getAddr(), to->getAddrLen());
}

int Socket::sendTo(const iovec* buffers, size_t length, const Address::ptr to, int flags)
{
    //这里在第一次会预判错误
    if(WYZE_UNLICKLY(!isVaild())) {
        newSock();
        if(WYZE_UNLICKLY(!isVaild())) 
            return -1;
    } 

    if(!to) {
       WYZE_LOG_ERROR(g_logger) << "sendTo error, arg --> to is nullptr";
        return -1; 
    }

    if(to->getFamily() != m_family) {
        WYZE_LOG_ERROR(g_logger) << "sendTo error, sock.family=" 
            << m_family << " to.family=" << to->getFamily();
        return -1;
    }

    msghdr msg;
    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = (iovec*) buffers;
    msg.msg_iovlen = length;
    msg.msg_name = to->getAddr();
    msg.msg_namelen = to->getAddrLen();
    return ::sendmsg(m_sock, &msg, flags);
}

int Socket::recv(void* buffer, size_t length, int flags)
{
    if(WYZE_UNLICKLY(!isConnected()))
        return -1;
    return ::recv(m_sock, buffer, length, flags);
}

int Socket::recv(iovec* buffers, size_t length, int flags)
{
    if(WYZE_UNLICKLY(!isConnected()))
        return -1;
    
    msghdr msg;
    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = buffers;
    msg.msg_iovlen = length;
    return ::recvmsg(m_sock, &msg, flags);
}
int Socket::recvFrom(void* buffer, size_t length, Address::ptr& from, int flags)
{
    //这里在第一次会预判错误
    if(WYZE_UNLICKLY(!isVaild())) {
        newSock();
        if(WYZE_UNLICKLY(!isVaild())) 
            return -1;
    } 

    //TODO::这里要考虑，返回的 Address 的初始化
    switch(m_family) {
        case AF_INET:
            from.reset(new IPv4Address());
            break;
        case AF_INET6:
            from.reset(new IPv6Address());
            break;
        case AF_UNIX:
            from.reset(new UnixAddress());
        default:
            WYZE_LOG_ERROR(g_logger) << "recvFrom error, sock.family=" << m_family;
            return -1;
    }
    socklen_t len = from->getAddrLen();
    int rt = ::recvfrom(m_sock, buffer, length, flags, from->getAddr(), &len);

    if(m_family == AF_UNIX) {
        UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(from);
        addr->setAddrlen(len);
    }

    return rt;
}

int Socket::recvFrom(iovec* buffers, size_t length, Address::ptr& from, int flags)
{
    //这里在第一次会预判错误
    if(WYZE_UNLICKLY(!isVaild())) {
        newSock();
        if(WYZE_UNLICKLY(!isVaild())) 
            return -1;
    } 


    //TODO::这里要考虑，返回的 Address 的初始化
    switch(m_family) {
        case AF_INET:
            from.reset(new IPv4Address());
            break;
        case AF_INET6:
            from.reset(new IPv6Address());
            break;
        case AF_UNIX:
            from.reset(new UnixAddress());
        default:
            WYZE_LOG_ERROR(g_logger) << "recvFrom error, sock.family=" << m_family;
            return -1;
    }

    //TODO::这里要考虑，返回的 Address 的初始化
    msghdr msg;
    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = buffers;
    msg.msg_iovlen = length;
    msg.msg_name = from->getAddr();
    msg.msg_namelen = from->getAddrLen();

    int rt = ::recvmsg(m_sock, &msg, flags);

    if(m_family == AF_UNIX) {
        UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(from);
        addr->setAddrlen(msg.msg_namelen);
    }

    return rt; 
}

Address::ptr Socket::getRemoteAddress()
{
    if(m_remoteAddress) 
        return m_remoteAddress;

    Address::ptr result;
    switch(m_family) {
        case AF_INET:
            result.reset(new IPv4Address());
            break;
        case AF_INET6:
            result.reset(new IPv6Address());
            break;
        case AF_UNIX:
            result.reset(new UnixAddress());
            break;
        default:
            result.reset(new UnknownAddress(m_family));
            break;
    }

    socklen_t addrlen = result->getAddrLen();
    if(getpeername(m_sock, result->getAddr(), &addrlen)) {
        WYZE_LOG_ERROR(g_logger) << "getsockname error sock=" << m_sock 
            << " errno=" << errno << " errstr=" << strerror(errno);
        return Address::ptr(new UnknownAddress(m_family));
    }

    if(m_family == AF_UNIX) {
        UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
        addr->setAddrlen(addrlen);
    }

    m_remoteAddress = result;
    return m_remoteAddress;
}

Address::ptr Socket::getLocalAddress()
{
    if(m_localAddress) 
        return m_localAddress;

    Address::ptr result;
    switch(m_family) {
        case AF_INET:
            result.reset(new IPv4Address());
            break;
        case AF_INET6:
            result.reset(new IPv6Address());
            break;
        case AF_UNIX:
            result.reset(new UnixAddress());
            break;
        default:
            result.reset(new UnknownAddress(m_family));
            break;
    }

    socklen_t addrlen = result->getAddrLen();
    if(getsockname(m_sock,result->getAddr(), &addrlen)) {
        WYZE_LOG_ERROR(g_logger) << "getsockname error sock=" << m_sock 
            << " errno=" << errno << " errstr=" << strerror(errno);
        return Address::ptr(new UnknownAddress(m_family));
    }

    if(m_family == AF_UNIX) {
        UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
        addr->setAddrlen(addrlen);
    }

    m_localAddress = result;
    return m_localAddress;
}

int Socket::getError()
{
    int error = 0;
    if(!getOption(SOL_SOCKET, SO_ERROR, error)) 
        return -1;
    return error;
}

std::ostream& Socket::dump(std::ostream& os) const
{
    os << "[Socket sock=" << m_sock
       << " is_connected=" << m_isConnected
       << " family=" << m_family
       << " type=" << m_type
       << " protocol=" << m_protocol;
    if(m_localAddress) {
        os << " local_address=" << m_localAddress->toString();
    }
    if(m_remoteAddress) {
        os << " remote_address=" << m_remoteAddress->toString();
    }
    os << "]";
    return os;
}

bool Socket::cancelRead()
{
    IOManager* iom = IOManager::GetThis();
    if(iom == nullptr) 
        return false;
    return iom->canceEvent(m_sock, IOManager::READ);
}

bool Socket::cancelWrite()
{
    IOManager* iom = IOManager::GetThis();
    if(iom == nullptr)
        return false;
    return iom->canceEvent(m_sock, IOManager::WRITE);
}

bool Socket::cancelAccept()
{
    return cancelRead();
}

bool Socket::cancelAll()
{
    IOManager* iom = IOManager::GetThis();
    if(iom == nullptr)
        return false;
    return iom->canceAll(m_sock);
}

}