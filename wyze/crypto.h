#ifndef _WYZE_CRYPTO_H_
#define _WYZE_CRYPTO_H_

#include <memory>
#include <string>
#include <openssl/sha.h>
#include <openssl/md5.h>
#include <openssl/rsa.h>
#include <functional>

namespace wyze {

enum class HashType {
        MD5,
        MD128 = MD5,
        SHA1,
        SHA160 = SHA1,
        SHA224,
        SHA256,
        SHA384,
        SHA512
};

class Hash {
public:
    using ptr = std::shared_ptr<Hash>;
    Hash(HashType type);
    ~Hash();
    HashType getHashType() const { return m_type; }

    bool addData(const void* data, size_t len);
    std::string hashData();
    static std::string HashData(HashType type,const unsigned char* data, size_t len);
private:
    HashType m_type;
    size_t m_digest_length;
    void* m_ctx;
    std::function<int(const void*, size_t)> m_updata_cb;
    std::function<int(unsigned char*)> m_final_cb;
};


class Rsa {
public:
    using ptr = std::shared_ptr<Rsa>;

    enum class RsaKeyType{
        PBULIC_KEY,
        PRIVATE_KEY,
        ALL_KEY,
        UNKNOW_KEY,
    };

    Rsa(const std::string& pub_file, const std::string& pri_file);
    Rsa(RsaKeyType type, const std::string& file);
    ~Rsa();
    
    RsaKeyType getKeyType() const { return m_keyType; }
    int getKeyLen() const { return m_keyLen; }

    std::string encrypt(const std::string& data);
    std::string decrypt(const std::string& data);

    std::string sign(HashType type, const std::string& data);
    bool verify(HashType type, const std::string& data, const std::string& sign_data);

    static bool GenerateRsaKey(const std::string& path, int key_bit_size = 1024);
private:
    int getRsaType(HashType type);

private:
    RsaKeyType m_keyType;
    int m_keyLen;
    RSA* m_pubKey;
    RSA* m_priKey;    
};

class AesCbc {
public:
    using ptr = std::shared_ptr<AesCbc>;
    AesCbc(const std::string& key = "1234567887654321", int keybits = 128,
            const std::string& vec = "0123456789abcdef");
    std::string getKey() const { return m_key; }
    std::string getVec() const { return m_vec; }
    int getKeybits() const { return m_keybits; }

    std::string encrypt(const std::string& data);
    std::string decrypt(const std::string& data);

private:
    const std::string m_key;
    const int m_keybits;
    const std::string m_vec;

};


}

#endif