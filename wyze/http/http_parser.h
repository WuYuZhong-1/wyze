/*
 * @Author: wyze 631848287@qq.com
 * @Date: 2022-12-01 15:04:40
 * @LastEditors: wyze 631848287@qq.com
 * @LastEditTime: 2023-02-21 09:54:43
 * @FilePath: /wyze/wyze/http/http_parser.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
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
    void reset(char* data, size_t offset, size_t len);
    int isFinished();   
    int hasError();
    void setError(int v) { m_error = v;}

    HttpRequest::ptr getData() const { return m_data; }
    HttpRequest::ptr getData() { return m_data; }
    uint64_t getContentLength();
    bool getIsClose();
    http_parser& getParser() { return m_parser; }
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
    void reset(char* data, size_t offset, size_t len);
    size_t execute(char* data, size_t len, size_t offset = 0);
    int isFinished();   
    int hasError();
    void setError(int v) { m_error = v;}

    HttpResponse::ptr getData() const { return m_data; }
    HttpResponse::ptr getData() { return m_data; }
    uint64_t getContentLength();
    bool getIsClose();
    httpclient_parser& getParser() { return m_parser; }
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

