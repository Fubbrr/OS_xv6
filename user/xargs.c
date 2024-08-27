#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

int main(int argc, char *argv[]) 
{
    // 检查是否至少提供了一个命令参数
    if (argc < 2) 
    {
        fprintf(2, "xargs: too few inputs\n");
        exit(1);
    }

    // 存储命令和其参数的数组
    int num_args = 0;
    char *cmd_args[MAXARG];

    // 将命令行参数（除了第一个）存储到 cmd_args 数组中
    for (int i = 1; i < argc; ++i) 
    {
        cmd_args[num_args++] = argv[i];
    }

    int base_count = num_args; // 记录初始参数数量

    char input_char; // 用于读取字符
    char *current_param; // 当前参数的指针
    char param_buffer[512]; // 缓冲区
    current_param = param_buffer; // 初始化 current_param 指针
    int buffer_index = 0; // 当前字符在缓冲区中的位置

    // 从标准输入读取字符直到文件结束
    while (read(0, &input_char, 1) > 0)
    {
        if (input_char == '\n') 
        {
            // 遇到换行符，结束当前参数
            current_param[buffer_index] = '\0';
            buffer_index = 0;

            // 将当前参数添加到 cmd_args 数组中
            cmd_args[num_args++] = current_param;
            cmd_args[num_args] = 0; // 末尾添加 NULL 作为 exec 的参数结束标记

            // 创建子进程并执行命令
            if (fork()) 
            {
                // 父进程等待子进程完成
                wait(0);
            } 
            else 
            {
                // 子进程执行指定的命令
                exec(argv[1], cmd_args);
                // 如果 exec 返回，说明失败，退出子进程
                exit(1);
            }

            // 恢复 cmd_args 数组为原始状态
            num_args = base_count;
        } 
        else if (input_char == ' ') 
        {
            // 遇到空格，结束当前参数
            current_param[buffer_index] = '\0';
            buffer_index = 0;
            cmd_args[num_args++] = current_param;

            // 准备处理下一个参数
            char new_param_buffer[512];
            current_param = new_param_buffer;
        } 
        else 
        {
            // 其他字符，添加到当前参数中
            current_param[buffer_index++] = input_char;
        }
    }

    // 程序正常退出
    exit(0);
}

