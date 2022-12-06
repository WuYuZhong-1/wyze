#include "sockstream.h"

namespace wyze {

SockStream::SockStream(Socket::ptr sock, bool owner)
    : m_sock(sock)
    , m_owner(owner)
{
}

SockStream::~SockStream()
{
    if(m_owner && m_sock)
        m_sock->close();
}

int SockStream::read(void* buffer, size_t length)
{
    return m_sock->recv(buffer, length);
}

int SockStream::read(ByteArray::ptr ba, size_t length) 
{
    std::vector<iovec> iovs;
    ba->getWriteBuffers(iovs, length);
    int rt = m_sock->recv(&iovs[0], iovs.size());
    if(rt > 0) 
        ba->setPosition(ba->getPosition() + rt);
    return rt;
}

int SockStream::write(const void* buffer, size_t length) 
{
    return m_sock->send(buffer, length);
}

int SockStream::write(ByteArray::ptr ba, size_t length) 
{
    std::vector<iovec> iovs;
    ba->getReadBuffers(iovs, length);
    int rt = m_sock->send(&iovs[0], iovs.size());
    if(rt > 0) 
        ba->setPosition(ba->getPosition() + rt);
    return rt;
}

void SockStream::close() 
{
    if(m_sock) 
        m_sock->close();
}


bool SockStream::isConnected() const 
{
    return m_sock && m_sock->isConnected();
}

}