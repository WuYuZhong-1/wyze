#include "bytearray.h"
#include "log.h"
#include "endian.h"
#include "macro.h"

#include <string.h>
#include <fstream>
#include <sstream>
#include <string>
#include <iomanip>
#include <errno.h>

namespace wyze {

static Logger::ptr g_logger = WYZE_LOG_NAME("system");

ByteArray::Node::Node(size_t s)
    : ptr(new char[s])
    , size(s)
    , next(nullptr)
{
}

ByteArray::Node::Node()
    : ptr(nullptr)
    , size(0)
    , next(nullptr)
{
}

ByteArray::Node::~Node()
{
    if(ptr)
        delete[] ptr;
    ptr = nullptr;
    next = nullptr;
    size = 0;
}



ByteArray::ByteArray(size_t base_size)
    : m_basesize(base_size)
    , m_position(0)
    , m_capacity(base_size)
    , m_size(0)
    , m_endian(WYZE_BIG_ENDIAN)
    , m_root(new Node(base_size))
    , m_cur(m_root)
{
}

ByteArray::~ByteArray()
{
    Node* tmp = m_root;
    while(tmp) {
        m_cur = tmp;
        tmp = m_cur->next;
        delete m_cur;
    }
}

void ByteArray::writeFint8(int8_t value)
{
    write(&value, sizeof(value));
}

void ByteArray::writeFuint8(uint8_t value)
{
    write(&value, sizeof(value));
}

void ByteArray::writeFint16(int16_t value)
{
    if(m_endian != WYZE_BYTE_ORDER)
        value = byteswap(value);
    write(&value, sizeof(value));
}

void ByteArray::writeFuint16(uint16_t value)
{
    if(m_endian != WYZE_BYTE_ORDER)
        value = byteswap(value);
    write(&value, sizeof(value));
}

void ByteArray::writeFint32(int32_t value)
{
    if(m_endian != WYZE_BYTE_ORDER)
        value = byteswap(value);
    write(&value, sizeof(value));
}
void ByteArray::writeFuint32(uint32_t value)
{
    if(m_endian != WYZE_BYTE_ORDER)
        value = byteswap(value);
    write(&value, sizeof(value));
}

void ByteArray::writeFint64(int64_t value)
{
    if(m_endian != WYZE_BYTE_ORDER)
        value = byteswap(value);
    write(&value, sizeof(value));
}

void ByteArray::writeFuint64(uint64_t value)
{
    if(m_endian != WYZE_BYTE_ORDER)
        value = byteswap(value);
    write(&value, sizeof(value));
}

static uint32_t EncodeZigzag32(int32_t v) 
{
    //这里的负数太大，把负数转为单数，整数转为 双数
    if(v < 0) 
        return ((uint32_t)(-v)) * 2 - 1;
    else 
        return v * 2;
}

static uint64_t EncodeZigzag64(int64_t v)
{
    if(v < 0) 
        return ((uint64_t)(-v)) * 2 - 1;
    else 
        return v * 2;
}

static int32_t DecodeZigzag32(uint32_t v) 
{
    return (v >> 1) ^ -(v & 1);
}

static int64_t DecodeZigzag64(uint64_t v)
{
    return (v >> 1) ^ -(v & 1);
}

void ByteArray::writeInt32(int32_t value)
{
    writeUint32(EncodeZigzag32(value));
}

void ByteArray::writeUint32(uint32_t value)
{
    uint8_t tmp[5] = {0};
    uint8_t i = 0;
    while(value >= 0x80) {
        tmp[i++] = (value & 0x7F) | 0x80;
        value >>= 7;
    }
    tmp[i++] = value;
    write(tmp, i);
}

void ByteArray::writeInt64(int64_t value)
{
    writeUint64(EncodeZigzag64(value));
}

void ByteArray::writeUint64(uint64_t value)
{
    uint8_t tmp[10]= {0};
    uint8_t i = 0;
    while(value >= 0x80) {
        tmp[i++] = (value & 0x7F) | 0x80;
        value >>= 7;
    }
    tmp[i++] = value;
    write(tmp, i);
}

void ByteArray::writeFloat(float value)
{
    uint32_t v;
    memcpy(&v, &value, sizeof(value));
    writeFuint32(v);
}

void ByteArray::writeDouble(double value)
{
    uint64_t v;
    memcpy(&v, &value, sizeof(value));
    writeFuint64(v);
}

//length:int16, data
void ByteArray::writeStringF16(const std::string& value)
{
    writeFuint16(value.size());
    write(value.c_str(), value.size());
}

//length:int32, data
void ByteArray::writeStringF32(const std::string& value)
{
    writeFuint32(value.size());
    write(value.c_str(), value.size());
}

//length:int64, data 
void ByteArray::writeStringF64(const std::string& value)
{
    writeFuint64(value.size());
    write(value.c_str(), value.size());
}

//length:varint, data
void ByteArray::writeStringVint(const std::string& value)
{
    writeUint64(value.size());
    write(value.c_str(), value.size());
}

//data
void ByteArray::writeStringWithoutLength(const std::string value)
{
    write(value.c_str(), value.size());
}

//read
int8_t ByteArray::readFint8()
{
    int8_t v;
    read(&v, sizeof(v));
    return v;
}

uint8_t ByteArray::readFuint8()
{
    uint8_t v;
    read(&v, sizeof(v));
    return v;
}

#define XX(type)                        \
    type v;                             \
    read(&v, sizeof(v));                \
    if(m_endian != WYZE_BYTE_ORDER)     \
        v = byteswap(v);                \
    return v;

int16_t ByteArray::readFint16()
{
    XX(int16_t);
}

uint16_t ByteArray::readFuint16()
{
    XX(uint16_t);
}

int32_t ByteArray::readFint32()
{
    XX(int32_t);
}

uint32_t ByteArray::readFuint32()
{
    XX(uint32_t);
}

int64_t ByteArray::readFint64()
{
    XX(int64_t);
}

uint64_t ByteArray::readFuint64()
{
    XX(uint64_t);
}

#undef XX

int32_t ByteArray::readInt32()
{
    return DecodeZigzag32(readUint32());
}

uint32_t ByteArray::readUint32()
{
    uint32_t result = 0;
    for(int i = 0; i < 32; i += 7) {
        uint8_t b = readFuint8();
        if(b < 0x80) {
            result |= ((uint32_t)b) << i;
            break;
        }
        else {
            result |= (((uint32_t)(b & 0x7F)) << i);
        }
    }
    return result;
}

int64_t ByteArray::readInt64()
{
    return DecodeZigzag64(readUint64());
}

uint64_t ByteArray::readUint64()
{
    uint64_t result = 0;
    for(int i = 0; i < 64; i += 7) {
        uint8_t b = readFuint8();
        if(b < 0x80) {
            result |= ((uint32_t)b) << i;
            break;
        }
        else {
            result |= (((uint32_t)(b & 0x7F)) << i);
        }
    }
    return result;
}

float ByteArray::readFloat()
{
    uint32_t v = readFuint32();
    float value;
    memcpy(&value, &v, sizeof(v));
    return value;
}

double ByteArray::readDouble()
{
    uint64_t v = readFuint64();
    double value;
    memcpy(&value, &v, sizeof(v));
    return value;
}

//length: int16, data
std::string ByteArray::readStringF16()
{
    uint16_t len = readFuint16();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}

//length: int32, data
std::string ByteArray::readStringF32()
{
    uint32_t len = readFuint32();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}

//length: int64, data
std::string ByteArray::readStringF64()
{
    uint64_t len = readFuint64();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}

//length: varint, data
std::string ByteArray::readStringVint()
{
    uint64_t len = readUint64();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}

//内部操作
void ByteArray::clear()
{
    m_position = m_size = 0;
    m_capacity = m_basesize;
    Node* tmp = m_root->next;
    while(tmp) {
        m_cur = tmp;
        tmp = tmp->next;
        delete m_cur;
    }
    m_cur = m_root;
    m_root->next = nullptr;
}

void ByteArray::write(const void* buf, size_t size)
{
    if(size == 0)
        return ;
    
    addCapacity(size);
    // m_position 表示当前读写的位置，因为 m_basesize 是一块内存的大小， 该对象可能有多个 内存快，
    size_t npos = m_position % m_basesize;          //得到某个内存块的具体位置
    size_t ncap = m_cur->size - npos;               //得到 当前内存块还可以存多少内存
    size_t bpos  = 0;                               //buf 别写入的位置

    while(size > 0) {
        if(ncap >= size) {  //当前内存块可以写完
            memcpy(m_cur->ptr + npos, (const char*)buf + bpos, size);
            if(m_cur->size == (npos + size))
                m_cur = m_cur->next;            //当前内存快写满了

            m_position += size;                 //保存当前位置
            size = 0;                           //退出条件
        }
        else {
            //先把当前内存块的剩余写完
            memcpy(m_cur->ptr + npos, (const char*)buf + bpos, ncap);
            m_position += ncap;                 //保存当前的位置
            bpos += ncap;                       //buf 写入的位置
            size -=  ncap;                      // buf 还有多少没有写入
            m_cur = m_cur->next;                //指向下一块内存
            ncap = m_cur->size;                 //下一块内存有多少可以写
            npos = 0;                           //下一块内存的可写位置
        }
    }

    if( m_position > m_size)
        m_size = m_position;        //保存写入数据的大小

}

void ByteArray::read(void* buf, size_t size)
{
    if(size > getReadSize()) {
        throw std::out_of_range("not enough len");
    }

    size_t npos = m_position % m_basesize;      //当前某个内存块读数据的位置
    size_t ncap = m_cur->size - npos;           //当前某个内存块剩余可读数据
    size_t bpos = 0;                            //读入 buf 的位置

    while(size > 0) {
        if(ncap >= size) {
            memcpy((char*)buf + bpos, m_cur->ptr + npos, size);
            if(m_cur->size == (npos + size))
                m_cur = m_cur->next;
            m_position += size;         //操作的位置
            size = 0;
        }
        else {
            memcpy((char*)buf + bpos, m_cur->ptr + npos, ncap);
            m_position += ncap;
            bpos += ncap;
            size -= ncap;
            m_cur = m_cur->next;
            ncap = m_cur->size;
            npos = 0;
        }
    }
}

void ByteArray::read(void* buf, size_t size, size_t position) const
{
    if(size > (m_size - position)) {
        throw std::out_of_range("not enough len");
    }

    Node* cur = m_cur;
    size_t npos = position % m_basesize;      //当前某个内存块读数据的位置
    size_t ncap = cur->size - npos;           //当前某个内存块剩余可读数据
    size_t bpos = 0;                            //读入 buf 的位置

    while(size > 0) {
        if(ncap >= size) {
            memcpy((char*)buf + bpos, cur->ptr + npos, size);
            if(cur->size == (npos + size))
                cur = cur->next;
            size = 0;
        }
        else {
            memcpy((char*)buf + bpos, cur->ptr + npos, ncap);
            bpos += ncap;
            size -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
    }
}


void ByteArray::setPosition(size_t v)
{
    if(v > m_capacity) 
        throw std::out_of_range("setPosition out of range");
    
    m_position = v;
    if(m_position > m_size) 
        m_size = m_position;
    
    m_cur = m_root;
    while( v > m_cur->size) {      
        v -= m_cur->size;
        m_cur = m_cur->next;
    }

    if(v == m_cur->size) 
        m_cur = m_cur->next;
}

bool ByteArray::writeToFile(const std::string& name) const
{
    std::ofstream ofs;
    ofs.open(name, std::ios::trunc | std::ios::binary);
    if(!ofs) {
        WYZE_LOG_ERROR(g_logger) << "writeToFile name=" << name 
            << " error,  errno=" << errno << " errstr=" << strerror(errno);
        return false;
    }

    int64_t read_size = getReadSize();
    std::string buf;
    buf.resize(read_size);
    read(&buf[0], read_size, m_position);
    ofs.write(&buf[0], read_size);

    return true;
}

bool ByteArray::readFromFile(const std::string& name)
{
    std::ifstream ifs;
    ifs.open(name, std::ios::binary);
    if(!ifs) {
        WYZE_LOG_ERROR(g_logger) << "readFromFile name" << name
            << " error, errno=" << errno << " errstr=" << strerror(errno);
        return false;
    }

    std::shared_ptr<char> buf(new char[m_basesize], [](char* ptr){ delete[] ptr; });
    while(!ifs.eof()) {
        ifs.read(buf.get(), m_basesize);
        write(buf.get(), ifs.gcount());
    }

    return true;
}

bool ByteArray::isLittleEndian() const
{
    return m_endian == WYZE_LITTLE_ENDIAN;
}

void ByteArray::setIsLittleEndian(bool val)
{
    if(val) 
        m_endian = WYZE_LITTLE_ENDIAN;
    else 
        m_endian = WYZE_BIG_ENDIAN;
}

std::string ByteArray::toString() const
{
    std::string buf;
    buf.resize(getReadSize());
    if(buf.empty())
        return buf;

    read(&buf[0], buf.size(), m_position);
    return buf;
}

std::string ByteArray::toHexString() const
{
    std::string buf = toString();
    std::stringstream ss;

    for(size_t i = 0; i < buf.size(); ++i) {
        if(i > 0 && (i % 32 == 0)) 
            ss << std::endl;
        ss << std::setw(2) << std::setfill('0') << std::hex 
            << (int)(uint8_t)buf[i] << " ";
    }

    return ss.str();
}

//只获取内容，不修改position
uint64_t ByteArray::getReadBuffers(std::vector<iovec>& buffers, uint64_t len) const
{
    if(len == 0)
        return 0;

    len = len > getReadSize() ? getReadSize() : len;
    uint64_t size = len;

    size_t npos = m_position % m_basesize;
    size_t ncap = m_cur->size - npos;
    struct iovec iov;
    Node* cur = m_cur;      //不改变当前位置

    while( len > 0) {
        if(ncap >= len) {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        }
        else {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = ncap;
            len -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
        buffers.push_back(iov);
    }
    return size;
}

uint64_t ByteArray::getReadBuffers(std::vector<iovec>& buffers, uint64_t len, uint64_t position) const
{
    if(len == 0 || position > m_size)
        return 0;

    len = (len > (m_size - position)) ? (m_size - position) : len;
    uint64_t size = len;

    size_t npos = position % m_basesize;
    size_t count = position / m_basesize;
    Node* cur = m_root;
    while(count > 0) {
        cur = cur->next;
        --count;
    }   

    size_t ncap = m_cur->size - npos;
    struct iovec iov;
    while( len > 0) {
        if(ncap >= len) {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        }
        else {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = ncap;
            len -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
        buffers.push_back(iov);
    }
    return size;
}

//增加容量，不修改position
uint64_t ByteArray::getWriteBuffers(std::vector<iovec>& buffers, uint64_t len)
{
    if(len == 0) 
        return 0;
    
    addCapacity(len);
    uint64_t size = len;

    size_t npos = m_position % m_basesize;
    size_t ncap = m_cur->size - npos;
    struct iovec iov;
    Node* cur = m_cur;
    while(len > 0) {
        if(ncap >= len) {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        }
        else {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = ncap;

            len -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
        buffers.push_back(iov);
    }
    return size;
}

void ByteArray::addCapacity(size_t size)
{
    if(size == 0) 
        return ;
    
    size_t old_cap = getCapacity();
    if(old_cap > size)
        return ;
    
    size = size - old_cap;
    size_t count = (size / m_basesize) + ((size % m_basesize) ? 1: 0);
    Node* tmp = m_root;

    while(tmp->next)
        tmp = tmp->next;

    Node* first = nullptr;
    for(size_t i =0 ; i < count; ++i) {
        tmp->next = new Node(m_basesize);
        if(first == nullptr) 
            first = tmp->next;
        tmp = tmp->next;
        m_capacity += m_basesize;
    }

    if(old_cap == 0) 
        m_cur = first;
}

}