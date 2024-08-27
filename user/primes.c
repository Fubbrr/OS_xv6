#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    //创建管道，管道的描述符分别是 p1[0] (读端) 和 p1[1] (写端)
    int p1[2];
    pipe(p1);

    //创建一个子进程
    int f1 = fork();

    if (f1 < 0)
    {
        fprintf(2, "xargs: error in fork\n");
        exit(1);
    }

    if (f1 == 0)
    {
        int prime = 0;
        close(p1[1]); // 关闭主管道的写端，因为子进程只读

        while (1)
        {
            // 从管道中读取数值
            if (read(p1[0], &prime, sizeof(prime)) == 0)
            {
                // 如果管道读到 EOF（即父进程已经关闭了写端），关闭读端并退出
                close(p1[0]);
                exit(0);
            }

            // 输出当前读取到的素数
            printf("prime %d\n", prime);

            // 创建一个新的管道用于过滤掉当前素数的倍数
            int p2[2];
            pipe(p2);

            // 创建一个新的子进程
            int f2 = fork();
            if (f2 < 0)
            {
                // 如果 fork 失败，输出错误信息并退出
                fprintf(2, "xargs: error in fork\n");
                exit(1);
            }

            if (f2 > 0)
            {
                // 父进程（新的子进程的父进程）
                int num = 0;
                close(p2[0]); // 关闭子管道的读端，因为父进程只写

                // 从主管道中读取数据，并将不为当前素数倍数的数据写入子管道
                while (read(p1[0], &num, sizeof(num)) != 0)
                {
                    if (num % prime != 0)
                    {
                        write(p2[1], &num, sizeof(num));
                    }
                }

                // 关闭管道
                close(p1[0]);
                close(p2[1]);

                // 等待子进程退出
                wait(0);
                exit(0);
            }

            // 孙进程部分
            close(p1[0]); // 关闭主管道的读端
            close(p2[1]); // 关闭子管道的写端
            p1[0] = p2[0]; // 更新主管道的读端为子管道的读端
        }
    }

    //关闭主管道的读端，因为父进程只写
    close(p1[0]); 

    //将 2 到 35 的整数写入管道
    for (int i = 2; i <= 35; i++)
    {
        write(p1[1], &i, sizeof(i));
    }

    //关闭主管道的写端
    close(p1[1]);

    //等待子进程退出
    wait(0);
    exit(0);
}

