#ifndef _WYZE_HTTP_CONNECTION_H_
#define _WYZE_HTTP_CONNECTION_H_

#include "http.h"
#include "../sockstream.h"

namespace wyze {
namespace http {

class HttpConnection : public  SockStream {
public:
    using ptr = std::shared_ptr<HttpConnection>;
    HttpConnection(Socket::ptr sock, bool owner = true);
    ~HttpConnection();
    HttpResponse::ptr recvResponse();
    int sendRequest(HttpRequest::ptr req);

private:
    void saveLeftData(char* data, size_t len, size_t nparse);

private:
    size_t left_size;
    char *left_data;
};
}
}

#endif // !_WYZE_HTTP_CONNECTION_H_