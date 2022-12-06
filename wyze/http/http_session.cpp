#include "http_session.h"
#include "http_parser.h"
#include "../config.h"
#include "../log.h"

namespace wyze {
namespace http {

static Logger::ptr g_logger = WYZE_LOG_NAME("system");

static ConfigVar<uint64_t>::ptr g_http_request_buffer_size =
    Config::Lookup("http.request.buffer_size"
            ,(uint64_t)(4 * 1024), "http request buffer size");

static ConfigVar<uint64_t>::ptr g_http_request_max_body_size = 
    Config::Lookup("http.request.max_body_size"
            ,(uint64_t)(64 * 1024 * 1024), "http request max body size");

static uint64_t s_http_request_buffer_size = 0;
static uint64_t s_http_request_max_body_size = 0;

namespace {
struct _RequestSizeIniter {
    _RequestSizeIniter(){
        s_http_request_buffer_size = g_http_request_buffer_size->getValue();
        s_http_request_max_body_size = g_http_request_max_body_size->getValue();

        g_http_request_buffer_size->addListener(
            [](const uint64_t ov, const uint64_t nv) {
                s_http_request_buffer_size = nv;
            }
        );

        g_http_request_max_body_size->addListener(
            [](const uint64_t ov, const uint64_t nv) {
                s_http_request_max_body_size = nv;
            }
        );
    }
};
static _RequestSizeIniter _init;
}

HttpSession::HttpSession(Socket::ptr sock, bool owner)
    : SockStream(sock, owner)
    , left_size(0)
    , left_data(new char[(s_http_request_buffer_size >  (4 *1024)) ?  s_http_request_buffer_size : (4 *1024)])
{
}

HttpSession::~HttpSession()
{
    delete[] left_data;
}

HttpRequest::ptr HttpSession::recvRequest()
{
    //这里会造成数据多读的现象
    HttpRequestParser::ptr parser(new HttpRequestParser);
    uint64_t buffer_size = (s_http_request_buffer_size >  (4 *1024)) ?  s_http_request_buffer_size : (4 *1024);   //避免在下面执行中 s_http_request_buffer_size 发生改变

    
    std::shared_ptr<char> buffer(new char[buffer_size], [](char* ptr) {
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
        if(len <= 0) 
            return nullptr;

        len += offset;  //读到的长度
        nparse = parser->execute(data, len, nparse);    
        if(parser->hasError())
            return nullptr;
        
        offset = len;
        if(offset == (int)buffer_size) 
            return nullptr;
        
        if(parser->isFinished()) {
            break;
        }
    }while(true);

    int64_t length = parser->getContentLength();            
    if(length > 0) {            //有消息体
        std::string body;
        body.reserve(length);

        if( (length +  nparse) > (uint64_t)offset) {   //需要读取数据

            body.append(data + nparse, offset);
            length = length + nparse - offset;
            if(readFixSize(&body[body.size()], length) <= 0) 
                return nullptr;
        }
        else {                  //需要保存数据
            body.append(data + nparse, length);
            saveLeftData(data, offset, nparse + length);
        }

        parser->getData()->setBody(body);
    }
    else {      //没有消息体，但是要保存剩余数据
       saveLeftData(data, offset, nparse); 
    }
    parser->getIsClose();
    return parser->getData();
}

int HttpSession::sendResponse(HttpResponse::ptr rsp)
{
    std::string data = rsp->toString();
    return writeFixSize(data.c_str(),data.size());
}

void HttpSession::saveLeftData(char* data, size_t len, size_t nparse)
{
    left_size = len - nparse;
    memcpy(left_data, data, left_size);
}

}
}