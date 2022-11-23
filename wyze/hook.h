#ifndef _WYZE_HOOK_H_
#define _WYZE_HOOK_H_

#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdint.h>

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

typedef int (*nanosleep_fun)(const struct timespec *reg, struct timespec *rem);
using nanosleep_fun = int (*)(const struct timespec *reg, struct timespec *rem);
extern nanosleep_fun nanosleep_f;

//socket
typedef int (*socket_fun)(int domain, int type, int protocol);
using socket_fun = int (*)(int domain, int type, int protocol);
extern socket_fun socket_f;

typedef int (*connect_fun)(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
using connect_fun = int (*)(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
extern connect_fun connect_f;

typedef int (*accept_fun)(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
using accept_fun = int (*)(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
extern accept_fun accept_f;

typedef int (*close_fun)(int fd);
using close_fun = int (*)(int fd);
extern close_fun close_f;

//read
typedef ssize_t (*read_fun)(int fd, void *buf, size_t count);
using read_fun = ssize_t (*)(int fd, void *buf, size_t count);
extern read_fun read_f;

typedef ssize_t (*readv_fun)(int fd, const struct iovec *iov, int iovcnt);
using readv_fun = ssize_t (*)(int fd, const struct iovec *iov, int iovcnt);
extern readv_fun readv_f;

typedef ssize_t (*recv_fun)(int sockfd, void *buf, size_t len, int flags);
using recv_fun = ssize_t (*)(int sockfd, void *buf, size_t len, int flags);
extern recv_fun recv_f;

typedef ssize_t (*recvfrom_fun)(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
using recvfrom_fun = ssize_t (*)(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
extern recvfrom_fun recvfrom_f;

typedef ssize_t (*recvmsg_fun)(int sockfd, struct msghdr *msg, int flags);
using recvmsg_fun = ssize_t (*)(int sockfd, struct msghdr *msg, int flags);
extern recvmsg_fun recvmsg_f;

// write
typedef ssize_t (*write_fun)(int fd, const void *buf, size_t count);
using write_fun = ssize_t (*)(int fd, const void *buf, size_t count);
extern write_fun write_f;

typedef ssize_t (*writev_fun)(int fd, const struct iovec *iov, int iovcnt);
using writev_fun = ssize_t (*)(int fd, const struct iovec *iov, int iovcnt);
extern writev_fun writev_f;

typedef ssize_t (*send_fun)(int sockfd, const void *buf, size_t len, int flags);
using send_fun = ssize_t (*)(int sockfd, const void *buf, size_t len, int flags);
extern send_fun send_f;

typedef ssize_t (*sendto_fun)(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
using sendto_fun = ssize_t (*)(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
extern sendto_fun sendto_f;

typedef ssize_t (*sendmsg_fun)(int sockfd, const struct msghdr *msg, int flags);
using sendmsg_fun = ssize_t (*)(int sockfd, const struct msghdr *msg, int flags);
extern sendmsg_fun sendmsg_f;

// other
typedef int (*fcntl_fun)(int fd, int cmd, ... /*arg*/);
using fcntl_fun = int (*)(int fd, int cmd, ... /*arg*/);
extern fcntl_fun fcntl_f;

typedef int (*ioctl_fun)(int fd, unsigned long request, ...);
using ioctl_fun = int (*)(int fd, unsigned long request, ...);
extern ioctl_fun ioctl_f;

typedef int (*getsockopt_fun)(int sockfd, int level, int optname, void *optval, socklen_t *optlen);
using getsockopt_fun = int (*)(int sockfd, int level, int optname, void *optval, socklen_t *optlen);
extern getsockopt_fun getsockopt_f;

typedef int (*setsockopt_fun)(int sockfd, int level, int optname, const void *optval, socklen_t optlen);
using setsockopt_fun = int (*)(int sockfd, int level, int optname, const void *optval, socklen_t optlen);
extern setsockopt_fun setsockopt_f;

extern int connect_with_tiemout(int fd, const struct sockaddr* addr, socklen_t addrlen, uint64_t timeout_ms);
}

#endif //_WYZE_HOOK_H_