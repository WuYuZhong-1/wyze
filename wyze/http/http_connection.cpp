#include "http_connection.h"
#include "http_parser.h"
#include "../config.h"
#include "../log.h"
#include <sstream>
#include <strings.h>

namespace wyze {
namespace http {

static Logger::ptr g_logger = WYZE_LOG_NAME("system");

static ConfigVar<uint64_t>::ptr g_http_response_buffer_size =
    Config::Lookup("http.response.buffer_size"
            ,(uint64_t)(4 * 1024), "http response buffer size");

static ConfigVar<uint64_t>::ptr g_http_response_max_body_size = 
    Config::Lookup("http.response.max_body_size"
            ,(uint64_t)(64 * 1024 * 1024), "http response max body size");

static uint64_t s_http_response_buffer_size = 0;
static uint64_t s_http_response_max_body_size = 0;

namespace {
struct _ResponseSizeIniter {
    _ResponseSizeIniter(){
        s_http_response_buffer_size = g_http_response_buffer_size->getValue();
        s_http_response_max_body_size = g_http_response_max_body_size->getValue();

        g_http_response_buffer_size->addListener(
            [](const uint64_t ov, const uint64_t nv) {
                s_http_response_buffer_size = nv;
            }
        );

        g_http_response_max_body_size->addListener(
            [](const uint64_t ov, const uint64_t nv) {
                s_http_response_max_body_size = nv;
            }
        );
    }
};
static _ResponseSizeIniter _init;
}

std::string HttpResult::toString() const
{
    std::stringstream ss;
    ss << "HttpResult code:" << (int)res
        << " desc:" << strerr
        << " response:" << (rsp ? "\n" + rsp->toString() : "nullptr");
    return ss.str();
}

std::ostream& operator<<(std::ostream& os, const HttpResult& res)
{
    return os << res.toString();
}

HttpConnection::HttpConnection(Socket::ptr sock, bool owner)
    : SockStream(sock, owner)
    , left_size(0)
    , left_data(new char[(s_http_response_buffer_size >  (4 *1024)) ?  s_http_response_buffer_size : (4 *1024)])
    , m_createTimes(GetCurrentMS())
    , m_request(0)
{
}

HttpConnection::~HttpConnection()
{
    delete[] left_data;
    WYZE_LOG_DEBUG(g_logger) << "~HttpConnection";
}

HttpResponse::ptr HttpConnection::recvResponse()
{
    //这里会造成数据多读的现象
    HttpResponseParser::ptr parser(new HttpResponseParser);
    uint64_t buffer_size = (s_http_response_buffer_size >  (4 *1024)) ?  s_http_response_buffer_size : (4 *1024);   //避免在下面执行中 s_http_response_buffer_size 发生改变

    
    std::shared_ptr<char> buffer(new char[buffer_size + 1], [](char* ptr) {
            delete[] ptr;
        });
    char* data = buffer.get();
    int offset = ((buffer_size < left_size) ? 0 : left_size); //这里的避免程序本亏，但是会有数据丢失
    memcpy(data, left_data, offset);
    size_t nparse = 0;
    
    //data 数据地址
    // offset 读取的数据长度
    // nparse 解析到的位置
    do {
        int len = read(data + offset, buffer_size - offset);
        if(len <= 0) {
            WYZE_LOG_DEBUG(g_logger) << "------len=" << len
                << " errno=" << errno << " errstr=" << strerror(errno);
            close();
            return nullptr;
        }

        len += offset;  //读到的长度
        data[len] = '\0';
        nparse = parser->execute(data, len, nparse);    
        if(parser->hasError()){
            close();
            WYZE_LOG_WARN(g_logger) << "parser header error:" << parser->hasError();
            return nullptr;
        }
        
        offset = len;
        if(parser->isFinished()) 
            break;
        
        if(offset == (int)buffer_size){  //这里表示读了 buffer_size 还不能解析出头部 最小 4 *1024
            close();
            WYZE_LOG_WARN(g_logger) << "read full buffer size=" <<  buffer_size
                << ", but not parser data";
            return nullptr;
        }

    }while(true);

    auto& client_parser = parser->getParser();
    if(client_parser.chunked) {
        
        std::string body;
        
        do {
            offset -= nparse;                       //剩余的数据长度
            parser->reset(data, nparse, offset);   //清除前面的数据，接续chunked 数据
            nparse = 0;
            do {
                int len = read(data + offset, buffer_size - offset);
                if(len <= 0) {
                    WYZE_LOG_DEBUG(g_logger) << "------len=" << len
                        << " errno=" << errno << " errstr=" << strerror(errno);
                    close();
                    return nullptr;
                }
                
                len += offset;  //读到的数据长度
                data[len] = '\0';
                nparse = parser->execute(data, len, nparse);    
                if(parser->hasError()){
                    close();
                    WYZE_LOG_WARN(g_logger) << "parser chunk header error:" << parser->hasError();
                    return nullptr;
                }
                
                offset = len;
                if(parser->isFinished()) 
                    break;
                
                if(offset == (int)buffer_size){  //这里表示读了 buffer_size 还不能解析出头部 最小 4 *1024
                    close();
                    WYZE_LOG_WARN(g_logger) << "read full buffer size=" <<  buffer_size
                        << ", but not parser data";
                    return nullptr;
                }

            }while(true);

            // WYZE_LOG_DEBUG(g_logger) << "content_len=" << client_parser.content_len;

            //(client_parser.content_len + 2) 表示 data + \r\n
            //(offset - nparse) 表示解析剩余数据
            if((client_parser.content_len + 2) <= (int)(offset - nparse)) {
                //读出来的数据够多
                body.append(data + nparse, client_parser.content_len);
                nparse += client_parser.content_len + 2;        //解析的数据
            } 
            else {

                //读出来的数据不够, 先把读到的数据
                body.append(data + nparse, offset - nparse);
                int left = client_parser.content_len - (offset - nparse) + 2;  //还需要读取的数据, 把数据的末尾 \r\n 也读取出来
                while( left > 0) {

                    int rt = read(data, left > (int)buffer_size ? (size_t)buffer_size : left);
                    if(rt <= 0) {
                        close();
                        return nullptr;
                    }

                    body.append(data, rt);
                    left -= rt;
                }
                
                body.resize(body.size()-2);
                offset = nparse = 0;
            }

            if(body.size() > (size_t)s_http_response_max_body_size){
                close();
                WYZE_LOG_WARN(g_logger) << "max body size=" <<  s_http_response_max_body_size
                            << ", real body size=" << body.size();
                return nullptr;
            }

        }while(!client_parser.chunks_done);
        parser->getData()->setBody(body);
        saveLeftData(data, offset, nparse);

    }
    else {

        int64_t length = parser->getContentLength(); 
        if(length > (int64_t)s_http_response_max_body_size){
            close();
            WYZE_LOG_WARN(g_logger) << "max body size=" <<  s_http_response_max_body_size
                        << ", real body size=" << length;
            return nullptr;
        }
        
        if(length > 0) {            //有消息体
            std::string body;
            // body.reserve(length);    //申请空间，不创建
            body.resize(length);        //申请空间，创建对象

            if( (length +  nparse) > (uint64_t)offset) {   //需要读取数据

                // body.append(data + nparse, offset);
                memcpy(&body[0], data + nparse, offset);
                length = length + nparse - offset;
                // if(readFixSize(&body[body.size()], length) <= 0) 
                if(readFixSize(&body[offset - nparse], length) <= 0) {
                    close();
                    return nullptr;
                }
            }
            else {                  //需要保存数据
                // body.append(data + nparse, length);
                memcpy(&body[0], data + nparse, length);
                saveLeftData(data, offset, nparse + length);
            }

            parser->getData()->setBody(body);
        }
        else {      //没有消息体，但是要保存剩余数据
            saveLeftData(data, offset, nparse); 
        }
    }

    parser->getIsClose();
    return parser->getData();
}

int HttpConnection::sendRequest(HttpRequest::ptr req)
{
    std::string data = req->toString();
    return writeFixSize(data.c_str(),data.size());
}

void HttpConnection::saveLeftData(char* data, size_t len, size_t nparse)
{
    left_size = len - nparse;
    memcpy(left_data, data, left_size);
}

HttpResult::ptr HttpConnection::DoGet(const std::string& url
                                ,uint64_t timeout_ms
                                ,const std::map<std::string, std::string>& headers
                                ,const std::string& body)
{
    Uri::ptr uri = Uri::Create(url);
    if(!uri) {
        return std::make_shared<HttpResult>(HttpResult::Error::INVALID_URL,
                                nullptr, "invalid url: " + url);
    }
    return DoGet(uri, timeout_ms, headers, body);
}

 HttpResult::ptr HttpConnection::DoGet(Uri::ptr uri
                                ,uint64_t timeout_ms
                                ,const std::map<std::string, std::string>& headers
                                ,const std::string& body)
{
    return DoRequest(HttpMethod::GET, uri, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnection::DoPost(const std::string& url
                                ,uint64_t timeout_ms
                                ,const std::map<std::string, std::string>& headers
                                ,const std::string& body)
{
    Uri::ptr uri = Uri::Create(url);
    if(!uri) {
        return std::make_shared<HttpResult>(HttpResult::Error::INVALID_URL,
                                        nullptr, "invalid url: " + url);
    }
    return DoPost(uri, timeout_ms, headers, body);
}
    
HttpResult::ptr HttpConnection::DoPost(Uri::ptr uri
                                ,uint64_t timeout_ms
                                ,const std::map<std::string, std::string>& headers
                                ,const std::string& body)
{
    return DoRequest(HttpMethod::POST, uri, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnection::DoRequest(HttpMethod method
                                ,const std::string& url
                                ,uint64_t timeout_ms
                                ,const std::map<std::string, std::string>& headers
                                ,const std::string& body)
{
    Uri::ptr uri = Uri::Create(url);
    if(!uri) {
        return std::make_shared<HttpResult>(HttpResult::Error::INVALID_URL,
                                nullptr, "invalid url: " + url);
    }
    return DoRequest(method, uri, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnection::DoRequest(HttpMethod method
                                ,Uri::ptr uri
                                ,uint64_t timeout_ms
                                ,const std::map<std::string, std::string>& headers
                                ,const std::string& body)
{
    HttpRequest::ptr req = std::make_shared<HttpRequest>();
    req->setPath(uri->getPath());
    req->setQuery(uri->getQuery());
    req->setFragment(uri->getFragment());
    req->setMethod(method);
    
    bool has_host = false;
    bool find_connection = false;
    bool find_host = false;
    for(auto& i : headers) {
        if(!find_connection && strcasecmp(i.first.c_str(), "connection") == 0) {
            if(strcasecmp(i.second.c_str(), "keep-alive") == 0) {
                req->setClose(false);
            }
            find_connection = true;
            continue;
        }

        if(!find_host && strcasecmp(i.first.c_str(), "host") == 0) {
            has_host = !i.second.empty();
            find_host = true;
        }

        req->setHeader(i.first, i.second);
    }

    if(!has_host)
        req->setHeader("host", uri->getHost());
    
    req->setBody(body);

    return DoRequest(req, uri, timeout_ms);
}
    
HttpResult::ptr HttpConnection::DoRequest(HttpRequest::ptr req
                                ,Uri::ptr uri
                                ,uint64_t timeout_ms)
{
    Address::ptr addr = uri->createAddress();
    if(!addr) {
        return std::make_shared<HttpResult>(HttpResult::Error::INVALID_HOST,
                                nullptr, "invalid host: " + uri->getHost());
    }

    Socket::ptr sock = Socket::CreateTCP(addr);
    if(!sock->connect(addr)) {
        return std::make_shared<HttpResult>(HttpResult::Error::CONNECT_FAIL,
                                nullptr, "connect fail: " + addr->toString());
    }

    sock->setRecvTimeout(timeout_ms);
    HttpConnection::ptr conn = std::make_shared<HttpConnection>(sock);
    int rt = conn->sendRequest(req);
    if(rt == 0) {
        return std::make_shared<HttpResult>(HttpResult::Error::CLOSE_BY_PEER,
                                nullptr, "send request closed by peer: " + addr->toString());
    }
    if(rt < 0) {
        return std::make_shared<HttpResult>(HttpResult::Error::SOCKET_ERROR,
                                nullptr, "send request socket error, errno=" + std::to_string(errno)
                                + " errstr=" + std::string(strerror(errno)));
    }

    HttpResponse::ptr rsp= conn->recvResponse();
    if(!rsp) {
        if(errno == 0) {
            return std::make_shared<HttpResult>(HttpResult::Error::CLOSE_BY_PEER,
                                nullptr, "recv response closed by peer: " + addr->toString());
        }
        else {
            return std::make_shared<HttpResult>(HttpResult::Error::TIMEOUT
                    , nullptr, "recv response timeout: " + addr->toString()
                    + " timeout_ms:" + std::to_string(timeout_ms));
        }
    }
    return std::make_shared<HttpResult>(HttpResult::Error::OK, rsp, "ok");
}

HttpConnectionPool::HttpConnectionPool(const std::string &host, uint16_t port,
                        uint32_t max_size, uint32_t max_alive_time,
                        uint32_t max_request, const std::string& vhost)
    :m_host(host), m_vhost(vhost), m_port(port)
    ,m_maxSize(max_size), m_maxAliveTime(max_alive_time + GetCurrentMS())
    ,m_maxRequest(max_request)
{
}

HttpConnection::ptr 
HttpConnectionPool::getConnection()
{
    std::vector<HttpConnection*> invalid_conn;
    HttpConnection* raw_ptr = nullptr;

    {
        MutexType::Lock lock(m_mutex);
        while(!m_conns.empty()) {

            auto raw_conn = m_conns.front();
            m_conns.pop_front();

            if(!raw_conn->isConnected()
                || ((raw_conn->m_createTimes + m_maxAliveTime) <= GetCurrentMS()) ) {
                invalid_conn.push_back(raw_conn);
                continue;
            }

            raw_ptr = raw_conn;
            break;
        }
        m_total = invalid_conn.size();
    }

    for(auto& i : invalid_conn)
        delete i;
    
    if(!raw_ptr) {
        IPAddress::ptr addr = Address::LookupAnyIPAddress(m_host);
        if(!addr) {
            WYZE_LOG_ERROR(g_logger) << "get addr fail: " << m_host;
            return nullptr;
        }
        addr->setPort(m_port);

        Socket::ptr sock = Socket::CreateTCP(addr);
        if(!sock->connect(addr)) {
            WYZE_LOG_ERROR(g_logger) << "sock fail: " << *addr;
            return nullptr;
        }

        raw_ptr = new HttpConnection(sock);
        ++m_total;
    }

    return HttpConnection::ptr(raw_ptr, std::bind(&HttpConnectionPool::ReleasePtr
                            ,std::placeholders::_1, this));
}

void HttpConnectionPool::ReleasePtr(HttpConnection* ptr, HttpConnectionPool* pool)
{
    ++ptr->m_request;
    if(!ptr->isConnected() || ((uint32_t)pool->m_total > pool->m_maxSize)
            || (ptr->m_createTimes + pool->m_maxAliveTime) <= GetCurrentMS()
            || (ptr->m_request >= pool->m_maxRequest)) {
        delete ptr;
        --pool->m_total;
        return ;
    }

    MutexType::Lock lock(pool->m_mutex);
    pool->m_conns.push_back(ptr);
}

HttpResult::ptr 
HttpConnectionPool::doGet(const std::string& url, uint64_t timeout_ms
                        ,const std::map<std::string, std::string>& headers
                        ,const std::string& body )
{
    return doRequest(HttpMethod::GET, url, timeout_ms, headers, body);
}

HttpResult::ptr 
HttpConnectionPool::doGet(Uri::ptr uri, uint64_t timeout_ms
                        ,const std::map<std::string, std::string>& headers
                        ,const std::string& body )
{
    std::stringstream ss;
    ss << uri->getHost()
        << (uri->getQuery().empty() ? "" : "?")
        << uri->getQuery()
        << (uri->getFragment().empty() ? "" : "#")
        << uri->getFragment();

    return doRequest(HttpMethod::GET, ss.str(), timeout_ms, headers, body);
}

HttpResult::ptr 
HttpConnectionPool::doPost(const std::string& url, uint64_t timeout_ms
                        ,const std::map<std::string, std::string>& headers
                        ,const std::string& body )
{
    return doRequest(HttpMethod::POST, url, timeout_ms, headers, body);
}
    
HttpResult::ptr 
HttpConnectionPool::doPost(Uri::ptr uri, uint64_t timeout_ms
                        ,const std::map<std::string, std::string>& headers
                        ,const std::string& body)
{
    std::stringstream ss;
    ss << uri->getHost()
        << (uri->getQuery().empty() ? "" : "?")
        << uri->getQuery()
        << (uri->getFragment().empty() ? "" : "#")
        << uri->getFragment();

    return doRequest(HttpMethod::GET, ss.str(), timeout_ms, headers, body);
}

HttpResult::ptr 
HttpConnectionPool::doRequest(HttpMethod method, const std::string& url, uint64_t timeout_ms
                            ,const std::map<std::string, std::string>& headers
                            ,const std::string& body )
{
    HttpRequest::ptr req = std::make_shared<HttpRequest>();
    req->setPath(url);
    req->setMethod(method);
    req->setClose(false);

    bool has_host = false;
    bool find_host = false;
    bool find_close = false;
    for(auto& i : headers) {
        if(!find_close && strcasecmp(i.first.c_str(), "connection") == 0) {
            find_close = true;
            continue;
        }

        if(!find_host && strcasecmp(i.first.c_str(), "host") == 0) {
            has_host = !i.second.empty();
            find_host = true;
        }
        req->setHeader(i.first, i.second);
    }

    if(!has_host) {
        if(m_vhost.empty()) {
            req->setHeader("host", m_host);
        }
        else {
            req->setHeader("host", m_vhost);
        }
    }

    req->setBody(body);
    return doRequest(req, timeout_ms);


}

HttpResult::ptr 
HttpConnectionPool::doRequest(HttpMethod method, Uri::ptr uri, uint64_t timeout_ms
                            ,const std::map<std::string, std::string>& headers
                            ,const std::string& body)
{
    std::stringstream ss;
    ss << uri->getHost()
        << (uri->getQuery().empty() ? "" : "?")
        << uri->getQuery()
        << (uri->getFragment().empty() ? "" : "#")
        << uri->getFragment();

    return doRequest(method, ss.str(), timeout_ms, headers, body);
}
    
HttpResult::ptr 
HttpConnectionPool::doRequest(HttpRequest::ptr req,uint64_t timeout_ms)
{
    auto conn = getConnection();
    if(!conn) {
        return std::make_shared<HttpResult>(HttpResult::Error::POOL_GET_CONNECTION,
                    nullptr, "pool host:" + m_host + " port:" + std::to_string(m_port));
    }

    auto sock = conn->getSocket();
    if(!sock) {
        return std::make_shared<HttpResult>(HttpResult::Error::POOL_INVALID_CONNECTION,
                    nullptr, "pool host:" + m_host + " port:" + std::to_string(m_port));
    }
    sock->setRecvTimeout(timeout_ms);
    auto addr = sock->getRemoteAddress();

    int rt = conn->sendRequest(req);
    if(rt == 0) {
        return std::make_shared<HttpResult>(HttpResult::Error::CLOSE_BY_PEER,
                                nullptr, "send request closed by peer: " + addr->toString());
    }
    if(rt < 0) {
        return std::make_shared<HttpResult>(HttpResult::Error::SOCKET_ERROR,
                                nullptr, "send request socket error, errno=" + std::to_string(errno)
                                + " errstr=" + std::string(strerror(errno)));
    }

    HttpResponse::ptr rsp= conn->recvResponse();
    if(!rsp) {
        if(errno == 0) {
            return std::make_shared<HttpResult>(HttpResult::Error::CLOSE_BY_PEER,
                                nullptr, "recv response closed by peer: " + addr->toString());
        }
        else {
            return std::make_shared<HttpResult>(HttpResult::Error::TIMEOUT
                    , nullptr, "recv response timeout: " + addr->toString()
                    + " timeout_ms:" + std::to_string(timeout_ms));
        }
    }
    return std::make_shared<HttpResult>(HttpResult::Error::OK, rsp, "ok");
}

}
}