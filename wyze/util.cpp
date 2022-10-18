#include "util.h"
#include <sys/syscall.h>
#include <unistd.h>
#include <sys/time.h>
#include <execinfo.h>
#include <stdlib.h>
#include <cxxabi.h>
#include "log.h"
#include "fiber.h"


namespace wyze {

Logger::ptr  g_logger = WYZE_LOG_NAME("system");

pid_t GetThreadId() 
{
    return syscall(SYS_gettid);
    // return pthread_self();  poxsi 对线程的编号，不是linux 系统的编号
}

uint32_t GetFiberId()
{
    return Fiber::GetFiberId();
}

uint64_t GetCurrentMS()
{
    struct timeval tv = {0,0};
    int rt = gettimeofday(&tv, NULL);
    if(rt != 0) 
        return 0;
    return tv.tv_sec * 1000ul + tv.tv_usec / 1000;
}


uint64_t GetCurrentUS()
{
    struct timeval tv = {0, 0};
    int rt = gettimeofday(&tv, NULL);
    if(rt != 0)
        return 0;
    return tv.tv_sec * 1000ul * 1000ul + tv.tv_usec;
}


static std::string demangle(const char* str) 
{
    size_t size = 0;
    int status = 0;
    std::string rt;
    rt.resize(256);
    if(1 == sscanf(str, "%*[^(]%*[^_]%255[^)+]", &rt[0])) {
        // std::cout << rt << std::endl;
        char* v = abi::__cxa_demangle(&rt[0], nullptr, &size, &status);
        if(v) {
            std::string result(v);
            free(v);
            return result;
        }
    }
    if(1 == sscanf(str, "%255s", &rt[0])) {
        return rt;
    }
    return str;
}

void Backtrace(std::vector<std::string>& vec, int size, int skip)
{
    void** array = (void**) malloc(sizeof(void*) * size);
    char** strings = nullptr;
    size_t s = ::backtrace(array, size);

    do {
        strings = backtrace_symbols(array, s);
        if(strings == nullptr) {
            WYZE_LOG_ERROR(g_logger) << "backtrace_symbols error";
            break;
        }

        for(size_t i = skip; i < s; ++i) {
            vec.push_back(demangle(strings[i]));
        }

    }while(0);

    if(array) 
        free(array);

    if(strings) 
        free(strings);
    
}

std::string BacktraceToString(int size, 
        int skip, const std::string& prefix)
{
    std::vector<std::string> vec;
    std::stringstream ss;

    Backtrace(vec, size, skip);

    for(size_t i = 0; i < vec.size(); ++i) {
        ss << prefix << vec[i] << std::endl;
    }

    return ss.str();
}


}