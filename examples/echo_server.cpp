#include "../wyze/wyze.h"

static wyze::Logger::ptr g_logger = WYZE_LOG_ROOT();

class EchoServer : public wyze::TcpServer {
public:
    EchoServer(int type);
    void handleClient(wyze::Socket::ptr client) override;
private:
    int m_type;
};

EchoServer::EchoServer(int type) 
    : m_type(type) { }

void EchoServer::handleClient(wyze::Socket::ptr client)
{
    WYZE_LOG_INFO(g_logger) << "handleClient: " << *client;
    wyze::ByteArray::ptr ba(new wyze::ByteArray);

    while(true) {
        ba->clear();
        std::vector<iovec> iovs;
        ba->getWriteBuffers(iovs, 1024 * 4);    //申请内存，但是还没有填充数据,这里就设置了 iovec的每个数据内存大小
        // WYZE_LOG_INFO(g_logger) << iovs.size();
        int rt = client->recv(&iovs[0], iovs.size());
        if(rt == 0) {
            WYZE_LOG_INFO(g_logger) << "client close: " << *client;
            break;
        }
        else if(rt < 0) {
            WYZE_LOG_INFO(g_logger) << "client:" << *client <<  
                " error=" << errno << " errstr" << strerror(errno);
            break;
        }

        ba->setPosition(ba->getPosition() + rt);
        ba->setPosition(0);

        if(m_type == 1) {
            WYZE_LOG_INFO(g_logger) << "\n" << ba->toString();
        }
        else {
            WYZE_LOG_INFO(g_logger) << "\n" << ba->toHexString();
        }
    }
}

void run(int type)
{
    WYZE_LOG_INFO(g_logger) << " server type=" << type;
    EchoServer::ptr es(new EchoServer(type));
    auto addr = wyze::IPAddress::LookupAnyIPAddress("0.0.0.0:8020");
    while(!es->bind(addr))
        sleep(2);
    es->start();
}

int main(int argc, char** argv)
{
    int type = 1;

    if(argc < 2 ) {
        WYZE_LOG_INFO(g_logger) << "used as [" << argv[0] << " -t] or ["<< argv[0] << " -b]";
        return 0;
    }

    if(strcmp(argv[1], "-b") == 0)
        type = 2;

    wyze::IOManager iom(2);
    iom.schedule(std::bind(&run,type));
    return 0;
}