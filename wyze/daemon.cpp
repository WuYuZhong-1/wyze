#include "daemon.h"
#include "log.h"
#include "config.h"
#include <sstream>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

namespace wyze {

static Logger::ptr g_logger = WYZE_LOG_NAME("system");

static ConfigVar<uint32_t>::ptr g_daemon_restart_interval
    = Config::Lookup("daemon.restart_interval", (uint32_t)5, "daemon restart interval");

std::string ProcessInfo::toString() const
{
    std::stringstream ss;
    ss << "[ProcessInfo parent_id=" << parent_id
        << " parent_start_time=" << TimeToStr(parent_start_time)
        << " main_id=" << main_id
        << " main_start_time=" << TimeToStr(main_start_time)
        << " restart_count=" << restart_count << "]";
    return ss.str();
}

std::ostream& operator<<(std::ostream& os, const ProcessInfo& info)
{
    return os << info.toString(); 
}

static int real_start(int argc, char** argv, 
            std::function<int(int argc, char** argv)> main_cb)
{
    return main_cb(argc, argv);
}


static int real_daemon(int argc, char** argv, 
        std::function<int(int argc, char** argv)> main_cb)
{
    int rt = daemon(1, 0);
    if(rt != 0) {
        return rt;
    }

    PIMgr::GetInstance()->parent_id = getpid();
    PIMgr::GetInstance()->parent_start_time = time(nullptr);

    while(true) {
        pid_t pid = fork();
        if(pid == 0) {
            //子进程
            PIMgr::GetInstance()->main_id = getpid();
            PIMgr::GetInstance()->main_start_time = time(nullptr);
            WYZE_LOG_INFO(g_logger) << *PIMgr::GetInstance();
            return real_start(argc, argv, main_cb);
        }
        else if( pid < 0) {
            WYZE_LOG_ERROR(g_logger) << "fork fail return=" << pid
                << " errno=" << errno << " errstr=" << strerror(errno); 
        }
        else {
            //父进程
            int status = 0;
            waitpid(pid, &status, 0);
            if(WIFEXITED(status)) {
                //正常退出
                WYZE_LOG_INFO(g_logger) << "main normal exit status=" << WEXITSTATUS(status);
                return WEXITSTATUS(status);
            }
            WYZE_LOG_ERROR(g_logger) << "child crash pid=" << pid << " status=" << status;

            ++PIMgr::GetInstance()->restart_count;
            sleep(g_daemon_restart_interval->getValue());
        }
    }
    return 0;
}

int start_daemon(int argc, char** argv,
                    std::function<int(int argc, char** argv)> main_cb,
                    bool is_daemon)
{
    if(!is_daemon)
        return real_start(argc, argv, main_cb);
    return real_daemon(argc, argv, main_cb);
}



}