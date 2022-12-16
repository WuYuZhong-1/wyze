#ifndef _WYZE_DAEMON_H_
#define _WYZE_DAEMON_H_

#include <unistd.h>
#include <functional>
#include "singleton.h"
#include <string>
#include <iostream>

namespace wyze{

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

}

#endif //_WYZE_DAEMON_H_