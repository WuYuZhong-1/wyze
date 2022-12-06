#ifndef _WYZE_SOCKSTREAM_H_
#define _WYZE_SOCKSTREAM_H_

#include "stream.h"
#include "socket.h"

namespace wyze {

class SockStream : public Stream {
public:
    using ptr = std::shared_ptr<SockStream>;
    SockStream(Socket::ptr sock, bool owner = true);
    ~SockStream();

    int read(void* buffer, size_t length) override;
    int read(ByteArray::ptr ba, size_t length) override;
    int write(const void* buffer, size_t length) override;
    int write(ByteArray::ptr ba, size_t length) override;
    void close() override;

    Socket::ptr getSocket() const { return m_sock; }
    bool isConnected() const;
    
private:
    Socket::ptr m_sock;
    bool m_owner;
};

}

#endif // !_WYZE_SOCKSTREAM_H_