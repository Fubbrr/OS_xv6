#include "kernel/types.h"
#include "user/user.h"
#include "stddef.h"

int main(int argc, char *argv[])
{
    int p1[2], p2[2];
    // 缓冲区
    char buf[8];
    // 0 指管道的读取端，1 指管道的写入端

    // 创建两个管道
    pipe(p1);
    pipe(p2);
    
    // 创建子进程
    if (fork() == 0) {
        // 子进程
        // 从父进程读取字节
        read(p1[0], buf, 4);
        printf("%d: received %s\n", getpid(), buf);
        // 向父进程写数据
        write(p2[1], "pong", strlen("pong"));
    }
    else {
        // 父进程
        // 向子进程写数据
        write(p1[1], "ping", strlen("ping"));
        wait(NULL); // 等待子进程结束
        // 从子进程读取数据
        read(p2[0], buf, 4);
        printf("%d: received %s\n", getpid(), buf);
    }
    exit(0);
}

