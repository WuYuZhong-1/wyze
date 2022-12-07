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


static char chunk_first[] = "HTTP/1.1 200 OK\r\n"
                            "Content-Type: text/plain\r\n"
                            "Transfer-Encoding: chunked\r\n\r\n"
                            "25\r\n"
                            "This is the data in the first chunk\r\n\r\n";
static char chunk_second[] = "1C\r\n"
                            "and this is the second one\r\n\r\n"
                            "3\r\n"
                            "con"
                            "8\r\n"
                            "sequence"
                            "0\r\n\r\n";
                            

void test_chunked()
{
    (void)chunk_second;
    wyze::http::HttpResponseParser parser;
    std::string tmp = chunk_first;
    int s = parser.execute(&tmp[0], tmp.size(), 0);
    WYZE_LOG_INFO(g_logger) << "execute rt=" << s
        << " has_error=" << parser.hasError()
        << " is_finished=" << parser.isFinished()
        << " total=" << tmp.size();
    WYZE_LOG_INFO(g_logger) << &tmp[s];
    auto& p = parser.getParser();
    WYZE_LOG_INFO(g_logger) << "chunked=" << p.chunked
                            << "  chunks_done=" << p.chunks_done
                            << "  chunk_size=" << p.content_len;

    parser.reset(&tmp[0], s, tmp.size() - s);
    tmp.resize(tmp.size() - s);
    WYZE_LOG_INFO(g_logger) << "size = " << tmp.size() << " data=" << tmp;

    s = parser.execute(&tmp[0], tmp.size(), 0);
    WYZE_LOG_INFO(g_logger) << "execute rt=" << s
        << " has_error=" << parser.hasError()
        << " is_finished=" << parser.isFinished()
        << " total=" << tmp.size();
    WYZE_LOG_INFO(g_logger) << &tmp[s];
    WYZE_LOG_INFO(g_logger) << "chunked=" << p.chunked
                            << "  chunks_done=" << p.chunks_done
                            << "  chunk_size=" << p.content_len;



    //这个解析可以解析到chunked数据
    // wyze::http::HttpResponseParser parser1;                    
    // int s1 = parser1.execute(&tmp[s], tmp.size() -s, 0);
    // WYZE_LOG_INFO(g_logger) << "execute rt=" << s1
    //     << " has_error=" << parser1.hasError()
    //     << " is_finished=" << parser1.isFinished()
    //     << " total=" << tmp.size();
    // WYZE_LOG_INFO(g_logger) << &tmp[s + s1];
    // auto p1 = parser1.getParser();
    // WYZE_LOG_INFO(g_logger) << "chunked=" << p1.chunked
    //                         << "  chunks_done=" << p1.chunks_done
    //                         << "  chunk_size=" << p1.content_len;

}


int main(int argc, char** argv) 
{
    // test_request();
    // WYZE_LOG_INFO(g_logger) << "------------------------------------";
    // test_response();
    test_chunked();
    return 0;
}