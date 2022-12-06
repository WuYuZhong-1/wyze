#ifndef _WYZE_HTTP_PARSER_H_
#define _WYZE_HTTP_PARSER_H_

#include "http.h"
#include "http11_parser.h"
#include "httpclient_parser.h"

namespace wyze {
namespace http{

class HttpRequestParser {
public:
    using ptr = std::shared_ptr<HttpRequestParser>;

    HttpRequestParser();
    //                   data 数据首地址   len 数据长度  offset 解析的数据
    size_t execute(char* data, size_t len, size_t offset = 0);
    int isFinished();   
    int hasError();
    void setError(int v) { m_error = v;}

    HttpRequest::ptr getData() const { return m_data; }
    uint64_t getContentLength();
    bool getIsClose();

private:
    http_parser m_parser;
    HttpRequest::ptr m_data;
    //1000: invalid method
    //1001: invalid version
    //1002: invalid field
    int m_error;
};

class HttpResponseParser {
public:
    using ptr = std::shared_ptr<HttpResponseParser>;

    HttpResponseParser();
    size_t execute(char* data, size_t len, size_t offset = 0);
    int isFinished();   
    int hasError();
    void setError(int v) { m_error = v;}

    HttpResponse::ptr getData() const { return m_data; }
    uint64_t getContentLength();
private:
    httpclient_parser m_parser;
    HttpResponse::ptr m_data;
    //1001: invalid version
    //1002: invalid field
    int m_error;
};

}
}

#endif //_WYZE_HTTP_PARSER_H_

