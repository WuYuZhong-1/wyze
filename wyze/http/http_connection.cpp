#include "http_connection.h"
#include "http_parser.h"
#include "../config.h"
#include "../log.h"

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

HttpConnection::HttpConnection(Socket::ptr sock, bool owner)
    : SockStream(sock, owner)
    , left_size(0)
    , left_data(new char[(s_http_response_buffer_size >  (4 *1024)) ?  s_http_response_buffer_size : (4 *1024)])
{
}

HttpConnection::~HttpConnection()
{
    delete[] left_data;
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
                int left = client_parser.content_len - (offset - nparse);  //还需要读取的数据
                while( left > 0) {

                    int rt = read(data, left > (int)buffer_size ? (size_t)buffer_size : left);
                    if(rt <= 0) {
                        close();
                        return nullptr;
                    }

                    body.append(data, rt);
                    left -= rt;
                }
                if(readFixSize(data, 2) <= 0) {  //把 数据的末尾 \r\n 去v掉
                    close();
                    return nullptr;
                }

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

}
}