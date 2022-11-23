#include "log.h"
#include "macro.h"
#include "address.h"

#include <arpa/inet.h>
#include <stddef.h>
#include <string.h>
#include <sstream>
#include <netdb.h>
#include <ifaddrs.h>

namespace wyze {

    static wyze::Logger::ptr g_logger = WYZE_LOG_NAME("system");

    //得到低位有几个 1
    template<class T>
    static T CreateMask(uint32_t bits)
    {
        return ( 1 << (sizeof(T) * 8 - bits)) -1 ;
    }

    // 一个值减 1 ，势必有一个bit 变为 0, 在与上之前的值。
    template<class T>
    static T CountBytes(T value) {
        uint32_t rt = 0;
        for(; value; ++rt) {
            value &= value -1;
        }
        return rt;
    }

    Address::ptr Address::Create(const sockaddr* addr, socklen_t addrlen)
    {
        if( addr == nullptr )
            return nullptr;

        Address::ptr result;
        switch(addr->sa_family) {
            case AF_INET:
                result.reset(new IPv4Address(*(const sockaddr_in*)addr));
                break;
            case AF_INET6:
                result.reset(new IPv6Address(*(const sockaddr_in6*)addr));
                break;
            default:
                result.reset(new UnknownAddress(*addr));
                break;
        }
        return result;
    }

    bool Address::Lookup(std::vector<Address::ptr>& result, const std::string& host,
                            int family, int type, int protocol)
    {
        addrinfo hints, *results = nullptr, *next = nullptr;

        memset(&hints, 0, sizeof(hints));
        hints.ai_flags = 0;
        hints.ai_family = family;
        hints.ai_socktype = type;
        hints.ai_protocol = protocol;

        std::string node;
        const char* service = nullptr;

        //检查 ipv6address service
        if( !host.empty() && host[0] == '[') {
            const char* endipv6 = (const char*)memchr(host.c_str() + 1, ']', host.size() -1);
            if(endipv6) {

                //check out of range
                if(strlen(endipv6) >=3 && *(endipv6 + 1) == ':') {
                    //获取服务
                    service = endipv6 + 2;
                }
                // endipv6 - host.c_str() - 1 地址相减
                node = host.substr(1, endipv6 - host.c_str() - 1);
            }
        }

        //检查 node service
        if(node.empty()) {
            service = (const char*)memchr(host.c_str(), ':', host.size());
            if(service) {
                if(!memchr(service + 1, ':', host.c_str() + host.size() - service - 1)) {
                    node = host.substr(0, service - host.c_str());
                    ++service;
                }
            }
        }

        if(node.empty()) {
            node = host;
        }

        int rt = getaddrinfo(node.c_str(), service, &hints, &results);
        if(rt) {
            WYZE_LOG_ERROR(g_logger) << "Address::Lookup getaddress(" << host << ", "
                << family << ", " << type << ") err=" << rt << "errstr=" 
                << gai_strerror(rt);
            return false;
        }

        for(next = results; next != nullptr; next = next->ai_next) {
            result.emplace_back(Create(next->ai_addr, (socklen_t)next->ai_addrlen));
        }

        freeaddrinfo(results);
        return true;

    }

    Address::ptr Address::LookupAny(const std::string& host,
                    int family, int type, int protocol)
    {
        std::vector<Address::ptr> result;
        if(Lookup(result, host, family, type, protocol)) {
            return result[0];
        }
        return nullptr;
    }

    std::shared_ptr<IPAddress> Address::LookAnyIPAddress(const std::string& host,
                        int family, int type, int protocol)
    {
        std::vector<Address::ptr> result;
        if(Lookup(result, host, family, type, protocol)) {

            // for(auto& i: result) {
            //     WYZE_LOG_DEBUG(g_logger) << i->toString();
            // }

            for(auto& i : result) {
                IPAddress::ptr v = std::dynamic_pointer_cast<IPAddress>(i);
                if(v) {
                    return v;
                }
            }
        }
        return nullptr;
    }

    bool Address::GetInterfaceAddresses(std::multimap<std::string, 
                                        std::pair<Address::ptr, uint32_t>>& result,
                                        int family)
    {
        struct ifaddrs *results = nullptr, *next = nullptr;
        if( getifaddrs(&results) != 0) {
            WYZE_LOG_ERROR(g_logger) << "Address::GetInterfaceAddress getifaddrs() err="
                << errno << " errstr=" << strerror(errno);
            return false;
        }

        try {
            for(next = results; next; next = next->ifa_next) {
                Address::ptr addr;
                uint32_t prefix_len = ~0u;

                if(family != AF_UNSPEC && family != next->ifa_addr->sa_family) 
                    continue;

                switch(next->ifa_addr->sa_family) {
                    case AF_INET: 
                        {
                            addr = Create(next->ifa_addr, sizeof(sockaddr_in));
                            uint32_t netmask = ((sockaddr_in*)next->ifa_netmask)->sin_addr.s_addr;
                            prefix_len = CountBytes(netmask);
                        }
                        break;
                    case AF_INET6:
                        {
                            addr = Create(next->ifa_addr, sizeof(sockaddr_in6));
                            in6_addr& netmask = ((sockaddr_in6*)next->ifa_netmask)->sin6_addr;
                            prefix_len = 0;
                            for(int i = 0; i < 16; ++i) {
                                prefix_len += CountBytes(netmask.s6_addr[i]);
                            }
                        }
                        break;
                    default:
                        break;
                }

                if(addr){
                    result.insert(std::make_pair(next->ifa_name,
                                std::make_pair(addr, prefix_len)));
                }
            }
        }
        catch(...) {
            WYZE_LOG_ERROR(g_logger) << "Address::GetInterfaceAddress exception";
            freeifaddrs(results);
            return false;
        }

        freeifaddrs(results);
        return true;
    }

    bool Address::GetInterfaceAddresses(std::vector<std::pair<Address::ptr, uint32_t>>& result,
                                        const std::string& iface, int family)
    {
        if( iface.empty() || iface == "*") {
            //AF_UNSPEC  为指明
            if(family == AF_INET || family == AF_UNSPEC) {
                result.emplace_back(std::make_pair(Address::ptr( new IPv4Address()), 0u));
            }

            if(family == AF_INET6 || family == AF_UNSPEC) {
                result.emplace_back(std::make_pair(Address::ptr(new IPv6Address()), 0u));
            }
            return true;
        }


        std::multimap<std::string, std::pair<Address::ptr, uint32_t>> results;

        if(!GetInterfaceAddresses(results, family)) {
            return false;
        }

        auto its = results.equal_range(iface);
        for(; its.first != its.second; ++its.first) {
            result.emplace_back(its.first->second);
        }

        return true;
    }

    int Address::getFamily() const
    {
        return getAddr()->sa_family;
    }

    std::string Address::toString()
    {
        std::stringstream ss;
        insert(ss);
        return ss.str();
    }

    bool Address::operator<(const Address& rhs) const
    {
        socklen_t minlen = std::min(getAddrLen(), rhs.getAddrLen());
        int rt = memcmp(getAddr(), rhs.getAddr(), minlen);

        if(rt  < 0)
            return true;
        else if (rt > 0)
            return false;
        else if( getAddrLen() < rhs.getAddrLen())
            return true;
        return false;
    }

    bool Address::operator==(const Address& rhs) const
    {
        return getAddrLen() == rhs.getAddrLen()
                && memcmp(getAddr(), rhs.getAddr(), getAddrLen()) == 0;
    }

    bool Address::operator!=(const Address& rhs) const
    {
        return !(*this == rhs);
    }


    IPAddress::ptr IPAddress::Create(const char* addr, uint16_t port)
    {
        addrinfo hints, *phints = nullptr;
        memset(&hints, 0, sizeof(hints));
        
        //AI_NUMERICHOST标志抑制任何潜在的长网络‐工作主机地址查找
        hints.ai_flags = AI_NUMERICHOST;
        hints.ai_family = AF_UNSPEC;

        int rt = getaddrinfo(addr, NULL, &hints, &phints);
        if(rt) {
            WYZE_LOG_ERROR(g_logger) << "IPAddress::Create(" << addr
                <<", " << port << ") errno=" << rt << " errstr=" << strerror(rt);
            return nullptr;
        }

        try {
            IPAddress::ptr result = std::dynamic_pointer_cast<IPAddress>(
                Address::Create(phints->ai_addr, (socklen_t)phints->ai_addrlen)
            );

            if(result) {
                result->setPort(port);
            }

            freeaddrinfo(phints);
            return result;
        }
        catch(...) {
            freeaddrinfo(phints);
            return nullptr;
        }
    }


    //这里传入的参数为  192.168.5.10  80
    IPv4Address::ptr IPv4Address::Create(const char* address, uint16_t port)
    {
        IPv4Address::ptr addr(new IPv4Address);
        
        addr->m_addr.sin_port = htons(port);
        int rt = inet_pton(AF_INET, address, &(addr->m_addr.sin_addr));
        if( rt <= 0) {
            WYZE_LOG_ERROR(g_logger) << "IPv4Address::Create(" << address << ", " 
                                        << port << ") rt=" << rt << " errno=" << errno
                                        << " errstr=" << strerror(errno);
            return nullptr;
        }
        return addr;

    }

    IPv4Address::IPv4Address(const sockaddr_in& address)
    {
        m_addr = address;
    }

    IPv4Address::IPv4Address(uint32_t address, uint16_t port)
    {
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sin_family = AF_INET;
        m_addr.sin_port = htons(port);
        m_addr.sin_addr.s_addr = htonl(address);
    }

    const sockaddr* IPv4Address::getAddr() const
    {
        return (sockaddr*)&m_addr;
    }

    sockaddr* IPv4Address::getAddr()
    {
        return (sockaddr*)&m_addr;
    }

    socklen_t IPv4Address::getAddrLen() const
    {
        return sizeof(m_addr);
    }

    std::ostream& IPv4Address::insert(std::ostream& os) const 
    {
        char buf[32] = {0};
        const char* pbuf  = inet_ntop(AF_INET, &(m_addr.sin_addr.s_addr), buf, sizeof(buf));
        if(pbuf == nullptr){
            WYZE_LOG_WARN(g_logger) << "IPv4Address is't inited";
            return os;
        }

        os << pbuf << ":" << ntohs(m_addr.sin_port);
        return os;
    }

    IPAddress::ptr IPv4Address::broadcastAddress(uint32_t prefix_len) 
    {
        if(prefix_len > 32) 
            return nullptr;

        struct sockaddr_in baddr(m_addr);
        //这里的 CreateMask 则表示 得到 低位多少个 111
        // baddr 中的字节序为大端模式，高位数据放在内存的低地址
        //我们想要 主机的ip全部为1, 则或上即可
        //但是两者的字节序必须相同
        baddr.sin_addr.s_addr |= htonl(CreateMask<uint32_t>(prefix_len));

        return IPv4Address::ptr(new IPv4Address(baddr));
    }

    IPAddress::ptr IPv4Address::networkAddress(uint32_t prefix_len) 
    {
        if(prefix_len > 32) 
            return nullptr;

        struct sockaddr_in baddr(m_addr);

        baddr.sin_addr.s_addr &= ~htonl(CreateMask<uint32_t>(prefix_len));
        baddr.sin_addr.s_addr |= htonl(1);

        return IPv4Address::ptr(new IPv4Address(baddr));
    }

    IPAddress::ptr IPv4Address::subnetMask(uint32_t prefix_len) 
    {
        sockaddr_in subnet;
        memset(&subnet, 0, sizeof(subnet));
        subnet.sin_family = AF_INET;
        subnet.sin_addr.s_addr = ~htonl(CreateMask<uint32_t>(prefix_len));
        return IPv4Address::ptr(new IPv4Address(subnet));
    }

    uint16_t IPv4Address::getPort() const 
    {
        return ntohs(m_addr.sin_port);
    }

    void IPv4Address::setPort(uint16_t v) 
    {
        m_addr.sin_port = htons(v);
    }

    IPv6Address::ptr IPv6Address::Create(const char* address, uint16_t port)
    {
        IPv6Address::ptr addr(new IPv6Address);
        
        addr->m_addr.sin6_port = htons(port);
        int rt = inet_pton(AF_INET6, address, &(addr->m_addr.sin6_addr));
        if(rt <= 0 ){
            WYZE_LOG_ERROR(g_logger) << "IPv6Address::Create(" << address << ", " 
                                        << port << ") rt=" << rt << " errno=" << errno
                                        << " errstr=" << strerror(errno);
            return nullptr;
        }

        return addr;
    }

    IPv6Address::IPv6Address()
    {
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sin6_family = AF_INET6;
    }

    IPv6Address::IPv6Address(const sockaddr_in6& addr)
    {
        m_addr = addr;
    }

    IPv6Address::IPv6Address(const uint8_t addr[16], uint16_t port)
    {
        //TODO::这里根本就用不到
        WYZE_ASSERT(false);

        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sin6_family = AF_INET6;
        m_addr.sin6_port = htons(port);
        memcpy(&m_addr.sin6_addr.s6_addr, addr, 16);
    }

    const sockaddr* IPv6Address::getAddr() const
    {
        return (sockaddr*)&m_addr;
    }

    sockaddr* IPv6Address::getAddr()
    {
        return (sockaddr*)&m_addr;
    }

    socklen_t IPv6Address::getAddrLen() const
    {
        return sizeof(m_addr);
    } 

    std::ostream& IPv6Address::insert(std::ostream& os) const
    {
        char buf[128] = {0};
        const char* pbuf = inet_ntop(AF_INET6, &(m_addr.sin6_addr), buf, sizeof(buf));
        if(pbuf == nullptr){
            WYZE_LOG_WARN(g_logger) << "IPv6Address is't inited";
            return os;
        }

        os << "["<<pbuf << "]:" << ntohs(m_addr.sin6_port);
        return os;
    }

    IPAddress::ptr IPv6Address::broadcastAddress(uint32_t prefix_len)
    {   
        //TODO:: 直接拷贝的，不理解
        sockaddr_in6 baddr(m_addr);
        baddr.sin6_addr.s6_addr[prefix_len / 8] |=
            CreateMask<uint8_t>(prefix_len % 8);
        for(int i = prefix_len / 8 + 1; i < 16; ++i) {
            baddr.sin6_addr.s6_addr[i] = 0xff;
        }
        return IPv6Address::ptr(new IPv6Address(baddr));
    }

    IPAddress::ptr IPv6Address::networkAddress(uint32_t prefix_len)
    {
        //TODO:: 直接拷贝的，不理解
        sockaddr_in6 baddr(m_addr);
        baddr.sin6_addr.s6_addr[prefix_len / 8] &=
            CreateMask<uint8_t>(prefix_len % 8);
        for(int i = prefix_len / 8 + 1; i < 16; ++i) {
            baddr.sin6_addr.s6_addr[i] = 0x00;
        }
        return IPv6Address::ptr(new IPv6Address(baddr));
    }

    IPAddress::ptr IPv6Address::subnetMask(uint32_t prefix_len)
    {
        //TODO:: 直接拷贝的，不理解
        sockaddr_in6 subnet;
        memset(&subnet, 0, sizeof(subnet));
        subnet.sin6_family = AF_INET6;
        subnet.sin6_addr.s6_addr[prefix_len /8] =
            ~CreateMask<uint8_t>(prefix_len % 8);

        for(uint32_t i = 0; i < prefix_len / 8; ++i) {
            subnet.sin6_addr.s6_addr[i] = 0xff;
        }
        return IPv6Address::ptr(new IPv6Address(subnet));
    }

    uint16_t IPv6Address::getPort() const
    {
        return ntohs(m_addr.sin6_port);
    }

    void IPv6Address::setPort(uint16_t v)
    {
        m_addr.sin6_port = htons(v);
    }

    static const size_t MAX_PATH_LEN = sizeof(((sockaddr_un*)0)->sun_path) -1;
    UnixAddress::UnixAddress()
    {
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sun_family = AF_UNIX;
        m_length = offsetof(sockaddr_un, sun_path) + MAX_PATH_LEN;
    }

    UnixAddress::UnixAddress(const std::string& path)
    {
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sun_family = AF_UNIX;
        m_length = path.size() + 1;

        if(!path.empty() && path[0] == '\0') {
            --m_length;
        }
        if(m_length > sizeof(m_addr.sun_path)) {
            throw std::logic_error("path too long");
        }
        memcpy(&m_addr.sun_path, path.c_str(), path.size());
        m_length += offsetof(sockaddr_un, sun_path);
    }

    const sockaddr* UnixAddress::getAddr() const 
    {
        return (sockaddr*)&m_addr;
    }

    sockaddr* UnixAddress::getAddr() 
    {
        return (sockaddr*)&m_addr;
    }

    socklen_t UnixAddress::getAddrLen() const
    {
        return m_length;
    } 

    std::ostream& UnixAddress::insert(std::ostream& os) const
    {
        return os << m_addr.sun_path;
    }


    UnknownAddress::UnknownAddress(int family)
    {
        memset(&m_addr, 0, sizeof(m_addr));
        m_addr.sa_family = family;
    }

    UnknownAddress::UnknownAddress(const sockaddr& addr)
    {
        m_addr = addr;
    }

    const sockaddr* UnknownAddress::getAddr() const
    {
        return (sockaddr*)&m_addr;
    }

    sockaddr* UnknownAddress::getAddr() 
    {
        return (sockaddr*)&m_addr;
    }

    socklen_t UnknownAddress::getAddrLen() const
    {
        return sizeof(m_addr);
    } 

    std::ostream& UnknownAddress::insert(std::ostream& os) const
    {
        os << "[UnknownAddress family=" << m_addr.sa_family << "]";
        return os;
    }

}