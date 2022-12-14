#include "util.h"
#include <sys/syscall.h>
#include <unistd.h>
#include <sys/time.h>
#include <execinfo.h>
#include <stdlib.h>
#include <cxxabi.h>
#include <ctype.h>
#include <time.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <algorithm>
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

std::string ToUpper(const std::string& name)
{
    std::string rt = name;
    std::transform(rt.begin(), rt.end(), rt.begin(), ::toupper);
    return rt;
}

std::string ToLower(const std::string& name)
{
    std::string rt = name;
    std::transform(rt.begin(), rt.end(), rt.begin(), ::tolower);
    return rt;
}


std::string TimeToStr(time_t ts, const std::string format)
{
    struct tm tm;
    localtime_r(&ts, &tm);
    char buf[64] = {0};
    strftime(buf, sizeof(buf), format.c_str(), &tm);
    return buf;
}

time_t StrToTime(const char* str, const char* format)
{
    struct tm t;
    memset(&t, 0, sizeof(t));
    if(!strptime(str, format, &t))
        return 0;
    return mktime(&t);
}

void FSUtil::ListAllFile(std::vector<std::string>& files
                                ,const std::string& path
                                ,const std::string& subfix)
{
    if(access(path.c_str(), F_OK) != 0)
        return ;

    DIR* dir = opendir(path.c_str());
    if(dir == nullptr)
        return;

    struct dirent* res = nullptr;
    while( (res = readdir(dir))) {
        if(res->d_type == DT_DIR) {
            if(!strcmp(res->d_name, ".")
                || !strcmp(res->d_name, "..") )
                continue;
            ListAllFile(files, path + "/" + res->d_name, subfix);
        }
        else if(res->d_type == DT_REG) {
            std::string filename(res->d_name);
            if(subfix.empty()) {
                files.push_back(path + "/" + filename);
            }
            else {
                if(filename.size() < subfix.size())
                    continue;
                if(filename.substr(filename.length()-subfix.size()) == subfix)
                    files.push_back(path + "/" + filename);
            }
        }
    }

    closedir(dir);
}

static int _lstat(const char* file, struct stat* st = nullptr)
{
    struct stat lst;
    int rt = lstat(file, &lst);
    if(st)
        *st = lst;
    return rt;
}

static int _mkdir(const char* dirname)
{
    if(access(dirname, F_OK) == 0)
        return 0;
    return mkdir(dirname, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}


//在创建过程中出错之前的文件夹是创建成功的
bool FSUtil::Mkdir(const std::string& dirname)
{
    if(_lstat(dirname.c_str()) == 0)
        return true;

    char* path = strdup(dirname.c_str());
    char* ptr = strchr(path + 1, '/');

    do{
        for(;ptr; *ptr ='/', ptr = strchr(ptr+1, '/')) {
            *ptr = '\0';
            if(_mkdir(path) != 0)
                break;
        }

        if(ptr != nullptr)
            break;
        else if(_mkdir(path) != 0) 
            break;
        free(path);
        return true;
    }while(false);

    free(path);
    return false;
}

bool FSUtil::IsRunningPidfile(const std::string& pidfile)
{
    if(_lstat(pidfile.c_str()) != 0)
        return false;

    std::ifstream ifs(pidfile);
    std::string line;
    if(!ifs || !std::getline(ifs, line))
        return false;
    
    if(line.empty())
        return false;

    pid_t pid = atoi(line.c_str());
    if(pid <= 1) 
        return false;

    if(kill(pid, 0) != 0)
        return false;

    return true;
}

bool FSUtil::Rm(const std::string& path)
{
    struct stat st;
    if(lstat(path.c_str(), &st))
        return true;
    
    if(!(st.st_mode & S_IFDIR))
        return Unlink(path, true);

    DIR* dir = opendir(path.c_str());
    if(!dir)
        return false;

    bool ret = true;
    struct dirent* res = nullptr;
    while( (res = readdir(dir))) {
        if(!strcmp(res->d_name, ".")
                || !strcmp(res->d_name, ".."))
            continue;
        std::string dirname = path + "/" + res->d_name;
        ret = Rm(dirname);
    }
    closedir(dir);
    if(::rmdir(path.c_str()))
        ret = false;

    return ret;
}

bool FSUtil::Mv(const std::string& from, const std::string& to)
{
    if(!Rm(to))
        return false;
    return rename(from.c_str(), to.c_str()) == 0;
}

bool FSUtil::Realpath(const std::string& path, std::string& rpath)
{
    if(_lstat(path.c_str()))
        return false;

    char* ptr = ::realpath(path.c_str(), nullptr);
    if(nullptr == ptr)
        return false;

    std::string(ptr).swap(rpath);
    free(ptr);
    return true;
}

bool FSUtil::Symlink(const std::string& from, const std::string& to)
{
    if(!Rm(to))
        return false;
    return ::symlink(from.c_str(), to.c_str());
}

bool FSUtil::Unlink(const std::string& filename, bool exist)
{
    if(!exist && _lstat(filename.c_str()))
        return true;
    return ::unlink(filename.c_str()) == 0;
}

std::string FSUtil::Dirname(const std::string& filename)
{
    if(filename.empty())
        return ".";

    auto pos = filename.rfind('/');
    if(pos == 0)
        return "/";
    else if(pos == std::string::npos)
        return ".";
    else
        return filename.substr(0, pos);
}

std::string FSUtil::Basename(const std::string& filename)
{
    if(filename.empty())
        return filename;

    auto pos = filename.rfind('/');
    if(pos == std::string::npos)
        return filename;
    return filename.substr(pos + 1);
}

bool FSUtil::OpenForRead(std::ifstream& ifs, const std::string& filename
                        ,std::ios_base::openmode mode)
{
    ifs.open(filename.c_str(), mode);
    return ifs.is_open();
}

bool FSUtil::OpenForWrite(std::ofstream& ofs, const std::string& filename
                        ,std::ios_base::openmode mode)
{
    ofs.open(filename.c_str(), mode);
    if(!ofs.is_open()) {
        std::string dir = Dirname(filename);
        if(!Mkdir(dir)) {
            WYZE_LOG_ERROR(g_logger) << "mkdir " << dir << " error, errno=" << errno 
                    << " errstr=" << strerror(errno);
            return false; 
        }
        ofs.open(filename.c_str(), mode);
    }
    return ofs.is_open();
}

}