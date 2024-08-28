//
// File-system system calls.
// Mostly argument checking, since we don't trust
// user code, and calls into file.c and fs.c.
//

#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "stat.h"
#include "spinlock.h"
#include "proc.h"
#include "fs.h"
#include "sleeplock.h"
#include "file.h"
#include "fcntl.h"

// Fetch the nth word-sized system call argument as a file descriptor
// and return both the descriptor and the corresponding struct file.
static int
argfd(int n, int *pfd, struct file **pf)
{
  int fd;
  struct file *f;

  if(argint(n, &fd) < 0)
    return -1;
  if(fd < 0 || fd >= NOFILE || (f=myproc()->ofile[fd]) == 0)
    return -1;
  if(pfd)
    *pfd = fd;
  if(pf)
    *pf = f;
  return 0;
}

// Allocate a file descriptor for the given file.
// Takes over file reference from caller on success.
static int
fdalloc(struct file *f)
{
  int fd;
  struct proc *p = myproc();

  for(fd = 0; fd < NOFILE; fd++){
    if(p->ofile[fd] == 0){
      p->ofile[fd] = f;
      return fd;
    }
  }
  return -1;
}

uint64
sys_dup(void)
{
  struct file *f;
  int fd;

  if(argfd(0, 0, &f) < 0)
    return -1;
  if((fd=fdalloc(f)) < 0)
    return -1;
  filedup(f);
  return fd;
}

uint64
sys_read(void)
{
  struct file *f;
  int n;
  uint64 p;

  if(argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argaddr(1, &p) < 0)
    return -1;
  return fileread(f, p, n);
}

uint64
sys_write(void)
{
  struct file *f;
  int n;
  uint64 p;

  if(argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argaddr(1, &p) < 0)
    return -1;

  return filewrite(f, p, n);
}

uint64
sys_close(void)
{
  int fd;
  struct file *f;

  if(argfd(0, &fd, &f) < 0)
    return -1;
  myproc()->ofile[fd] = 0;
  fileclose(f);
  return 0;
}

uint64
sys_fstat(void)
{
  struct file *f;
  uint64 st; // user pointer to struct stat

  if(argfd(0, 0, &f) < 0 || argaddr(1, &st) < 0)
    return -1;
  return filestat(f, st);
}

// Create the path new as a link to the same inode as old.
uint64
sys_link(void)
{
  char name[DIRSIZ], new[MAXPATH], old[MAXPATH];
  struct inode *dp, *ip;

  if(argstr(0, old, MAXPATH) < 0 || argstr(1, new, MAXPATH) < 0)
    return -1;

  begin_op();
  if((ip = namei(old)) == 0){
    end_op();
    return -1;
  }

  ilock(ip);
  if(ip->type == T_DIR){
    iunlockput(ip);
    end_op();
    return -1;
  }

  ip->nlink++;
  iupdate(ip);
  iunlock(ip);

  if((dp = nameiparent(new, name)) == 0)
    goto bad;
  ilock(dp);
  if(dp->dev != ip->dev || dirlink(dp, name, ip->inum) < 0){
    iunlockput(dp);
    goto bad;
  }
  iunlockput(dp);
  iput(ip);

  end_op();

  return 0;

bad:
  ilock(ip);
  ip->nlink--;
  iupdate(ip);
  iunlockput(ip);
  end_op();
  return -1;
}

// Is the directory dp empty except for "." and ".." ?
static int
isdirempty(struct inode *dp)
{
  int off;
  struct dirent de;

  for(off=2*sizeof(de); off<dp->size; off+=sizeof(de)){
    if(readi(dp, 0, (uint64)&de, off, sizeof(de)) != sizeof(de))
      panic("isdirempty: readi");
    if(de.inum != 0)
      return 0;
  }
  return 1;
}

uint64
sys_unlink(void)
{
  struct inode *ip, *dp;
  struct dirent de;
  char name[DIRSIZ], path[MAXPATH];
  uint off;

  if(argstr(0, path, MAXPATH) < 0)
    return -1;

  begin_op();
  if((dp = nameiparent(path, name)) == 0){
    end_op();
    return -1;
  }

  ilock(dp);

  // Cannot unlink "." or "..".
  if(namecmp(name, ".") == 0 || namecmp(name, "..") == 0)
    goto bad;

  if((ip = dirlookup(dp, name, &off)) == 0)
    goto bad;
  ilock(ip);

  if(ip->nlink < 1)
    panic("unlink: nlink < 1");
  if(ip->type == T_DIR && !isdirempty(ip)){
    iunlockput(ip);
    goto bad;
  }

  memset(&de, 0, sizeof(de));
  if(writei(dp, 0, (uint64)&de, off, sizeof(de)) != sizeof(de))
    panic("unlink: writei");
  if(ip->type == T_DIR){
    dp->nlink--;
    iupdate(dp);
  }
  iunlockput(dp);

  ip->nlink--;
  iupdate(ip);
  iunlockput(ip);

  end_op();

  return 0;

bad:
  iunlockput(dp);
  end_op();
  return -1;
}

static struct inode*
create(char *path, short type, short major, short minor)
{
  struct inode *ip, *dp;
  char name[DIRSIZ];

  if((dp = nameiparent(path, name)) == 0)
    return 0;

  ilock(dp);

  if((ip = dirlookup(dp, name, 0)) != 0){
    iunlockput(dp);
    ilock(ip);
    if(type == T_FILE && (ip->type == T_FILE || ip->type == T_DEVICE))
      return ip;
    iunlockput(ip);
    return 0;
  }

  if((ip = ialloc(dp->dev, type)) == 0)
    panic("create: ialloc");

  ilock(ip);
  ip->major = major;
  ip->minor = minor;
  ip->nlink = 1;
  iupdate(ip);

  if(type == T_DIR){  // Create . and .. entries.
    dp->nlink++;  // for ".."
    iupdate(dp);
    // No ip->nlink++ for ".": avoid cyclic ref count.
    if(dirlink(ip, ".", ip->inum) < 0 || dirlink(ip, "..", dp->inum) < 0)
      panic("create dots");
  }

  if(dirlink(dp, name, ip->inum) < 0)
    panic("create: dirlink");

  iunlockput(dp);

  return ip;
}

uint64
sys_open(void)
{
  char path[MAXPATH];
  int fd, omode;
  struct file *f;
  struct inode *ip;
  int n;

  if((n = argstr(0, path, MAXPATH)) < 0 || argint(1, &omode) < 0)
    return -1;

  begin_op();

  if(omode & O_CREATE){
    ip = create(path, T_FILE, 0, 0);
    if(ip == 0){
      end_op();
      return -1;
    }
  } else {
    if((ip = namei(path)) == 0){
      end_op();
      return -1;
    }
    ilock(ip);
    if(ip->type == T_DIR && omode != O_RDONLY){
      iunlockput(ip);
      end_op();
      return -1;
    }
  }

  if(ip->type == T_DEVICE && (ip->major < 0 || ip->major >= NDEV)){
    iunlockput(ip);
    end_op();
    return -1;
  }

  if((f = filealloc()) == 0 || (fd = fdalloc(f)) < 0){
    if(f)
      fileclose(f);
    iunlockput(ip);
    end_op();
    return -1;
  }

  if(ip->type == T_DEVICE){
    f->type = FD_DEVICE;
    f->major = ip->major;
  } else {
    f->type = FD_INODE;
    f->off = 0;
  }
  f->ip = ip;
  f->readable = !(omode & O_WRONLY);
  f->writable = (omode & O_WRONLY) || (omode & O_RDWR);

  if((omode & O_TRUNC) && ip->type == T_FILE){
    itrunc(ip);
  }

  iunlock(ip);
  end_op();

  return fd;
}

uint64
sys_mkdir(void)
{
  char path[MAXPATH];
  struct inode *ip;

  begin_op();
  if(argstr(0, path, MAXPATH) < 0 || (ip = create(path, T_DIR, 0, 0)) == 0){
    end_op();
    return -1;
  }
  iunlockput(ip);
  end_op();
  return 0;
}

uint64
sys_mknod(void)
{
  struct inode *ip;
  char path[MAXPATH];
  int major, minor;

  begin_op();
  if((argstr(0, path, MAXPATH)) < 0 ||
     argint(1, &major) < 0 ||
     argint(2, &minor) < 0 ||
     (ip = create(path, T_DEVICE, major, minor)) == 0){
    end_op();
    return -1;
  }
  iunlockput(ip);
  end_op();
  return 0;
}

uint64
sys_chdir(void)
{
  char path[MAXPATH];
  struct inode *ip;
  struct proc *p = myproc();
  
  begin_op();
  if(argstr(0, path, MAXPATH) < 0 || (ip = namei(path)) == 0){
    end_op();
    return -1;
  }
  ilock(ip);
  if(ip->type != T_DIR){
    iunlockput(ip);
    end_op();
    return -1;
  }
  iunlock(ip);
  iput(p->cwd);
  end_op();
  p->cwd = ip;
  return 0;
}

uint64
sys_exec(void)
{
  char path[MAXPATH], *argv[MAXARG];
  int i;
  uint64 uargv, uarg;

  if(argstr(0, path, MAXPATH) < 0 || argaddr(1, &uargv) < 0){
    return -1;
  }
  memset(argv, 0, sizeof(argv));
  for(i=0;; i++){
    if(i >= NELEM(argv)){
      goto bad;
    }
    if(fetchaddr(uargv+sizeof(uint64)*i, (uint64*)&uarg) < 0){
      goto bad;
    }
    if(uarg == 0){
      argv[i] = 0;
      break;
    }
    argv[i] = kalloc();
    if(argv[i] == 0)
      goto bad;
    if(fetchstr(uarg, argv[i], PGSIZE) < 0)
      goto bad;
  }

  int ret = exec(path, argv);

  for(i = 0; i < NELEM(argv) && argv[i] != 0; i++)
    kfree(argv[i]);

  return ret;

 bad:
  for(i = 0; i < NELEM(argv) && argv[i] != 0; i++)
    kfree(argv[i]);
  return -1;
}

uint64
sys_pipe(void)
{
  uint64 fdarray; // user pointer to array of two integers
  struct file *rf, *wf;
  int fd0, fd1;
  struct proc *p = myproc();

  if(argaddr(0, &fdarray) < 0)
    return -1;
  if(pipealloc(&rf, &wf) < 0)
    return -1;
  fd0 = -1;
  if((fd0 = fdalloc(rf)) < 0 || (fd1 = fdalloc(wf)) < 0){
    if(fd0 >= 0)
      p->ofile[fd0] = 0;
    fileclose(rf);
    fileclose(wf);
    return -1;
  }
  if(copyout(p->pagetable, fdarray, (char*)&fd0, sizeof(fd0)) < 0 ||
     copyout(p->pagetable, fdarray+sizeof(fd0), (char *)&fd1, sizeof(fd1)) < 0){
    p->ofile[fd0] = 0;
    p->ofile[fd1] = 0;
    fileclose(rf);
    fileclose(wf);
    return -1;
  }
  return 0;
}

// sys_mmap() - 将文件映射到虚拟内存空间
// 参数：
//   va: 虚拟地址（程序提供的建议地址）
//   len: 映射的长度（字节数）
//   prot: 映射区域的保护权限（如读、写权限）
//   flags: 映射选项（如是否共享）
//   fd: 文件描述符，指向需要映射的文件
//   off: 文件偏移量（从文件的哪个位置开始映射）
// 返回值：
//   成功时返回映射的虚拟地址，失败时返回 -1
uint64 sys_mmap(void) {
    uint64 va;         // 存储虚拟地址参数
    int len, prot, flags, fd, off; // 存储映射长度、保护权限、映射标志、文件描述符和偏移量参数

    // 从系统调用参数中获取各个参数的值，如果获取失败则返回 -1
    if (argaddr(0, &va) < 0) return -1;
    if (argint(1, &len) < 0 || argint(2, &prot) < 0
        || argint(3, &flags) < 0 || argint(4, &fd) < 0)
        return -1;
    if (argint(5, &off) < 0) return -1;

    // 参数检查和前置条件验证
    struct proc *p = myproc();  // 获取当前进程结构体
    if (len <= 0) return -1;    // 映射长度必须大于 0
    if (p->ofile[fd] == 0) return -1; // 检查文件描述符是否有效
    else filedup(p->ofile[fd]); // 增加文件的引用计数

    // 检查映射的文件是否符合可写权限要求
    if (p->ofile[fd]->writable == 0 &&  // 如果文件不可写
        (prot & PROT_WRITE) == PROT_WRITE &&  // 并且请求了写权限
        (flags & MAP_SHARED) == MAP_SHARED)   // 且映射是共享的
        return -1;  // 返回 -1 表示不允许此映射

    // 在进程的虚拟内存区域数组中分配一个 vma 结构体
    int i;
    for (i = 0; i < MAXVMA; ++i) {
        if (p->vma[i].valid == 0) { // 找到一个空闲的 vma 结构体
            p->vma[i].valid = 1;    // 标记为有效
            break;
        }
    }
    if (i == MAXVMA) return -1; // 如果没有空闲的 vma 结构体，返回 -1

    // 设置 vma 结构体的各个字段
    p->vma[i].va = p->curend - len;  // 分配一个新的虚拟地址
    p->curend = p->vma[i].va;        // 更新当前进程的虚拟地址空间结束位置
    p->vma[i].len = len;             // 设置映射长度
    uint pteflags = 0;               // 初始化页表项标志位
    if ((prot & PROT_READ) == PROT_READ) pteflags |= PTE_R; // 设置读权限
    if ((prot & PROT_WRITE) == PROT_WRITE) pteflags |= PTE_W; // 设置写权限
    p->vma[i].prot = pteflags;       // 设置 vma 的权限
    p->vma[i].flags = flags;         // 设置映射标志
    p->vma[i].fd = fd;               // 设置文件描述符
    p->vma[i].off = off;             // 设置文件偏移量
    p->vma[i].f = p->ofile[fd];      // 保存文件指针

    return (uint64)p->vma[i].va;     // 返回映射的虚拟地址
}

// sys_munmap() - 解除文件与虚拟内存的映射
// 参数：
//   va: 要解除映射的虚拟地址
//   len: 解除映射的长度（字节数）
// 返回值：
//   成功时返回 0，失败时返回 -1
uint64 sys_munmap(void) {
    uint64 va;  // 存储要解除映射的虚拟地址
    int len;    // 存储解除映射的长度

    // 从系统调用参数中获取 va 和 len 的值，如果获取失败则返回 -1
    if (argaddr(0, &va) < 0) return -1;
    if (argint(1, &len) < 0) return -1;

    // 调用 subunmap 函数执行实际的解除映射操作
    if (subunmap(va, len) == -1) return -1;

    return 0; // 成功时返回 0
}

// pgfault() 处理页面错误。当进程访问的虚拟地址在其地址空间中
// 没有映射时，会触发页面错误，此函数负责为该地址分配物理内存并建立映射。
// 参数：va - 触发页面错误的虚拟地址。
// 返回值：0 表示成功，-1 表示失败。
uint64
pgfault(uint64 va) {
    struct proc *p = myproc();  // 获取当前进程
    struct vma_t *v = 0;  // 初始化指向虚拟内存区域（VMA）的指针

    // 遍历进程的 VMA 数组，寻找包含触发页面错误地址 va 的 VMA
    for (int i = 0; i < MAXVMA; ++i) {
        if (p->vma[i].valid == 1 && p->vma[i].va <= va
            && va <= p->vma[i].va + p->vma[i].len) {
            v = &p->vma[i];  // 找到匹配的 VMA
            break;
        }
    }
    if (v == 0) return -1;  // 如果未找到匹配的 VMA，返回错误

    // 为触发页面错误的地址分配物理页，并将其映射到虚拟地址 va
    uint64 pa = (uint64)kalloc();  // 分配一页物理内存
    if (pa == 0) return -1;  // 内存分配失败，返回错误
    memset((char *)pa, 0, PGSIZE);  // 将分配的内存清零

    // 在进程的页表中建立 va 到 pa 的映射
    if (mappages(p->pagetable, va, PGSIZE, pa, v->prot|PTE_U) == -1) {
        kfree((void *)pa);  // 如果映射失败，释放已分配的物理内存
        return -1;
    }

    // 从文件中读取数据并写入映射的内存
    ilock(v->f->ip);  // 锁定文件的 inode
    int ret = readi(v->f->ip, 1, va, v->off + va - v->va, PGSIZE);
    if (ret == -1) {
        kfree((void *)pa);  // 读取失败时释放内存
        iunlock(v->f->ip);  // 释放 inode 锁
        return -1;
    }
    iunlock(v->f->ip);  // 释放 inode 锁

    return 0;  // 返回成功
}

// subunmap() 解除映射进程的虚拟内存区域。
// 参数：va - 要解除映射的虚拟地址，len - 解除映射的长度。
// 返回值：0 表示成功，-1 表示失败。
uint64
subunmap(uint64 va, int len) {
    if (len == 0) return 0;  // 如果长度为 0，不做任何操作，直接返回成功
    struct proc *p = myproc();  // 获取当前进程
    struct vma_t *v = 0;  // 初始化指向 VMA 的指针

    // 遍历进程的 VMA 数组，寻找包含要解除映射的地址 va 的 VMA
    for (int i = 0; i < MAXVMA; ++i) {
        if (p->vma[i].valid == 1 && p->vma[i].va <= va
            && va <= p->vma[i].va + p->vma[i].len) {
            v = &p->vma[i];  // 找到匹配的 VMA
            break;
        }
    }
    if (v == 0) return -1;  // 如果未找到匹配的 VMA，返回错误

    // 检查 va 对应的页表项
    va = PGROUNDDOWN(va);  // 将 va 向下对齐到页边界
    pte_t *pte = walk(p->pagetable, va, 0);  // 获取对应的页表项
    if (pte == 0) return -1;  // 如果页表项不存在，返回错误
    uint64 pteflags = PTE_FLAGS(*pte);  // 获取页表项的标志位
    uint64 pa;
    if ((pa = walkaddr(p->pagetable, va)) != 0) {
        // 如果已安装映射，检查是否需要写回到文件
        if (v->flags & MAP_SHARED) {
            if ((pteflags & PTE_D) == PTE_D) {
                int ret = filewrite(v->f, va, len);
                if (ret == -1) return -1;  // 写回失败，返回错误
            }
        }

        // 解除映射
        uvmunmap(p->pagetable, va, len / PGSIZE, 1);
    }

    // 根据是否部分或全部解除映射，调整 VMA 的起始地址和长度
    if (v->va == va && v->len == len) {
        // 完全解除映射
        v->len -= len;
    } else if (v->va == va && v->len != len) {
        // 部分解除映射，解除文件开头部分
        v->va += len;
        v->len -= len;
    } else if (v->va != va && v->len != len) {
        // 部分解除映射，解除文件末尾部分
        v->len -= len;
    }

    if (v->len == 0) {
        // 如果完全解除映射，清空 VMA 并关闭文件
        v->va = 0;
        v->valid = 0;
        fileclose(v->f);

        // 调整 curend，使其指向未映射区域的末端
        if (va == p->curend) {
            p->curend += len;
            for (uint64 unva = PGROUNDDOWN(p->curend);
                unva < MAXVA - 2 * PGSIZE; unva += PGSIZE) {
                // 遍历虚拟地址空间，调整 curend
                int i;
                for (i = 0; i < MAXVMA; ++i) {
                    if (p->vma[i].va == unva && p->vma[i].valid == 1)
                        break;
                }
                if (i == MAXVMA) p->curend += PGSIZE;  // 如果地址未被映射，继续调整 curend
                else break;  // 如果找到已映射地址，停止调整 curend
            }
        }
    }

    return 0;  // 返回成功
}
