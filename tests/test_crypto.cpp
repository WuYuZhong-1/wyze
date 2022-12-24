#include "../wyze/crypto.h"
#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fstream>
#include <string.h>

static std::string str1 = "hello";
static std::string str2 = ", world";

void hash()
{
    wyze::Hash hash(wyze::HashType::MD5);
    hash.addData(str1.c_str(), str1.size());
    hash.addData(str2.c_str(), str2.size());
    std::cout << "MD5: " << hash.hashData() << std::endl;

    wyze::Hash hash2(wyze::HashType::SHA512);
    hash2.addData(str1.c_str(), str1.size());
    hash2.addData(str2.c_str(), str2.size());
    std::cout << "SHA512: " << hash2.hashData() << std::endl;
    
}

void rsa()
{
    wyze::Rsa::GenerateRsaKey("/home/wuyz/learn/wyze/bin/pem");
    wyze::Rsa rsa("/home/wuyz/learn/wyze/bin/pem/public.pem",
                    "/home/wuyz/learn/wyze/bin/pem/private.pem");

    std::string text = "hello world";
    std::cout << rsa.decrypt(rsa.encrypt(text)) << std::endl;
    std::cout << rsa.verify(wyze::HashType::SHA224,text, 
            rsa.sign(wyze::HashType::SHA224, text)) << std::endl;
}

void aesCbc()
{
    struct stat st;
    if(stat("/home/wuyz/learn/wyze_c-c-/openssl.txt", &st) != 0) {
        std::cout << "stat:" << strerror(errno) << std::endl;
        return;
    }
    std::cout << "text size：" << st.st_size << std::endl;
    std::string text;
    text.resize(st.st_size + 1);

    std::ifstream ifs("/home/wuyz/learn/wyze_c-c-/openssl.txt");
    ifs.read((char*)text.c_str(), text.size());
    if(ifs.gcount() != st.st_size) {
        std::cout << "read text error" << std::endl;
        return;
    }

    wyze::AesCbc aes;
    std::string data = aes.decrypt(aes.encrypt(text));
    std::cout << "data size：" << data.size() << std::endl;
    std::cout << data << std::endl;
}

int main(int argc, char** argv)
{
    // hash();
    // rsa();
    aesCbc();
    return 0;
}