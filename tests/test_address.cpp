#include "../wyze/wyze.h"
#include <arpa/inet.h>
#include <sys/un.h>
#include <stddef.h>
#include <map>

wyze::Logger::ptr g_logger = WYZE_LOG_ROOT();

void test_addrlen()
{
    struct sockaddr_un unaddr;
    struct sockaddr_in i4addr;
    struct sockaddr_in6 i6addr;

    WYZE_LOG_INFO(g_logger) << "sockaddr_un = " << sizeof(unaddr);
    WYZE_LOG_INFO(g_logger) << "sockaddr_in = " << sizeof(i4addr);
    WYZE_LOG_INFO(g_logger) << "sockaddr_in6 = " << sizeof(i6addr);

    const size_t max_path_len = sizeof(((sockaddr_un*)0)->sun_path);
    WYZE_LOG_INFO(g_logger) << max_path_len;
    size_t offsetof_len = offsetof(sockaddr_un, sun_path) + max_path_len;
    WYZE_LOG_INFO(g_logger) << offsetof_len;

    WYZE_LOG_INFO(g_logger) << sizeof(unaddr.sun_path);
}


//得到末尾的掩码数
template<class T>
static T CreateMask(uint32_t bits) {
    return (1 << (sizeof(T) * 8 - bits)) - 1;
}

void test_mask()
{
    WYZE_LOG_INFO(g_logger) << CreateMask<uint32_t>(16);
}

template<class T>
static T CountBytes(T value) {
    uint32_t rt = 0;
    for(; value; ++rt) {
        value &= value -1;
    }
    return rt;
}

void test_countbyte()
{
    WYZE_LOG_INFO(g_logger) << CountBytes(0b1111111111111111);
}

void test_addresses()
{
    wyze::IPAddress::ptr ip = wyze::IPAddress::Create("192.168.5.68", 8989);
    WYZE_LOG_INFO(g_logger) << ip->broadcastAddress(24)->toString();
    WYZE_LOG_INFO(g_logger) << ip->networkAddress(24)->toString();
    WYZE_LOG_INFO(g_logger) << ip->subnetMask(24)->toString();

}

void test_IP() {
    wyze::IPAddress::ptr ip = wyze::IPAddress::Create("127.0.0.1", 8989);
    if(ip)
        WYZE_LOG_INFO(g_logger) << ip->toString();
}

void test_lookup()
{
    std::vector<wyze::Address::ptr> results;
    // bool rt = wyze::Address::Lookup(results, "baidu.com:http");                      //http 是字节流格式的 socket
    // bool rt = wyze::Address::Lookup(results, "baidu.com:tftp", AF_INET, SOCK_DGRAM);    //tftp 是报文格式的 socket
    bool rt = wyze::Address::Lookup(results, "baidu.com", AF_INET);    //tftp 是报文格式的 socket
    if(rt == false) {
        WYZE_LOG_ERROR(g_logger) << "wyze::Address::Lookup() error";
        return;
    }

    for(size_t i = 0; i < results.size(); ++i) {
        WYZE_LOG_INFO(g_logger) << "i = " << i << " - " << results[i]->toString();
    }
}


void test_interface()
{
    std::multimap<std::string, std::pair<wyze::Address::ptr, uint32_t>> result;
    wyze::Address::GetInterfaceAddresses(result);
    for(auto& a : result) {
        WYZE_LOG_INFO(g_logger) << a.first << ":[" << *a.second.first << ", " << a.second.second << "]";
    }

    auto its = result.equal_range("ens33");
    for(; its.first != its.second; ++its.first) {
        WYZE_LOG_INFO(g_logger) << *its.first->second.first;
    }
}

int main(int argc, char** argv)
{
    // test_addrlen();
    // test_mask();
    // test_countbyte();
    // test_addresses();
    // test_IP();
    // test_lookup();
    test_interface();

    return 0;
}