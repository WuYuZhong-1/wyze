#ifndef _WYZE_UTIL_H_
#define _WYZE_UTIL_H_

#include <pthread.h>
#include <stdint.h>
#include <vector>
#include <sstream>


namespace wyze {
    pid_t GetThreadId();
    uint32_t GetFiberId();
    uint64_t GetCurrentMS();
    uint64_t GetCurrentUS();
    void Backtrace(std::vector<std::string>& vec, int size, int skip);
    std::string BacktraceToString(int size = 64, 
            int skip = 2, const std::string& prefix = "    ");

}


#endif // _WYZE_UTIL_H_
