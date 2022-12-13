#include "../wyze/wyze.h"

static wyze::Logger::ptr g_logger = WYZE_LOG_ROOT();

void run()
{
    wyze::Address::ptr addr = wyze::Address::LookupAny("0.0.0.0:8020");
    wyze::http::HttpServer::ptr server(new wyze::http::HttpServer);
    while(!server->bind(addr))
        sleep(2);

    // auto sd = server->getServletDispatch();
    // sd->addServlet("/wyze/xx", [](wyze::http::HttpRequest::ptr req
    //                 ,wyze::http::HttpResponse::ptr rsp
    //                 ,wyze::http::HttpSession::ptr session) {
    //     rsp->setBody(req->toString());
    //     return 0;
    // });

    // sd->addGlobServlet("/wyze/*", [](wyze::http::HttpRequest::ptr req
    //                 ,wyze::http::HttpResponse::ptr rsp
    //                 ,wyze::http::HttpSession::ptr session) {
    //     rsp->setBody("Glob:\r\n" + req->toString());
    //     return 0;
    // });

    server->start();
}

int main(int argc, char** argv)
{
    wyze::IOManager iom(1);
    iom.schedule(&run);
    return 0;
}