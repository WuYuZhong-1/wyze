/*
 * @Author: wyze 631848287@qq.com
 * @Date: 2022-12-16 15:15:17
 * @LastEditors: wyze 631848287@qq.com
 * @LastEditTime: 2023-02-21 10:39:03
 * @FilePath: /wyze/tests/test_http_server.cpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "../wyze/wyze.h"

static wyze::Logger::ptr g_logger = WYZE_LOG_NAME("system");
void run()
{
    wyze::Address::ptr addr = wyze::Address::LookupAny("0.0.0.0:8020");
    wyze::http::HttpServer::ptr server(new wyze::http::HttpServer(true));
    while(!server->bind(addr))
        sleep(2);

    auto sd = server->getServletDispatch();
    sd->addServlet("/wyze/xx", [](wyze::http::HttpRequest::ptr req
                    ,wyze::http::HttpResponse::ptr rsp
                    ,wyze::http::HttpSession::ptr session) {
        rsp->setBody(req->toString());
        return 0;
    });

    sd->addGlobServlet("/wyze/*", [](wyze::http::HttpRequest::ptr req
                    ,wyze::http::HttpResponse::ptr rsp
                    ,wyze::http::HttpSession::ptr session) {
        rsp->setBody("Glob:\r\n" + req->toString());
        return 0;
    });

    sd->addServlet("/wyze/sleep", [](wyze::http::HttpRequest::ptr req
                    ,wyze::http::HttpResponse::ptr rsp
                    ,wyze::http::HttpSession::ptr session) {
        WYZE_LOG_INFO(g_logger) << "start sleep" ;
        sleep(30);
        WYZE_LOG_INFO(g_logger) << "end sleep" ;
        rsp->setBody("sleep:\r\n" + req->toString());
        return 0;
    });
    
    server->start();
}

int main(int argc, char** argv)
{
    wyze::IOManager iom(1);
    iom.schedule(&run);
    return 0;
}