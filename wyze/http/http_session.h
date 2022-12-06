#ifndef _WYZE_HTTP_SESSION_H_
#define _WYZE_HTTP_SESSION_H_

#include "http.h"
#include "../sockstream.h"

namespace wyze {
namespace http {

class HttpSession : public  SockStream {
public:
    using ptr = std::shared_ptr<HttpSession>;
    HttpSession(Socket::ptr sock, bool owner = true);
    ~HttpSession();
    HttpRequest::ptr recvRequest();
    int sendResponse(HttpResponse::ptr rsp);

private:
    void saveLeftData(char* data, size_t len, size_t nparse);

private:
    size_t left_size;
    char *left_data;
};
}
}

#endif // !_WYZE_HTTP_SESSION_H_