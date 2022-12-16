#ifndef _WYZE_UTIL_H_
#define _WYZE_UTIL_H_

#include <pthread.h>
#include <stdint.h>
#include <vector>
#include <sstream>
#include <time.h>
#include <iostream>


namespace wyze {
    pid_t GetThreadId();
    uint32_t GetFiberId();
    uint64_t GetCurrentMS();
    uint64_t GetCurrentUS();
    void Backtrace(std::vector<std::string>& vec, int size, int skip);
    std::string BacktraceToString(int size = 64, 
            int skip = 2, const std::string& prefix = "    ");
    std::string ToUpper(const std::string& name);
    std::string ToLower(const std::string& name);
    std::string TimeToStr(time_t ts = time(0), const std::string format = "%Y-%m-%d %H:%M:%S");
    time_t StrToTime(const char* str, const char* format = "%Y-%m-%d %H:%M:%S");

    class FSUtil {
    public:
        static void ListAllFile(std::vector<std::string>& files
                                ,const std::string& path
                                ,const std::string& subfix);
        static bool Mkdir(const std::string& dirname);
        static bool IsRunningPidfile(const std::string& pidfile);
        static bool Rm(const std::string& path);
        static bool Mv(const std::string& from, const std::string& to);
        static bool Realpath(const std::string& path, std::string& rpath);
        static bool Symlink(const std::string& from, const std::string& to);
        static bool Unlink(const std::string& filename, bool exist = false);
        static std::string Dirname(const std::string& filename);
        static std::string Basename(const std::string& filename);
        static bool OpenForRead(std::ifstream& ifs, const std::string& filename
                                ,std::ios_base::openmode mode);
        static bool OpenForWrite(std::ofstream& ofs, const std::string& filename
                                ,std::ios_base::openmode mode);
    };

}


#endif // _WYZE_UTIL_H_
