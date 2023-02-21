#include "daemon.h"
// #include "common.h"
#include <sstream>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>

#define QCout std::cout 

namespace wyze {

static DetectInfo* g_detectInfo = nullptr;
static uint64_t detect_thread_time(uint64_t interval);
static void init_shm(int shmid);
static void reset_real_size();

static std::string TimeToStr(time_t ts = time(nullptr), const std::string format = "%Y-%m-%d %H:%M:%S")
{
    struct tm tm;
    localtime_r(&ts, &tm);
    char buf[64] = {0};
    strftime(buf, sizeof(buf), format.c_str(), &tm);
    return buf;
}

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

    int shmid = shmget(IPC_PRIVATE, sizeof(DetectInfo), IPC_CREAT | 0600);
    if(shmid == -1) {
        return shmid;
    }

    init_shm(shmid);

    while(true) {
        pid_t pid = fork();
        if(pid == 0) {
            //子进程
            reset_real_size();

            PIMgr::GetInstance()->main_id = getpid();
            PIMgr::GetInstance()->main_start_time = time(nullptr);
            rt = real_start(argc, argv, main_cb);

            shmdt((void*)g_detectInfo);
            return rt;
        }
        else if( pid < 0) {
            QCout << "fork fail return=" << pid
                << " errno=" << errno << " errstr=" << strerror(errno) << std::endl;
        }
        else {
            //父进程

            int status = 0;
            uint64_t old_time = time(nullptr);
            do {
                rt = waitpid(pid, &status, WNOHANG);

                if(rt == 0) {
                    //没有等待到子进程，判断是否进入检测线程存活
                    if(old_time + g_detectInfo->interval < (uint64_t)time(nullptr)) {
                        //需要检测线程是否存活
                        old_time = detect_thread_time(g_detectInfo->interval);
                        if(old_time == 1) {
                            //需要杀死子进程
                            kill(pid, SIGKILL);
                        }
                    }
                }
                else {
                    break;
                }

                sleep(5);
            }while(rt == 0);

            if(WIFEXITED(status)) {
                //正常退出
                QCout << "main normal exit status=" << WEXITSTATUS(status) << std::endl;
                rt = WEXITSTATUS(status);
                if(rt != 64)
                    break;
            }
            QCout << "child crash pid=" << pid << " status=" << status << std::endl;

            ++PIMgr::GetInstance()->restart_count;
            sleep(5);
        }
    }

    shmdt((void*)g_detectInfo);
    shmctl(shmid, IPC_RMID, NULL);

    return rt;
}

int start_daemon(int argc, char** argv,
                    std::function<int(int argc, char** argv)> main_cb,
                    bool is_daemon)
{
    if(!is_daemon)
        return real_start(argc, argv, main_cb);
    return real_daemon(argc, argv, main_cb);
}

static void init_shm(int shmid)
{
    g_detectInfo = (DetectInfo*)shmat(shmid, nullptr, 0);
    if(g_detectInfo == (void*)-1)
        exit(1);
    g_detectInfo->real_size = 0;
    g_detectInfo->interval = 3 * 60;
}

static void reset_real_size()
{
    g_detectInfo->real_size = 0;
}

int update_thread_time(const std::string& desc)
{
    if(!g_detectInfo)
        return -1;

    int thread_id = syscall(SYS_gettid);

    //检测当前线程是否已经存在数据结构中
    for(uint32_t i = 0; i < g_detectInfo->real_size; ++i) {

        if(g_detectInfo->infos[i].thread_id == thread_id) {     //如果该线程存在，则更新当前线程的存活时间
            g_detectInfo->infos[i].record_time = time(nullptr);
            return 0;
        }
    }

    if(g_detectInfo->real_size >= THREAD_INFO_ARRAY)
        return -1;

    static std::mutex s_mutex;                      //这个锁，每个进程都有一个，主要是在子进程中使用
    std::unique_lock<std::mutex> lock(s_mutex);
    //到这里表示首次更新当前线程时间

    g_detectInfo->infos[g_detectInfo->real_size].thread_id = thread_id;
    strncpy(g_detectInfo->infos[g_detectInfo->real_size].thread_desc,desc.c_str(), 
            sizeof(g_detectInfo->infos[g_detectInfo->real_size].thread_desc) > desc.size() ? 
            desc.size() : sizeof(g_detectInfo->infos[g_detectInfo->real_size].thread_desc));
    g_detectInfo->infos[g_detectInfo->real_size].record_time = time(nullptr);
    ++g_detectInfo->real_size;

   return 0;
}

void set_detect_interval(uint64_t interval)
{
    if(!g_detectInfo)
        return ;

    g_detectInfo->interval = interval * 2;
}

//注意：这里的 interval 时间间隔应该要能大于两次存活时间间隔
static uint64_t detect_thread_time(uint64_t interval)
{
    if(!g_detectInfo)
        return 1;
    
    uint64_t cur_time = time(nullptr);
    uint64_t timestamp =  cur_time - interval;  //最晚的时间必须大过该时间戳

    for(uint32_t i = 0; i < g_detectInfo->real_size; ++i) {
        if(g_detectInfo->infos[i].record_time < timestamp) {    //表示该进程已经死亡，没有按时更新时间节点
            return 1;
        }
    }
    return cur_time;
}

}
