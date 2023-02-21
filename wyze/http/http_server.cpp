/*
 * @Author: wyze 631848287@qq.com
 * @Date: 2022-12-16 15:15:17
 * @LastEditors: wyze 631848287@qq.com
 * @LastEditTime: 2023-02-21 11:30:13
 * @FilePath: /wyze/wyze/http/http_server.cpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
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
            // WYZE_LOG_WARN(g_logger) << "recv http request fail, errno="
            //     << errno << " errstr=" << strerror(errno)
            //     << " client:" << *client;
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