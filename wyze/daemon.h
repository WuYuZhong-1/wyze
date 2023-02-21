#ifndef _WYZE_DAEMON_H_
#define _WYZE_DAEMON_H_

#include <unistd.h>
#include <functional>
#include "singleton.h"
#include <string>
#include <iostream>

namespace wyze{

#define THREAD_INFO_ARRAY       30      //数组大小

struct ThreadInfo {
    uint64_t record_time = 0;
    int thread_id = 0;
    char thread_desc[120];
};

struct DetectInfo {
    uint32_t real_size = 0;                 //记录有真实有多少个线程信息
    uint64_t interval = 0;                 //时间间隔
    ThreadInfo infos[THREAD_INFO_ARRAY];
};

struct ProcessInfo {
    pid_t parent_id = 0;
    pid_t main_id = 0;
    uint64_t parent_start_time = 0;
    uint64_t main_start_time = 0;
    uint32_t restart_count = 0;

    std::string toString() const;
};



using PIMgr = Single<ProcessInfo>;

std::ostream& operator<<(std::ostream& os, const ProcessInfo& info);

int start_daemon(int argc, char** argv,
                    std::function<int(int argc, char** argv)> main_cb,
                    bool is_daemon);

int update_thread_time(const std::string& desc = "");
void set_detect_interval(uint64_t interval = 120 );

}

#endif //_WYZE_DAEMON_H_
