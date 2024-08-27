# OS_xv6 实验报告

*2252426 付柏瑞*



## Tools&Guidance

**安装[适用于 Linux 的 Windows 子系统](https://docs.microsoft.com/en-us/windows/wsl/install-win10)**

- 启用虚拟化命令并将WSL的默认版本设置为WSL2

以管理员的方式运行Powershell并在命令行中输入以下内容：

```
dism.exe /online /enable-feature /featurename:Microsoft-Windows-Subsystem-Linux /all /norestart

wsl --set-default-version 2
```

![image-20240827022010957](./assets/image-20240827022010957.png)

- 在 Windows 中，您可以访问*“\\wsl$\”*目录下的所有 WSL 文件。例如，Ubuntu 20.04 安装的主目录应 位于 *“\\wsl$\Ubuntu-20.04\home\<username>\”。*
- 从 [Microsoft Store 添加 Ubuntu 20.04](https://www.microsoft.com/en-us/p/ubuntu/9nblggh4msv6)，设置用户名和密码

![image-20240827020617118](./assets/image-20240827020617118.png)

- 安装此类所需的所有软件

```
sudo apt-get update && sudo apt-get upgrade

sudo apt-get install git build-essential gdb-multiarch qemu-system-misc gcc-riscv64-linux-gnu binutils-riscv64-linux-gnu
```

- 测试安装

  ```
  qemu-system-riscv64 --version
  
  riscv64-linux-gnu-gcc --version
  ```

![image-20240827022337692](./assets/image-20240827022337692.png)

一些指针常见习语特别值得记住：

- 如果 ， 则 和 是不同的数字：第一个是 but 第二个是 。 当向指针添加整数时，如第二种情况， 整数隐式乘以对象的大小 指针指向。`int *p = (int*)100``(int)p + 1``(int)(p + 1)``101``104`
- `p[i]`定义为与 相同 ， 引用 p 指向的内存中的第 i 个对象。 上述添加规则有助于此定义起作用 当对象大于 1 字节时。`*(p+i)`
- `&p[i]`与 相同，生成 p 指向的内存中第 i 个对象的地址。`(p+i)`

尽管大多数 C 程序从来不需要在指针和整数之间进行强制转换， 操作系统经常这样做。 每当看到涉及内存地址的添加时， 问问自己是整数加法还是指针加法 并确保所添加的值适当地乘以 或者没有。

- 您可能会发现 print 语句可能会产生大量输出 您想要搜索的;一种方法是在`脚本`中运行 `make qemu`（在您的机器上运行），它会将所有控制台输出记录到 文件，然后您可以搜索该文件。别忘了 退出`脚本`。man script

- 在许多情况下，print 语句就足够了，但是 有时能够单步执行某些汇编代码或 检查堆栈上的变量很有帮助。要将 gdb 与 xv6，在一个窗口中运行 make， 在另一个 run （or ） 中 窗口，设置一个中断点，然后是 'c' （continue）， xv6 将运行，直到它命中 断点。（请参阅[使用 GNU 调试器](https://pdos.csail.mit.edu/6.828/2019/lec/gdb_slides.pdf)以获取有用的 GDB 提示。make qemu-gdbgdbriscv64-linux-gnu-gdb

- 如果您想查看编译器的程序集是什么 生成内核或找出指令所在的位置 一个特定的内核地址，请参阅 `kernel.asm` 文件，其中 Makefile 在编译内核时生成。（Makefile 还为所有用户程序生成 `.asm`。

- 如果内核 panic，它将打印一条错误消息，列出 崩溃时 program counter 的值;您可以 搜索 `kernel.asm` 以找出程序在哪个函数中 counter 是崩溃时，或者你可以运行 （run 了解详细信息）。如果要获取回溯，请使用 gdb 重新启动：运行 在一个窗口中运行 gdb（或 riscv64-linux-gnu-gdb）在 另一个窗口，在 panic 中设置断点 （'B panic'），然后是 后跟 'c' （continue）。当内核达到断点时， 键入 'bt' 以获取回溯。addr2line -e kernel/kernel *pc-value*man addr2line

- 如果您的内核挂起（例如，由于死锁）或无法执行 进一步（例如，由于执行内核时的页面错误 指令），您可以使用 gdb 来找出它的悬挂位置。跑 在一个窗口中运行 'make qemu-gdb'，运行 gdb （riscv64-linux-gnu-gdb） 在另一个窗口中，后跟 'c' （continue）。当 内核似乎挂起，在 qemu-gdb 窗口中按 Ctrl-C 键，然后键入 'bt' 来获取回溯。

- `QEMU` 有一个 “monitor” ，可以让你查询 state 模拟机器。您可以通过键入 control-a c （“c” 代表控制台）。一个特别有用的 monitor 命令为 `info mem` 来打印页表。 您可能需要使用 `cpu` 命令来选择哪个 `核心信息 mem` 查看，或者您可以启动 QEMU 使用 `make CPUS=1 qemu` 来使只有一个内核。

  

## Lab1: Xv6 and Unix utilities

本实验将使您熟悉 xv6 及其系统调用。

### Boot xv6 ([easy](https://pdos.csail.mit.edu/6.828/2021/labs/guidance.html))

获取实验室的 xv6 源代码并签出 `util` 分支：

```
git clone git://g.csail.mit.edu/xv6-labs-2021
cd xv6-labs-2021
git checkout util
```

![image-20240827023518010](./assets/image-20240827023518010.png)xv6-labs-

2021 存储库与本书的 xv6-riscv;它主要添加一些文件。如果你好奇，请看 在 git log 中：

```
$ git log
```

构建并运行xv6：

```
make qemu
```

![image-20240827023716316](./assets/image-20240827023716316.png)

如果在提示符处键入 `ls`，您应该会看到类似的输出 更改为以下内容：

![image-20240827023750384](./assets/image-20240827023750384.png)

这些文件是 `mkfs` 包含在 初始文件系统;大多数是您可以运行的程序。您刚刚运行了其中一个：`ls`。

xv6 没有 `ps` 命令，但是，如果您键入， 内核将打印有关每个进程的信息。 如果你现在尝试，你会看到两行：一行用于 `init`， 一个用于 `sh`。Ctrl-p

![image-20240827023856019](./assets/image-20240827023856019.png)

要退出 qemu，请键入：Ctrl-a x.

### sleep ([easy](https://pdos.csail.mit.edu/6.828/2021/labs/guidance.html))

#### 实验目的

- 为 xv6 实施 UNIX 程序 `sleep`

- 你的`睡眠`应该暂停 对于用户指定的刻度数。刻度是时间的概念 由 XV6 内核定义，即两次中断之间的时间 从计时器芯片。您的解决方案应该在文件 `user/sleep.c` 中。

#### 实验步骤

1. 进入user文件夹，创建sleep.c文件

   ``````
   nano sleep.c
   ``````

2. 输入文件内容

   - 包括头文件的函数使用，argc 是命令行总的参数个数 ，argv[] 是 argc 个参数，其中第 0 个参数是程序的全名，以后的参数是命令行后面跟的用户输入的参数

   - 该程序接受一个命令行参数（表示 ticks 数量），并使当前进程暂停相应的 ticks 数量。

     - 如果用户没有提供参数或者提供了多个参数，程序应该打印出错误信息。
     - 命令行参数作为字符串传递，使用 atoi（参见 user/ulib.c）将其转换为整数。
     - 使用系统调用 sleep，最后确保 main 调用 exit() 以退出程序。

     ``````c
     #include "kernel/types.h"
     #include "user/user.h"
     
     
     int main(int argc, char *argv[])
     {
     	// 如果命令行参数不等于2个，则打印错误信息
         if (argc != 2)
         {
         	//2 表示标准错误
             write(2, "Incorrect number of characters\n", strlen("Incorrect number of characters\n"));
             exit(1);
         }
     	// 把字符串型参数转换为整型
         int time = atoi(argv[1]);
         sleep(time);
         exit(0);
     }
     ``````
     

![image-20240827030841616](./assets/image-20240827030841616.png)

- 退出nano

  **ctrl+o      enter     ctrl+x**

4. 在XV6的`Makefile`中添加你的程序，以确保它在编译时被编译和链接。

   打开XV6根目录下的`Makefile`文件。找到类似于`UPROGS`的部分（用户程序列表），并添加你的程序名称（去掉扩展名）：

   ``````
   nano Makefile
   ``````

![image-20240827031522484](./assets/image-20240827031522484.png)

5. 测试结果

   程序在以下情况下暂停，解决方案是正确的 

   ``````
   make qemu
   sleep 20
   ``````

   如上所示运行。 运行以查看是否确实将 睡眠测试。

   ![image-20240827032733457](./assets/image-20240827032733457.png)

   

   ```
   ./grade-lab-util sleep 
   make GRADEFLAGS=sleep grade
   ```


![image-20240827033251648](./assets/image-20240827033251648.png)

#### 实验中遇到的问题和解决方法

- **程序不会运行****

  **解决方法：**

  - 确保程序正确集成到XV6的编译系统中。通过修改`Makefile`和重新编译XV6，确保新程序被正确编译和链接。在XV6环境中运行测试命令，验证程序是否按预期工作。

- **没有找到文件编辑器和不太清楚Linux使用方法**

  **解决方法：**

  - 网上寻找资料和同学讨论，结合gpt，最后没有使用vim编辑，选择了nano编辑器，因为nano提供了一个容易上手的界面，更适合初学者。

  - **学习基本Linux命令**：掌握一些基本的Linux命令可以显著提高你的开发效率。例如：

    - `ls`：列出目录内容。
    - `cd`：改变当前目录。
    - `cp`：复制文件或目录。
    - `mv`：移动或重命名文件或目录。
    - `rm`：删除文件或目录。
    - `mkdir`：创建目录。

    可以使用`man`命令查看每个命令的详细使用说明，例如`man ls`。

  - **练习文件编辑和编译**：熟悉如何使用文本编辑器（如`vim`、`nano`或`emacs`）来编辑代码文件，并使用编译器编译代码。你可以通过在线教程或使用书籍来学习这些基本操作。

  - **设置开发环境**：如果你觉得在Linux中工作困难，考虑使用图形化界面的IDE或编辑器，比如VS Code，它支持终端集成和远程开发，可以让你更轻松地管理代码和编译流程。

#### 实验心得

通过这次实验，我深入了解了以下几个方面：

- **系统调用的使用**：通过实现`sleep`功能，我学习了如何在用户程序中使用系统调用，使程序能够暂停执行。这增强了我对系统调用机制的理解。
- **命令行参数处理**：我掌握了如何处理命令行参数，包括验证参数数量和处理参数值。这对于开发健壮的命令行工具至关重要。
- **编译和集成**：我学会了如何将用户程序集成到XV6操作系统中，并通过修改`Makefile`和重新编译系统来测试程序。这提高了我的开发和调试技能。

整体而言，这次实验不仅让我熟悉了XV6操作系统的开发流程，还提升了我在Linux环境下编写和测试程序的能力。通过解决实际问题，我学会了如何编写更加健壮和可靠的代码，并对操作系统的内部机制有了更深刻的理解。

### pingpong ([easy](https://pdos.csail.mit.edu/6.828/2021/labs/guidance.html))

#### 实验目的

- 编写一个使用 UNIX 系统调用 ''ping-pong'' 的程序 一对管道上的两个进程之间的字节，每个管道一个 方向。

- 父级应向子级发送一个字节; 孩子应打印“<PID>： received ping”， 其中 <pid> 是其进程 ID， 将管道上的字节写入父级， 并退出; 

- 父级应该从子级读取字节， 打印 “<PID>： received pong”， 并退出。 

- 你 解决方案应该在文件 `user/pingpong.c` 中。

#### 实验步骤

1. 进入user文件夹，创建pingpong.c 的文件，代码如下：

   - 主函数没有使用 `argc` 和 `argv` 参数，但它们可以用于扩展功能或传递参数。

   - `p1` 和 `p2` 是两个整型数组，每个数组有两个元素，用于存储管道的文件描述符（读取端和写入端）。
   - `buf` 是一个字符数组，用于存储从管道读取的数据。

   - `fork()` 调用创建一个新的子进程。返回值为0表示这是子进程，父进程会接收到子进程的PID。

   - 子进程从管道 `p1` 的读取端读取4个字节的数据（即 `"ping"`）。打印接收到的数据。向管道 `p2` 的写入端写入 `"pong"`。

   - 父进程向管道 `p1` 的写入端写入 `"ping"`，使用 `wait(NULL)` 等待子进程完成，从管道 `p2` 的读取端读取4个字节的数据（即 `"pong"`）。打印接收到的数据。

   ``````c
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
   
   ``````

2. 更新 Makefile

   在 Makefile 中找到 UPROGS 变量的定义，并添加 pingpong

3. 实验结果

   ![image-20240827040239597](./assets/image-20240827040239597.png)

   ![image-20240827040400652](./assets/image-20240827040400652.png)

#### 实验中遇到的问题和解决方法

1. **管道读写不匹配**：

- **问题**：如果管道的读写操作不匹配（如读取多于写入或读取少于写入），可能导致数据丢失或读取错误。

- **解决方法**

  确保写入的数据量与读取的数据量一致。例如，如果写入了4个字节的数据，读取时也要读取4个字节。并且要根据实际需求调整缓冲区大小。示例：

  ```c
  // 确保读取数据量和写入数据量一致
  read(p1[0], buf, 4);
  write(p2[1], "pong", 4);
  ```

2. **父进程在子进程完成前读取数据**：

- **问题**：如果父进程在子进程写入数据之前尝试读取数据，可能会读取不到数据或出现读取错误。

- **解决方法**

  确保父进程在子进程写入数据之后再读取数据。使用 

  ```c
  wait(NULL)
  ```

   确保子进程完成。示例：

  ```c
  // 父进程
  write(p1[1], "ping", 4);
  wait(NULL); // 等待子进程完成
  read(p2[0], buf, 4);
  ```

#### 实验心得

1. **进程间通信的基础**：
   - 实验加深了对进程间通信机制的理解。管道是一个简单而强大的工具，可以让两个进程之间进行数据交换。掌握如何使用管道进行通信是编写多进程程序的基础。
2. **错误处理的重要性**：
   - 通过实验，意识到在系统编程中，处理错误是至关重要的。系统调用可能会失败，因此在使用 `fork()`, `pipe()`, `read()`, `write()` 等函数时，必须检查返回值并适当地处理错误。
3. **进程同步**：
   - 理解了父子进程之间如何通过管道同步工作。父进程和子进程的行为是交替的，确保正确的同步机制（如使用 `wait()` 等待子进程）可以确保数据的正确传递和处理。
4. **资源管理**：
   - 认识到在创建和使用管道时，必须妥善管理系统资源。例如，及时关闭不再使用的管道端口是良好的编程习惯，可以避免资源泄漏和不必要的系统负担。
5. **调试技巧**：
   - 在调试过程中，逐步检查程序的每个部分，并使用 `printf` 或 `perror` 输出调试信息，有助于迅速定位问题。理解每一步的执行顺序和数据流向是解决问题的关键。
6. **实践中的挑战**：
   - 在实际应用中，可能遇到各种与环境相关的问题，如不同操作系统的管道实现细节、不同版本的库函数行为等。通过实际操作和调试这些细节，可以更好地掌握系统编程的技能。

总的来说，这次实验是对进程间通信的一个深入理解，通过实践加深了对相关系统调用和编程模式的掌握。

### primes ([moderate](https://pdos.csail.mit.edu/6.828/2021/labs/guidance.html))/([hard](https://pdos.csail.mit.edu/6.828/2021/labs/guidance.html))

#### 实验目的

- 使用 pipes 编写 prime sieve 的并发版本。
- 使用 `pipe` 和 `fork` 进行设置 管道。第一个进程提供数字 2 到 35 进入管道。
- 对于每个素数，您将安排 创建一个通过管道从其左邻居读取数据的进程 并通过另一个管道写入其右侧邻居。由于 xv6 具有 文件描述符和进程的数量有限，第一个 进程可以在 35 时停止。
- 解决方案应位于文件 `user/primes.c` 中。

#### 实验步骤

1.进入user文件夹，创建primes.c 的文件，代码如下：

``````c
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

``````

![image-20240827042159485](./assets/image-20240827042159485.png)

- **主进程**:

  - 创建初始管道，并将 2 到 35 的整数写入管道。

  - 关闭写端，等待子进程完成。

- **第一个子进程**:

  - 从主管道中读取数据，识别并打印素数。

  - 创建新的管道和子进程，筛选掉当前素数的倍数。

- **新子进程**:
  - 从主管道中读取数据，筛选数据，将筛选后的数据写入新的子管道。

- **管道和进程**:
  - 使用管道在进程之间传递数据，通过递归创建子进程来实现素数筛选。

- **关闭管道端口**: 
  - 确保每个进程在适当的时机关闭不再使用的管道端口，防止资源泄漏或管道死锁。

- **管道端口更新**: 
  - 确保在新的子进程中正确更新管道的读端，以便继续处理数据流。

2. 更新 Makefile

   在 Makefile 中找到 UPROGS 变量的定义，并添加 find

3. 实验结果

![image-20240827184844179](./assets/image-20240827184844179.png)

![image-20240827184807360](./assets/image-20240827184807360.png)

#### 实验中遇到的问题和解决方法

- **管道描述符错误使用**

​	**问题**: 在代码中，写管道数据时错误地使用了错误的管道描述符。：

```c
write(p2[2], &num, sizeof(num));
```

​	此处 `p2[2]` 是错误的，正确的描述符应为 `p2[1]`，即管道的写端。

​	**解决方法**: 确保在操作管道时使用正确的描述符。管道描述符的索引应为 0（读端）和 1（写端）。所以，正确的写操作应为：

```c
write(p2[1], &num, sizeof(num));
```

- **子进程和孙进程的管道管理**

 	**问题**: 在创建新管道并生成新的子进程时，没有正确关闭不再需要的管道端口，导致资源泄漏或死锁。

​	 **解决方法**: 在每个进程中，确保关闭不再需要的管道端口。例如：

```c
close(p1[0]); // 关闭主管道的读端
close(p2[1]); // 关闭子管道的写端
```

​	每个进程在操作管道时只保留需要的端口，其它端口应当关闭。

- **进程同步问题**

​	**问题**: 进程创建和数据处理时，如果进程间同步不正确，可能会导致数据丢失或程序行为不如预期。

​	**解决方法**: 确保在进程结束之前，所有子进程都已完成它们的任务，并使用 `wait()` 来同步进程。在主进程中等待子进程完成，以确	保子进程正确地执行完毕。

#### 4. **管道 EOF 处理**

​	**问题**: 子进程在读取管道时可能会遇到 EOF（管道读端关闭），这时子进程需要正确地处理 EOF 并退出。

​	**解决方法**: 检查 `read()` 的返回值，判断是否读取到 EOF。

```c
if (read(p1[0], &prime, sizeof(prime)) == 0)
{
    close(p1[0]);
    exit(0);
}
```

在读取到 EOF 时关闭管道读端并退出。

#### 实验心得

- **掌握了进程间通信（IPC）的基本技巧**:
  - 通过管道实现进程间的数据传递。
  
  - 使用 `fork()` 创建子进程并进行数据处理。
  
  - 处理进程间的同步，确保数据在进程间正确传递。
  
- **理解了素数筛选算法的实现**:

  - 利用管道和进程递归地筛选素数，通过逐步过滤掉非素数的倍数，逐步缩小数据范围。

- **学会了处理进程和管道的资源管理**:

  -  通过关闭不再使用的管道端口来防止资源泄漏。

  -  正确地同步进程，确保所有子进程的任务完成后再退出。

- **提升了调试和错误处理能力**:
  - 通过实验中的错误和调试，提升了发现问题和解决问题的能力。
  - 学会了使用系统调用的错误处理机制，确保程序的健壮性和可靠性。


- **增强了对操作系统底层机制的理解**:
  - 深入理解了操作系统中进程和管道的工作机制，加深了对系统调用和进程管理的理解。


### find ([moderate](https://pdos.csail.mit.edu/6.828/2021/labs/guidance.html))

#### 实验目的

- 编写 UNIX find 程序的简单版本：查找所有文件 在具有特定名称的目录树中。
- 您的解决方案 应该在文件 `user/find.c` 中。

#### 实验步骤

1.进入user文件夹，创建find.c 的文件，代码如下：

``````c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

//在ls.c基础上修改
void find(char *path, char *file)
{   
    //文件名缓冲区
    char buf[512], *p;
    //文件描述符
    int fd;
   
    struct dirent de;
    struct stat st;

    //open() 函数打开路径，返回一个文件描述符
    //错误返回 -1
    if ((fd = open(path, 0)) < 0)
    {
        //无法打开此路径
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    //fstat()返回文件的结点中的所有信息,获得一个已存在文件的模式，并将此模式赋值给它的副本
    //错误返回 -1
    if (fstat(fd, &st) < 0)
    {
        fprintf(2, "find: cannot stat %s\n", path);
        //关闭文件描述符 fd
        close(fd);
        return;
    }
    
    switch(st.type)
    {
        //目录类型不对
        case T_FILE:
            fprintf(2, "find: %s is not a directory\n", path);
            break;
        //目录类型正确
        case T_DIR:
            //路径过长放不入缓冲区
            if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf)
            {
                fprintf(2, "find: path too long\n");
                break;
            }
            //将path指向的字符串即绝对路径复制到buf
            strcpy(buf, path);
            //加 "/" 前缀
            p = buf + strlen(buf);
            *p++ = '/';
            //读取fd
            //判断read返回字节数与de长度相等
            while(read(fd, &de, sizeof(de)) == sizeof(de))
            {
                if(de.inum == 0)
                    continue;
                //不要递归为 “.” 和 “..”
                //字符串比较函数
                if (!strcmp(de.name, ".") || !strcmp(de.name, ".."))
                    continue;
                //把文件名信息复制p
                memmove(p, de.name, DIRSIZ);
                //设置文件名结束符
                p[DIRSIZ] = 0;
                // stat 以文件名作为参数，返回一个已存在文件的模式，并将此模式赋值给它的副本
                //出错，则返回 -1
                if(stat(buf, &st) < 0)
                {
                    printf("find: cannot stat %s\n", buf);
                    continue;
                }
                //目录类型
                if (st.type == T_DIR)
                {
                    //递归查找
                    find(buf, file);
                }
                //文件类型，查找成功
                else if (st.type == T_FILE && !strcmp(de.name, file))
                {
                    //缓冲区存放的路径
                    printf("%s\n", buf);
                } 
            }
        break;
    }
    //关闭文件描述符
    close(fd);
    return;
}


int main(int argc, char *argv[])
{
  //如果参数不是3个
  if(argc != 3)
  {
    write(2, "Incorrect number of characters\n", strlen("Incorrect number of characters\n"));
    exit(1);
  }
  else
  {
    //寻找函数
    find(argv[1], argv[2]);
    exit(0);
  }
}

``````

- 在 `user/ls.c` 中
-  `fmtname()` 函数，目的是将路径格式化为文件名，也就是把名字变成前面没有左斜杠 `/` ，仅仅保存文件名。
- `ls()` 函数，首先函数里面声明了需要用到的变量，包括文件名缓冲区、文件描述符、文件相关的结构体等等。其次使用 `open()` 函数进入路径，判断此路径是文件还是文件名。
- **头文件**:
  - `kernel/types.h`、`kernel/stat.h`、`kernel/fs.h`：这些头文件定义了系统调用和数据结构，比如文件类型 `T_FILE` 和 `T_DIR`，文件状态 `stat`，以及目录项 `dirent` 等。
  - `user/user.h`：包含用户态程序所需的函数，如 `open`、`fstat`、`stat`、`close` 等。
- **`find` 函数**:
  - **参数**:
    - `path`: 需要搜索的目录路径。
    - `file`: 需要查找的文件名。
  - **流程**:
    1. 使用 `open()` 打开指定路径，并返回一个文件描述符 `fd`。如果打开失败，打印错误信息并返回。
    2. 使用 `fstat()` 获取路径对应文件的状态信息，判断它是文件还是目录。
    3. 根据文件类型执行不同操作：
       - 如果是文件类型 `T_FILE`，说明路径不是一个目录，打印错误信息。
       - 如果是目录类型 `T_DIR`，则继续处理：
         - 检查路径长度是否超过缓冲区限制。
         - 使用 `read()` 读取目录项 `dirent`，跳过 `.` 和 `..` 两个特殊目录项。
         - 对每个目录项，使用 `stat()`获取其状态信息：
           - 如果是目录，递归调用 `find()` 函数继续查找。
           - 如果是文件且文件名匹配，打印文件的完整路径。
- **`main` 函数**:
  - 检查命令行参数个数是否为3（程序名、目录路径、文件名）。
  - 如果参数数量不正确，输出错误信息并退出。
  - 否则调用 `find()` 函数执行查找操作。

2. 更新 Makefile

   在 Makefile 中找到 UPROGS 变量的定义，并添加 find

3. 实验结果

![image-20240827165218158](./assets/image-20240827165218158.png)

![image-20240827170914809](./assets/image-20240827170914809.png)

#### 实验中遇到的问题和解决方法

1. **`No rule to make target 'user/_find', needed by 'fs.img'.`**

   - **原因分析**: `Makefile` 中缺少构建目标 `user/_find` 的规则，或者文件路径不正确。此错误意味着 `make` 无法找到生成目标文件 `user/_find` 所需的规则。

   - **解决方法**

      通过在 Makefile中添加一条明确的规则来指定如何生成 

     ```
     user/_find\
     ```

     确保路径使用的是正斜杠 '/'，并且源文件路径正确。例如：

     添加完这条规则后，重新运行即可解决该问题。

2. 路径名中使用反斜杠导致的错误

   - **原因分析**: 反斜杠 `'\'` 在 Unix 类系统中通常被用作转义字符，而不是路径分隔符。这可能导致路径解析错误。

   - **解决方法**: 

     将路径中的反斜杠替换为正斜杠 `'/'`，确保所有路径在 `Makefile` 和代码中都使用标准的 Unix 路径分隔符。

3. **`find` 函数中无法正确递归查找文件**

   - **原因分析**: 递归遍历目录时，可能出现路径拼接错误或目录项判断失误，导致查找功能异常。


   - **解决方法**: 

     检查代码中 `strcpy` 和 `strcat` 的使用，确保路径拼接正确。确保 `strcmp` 函数用于正确过滤 `.` 和 `..` 目录。


#### 实验心得

通过本次实验，我深入理解了 Unix 系统下的文件和目录操作，特别是 `open`, `stat`, `fstat`, 和 `read` 等系统调用的用法。构建一个简化的 `find` 命令让我更好地理解了文件系统的目录遍历机制以及递归算法在实际应用中的重要性。

同时，在处理 `Makefile` 的过程中，我体会到了自动化构建工具的重要性。遇到的错误提醒我需要特别注意路径和文件名的正确性，以及构建规则的明确性。通过查阅文档和调试，我学会了如何排查和解决 `Makefile` 中的构建问题，这对以后的开发工作具有很大的帮助。

总的来说，这次实验不仅加深了我对系统编程的理解，还提高了我分析和解决问题的能力。我更加意识到在编写和调试代码时，细节的把握和对问题的冷静分析是多么重要。

### xargs ([moderate](https://pdos.csail.mit.edu/6.828/2021/labs/guidance.html))

#### 实验目的

- 编写 UNIX xargs 程序的简单版本：从 标准输入并为每行运行一个命令，将行提供为 参数添加到命令中。
- 您的解决方案 应位于文件 `user/xargs.c` 中。

#### 实验步骤

1. 进入user文件夹，创建 xargs.c 的文件，代码如下：

``````c
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
``````

这个程序实现了一个类似于 `xargs` 的功能，用于从标准输入读取参数并将这些参数作为参数传递给指定的命令。

- **检查命令行参数：**

程序首先检查是否提供了至少一个命令参数。如果没有，程序将打印错误消息并退出。

- **初始化命令和参数数组：**

该数组用于存储命令及其参数。`MAXARG` 是一个常量，定义了可以传递给命令的最大参数数量。

- **填充命令和初始参数：**

将命令行参数（从第二个参数开始）存储到 `cmd_args` 数组中，并记录初始参数数量。

- **从标准输入读取字符并处理参数：**

`param_buffer` 用于暂存每个参数的字符，`current_param` 指向当前参数的缓冲区。

- **处理每一行输入：**

  - 当遇到换行符 (`'\n'`)，程序认为一个完整的参数已经结束，将其添加到 `cmd_args` 数组中，然后创建一个子进程来执行指定的命令。

  - 遇到空格 (`' '`) 时，结束当前参数，将其添加到 `cmd_args` 数组中，并准备处理下一个参数。

  - 其他字符直接添加到当前参数缓冲区。

2. 更新 Makefile

​	在 Makefile 中找到 UPROGS 变量的定义，并添加 xargs

3.  实验结果

   ![image-20240827181049051](./assets/image-20240827181049051.png)

   ![image-20240827180219248](./assets/image-20240827180219248.png)

#### 实验中遇到的问题和解决方法

**参数缓冲区管理问题：**

- **问题：** 代码在处理空格时创建了新的 `param_buffer`。这可能导致上一个参数的缓冲区被覆盖，因 `current_param` 指向的新缓冲区 `new_param_buffer` 是局部变量，在下次循环时会被覆盖。
- **解决方法：** 使用 `current_param` 指向的缓冲区必须保持有效，或考虑使用动态分配内存来存储参数。

**命令参数数量限制：**

- **问题：** `MAXARG` 限制了参数数量，可能在处理大量参数时导致溢出。
- **解决方法：** 增加 `MAXARG` 的大小或实现动态调整参数数量的方法。

**错误处理：**

- **问题：** `exec` 失败时没有明确错误信息。
- **解决方法：** 在 `exec` 失败时打印错误信息，帮助调试。

#### 实验心得

1. **理解进程管理和参数传递：**
   - 在实验过程中，更加深入地理解了进程创建 (`fork`) 和执行 (`exec`) 的机制，以及如何传递参数到子进程中。
2. **缓冲区管理的重要性：**
   - 处理输入时要特别注意缓冲区的生命周期和管理，避免由于局部变量重用导致的数据覆盖问题。
3. **系统调用的实际应用：**
   - 通过实际编写程序，理解了如何使用系统调用 `read`, `write`, `fork`, `exec`, 和 `wait` 等来实现功能。
4. **调试技巧：**
   - 通过调试工具和输出调试信息，帮助发现和解决代码中的问题，如内存管理问题和进程控制问题。

### Score

![image-20240827211521823](./assets/image-20240827211521823.png)



## Lab2: system calls

在上一个实验中，您使用了 systems 调用编写了一些实用程序。在 在本实验中，您将向 XV6 添加一些新的系统调用，这将有所帮助 您了解它们的工作原理，并将让您接触到一些 xv6 内核的内部结构。稍后将添加更多系统调用 实验室。

要启动实验室，请切换到 syscall 分支：

```
git fetch
git checkout syscall
make clean
```

### System call tracing ([moderate](https://pdos.csail.mit.edu/6.828/2021/labs/guidance.html))

#### 实验目的

- 添加一个系统调用跟踪功能，该功能 可能会在调试后续实验时有所帮助。

- 创建一个新的跟踪系统调用，它将控制跟踪。它应该 取一个参数，一个整数 “mask”，其位指定哪个 对 trace 的系统调用。
- 例如，要跟踪 fork 系统调用， 程序调用 trace（1 << SYS_fork），其中 SYS_fork`是一个 来自 kernel/syscall.h 的 syscall 编号。
- 修改 xv6 内核在每次系统调用即将 return，如果在掩码中设置了系统调用的号码。 该行应包含 进程 ID、系统调用的名称和 返回值;
- 无需打印 System Call 参数。`trace` 系统调用应启用跟踪 对于调用它的进程以及它随后分叉的任何子进程， 但不应影响其他进程。

#### 实验步骤

1. 作为系统调用，先要定义一个系统调用的序号。在 kernel/syscall.h 添加宏定义，模仿已经存在的系统调用序号的宏定义

   定义 `SYS_trace` 如下：

   ``````
   #define SYS_trace 22
   ``````

   ![image-20240827191056836](./assets/image-20240827191056836.png)

2. user 目录下的文件，官方已经给出了用户态的 trace 函数( user/trace.c )

   直接在 user/user.h 文件中声明用户态可以调用 trace 系统调用

   查看 trace.c 文件，可以看到 trace(atoi(argv[1])) < 0 ，即 trace 函数传入的是一个数字，并和 0 进行比较、

   结合实验提示，我们知道传入的参数类型是 int ，并且由此可以猜测到返回值类型应该是 int 。

   这样就可以把 trace 这个系统调用加入到内核中声明了：

   打开 user/user.h，添加：

   ``````
   int trace(int);
   ``````

   ![image-20240827202701737](./assets/image-20240827202701737.png)

3. 查看 user/usys.pl文件，这里 perl 语言会自动生成汇编语言 **usys.S** ，是用户态系统调用接口。所以在 **user/usys.pl** 文件加入下面的语句：

   ``````
   entry("trace");
   ``````

   ![image-20240827202644884](./assets/image-20240827202644884.png)

4. 行 ecall 指令会跳转到kernel/syscall.c 中 syscall 那个函数处，执行此函数。下面是 syscall 函数的源码：

   ``````c
   void syscall(void)
   {
     int num;
     struct proc *p = myproc();
   
     num = p->trapframe->a7;
     if(num > 0 && num < NELEM(syscalls) && syscalls[num]) {
       p->trapframe->a0 = syscalls[num]();
     } else {
       printf("%d %s: unknown sys call %d\n",
               p->pid, p->name, num);
       p->trapframe->a0 = -1;
     }
   }
   ``````

   其中， num = p->trapframe->a7; 从寄存器 a7 中读取系统调用号，所以上面的 usys.S 文件就是系统调用用户态和内核态的切换接口。接下来是 p->trapframe->a0 = syscalls[num]\(); 语句，通过调用 syscalls[num\](); 函数，把返回值保存在了 a0 寄存器中。

5. 我们看看 syscalls[num]\(); 函数，这个函数在当前文件中。该函数调用了系统调用命令。

      ```c
      static uint64 (*syscalls[])(void) = {
        [SYS_fork]    sys_fork,
        [SYS_exit]    sys_exit,
        ...
      }
      ```

      把新增的 trace 系统调用添加到函数指针数组 *syscalls[]

      ![image-20240827203730673](./assets/image-20240827203730673.png)

6. 在文件开头给内核态的系统调用 `trace` 加上声明，在 kernel/syscall.c 加上：

   ```c
   extern uint64 sys_trace(void);
   ```

   ![image-20240827203835131](./assets/image-20240827203835131.png)

7. 在实现这个函数之前，我们可以看到实验最后要输出每个系统调用函数的调用情况，依照实验说明给的示例，可以知道最后输出的格式如下：<pid>: syscall <syscall_name> -> <return_value>
   其中， <pid> 是进程序号， <syscall_name> 是函数名称， <return_value> 是该系统调用的返回值。

   根据提示，我们的 trace 系统调用应该有一个参数，一个整数“mask(掩码)”，其指定要跟踪的系统调用。

   我们在 kernel/proc.h 文件的 proc 结构体中，新添加一个变量 mask ，使得每一个进程都有自己的 mask ，即要跟踪的系统调用。

   ```c
   struct proc {
    ...
    int mask; // Mask
   };
   ```

   ![image-20240827204150170](./assets/image-20240827204150170.png)

8. 然后我们就可以在 kernel/sysproc.c 给出 sys_trace 函数的具体实现了，只要把传进来的参数给到现有进程的 mask 就好了：

   ``````c
   uint64
   sys_trace(void)
   {
     int mask;
     // 取 a0 寄存器中的值返回给 mask
     if(argint(0, &mask) < 0)
       return -1;
     
     // 把 mask 传给现有进程的 mask
     myproc()->mask = mask;
     return 0;
   }
   ``````

   ![image-20240827204405870](./assets/image-20240827204405870.png)

9. 接下来我们就要把输出功能实现，因为 RISCV 的 C 规范是把返回值放在 a0 中，所以我们只要在调用系统调用时判断是不是 mask 规定的输出函数，如果是就输出。

   因为 proc 结构体(见 kernel/proc.h )里的 name 是整个线程的名字，不是函数调用的函数名称，所以我们不能用 p->name ，而要自己定义一个数组

   在 kernel/syscall.c 中定义，系统调用名字一定要按顺序，第一个为空
   ``````c
   static char *syscall_names[] = {
     "", "fork", "exit", "wait", "pipe", 
     "read", "kill", "exec", "fstat", "chdir", 
     "dup", "getpid", "sbrk", "sleep", "uptime", 
     "open", "write", "mknod", "unlink", "link", 
     "mkdir", "close", "trace"};
   ``````

   ![image-20240827204735814](./assets/image-20240827204735814.png)

10. kernel/syscall.c 中的 syscall 函数中添加打印调用情况语句。 mask 是按位判断的，所以判断使用的是按位运算。

   进程序号直接通过 p->pid 就可以取到，函数名称需要从我们刚刚定义的数组中获取，即 syscall_names[num] ，其中 num 是从寄存器 a7 中读取的系统调用号，系统调用的返回值就是寄存器 a0 的值了，直接通过 p->trapframe->a0 语句获取即可。

   ``````c
   if((1 << num) & p->mask) 
      printf("%d: syscall %s -> %d\n", p->pid, syscall_names[num], p->trapframe->a0);
   ``````

   ![image-20240827205034618](./assets/image-20240827205034618.png)

11. 在 kernel/proc.c 中 `fork` 函数调用时，添加子进程复制父进程的 `mask` 的代码：

    ``````c
    np->mask = p->mask;
    ``````

    ![image-20240827205558492](./assets/image-20240827205558492.png)

12. 更新 Makefile

    在 Makefile 中找到 UPROGS 变量的定义，并添加 $U/_trace\

    ![image-20240827202940731](./assets/image-20240827202940731.png)

13. 实验结果

       ![image-20240827210542394](./assets/image-20240827210542394.png)

       ![image-20240827210659870](./assets/image-20240827210659870.png)

#### 实验中遇到的问题和解决方法

1. **系统调用跟踪的实现细节**

​	**问题**: 在实现系统调用跟踪功能时，可能会遇到如何正确地记录和输出系统调用信息的困难。

​	**解决方法**:

- 确保正确地在系统调用处理程序中添加跟踪代码。例如，在操作系统的系统调用处理程序中插入打印语句或日志记录功能。
- 使用 `printf` 或类似函数将系统调用的参数、返回值等信息打印出来，以便跟踪。

2. **系统调用表的维护**

​	**问题**: 系统调用表没有正确更新或维护，导致系统调用跟踪信息不准确。

​	**解决方法**:

- 确保在系统调用表中正确地添加了新系统调用的条目，并且系统调用号与处理程序的映射是正确的。
- 检查系统调用表的定义和更新代码，确保它们与实际实现一致。

3. **系统调用参数的提取**

​	**问题**: 在跟踪系统调用时，提取和解析系统调用参数会出错，导致输出信息不准确。

​	**解决方法**:

- 确保正确地从进程的寄存器或堆栈中提取系统调用参数。
- 检查系统调用的参数传递方式（如寄存器传递或堆栈传递）并确保提取逻辑与实际参数传递方式一致。

1. **深入理解系统调用的工作原理**:
   - 通过实现系统调用跟踪功能，加深了对系统调用的理解，包括如何通过系统调用号和处理程序进行映射。
   - 学习了如何在操作系统中插入和管理系统调用处理代码。
2. **掌握系统调用调试技巧**:
   - 在调试系统调用跟踪功能时，学会了使用调试工具（如 `gdb`）来排查问题。
   - 体会到了系统调用调试中的挑战，例如如何处理系统调用参数的提取和记录。
3. **理解操作系统的设计和实现**:
   - 通过实现系统调用跟踪，理解了操作系统如何管理系统调用的执行，包括如何将系统调用与实际的处理程序关联起来。
   - 认识到系统调用跟踪对操作系统的稳定性和性能的影响。
4. **增强了代码的维护和管理能力**:
   - 学会了如何正确更新和维护系统调用表，确保系统调用的正确性和可靠性。
   - 提升了对操作系统内核代码的修改和扩展能力。
5. **体验了实验的挑战和解决策略**:
   - 在实验过程中遇到了各种挑战，例如系统调用跟踪信息不准确或系统崩溃，学会了如何通过仔细检查代码和调试工具来解决这些问题。
   - 认识到系统调用跟踪是一个复杂的任务，需要细致的工作和准确的实现。

通过系统调用跟踪实验，不仅提高了对操作系统内部机制的理解，还增强了调试和系统级编程的能力。这些经验和技能将为进一步的操作系统设计和开发打下坚实的基础。

### Sysinfo ([moderate](https://pdos.csail.mit.edu/6.828/2021/labs/guidance.html))

#### 实验目的

- 在本实验中，您将添加一个系统调用 sysinfo ，它收集有关正在运行的系统信息。

- 系统调用接受一个参数：一个指向 struct sysinfo 的指针(参见 kernel/sysinfo.h )。内核应该填写这个结构体的字段： freemem 字段应该设置为空闲内存的字节数， nproc 字段应该设置为状态不是 UNUSED 的进程数。
- 我们提供了一个测试程序 sysinfotest ；如果它打印 “sysinfotest：OK” ，则实验结果通过测试。

#### 实验步骤

1. 定义一个系统调用的序号。系统调用序号的宏定义在 kernel/syscall.h 文件中。 kernel/syscall.h 添加宏定义 SYS_sysinfo 如下：

   ``````c
   #define SYS_sysinfo  23
   ``````

2. 
   在 user/usys.pl 文件加入下面的语句：

   ``````c
   entry("sysinfo");
   ``````

3. 在 user/user.h 中添加 sysinfo 结构体以及 sysinfo 函数的声明：

   ``````c
   struct sysinfo;
   
   // system calls
   int sysinfo(struct sysinfo *);
   ``````

4. 在 kernel/syscall.c 中新增 sys_sysinfo 函数的定义：

   ``````c
   extern uint64 sys_sysinfo(void);
   ``````

   函数指针数组新增 `sys_trace` ：

   ``````c
   [SYS_sysinfo]   sys_sysinfo,
   ``````

   syscall_names 新增一个 sys_trace ：

   ``````c
   static char *syscall_names[] = {
     "", "fork", "exit", "wait", "pipe", 
     "read", "kill", "exec", "fstat", "chdir", 
     "dup", "getpid", "sbrk", "sleep", "uptime", 
     "open", "write", "mknod", "unlink", "link", 
     "mkdir", "close", "trace", "sysinfo"};
   ``````

5. 在 kernel/proc.c中新增函数 `nproc` 如下，通过该函数以获取可用进程数目：

   ``````c
   uint64
   nproc(void)
   {
     struct proc *p;
     uint64 num = 0;
     for (p = proc; p < &proc[NPROC]; p++)
     {
       acquire(&p->lock);
       if (p->state != UNUSED)
       {
         num++;
       }
       release(&p->lock);
     }
     return num;
   }
   ``````

   用于统计操作系统中非 `UNUSED` 状态的进程数量。这个函数遍历了进程表中的所有进程，并且对每个进程获取锁，以确保在读取进程状态时没有其他进程正在修改它。

6. 可以在 kernel/kalloc.c 中新增函数 `free_mem` ，以获取空闲内存数量：

   通过这个函数，你可以计算系统中空闲内存的总量。函数通过遍历空闲内存块链表来统计内存块的数量，并通过加锁来保证在统计过程中不会出现并发问题。

   ``````c
   uint64
   free_mem(void)
   {
     struct run *r;
     uint64 num = 0;
   
     // 获取内存管理锁以确保线程安全
     acquire(&kmem.lock);
   
     // 指向空闲内存块的链表头
     r = kmem.freelist;
   
     // 遍历空闲内存块链表
     while (r)
     {
       num++; // 每找到一个空闲内存块，计数器加1
       r = r->next; // 移动到下一个内存块
     }
   
     // 释放内存管理锁
     release(&kmem.lock);
   
     // 返回空闲内存的总大小
     // num 乘以每个页面的大小 PGSIZE
     return num * PGSIZE;
   }
   ``````

7. 在 kernel/defs.h 中添加上述两个新增函数的声明：

   ``````c
   // kalloc.c
   uint64          free_mem(void);
   // proc.c
   uint64          nproc(void);
   ``````

8. 在 kernel/sysproc.c 文件中添加 `sys_sysinfo` 函数的具体实现如下：

   sysinfo 需要将 `struct sysinfo` 复制回给 user 空间;参见 `sys_fstat（）` （`kernel/sysfile.c`） 和 `filestat（）` （`kernel/file.c`） 以获取示例，以了解如何操作 来做到这一点。

   ``````c
   #include "sysinfo.h"
   
   // 系统调用实现，返回系统信息
   uint64
   sys_sysinfo(void)
   {
     uint64 addr;            // 用户空间中接收系统信息的地址
     struct sysinfo info;    // 用于存储系统信息的结构体
     struct proc *p = myproc(); // 获取当前进程的指针
   
     // 从系统调用参数中获取地址
     if (argaddr(0, &addr) < 0)
       return -1; // 获取地址失败，返回 -1 表示错误
   
     // 获取系统信息
     info.freemem = free_mem(); // 获取系统中空闲内存的总量
     info.nproc = nproc();      // 获取系统中活跃进程的数量
   
     // 将系统信息复制到用户空间的指定地址
     if (copyout(p->pagetable, addr, (char *)&info, sizeof(info)) < 0)
       return -1; // 复制失败，返回 -1 表示错误
   
     return 0; // 成功，返回 0
   }
   ``````

9. 在 user 目录下添加一个 sysinfo.c 程序：

   ``````c
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
   ``````

   这个程序使用 `sysinfo` 系统调用来获取系统的空闲内存和活跃进程数，并将这些信息打印出来。

   通过检查命令行参数的数量，它确保程序正确地被调用。这个程序示例展示了如何从内核中提取系统级信息，并在用户空间进行处理和展示。

10. 在 Makefile 的 `UPROGS` 中添加：

    ```Makefile
    $U/_sysinfotest\
    $U/_sysinfo\
    ```

11. 实验结果

    ![image-20240827214434879](./assets/image-20240827214434879.png)

    ![image-20240827214304250](./assets/image-20240827214304250.png)

      - free space: 133386240: 系统中剩余的空闲内存量，单位为字节。
      - used process: 3: 当前系统中活跃的进程数量，这里是 3 个。

      - sysinfotest: start: 表示测试程序 sysinfotest 开始运行。

      - sysinfotest: OK: 表示测试通过，系统调用 sysinfo正常工作。

    这段输出说明：

    1. 你的操作系统成功启动，并能运行基本的 shell。
    2. 你实现的 `sysinfo` 系统调用能够正确返回系统的空闲内存和活跃进程数量。
    3. 你实现的 `sysinfo` 系统调用通过了 `sysinfotest` 的测试，验证了其功能的正确性。

#### 实验中遇到的问题和解决方法

1. **系统调用未正确注册的问题**:
   - **问题**: 在实现 `sysinfo` 系统调用时，系统调用表可能未正确更新，导致在用户程序中调用 `sysinfo` 时出现错误，如未定义的系统调用或返回错误代码。
   - **解决方法**: 检查系统调用的实现，确保在系统调用表中正确注册了 `sysinfo`。通常，这涉及到在 `syscall.h` 中添加相应的系统调用号，并在 `syscall.c` 中将系统调用号映射到具体的处理函数。此外，还需要确保用户空间的调用正确无误。

2. **数据传输的内存地址错误**:
   - **问题**: 在 `sysinfo` 系统调用中，将 `sysinfo` 结构体从内核传递到用户空间时，可能出现传输失败的情况，例如 `copyout` 函数返回错误。这通常是由于内存地址错误或权限问题引起的。
   - **解决方法**: 检查传递的地址是否正确无误，确保用户空间传递的地址在其进程的地址空间内。同时，仔细检查 `copyout` 函数的使用，确保其正确地将内核数据复制到用户空间。

3. **内核锁未正确处理**:
   - **问题**: 在实现 `free_mem()` 函数时，如果没有正确处理内核锁，可能导致并发访问时的竞争条件，导致内存统计不准确或者系统崩溃。
   - **解决方法**: 确保在访问内核中共享资源（如自由内存链表）时，正确使用锁进行同步。对资源加锁后再操作，操作完成后立即释放锁，避免死锁或竞争条件。

4. **编译错误与警告**:
   - **问题**: 在实现和测试 `sysinfo` 系统调用时，可能会遇到编译错误或警告，如类型不匹配、未定义的符号等。这些问题通常源于系统调用的定义与实现不一致。
   - **解决方法**: 逐一检查代码的类型声明、系统调用表的配置，以及用户和内核空间数据传递的函数。确保各部分的一致性，并参考现有的系统调用实现，确保风格和用法一致。

#### 实验心得

通过本次实验，我深入理解了操作系统中的系统调用机制，特别是如何在一个精简的操作系统中实现新的系统调用。这个过程让我认识到系统调用不仅仅是用户与内核之间的桥梁，还是内核资源管理和系统状态查询的重要手段。

在实验中，我遇到了一些实现和调试上的挑战，比如如何安全地在内核和用户空间之间传递数据、如何正确使用内核锁来防止竞争条件等。这些问题的解决让我掌握了更多操作系统内核编程的技巧，并加深了对操作系统设计的理解。

另外，通过 `sysinfo` 系统调用的实现和测试，我还进一步熟悉了如何在操作系统中实现高效的资源管理与状态监控功能。这些经验对于将来设计和优化更复杂的操作系统功能非常有价值。

总之，这次实验不仅巩固了我对操作系统核心概念的理解，也提升了我解决实际问题的能力，为后续的学习和研究打下了坚实的基础。

### Score

![image-20240827214337958](./assets/image-20240827214337958.png)



## Lab3: page tables

### Speed up system calls ([easy](https://pdos.csail.mit.edu/6.828/2021/labs/guidance.html))

#### 实验目的

某些操作系统（例如 Linux）通过共享 用户空间和内核之间的只读区域中的数据。这消除了 在执行这些系统调用时需要内核交叉。帮助您学习 如何将映射插入页表中，您的首要任务是实现此 优化了 XV6 中的 `getpid（）` 系统调用。

创建每个进程后，在 USYSCALL 中映射一个只读页面（VA 定义的 在 `memlayout.h` 中）。在本页的开头，存储一个 `struct usys调用`（也在 `memlayout.h` 中定义），并将其初始化为存储 当前进程的 PID。在本实验中，`ugetpid（）` 已被 ，并将自动使用 USYSCALL 映射。 如果 `ugetpid` 测试 运行 `pgtbltest` 时大小写通过。

#### 实验步骤

#### 实验中遇到的问题和解决方法

#### 实验心得



### Print a page table ([easy](https://pdos.csail.mit.edu/6.828/2021/labs/guidance.html))

#### 实验目的

#### 实验步骤

#### 实验中遇到的问题和解决方法

#### 实验心得

### Detecting which pages have been accessed ([hard](https://pdos.csail.mit.edu/6.828/2021/labs/guidance.html))

#### 实验目的

#### 实验步骤

#### 实验中遇到的问题和解决方法

#### 实验心得















#### 实验目的

#### 实验步骤

#### 实验中遇到的问题和解决方法

#### 实验心得
