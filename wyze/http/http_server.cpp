#include "http_server.h"
#include "../log.h"

namespace wyze {
namespace http {

static Logger::ptr g_logger = WYZE_LOG_NAME("system");

HttpServer::HttpServer(bool keepalive, IOManager* worker, IOManager* worker_accpet)
    : TcpServer(worker, worker_accpet)
    , m_isKeepalive(keepalive)
{
    m_dispatch.reset(new ServletDispatch);
}

void HttpServer::handleClient(Socket::ptr client) 
{
    HttpSession::ptr session(new HttpSession(client));

    do {
        auto req = session->recvRequest();
        if(!req) {
            WYZE_LOG_WARN(g_logger) << "recv http request fail, errno="
                << errno << " errstr=" << strerror(errno)
                << " client:" << *client;
            break;
        }
    
        HttpResponse::ptr rsp(new HttpResponse(req->getVersion()
                                , req->isColse() || !m_isKeepalive));
        rsp->setHeader("Server", getName());
        m_dispatch->handle(req, rsp, session);
        session->sendResponse(rsp);
        // WYZE_LOG_INFO(g_logger) << "request: " << std::endl
        //     << *req;
        // WYZE_LOG_INFO(g_logger) << "response: " << std::endl
        //     << *rsp;

        if(req->isColse())
            break;

    }while(m_isKeepalive);

    session->close();
}

}
}