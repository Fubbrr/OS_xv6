#include "kernel/param.h"  // 包含内核参数定义
#include "kernel/types.h"   // 包含通用类型定义
#include "kernel/sysinfo.h" // 包含 sysinfo 结构体定义
#include "user/user.h"      // 包含用户空间函数和系统调用定义

int
main(int argc, char *argv[])
{
    // 参数错误处理
    if (argc != 1)
    {
        fprintf(2, "Usage: %s need not param\n", argv[0]);
        exit(1); // 如果传递了参数，打印错误信息并退出
    }

    struct sysinfo info; // 定义 sysinfo 结构体变量，用于存储系统信息

    // 调用 sysinfo 系统调用，获取系统信息
    sysinfo(&info);

    // 打印系统信息中的空闲内存和活跃进程数量
    printf("free space: %d\nused process: %d\n", info.freemem, info.nproc);

    exit(0); // 正常退出
}

