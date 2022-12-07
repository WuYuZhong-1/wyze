#include "../wyze/wyze.h"

static wyze::Logger::ptr g_logger = WYZE_LOG_ROOT();

void run()
{
    auto addr = wyze::Address::LookupAny("0.0.0.0:8033");
    // wyze::UnixAddress::ptr addr2(new wyze::UnixAddress("/tmp/wyze_unix_addr"));
    std::vector<wyze::Address::ptr> addrs;
    std::vector<wyze::Address::ptr> fails;
    addrs.push_back(addr);
    // addrs.push_back(addr2);

    wyze::TcpServer::ptr tcp_server(new wyze::TcpServer);
    while(!tcp_server->bind(addrs, fails)) 
        sleep(2);

    tcp_server->start();
}

int main(int argc, char** argv) 
{
    wyze::IOManager iom(1);
    iom.schedule(run);
    return 0;
}