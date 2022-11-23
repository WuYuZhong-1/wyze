#ifndef _WYZE_MACRO_H_
#define _WYZE_MACRO_H_

#include <assert.h>
#include "util.h"
#include "log.h"

#if defined __GNUC__ || defined __llvm__
    #define WYZE_LICKLY(x)      __builtin_expect(!!(x), 1)  //告诉编译器，该成立的机率大
    #define WYZE_UNLICKLY(x)    __builtin_expect(!!(x), 0)      
#else
    #define WYZE_LICKLY(x)      (x)
    #define WYZE_UNLICKLY(x)    (x)
#endif


#define WYZE_ASSERT(x)  \
    if(WYZE_UNLICKLY(!(x))) {    \
        WYZE_LOG_ERROR(WYZE_LOG_ROOT()) << "ASSERTION: " #x \
            << "\nbacktrace:\n" \
            << wyze::BacktraceToString(100, 2, "    "); \
        assert(x);  \
    }

#define WYZE_ASSERT2(x, w)  \
    if(WYZE_UNLICKLY(!(x))) {  \
        WYZE_LOG_ERROR(WYZE_LOG_ROOT()) << "ASSERTION: " #x \
            << "\n" << w    \
            << "\nbacktrace:\n" \
            << wyze::BacktraceToString(100, 2, "    "); \
        assert(x);  \
    }



#endif // !_WYZE_MACRO_H_