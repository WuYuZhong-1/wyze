#ifndef _WYZE_MACRO_H_
#define _WYZE_MACRO_H_

#include <assert.h>
#include "util.h"
#include "log.h"

#define WYZE_ASSERT(x)  \
    if(!(x)) {    \
        WYZE_LOG_ERROR(WYZE_LOG_ROOT()) << "ASSERTION: " #x \
            << "\nbacktrace:\n" \
            << wyze::BacktraceToString(100, 2, "    "); \
        assert(x);  \
    }

#define WYZE_ASSERT2(x, w)  \
    if(!(x)) {  \
        WYZE_LOG_ERROR(WYZE_LOG_ROOT()) << "ASSERTION: " #x \
            << "\n" << w    \
            << "\nbacktrace:\n" \
            << wyze::BacktraceToString(100, 2, "    "); \
        assert(x);  \
    }



#endif // !_WYZE_MACRO_H_