#ifndef _WYZE_HTTP_CONNECTION_H_
#define _WYZE_HTTP_CONNECTION_H_

#include "http.h"
#include "../sockstream.h"
#include "../uri.h"
#include "../thread.h"
#include <iostream>
#include <string>
#include <map>
#include <memory>
#include <list>

namespace wyze {
namespace http {

struct HttpResult {
    using ptr = std::shared_ptr<HttpResult>;

    enum class Error {
        OK = 0,
        INVALID_URL,
        INVALID_HOST,
        CONNECT_FAIL,
        CLOSE_BY_PEER,
        SOCKET_ERROR,
        TIMEOUT,
        CREATE_SOCKET_ERROR,
        POOL_GET_CONNECTION,
        POOL_INVALID_CONNECTION,
    };

    HttpResult(Error _result
                ,HttpResponse::ptr _rsp
                ,const std::string& _err)
        :res(_result), rsp(_rsp), strerr(_err) {}
    std::string toString() const;

    Error res;
    HttpResponse::ptr rsp;
    std::string strerr;
};

std::ostream& operator<<(std::ostream& os, const HttpResult& res);

class HttpConnectionPool;

class HttpConnection : public  SockStream {
    friend class HttpConnectionPool;
public:
    using ptr = std::shared_ptr<HttpConnection>;
    HttpConnection(Socket::ptr sock, bool owner = true);
    ~HttpConnection();
    HttpResponse::ptr recvResponse();
    int sendRequest(HttpRequest::ptr req);

public:
    static HttpResult::ptr DoGet(const std::string& url
                                ,uint64_t timeout_ms = 500
                                ,const std::map<std::string, std::string>& headers = {}
                                ,const std::string& body = "");

    static HttpResult::ptr DoGet(Uri::ptr uri
                                ,uint64_t timeout_ms = 500
                                ,const std::map<std::string, std::string>& headers = {}
                                ,const std::string& body = "");

    static HttpResult::ptr DoPost(const std::string& url
                                ,uint64_t timeout_ms = 500
                                ,const std::map<std::string, std::string>& headers = {}
                                ,const std::string& body = "");
    
    static HttpResult::ptr DoPost(Uri::ptr uri
                                ,uint64_t timeout_ms = 500
                                ,const std::map<std::string, std::string>& headers = {}
                                ,const std::string& body = "");

    static HttpResult::ptr DoRequest(HttpMethod method
                                ,const std::string& url
                                ,uint64_t timeout_ms = 500
                                ,const std::map<std::string, std::string>& headers = {}
                                ,const std::string& body = "");

    static HttpResult::ptr DoRequest(HttpMethod method
                                ,Uri::ptr uri
                                ,uint64_t timeout_ms = 500
                                ,const std::map<std::string, std::string>& headers = {}
                                ,const std::string& body = "");
    
    static HttpResult::ptr DoRequest(HttpRequest::ptr req
                                ,Uri::ptr uri
                                ,uint64_t timeout_ms = 500);

private:
    void saveLeftData(char* data, size_t len, size_t nparse);

private:
    size_t left_size;
    char *left_data;
    uint64_t m_createTimes;
    uint64_t m_request;
};

class HttpConnectionPool {
public:
    using ptr = std::shared_ptr<HttpConnectionPool>;
    using MutexType = Mutex;

    HttpConnectionPool(const std::string &host, uint16_t port,
                        uint32_t max_size, uint32_t max_alive_time,
                        uint32_t max_request, const std::string& vhost = "");

    HttpConnection::ptr getConnection();

    HttpResult::ptr doGet(const std::string& url, uint64_t timeout_ms = 500
                            ,const std::map<std::string, std::string>& headers = {}
                            ,const std::string& body = "");

    HttpResult::ptr doGet(Uri::ptr uri, uint64_t timeout_ms = 500
                            ,const std::map<std::string, std::string>& headers = {}
                            ,const std::string& body = "");

    HttpResult::ptr doPost(const std::string& url, uint64_t timeout_ms = 500
                            ,const std::map<std::string, std::string>& headers = {}
                            ,const std::string& body = "");
    
    HttpResult::ptr doPost(Uri::ptr uri, uint64_t timeout_ms = 500
                            ,const std::map<std::string, std::string>& headers = {}
                            ,const std::string& body = "");

    HttpResult::ptr doRequest(HttpMethod method, const std::string& url
                                ,uint64_t timeout_ms = 500
                                ,const std::map<std::string, std::string>& headers = {}
                                ,const std::string& body = "");

    HttpResult::ptr doRequest(HttpMethod method, Uri::ptr uri
                                ,uint64_t timeout_ms = 500
                                ,const std::map<std::string, std::string>& headers = {}
                                ,const std::string& body = "");
    
    HttpResult::ptr doRequest(HttpRequest::ptr req,uint64_t timeout_ms = 500);

private:
    static void ReleasePtr(HttpConnection* ptr, HttpConnectionPool* pool);

private:
    std::string m_host;
    std::string m_vhost;        //这个是header里面需要填的，也可以用 m_host
    uint16_t m_port;
    uint32_t m_maxSize;         //一直保持的最大数
    uint32_t m_maxAliveTime;    //存活的最长时间
    uint32_t m_maxRequest;      //每个 HttpConnection 最大发送次数

    MutexType m_mutex;
    std::list<HttpConnection*> m_conns;
    std::atomic<int32_t> m_total = {0};
};

}
}

#endif // !_WYZE_HTTP_CONNECTION_H_