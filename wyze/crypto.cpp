#include "crypto.h"
#include "openssl/pem.h"
#include "openssl/aes.h"

namespace wyze {

Hash::Hash(HashType type)
    : m_type(type)
    , m_digest_length(0)
    , m_ctx(nullptr)
{
    switch(m_type) {
        case HashType::MD5: {
            m_digest_length = MD5_DIGEST_LENGTH;
            m_ctx = malloc(sizeof(MD5_CTX));
            MD5_Init((MD5_CTX*)m_ctx);
            m_updata_cb = std::bind(&MD5_Update, (MD5_CTX*)m_ctx
                            ,std::placeholders::_1, std::placeholders::_2);
            m_final_cb = std::bind(&MD5_Final, std::placeholders::_1,(MD5_CTX*)m_ctx);
            break;
        }
        case HashType::SHA1: {
            m_digest_length = SHA_DIGEST_LENGTH;
            m_ctx = malloc(sizeof(SHA_CTX));
            SHA1_Init((SHA_CTX*)m_ctx);
            m_updata_cb = std::bind(&SHA1_Update, (SHA_CTX*)m_ctx
                            ,std::placeholders::_1, std::placeholders::_2);
            m_final_cb = std::bind(&SHA1_Final, std::placeholders::_1, (SHA_CTX*)m_ctx);
            break;
        }
        case HashType::SHA224: {
            m_digest_length = SHA224_DIGEST_LENGTH;
            m_ctx = malloc(sizeof(SHA256_CTX));
            SHA224_Init((SHA256_CTX*)m_ctx);
            m_updata_cb = std::bind(&SHA224_Update, (SHA256_CTX*)m_ctx
                            ,std::placeholders::_1, std::placeholders::_2);
            m_final_cb = std::bind(&SHA224_Final, std::placeholders::_1, (SHA256_CTX*)m_ctx);
            break;
        }
        case HashType::SHA256: {
            m_digest_length = SHA256_DIGEST_LENGTH;
            m_ctx = malloc(sizeof(SHA256_CTX));
            SHA256_Init((SHA256_CTX*)m_ctx);
            m_updata_cb = std::bind(&SHA256_Update, (SHA256_CTX*)m_ctx
                            ,std::placeholders::_1, std::placeholders::_2);
            m_final_cb = std::bind(&SHA256_Final, std::placeholders::_1, (SHA256_CTX*)m_ctx);
            break;
        }
        case HashType::SHA384: {
            m_digest_length = SHA384_DIGEST_LENGTH;
            m_ctx = malloc(sizeof(SHA512_CTX));
            SHA384_Init((SHA512_CTX*)m_ctx);
            m_updata_cb = std::bind(&SHA384_Update, (SHA512_CTX*)m_ctx
                            ,std::placeholders::_1, std::placeholders::_2);
            m_final_cb = std::bind(&SHA384_Final, std::placeholders::_1, (SHA512_CTX*)m_ctx);
            break;
        }
        case HashType::SHA512: {
            m_digest_length = SHA512_DIGEST_LENGTH;
            m_ctx = malloc(sizeof(SHA512_CTX));
            SHA512_Init((SHA512_CTX*)m_ctx);
            m_updata_cb = std::bind(&SHA512_Update, (SHA512_CTX*)m_ctx
                            ,std::placeholders::_1, std::placeholders::_2);
            m_final_cb = std::bind(&SHA512_Final, std::placeholders::_1, (SHA512_CTX*)m_ctx);
            break;
        }
    }
}

Hash::~Hash()
{
    if(m_ctx) 
        free(m_ctx);
}


bool Hash::addData(const void* data, size_t len)
{
    return m_updata_cb(data, len) == 1;
}

std::string Hash::hashData()
{
    std::shared_ptr<unsigned char> md(new unsigned char[m_digest_length]);
    std::shared_ptr<char> hex(new char[m_digest_length * 2 + 1]);
    m_final_cb(md.get());
    for(size_t i = 0; i < m_digest_length; ++i) 
        sprintf(&(hex.get()[i * 2]), "%02X", md.get()[i]);
    return hex.get();
}

std::string Hash::HashData(HashType type,const unsigned char* data, size_t len)
{
    Hash hash(type);
    hash.addData(data, len);
    return hash.hashData();
}


Rsa::Rsa(const std::string& pub_file, const std::string& pri_file)
    : m_keyType(RsaKeyType::UNKNOW_KEY)
    , m_keyLen(0)
    , m_pubKey(nullptr)
    , m_priKey(nullptr)
{
    FILE* pub_fp = fopen(pub_file.c_str(), "r");
    FILE* pri_fp = fopen(pri_file.c_str(), "r");

    if(pub_fp != nullptr) {
        m_pubKey = RSA_new();
        PEM_read_RSAPublicKey(pub_fp, &m_pubKey, nullptr, nullptr);
    }

    if(pri_fp != nullptr) {
        m_priKey = RSA_new();
        PEM_read_RSAPrivateKey(pri_fp, &m_priKey, nullptr, nullptr);
    }

    if(pub_fp != nullptr && pri_fp != nullptr) {
        m_keyType = RsaKeyType::ALL_KEY;
        m_keyLen = RSA_size(m_pubKey);
        fclose(pub_fp);
        fclose(pri_fp);
    }
    else if(pub_fp != nullptr) {
        m_keyType = RsaKeyType::PBULIC_KEY;
        m_keyLen = RSA_size(m_pubKey);
        fclose(pub_fp);
    }
    else if(pri_fp != nullptr) {
        m_keyType = RsaKeyType::PRIVATE_KEY;
        m_keyLen = RSA_size(m_priKey);
        fclose(pri_fp);
    }
}

Rsa::Rsa(RsaKeyType type, const std::string& file)
    : m_keyType(type)
    , m_keyLen(0)
    , m_pubKey(nullptr)
    , m_priKey(nullptr)
{
    FILE* fp = fopen(file.c_str(), "r");
    if(fp == nullptr) {
        m_keyType = RsaKeyType::UNKNOW_KEY;
        return;
    }

    if(RsaKeyType::PBULIC_KEY == type){
        m_pubKey = RSA_new();
        PEM_read_RSAPublicKey(fp, &m_pubKey, nullptr, nullptr);
        m_keyLen = RSA_size(m_pubKey);
    }
    else if(RsaKeyType::PRIVATE_KEY == type) {
        m_priKey = RSA_new();
        PEM_read_RSAPrivateKey(fp, &m_priKey, nullptr, nullptr);
        m_keyLen = RSA_size(m_priKey);
    }

    fclose(fp);
}

Rsa::~Rsa()
{
    if(m_pubKey)
        RSA_free(m_pubKey);
    if(m_priKey)
        RSA_free(m_priKey);
}

std::string Rsa::encrypt(const std::string& data)
{
    if(m_pubKey == nullptr ||
        data.empty() || data.size() > (size_t)(m_keyLen - 11))
        return {};

    std::shared_ptr<unsigned char> enbuf(new unsigned char[m_keyLen]);
    int len = RSA_public_encrypt(data.size(), (const unsigned char*)data.c_str(),
                    enbuf.get(), m_pubKey, RSA_PKCS1_PADDING);
    
    return len == m_keyLen ? std::string((char*)enbuf.get(), len) : std::string();
}

std::string Rsa::decrypt(const std::string& data)
{
    if(m_priKey == nullptr || 
        data.empty() || data.size() > (size_t)m_keyLen)
        return {};
    
    std::shared_ptr<unsigned char> debuf(new unsigned char[m_keyLen]);
    int len = RSA_private_decrypt(data.size(), (const unsigned char*)data.c_str(),
                    debuf.get(), m_priKey, RSA_PKCS1_PADDING);
    return std::string((char*)debuf.get(), len);
}

int Rsa::getRsaType(HashType type)
{
    switch(type) {
        case HashType::MD5:
            return NID_md5;  break;
        case HashType::SHA1:
            return NID_sha1; break;
        case HashType::SHA224:
            return NID_sha224; break;
        case HashType::SHA256:
            return NID_sha256; break;
        case HashType::SHA384:
            return NID_sha384; break;
        case HashType::SHA512:
            return NID_sha512; break;
    }
    return 0;
}

std::string Rsa::sign(HashType type, const std::string& data)
{
    if(m_priKey == nullptr ||
        data.empty() || data.size() > (size_t)(m_keyLen - 11) )
        return {};
    
    std::shared_ptr<unsigned char> signData(new unsigned char[m_keyLen]);

    unsigned int signLen;
    RSA_sign(getRsaType(type), (const unsigned char*)data.c_str(), data.size()
                ,signData.get(), &signLen, m_priKey);

    return (int)signLen == m_keyLen ? std::string((char*)signData.get(), signLen) : std::string();
}

bool Rsa::verify(HashType type, const std::string& data, const std::string& sign_data)
{
    if(m_pubKey == nullptr ||
        data.empty() || data.size() > (size_t)(m_keyLen - 11) ||
        sign_data.empty() )
        return false;

    return RSA_verify(getRsaType(type), (const unsigned char*)data.c_str(), data.size()
                        , (const unsigned char*)sign_data.c_str(), sign_data.size(), m_pubKey);
}

bool Rsa::GenerateRsaKey(const std::string& path, int key_bit_size)
{
    RSA* rsa = RSA_new();
    BIGNUM* bn = BN_new();

    BN_set_word(bn, 12345);
    RSA_generate_key_ex(rsa, key_bit_size, bn, nullptr);

    bool ret = true;
    const std::string pubfile(path + "/public.pem");
    const std::string prifile(path + "/private.pem");
    FILE* pub_fp = nullptr;
    FILE* pri_fp = nullptr;

    do {
        pub_fp = fopen(pubfile.c_str(), "w");
        if(pub_fp == nullptr)  {
            ret = false;
            break;
        }

        pri_fp = fopen(prifile.c_str(), "w");
        if(pri_fp == nullptr)  {
            ret = false;
            break;
        }

        PEM_write_RSAPublicKey(pub_fp, rsa);
        PEM_write_RSAPrivateKey(pri_fp, rsa, nullptr, nullptr, 0, nullptr, nullptr);

    }while(0);

    if(pub_fp)
        fclose(pub_fp);
    if(pri_fp)
        fclose(pri_fp);
    BN_free(bn);
    RSA_free(rsa);

    return ret;
}


AesCbc::AesCbc(const std::string& key, int keybits,
                const std::string& vec)
    : m_key(key)
    , m_keybits(keybits)
    , m_vec(vec)
{
}

std::string AesCbc::encrypt(const std::string& data)
{
    AES_KEY key;
    AES_set_encrypt_key((const unsigned char*)m_key.c_str(), m_keybits, &key);
    size_t length = data.size() + 1;
    if(length % AES_BLOCK_SIZE != 0) {
        length = ((length / AES_BLOCK_SIZE) + 1) * AES_BLOCK_SIZE;
    }

    std::string encbuf;
    encbuf.resize(length);
    std::string vec = m_vec;

    AES_cbc_encrypt((const unsigned char*)data.c_str(), (unsigned char*)encbuf.c_str()
                        , length, &key, (unsigned char*)vec.c_str(), AES_ENCRYPT);
    return encbuf;
}

std::string AesCbc::decrypt(const std::string& data)
{
    if(data.size() % AES_BLOCK_SIZE != 0)
        return {};

    AES_KEY key;
    AES_set_decrypt_key((const unsigned char*)m_key.c_str(), m_keybits, &key);
    
    std::shared_ptr<unsigned char> decbuf(new unsigned char[data.size()]);
    std::string vec = m_vec;

    AES_cbc_encrypt((const unsigned char*)data.c_str(), decbuf.get(), data.size()
                        , &key, (unsigned char*)vec.c_str(), AES_DECRYPT);
    return std::string((char*) decbuf.get());
}

}