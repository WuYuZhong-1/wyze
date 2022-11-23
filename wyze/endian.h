#ifndef _WYZE_ENDIAN_H_
#define _WYZE_ENDIAN_H_



#include <byteswap.h>
#include <stdint.h>
#include <type_traits>

namespace wyze {

#define WYZE_LITTLE_ENDIAN 1
#define WYZE_BIG_ENDIAN 2

    template<class T>
    typename std::enable_if<sizeof(T) == sizeof(uint64_t), T>::type
    byteswap(T value) 
    {
        return (T)bswap_64((uint64_t)value);
    }

    template<class T>
    typename std::enable_if<sizeof(T) == sizeof(uint32_t), T>::type
    byteswap(T value)
    {
        return (T)bswap_32((uint32_t)value);
    }

    template<class T>
    typename std::enable_if<sizeof(T) == sizeof(uint16_t), T>::type
    byteswap( T value)
    {
        return (T)bswap_16((uint16_t)value);
    }

#if BYTE_ORDER == BIG_ENDIAN
    #define WYZE_BYTE_ORDER  WYZE_BIG_ENDIAN
#else
    #define WYZE_BYTE_ORDER WYZE_LITTLE_ENDIAN
#endif

#if WYZE_BYTE_ORDER == WYZE_BIG_ENDIAN
    template<class T>
    T byteswapOnLittleEndian(T t) {
        return t;
    }

    template<class T>
    T byteswapOnBigEndian(T t) {
        return byteswap(t);
    }
#else
    template<class T>
    T byteswapOnLittleEndian(T t) {
        return byteswap(t);
    }

    template<class T>
    T byteswapOnBigEndian(T t) {
        return t;
    }
#endif

}


#endif