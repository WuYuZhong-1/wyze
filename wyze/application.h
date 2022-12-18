#ifndef _WYZE_APPLICARION_H_
#define _WYZE_APPLICATION_H_

#include "http/http_server.h"
#include <vector>

namespace wyze {

class Application {
public:
    Application();
    bool init(int argc, char** argv);
    bool run();

private:
    int main(int argc, char** argv);
    int main_fiber();

private:
    int m_argc;
    char** m_argv;
    std::vector<http::HttpServer::ptr> m_http_servers;
};

}

#endif