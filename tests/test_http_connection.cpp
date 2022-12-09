#include "../wyze/wyze.h"

static wyze::Logger::ptr g_logger = WYZE_LOG_ROOT();

void test_pool()
{
    wyze::http::HttpConnectionPool::ptr pool(new wyze::http::HttpConnectionPool(
                    "www.sylar.top", 80, 10,  1000 * 20, 5));
    
    wyze::IOManager::GetThis()->addTimer(1000, [pool]() {
        auto r = pool->doGet("/", 500);
        WYZE_LOG_INFO(g_logger) << *r;
    }, true);
}

void run()
{
    wyze::Address::ptr addr = wyze::IPAddress::Create("39.100.72.123", 80);;
    if(!addr) {
        WYZE_LOG_INFO(g_logger)  << "get addr error" ;
        return;
    }
    WYZE_LOG_INFO(g_logger) << *addr; 

    wyze::Socket::ptr sock = wyze::Socket::CreateTCP(addr);
    bool rt = sock->connect(addr);
    if(!rt) {
        WYZE_LOG_INFO(g_logger) << "connect " << *addr << " failed";
        return ;
    }

    wyze::http::HttpConnection::ptr conn(new wyze::http::HttpConnection(sock));
    wyze::http::HttpRequest::ptr req(new wyze::http::HttpRequest);
    req->setPath("/blog/");
    req->setHeader("Host", "www.sylar.top");
    WYZE_LOG_INFO(g_logger) << "req: " << std::endl
        << *req;

    int slen = conn->sendRequest(req);
    WYZE_LOG_INFO(g_logger) << slen;
    auto rsp = conn->recvResponse();

    if(!rsp) {
        WYZE_LOG_INFO(g_logger) << "recv response error";
    }
    else {
        WYZE_LOG_INFO(g_logger) << "rsp: " << std::endl
            << *rsp;
    }
    WYZE_LOG_INFO(g_logger) << "========================================";
    auto get = wyze::http::HttpConnection::DoGet("http://www.baidu.com");
    WYZE_LOG_INFO(g_logger) << *get;
 
    WYZE_LOG_INFO(g_logger) << "========================================";
    test_pool();
}

int main(int argc, char** argv) 
{
    wyze::IOManager iom(2);
    iom.schedule(&run);
    return 0;
}