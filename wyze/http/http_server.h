#ifndef _WYZE_HTTP_SERVER_H_
#define _WYZE_HTTP_SERVER_H_

#include "../tcpserver.h"
#include "http_session.h"
#include "servlet.h"

namespace wyze {
namespace http {

class HttpServer : public TcpServer {
public:
    using ptr = std::shared_ptr<HttpServer>;
    HttpServer(bool keepalive = false
                , IOManager* worker = IOManager::GetThis()
                , IOManager* worker_accpet = IOManager::GetThis());
    
    ServletDispatch::ptr getServletDispatch() const { return m_dispatch; }
    void setServletDispatch(ServletDispatch::ptr v) { m_dispatch = v; }

protected:
    void handleClient(Socket::ptr client) override;
private:
    bool m_isKeepalive;
    ServletDispatch::ptr m_dispatch;
};

}
}

#endif //_WYZE_HTTP_SERVER_H_