#include "../wyze/wyze.h"

static wyze::Logger::ptr g_logger = WYZE_LOG_ROOT();

void test_request()
{
    wyze::http::HttpRequest::ptr req(new wyze::http::HttpRequest);
    req->setHeader("host", "www.baidu.com");
    req->setBody("hello wyze");
    WYZE_LOG_INFO(g_logger) << "\r\n" << req->toString();
}

void test_response()
{
    wyze::http::HttpResponse::ptr rsp(new wyze::http::HttpResponse);
    rsp->setHeader("server", "www.baidu.com");
    rsp->setBody("hello wyze");
    rsp->setClose(false);
    rsp->setStatus((wyze::http::HttpStatus)400);

    WYZE_LOG_INFO(g_logger) << "\r\n" << rsp->toString();
}




int main(int argc, char** argv)
{
    test_request();
    test_response();
    return 0;
}