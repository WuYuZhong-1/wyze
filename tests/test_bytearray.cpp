#include "../wyze/wyze.h"
#include <stdlib.h>
#include <time.h>
#include <vector>
//把 basesize 改大
//25

static wyze::Logger::ptr g_logger = WYZE_LOG_ROOT();

void test() {

#define XX(type, len, write_fun, read_fun, base_len)    {   \
    std::vector<type> vec;                                  \
    for(int i = 0; i < len; ++i) {                          \
        vec.push_back(rand());                              \
    }                                                       \
    wyze::ByteArray::ptr ba(new wyze::ByteArray(base_len)); \
    for(auto& i : vec) {                                    \
        ba->write_fun(i);                                   \
    }                                                       \
    ba->setPosition(0);                                     \
    for(size_t i = 0; i < vec.size(); ++i) {                \
        type v = ba->read_fun();                            \
        WYZE_ASSERT(v == vec[i]);                           \
    }                                                       \
    WYZE_ASSERT(ba->getReadSize() == 0);                    \
    WYZE_LOG_INFO(g_logger) << #write_fun "/" #read_fun     \
            " (" #type ") len=" << len                      \
            << "base_len=" << base_len                      \
            << " size=" << ba->getSize();                   \
}

    XX(int8_t,  100, writeFint8, readFint8, 1);
    XX(uint8_t, 100, writeFuint8, readFuint8, 1);
    XX(int16_t,  100, writeFint16,  readFint16, 1);
    XX(uint16_t, 100, writeFuint16, readFuint16, 1);
    XX(int32_t,  100, writeFint32,  readFint32, 1);
    XX(uint32_t, 100, writeFuint32, readFuint32, 1);
    XX(int64_t,  100, writeFint64,  readFint64, 1);
    XX(uint64_t, 100, writeFuint64, readFuint64, 1);

    XX(int32_t,  100, writeInt32,  readInt32, 1);
    XX(uint32_t, 100, writeUint32, readUint32, 1);
    XX(int64_t,  100, writeInt64,  readInt64, 1);
    XX(uint64_t, 100, writeUint64, readUint64, 1);

    WYZE_LOG_INFO(g_logger) << "-------------------------------------------------------------";    

    XX(int8_t,  100, writeFint8, readFint8, 10);
    XX(uint8_t, 100, writeFuint8, readFuint8, 10);
    XX(int16_t,  100, writeFint16,  readFint16, 10);
    XX(uint16_t, 100, writeFuint16, readFuint16, 10);
    XX(int32_t,  100, writeFint32,  readFint32, 10);
    XX(uint32_t, 100, writeFuint32, readFuint32, 10);
    XX(int64_t,  100, writeFint64,  readFint64, 10);
    XX(uint64_t, 100, writeFuint64, readFuint64, 10);

    XX(int32_t,  100, writeInt32,  readInt32, 10);
    XX(uint32_t, 100, writeUint32, readUint32, 10);
    XX(int64_t,  100, writeInt64,  readInt64, 10);
    XX(uint64_t, 100, writeUint64, readUint64, 10);
#undef XX

#define XX(type, len, write_fun, read_fun, base_len)    {   \
    std::vector<type> vec;                                  \
    for(int i = 0; i < len; ++i) {                          \
        vec.push_back(rand());                              \
    }                                                       \
    wyze::ByteArray::ptr ba(new wyze::ByteArray(base_len)); \
    for(auto& i : vec) {                                    \
        ba->write_fun(i);                                   \
    }                                                       \
    ba->setPosition(0);                                     \
    for(size_t i = 0; i < vec.size(); ++i) {                \
        type v = ba->read_fun();                            \
        WYZE_ASSERT(v == vec[i]);                           \
    }                                                       \
    WYZE_ASSERT(ba->getReadSize() == 0);                    \
    WYZE_LOG_INFO(g_logger) << #write_fun "/" #read_fun     \
            " (" #type ") len=" << len                      \
            << "base_len=" << base_len                      \
            << " size=" << ba->getSize();                   \
    ba->setPosition(0);                                     \
    WYZE_ASSERT(ba->writeToFile("/tmp/" #type "_" #len "-" #read_fun ".dat"));     \
    wyze::ByteArray::ptr ba2(new wyze::ByteArray(base_len * 2));                  \
    WYZE_ASSERT(ba2->readFromFile("/tmp/" #type "_" #len "-" #read_fun ".dat"));   \
    ba2->setPosition(0);                                                            \
    WYZE_ASSERT(ba->toString() == ba2->toString());                                \
    WYZE_ASSERT(ba->getPosition() == 0);                                           \
    WYZE_ASSERT(ba2->getPosition() == 0);                                          \
}

    WYZE_LOG_INFO(g_logger) << "-------------------------------------------------------------";    

    XX(int8_t,  100, writeFint8, readFint8, 10);
    XX(uint8_t, 100, writeFuint8, readFuint8, 10);
    XX(int16_t,  100, writeFint16,  readFint16, 10);
    XX(uint16_t, 100, writeFuint16, readFuint16, 10);
    XX(int32_t,  100, writeFint32,  readFint32, 10);
    XX(uint32_t, 100, writeFuint32, readFuint32, 10);
    XX(int64_t,  100, writeFint64,  readFint64, 10);
    XX(uint64_t, 100, writeFuint64, readFuint64, 10);

    XX(int32_t,  100, writeInt32,  readInt32, 10);
    XX(uint32_t, 100, writeUint32, readUint32, 10);
    XX(int64_t,  100, writeInt64,  readInt64, 10);
    XX(uint64_t, 100, writeUint64, readUint64, 10);
#undef XX
}

int main(int argc, char** argv)
{
    srand(time(nullptr));
    test();
    return 0;
}