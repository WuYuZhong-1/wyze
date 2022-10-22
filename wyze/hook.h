#ifndef _WYZE_HOOK_H_
#define _WYZE_HOOK_H_

#include <unistd.h>

namespace wyze {
    bool is_hook_enable();              //使能钩子函数
    void set_hook_enable(bool flag);    //设置是否使用钩子函数
}

extern "C" {

//sleep
typedef unsigned int (*sleep_fun)(unsigned int seconds);
using sleep_fun = unsigned int (*)(unsigned int seconds);
// using sleep_fun = unsigned int *(unsigned int seconds);
extern sleep_fun sleep_f;                                   //这里生命是为了给外部使用原始系统调用

typedef int (*usleep_fun)(useconds_t uesc);
using usleep_fun = int (*)(useconds_t usec);
extern usleep_fun usleep_f;



}

#endif //_WYZE_HOOK_H_