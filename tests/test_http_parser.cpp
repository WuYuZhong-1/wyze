#include "../wyze/wyze.h"
#include <string.h>

static wyze::Logger::ptr g_logger = WYZE_LOG_ROOT();

const char test_request_data[] = "POST / HTTP/1.1\r\n"
                                "Content-Length: 10\r\n"
                                "Host: ";
const char test_request_data2[] = "www.baidu.com\r\n\r\n"
                                "1234567890";

void test_request()
{
    wyze::http::HttpRequestParser parser;
    std::string tmp = test_request_data;
    WYZE_LOG_INFO(g_logger) << "-----------------------" << tmp.size();
    size_t s = parser.execute(&tmp[0], tmp.length());
    WYZE_LOG_INFO(g_logger) << "execute rt=" << s
        << " has_error=" << parser.hasError()
        << " is_finished=" << parser.isFinished()
        << " total=" << tmp.size()
        << " content-length=" << parser.getContentLength();

    tmp += test_request_data2;
    s = parser.execute(&tmp[0], tmp.size(), s);
    WYZE_LOG_INFO(g_logger) << "execute rt=" << s
        << " has_error=" << parser.hasError()
        << " is_finished=" << parser.isFinished()
        << " total=" << tmp.size()
        << " content-length=" << parser.getContentLength();

    WYZE_LOG_INFO(g_logger) << "\n"
                            << parser.getData()->toString()
                            << "-------------------\n"
                            << &tmp[s];
}

const char test_response_data[] = "HTTP/1.1 200 OK\r\n"
        "Date: Tue, 04 Jun 2019 15:43:56 GMT\r\n"
        "Server: Apache\r\n"
        "Last-Modified: Tue, 12 Jan 2010 13:48:00 GMT\r\n"
        "ETag: \"51-47cf7e6ee8400\"\r\n"
        "Accept-Ranges: bytes\r\n"
        "Content-Length: 81\r\n"
        "Cache-Control: max-age=86400\r\n"
        "Expires: Wed, 05 Jun 2019 15:43:56 GMT\r\n"
        "Connection: Close\r\n"
        "Content-Type: text/html\r\n\r\n"
        "<html>\r\n"
        "<meta http-equiv=\"refresh\" content=\"0;url=http://www.baidu.com/\">\r\n"
        "</html>\r\n";

void test_response()
{
    wyze::http::HttpResponseParser parser;
    std::string tmp = test_response_data;
    WYZE_LOG_INFO(g_logger) << "length = " << tmp.size();
    size_t s = parser.execute(&tmp[0], tmp.size());
    WYZE_LOG_INFO(g_logger) << "execute rt=" << s
        << " has_error=" << parser.hasError()
        << " is_finished=" << parser.isFinished()
        << " total=" << tmp.size()
        << " content-length=" << parser.getContentLength();
    tmp.resize(tmp.size() -s);
    WYZE_LOG_INFO(g_logger) << "\n"
                            << parser.getData()->toString()
                            << "-------------------\n"
                            << tmp;
}


int main(int argc, char** argv) 
{
    test_request();
    WYZE_LOG_INFO(g_logger) << "------------------------------------";
    test_response();
    return 0;
}