#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64 
sys_sigalarm(void) {
    int interval;
    uint64 handler;
    struct proc *p;
    // 要求时间间隔非负
    if (argint(0, &interval) < 0 || argaddr(1, &handler) < 0 || interval < 0) {
        return -1;
    }
    p = myproc();
    p->interval = interval;
    p->handler = handler;
    p->passedticks = 0;    // 重置过去时钟数

    return 0;
}





uint64 
sys_sigreturn(void) {
    // 获取当前进程的指针
    struct proc* p = myproc();

    // 检查保存的 trapframe 是否是正确的（即是否在 trapframe 地址加上 512 的位置）
    if (p->trapframecopy != p->trapframe + 512) {
        // 如果保存的 trapframe 地址不匹配，返回错误码 -1
        return -1;
    }

    // 恢复 trapframe 的内容到原始位置
    // 将保存的 trapframe 内容复制回到当前的 trapframe 中
    memmove(p->trapframe, p->trapframecopy, sizeof(struct trapframe));

    // 重置进程的 passedticks 计数器为 0
    p->passedticks = 0;

    // 将 trapframecopy 设置为 0，表示不再需要这个副本
    p->trapframecopy = 0;    

    // 返回 a0 寄存器的值，a0 通常用于存放系统调用的返回值
    return p->trapframe->a0;  
}

