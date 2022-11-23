#ifndef _WYZ_ADDRESS_H_
#define _WYZ_ADDRESS_H_

#include "endian.h"
#include <memory>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <vector>
#include <map>


    //9.10
namespace wyze {

    class IPAddress;

    class Address {
    public:
        using ptr = std::shared_ptr<Address>;

        static Address::ptr Create(const sockaddr* addr, socklen_t addrlen);
        static bool Lookup(std::vector<Address::ptr>& result, const std::string& host,
                            int family = AF_INET, int type = SOCK_STREAM, int protocol = 0);
        static Address::ptr LookupAny(const std::string& host,
                            int family = AF_INET, int type = 0, int protocol = 0);
        static std::shared_ptr<IPAddress> LookAnyIPAddress(const std::string& host,
                            int family = AF_INET, int type = 0, int protocol = 0);

        static bool GetInterfaceAddresses(std::multimap<std::string, std::pair<Address::ptr, uint32_t>>& result,
                                            int family = AF_INET);
        static bool GetInterfaceAddresses(std::vector<std::pair<Address::ptr, uint32_t>>& result,
                                            const std::string& iface, int family = AF_INET);
        
        virtual ~Address() { }

        int getFamily() const;
        std::string toString();

        virtual const sockaddr* getAddr() const = 0;
        virtual sockaddr* getAddr() = 0;
        virtual socklen_t getAddrLen() const = 0;  
        virtual std::ostream& insert(std::ostream& os) const = 0;  

        bool operator<(const Address& rhs) const;
        bool operator==(const Address& rhs) const;
        bool operator!=(const Address& rhs) const;
    };

    class IPAddress : public Address{
    public:
        using ptr = std::shared_ptr<IPAddress>;

        static IPAddress::ptr Create(const char* addr, uint16_t port = 0);

        virtual IPAddress::ptr broadcastAddress(uint32_t prefix_len) = 0;
        virtual IPAddress::ptr networkAddress(uint32_t prefix_len) = 0;
        virtual IPAddress::ptr subnetMask(uint32_t prefix_len) = 0;

        virtual uint16_t getPort() const = 0;
        virtual void setPort(uint16_t v) = 0;
    };

    class IPv4Address : public IPAddress {
    public:
        using ptr = std::shared_ptr<IPv4Address>;

        static IPv4Address::ptr Create(const char* addr, uint16_t port = 0);

        IPv4Address(const sockaddr_in& addr);
        IPv4Address(uint32_t addr = INADDR_ANY, uint16_t port = 0);

        const sockaddr* getAddr() const override;
        sockaddr* getAddr() override;
        socklen_t getAddrLen() const override; 
        std::ostream& insert(std::ostream& os) const override;

        IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
        IPAddress::ptr networkAddress(uint32_t prefix_len) override;
        IPAddress::ptr subnetMask(uint32_t prefix_len) override;

        uint16_t getPort() const override;
        void setPort(uint16_t v) override;
    
    private:
        sockaddr_in m_addr;
    };

    class IPv6Address : public IPAddress {
    public:
        using ptr = std::shared_ptr<IPv6Address>;

        static IPv6Address::ptr Create(const char* addr, uint16_t port = 0);

        IPv6Address();
        IPv6Address(const sockaddr_in6& addr);
        IPv6Address(const uint8_t addr[16], uint16_t port = 0);

        const sockaddr* getAddr() const override;
        sockaddr* getAddr() override;
        socklen_t getAddrLen() const override; 
        std::ostream& insert(std::ostream& os) const override;

        IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
        IPAddress::ptr networkAddress(uint32_t prefix_len) override;
        IPAddress::ptr subnetMask(uint32_t prefix_len) override;

        uint16_t getPort() const override;
        void setPort(uint16_t v) override;

    private:
        sockaddr_in6 m_addr;
    };

    class UnixAddress : public Address {
    public:
        using ptr = std::shared_ptr<UnixAddress>;

        UnixAddress();
        UnixAddress(const std::string& path);

        const sockaddr* getAddr() const override;
        sockaddr* getAddr() override;
        socklen_t getAddrLen() const override; 
        void setAddrlen(uint32_t v) { m_length = v; }
        std::ostream& insert(std::ostream& os) const override;

    private:
        sockaddr_un m_addr;
        socklen_t m_length;
    };

    class UnknownAddress: public Address {
    public:
        using ptr = std::shared_ptr<UnknownAddress>;

        UnknownAddress(int family);
        UnknownAddress(const sockaddr& addr);

        const sockaddr* getAddr() const override;
        sockaddr* getAddr() override;
        socklen_t getAddrLen() const override; 
        std::ostream& insert(std::ostream& os) const override;

    private:
        sockaddr m_addr;
    };

}


#endif