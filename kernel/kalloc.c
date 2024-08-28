// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
  char lockname[8]; 
} kmems[NCPU];

void
kinit()
{
  int i;
  for (i = 0; i < NCPU; ++i) {
    snprintf(kmems[i].lockname, 8, "kmem_%d", i);    // the name of the lock
    initlock(&kmems[i].lock, kmems[i].lockname);
  }
  //initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;
  int c; 

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;
  push_off();
  c = cpuid();
  pop_off();

  acquire(&kmems[c].lock);
  r->next = kmems[c].freelist;
  kmems[c].freelist = r;
  release(&kmems[c].lock);
}

struct run *steal(int cpu_id) {
    int i;
    int c = cpu_id; //当前 CPU 的 ID
    struct run *fast, *slow, *head; //用于操作链表的指针变量
    //检查传入的 cpu_id 是否与当前执行的 CPU ID 相匹配，若不匹配则引发 panic 错误
    if(cpu_id != cpuid()) {
        panic("steal");
    }
    //遍历其他 CPU 的空闲内存列表进行内存块的“偷窃”
    for (i = 1; i < NCPU; ++i) {
        // 循环遍历所有 CPU，按顺序检查各个 CPU 的 freelist
        if (++c == NCPU) {
            c = 0; // 如果已经检查完最后一个 CPU（即 NCPU-1），则从第一个 CPU（0）重新开始
        }
        // 获取当前 CPU 空闲内存链表的锁，以确保线程安全
        acquire(&kmems[c].lock);
        // 检查当前 CPU 是否有空闲内存块
        if (kmems[c].freelist) {
            // 初始化链表操作的指针
            slow = head = kmems[c].freelist;
            fast = slow->next;
            // 使用快慢指针算法找到链表的中点，目标是将链表拆分
            while (fast) {
                fast = fast->next; // fast 每次前进两步
                if (fast) {
                    slow = slow->next; // slow 每次前进一步
                    fast = fast->next; // fast 再前进两步
                }
            }
            // 将当前 CPU 的空闲链表更新为链表的后半部分
            kmems[c].freelist = slow->next;
            // 释放当前 CPU 空闲内存链表的锁
            release(&kmems[c].lock);
            // 将拆分的链表的前半部分与后半部分断开连接
            slow->next = 0;
            // 返回从当前 CPU “偷”到的内存块链表的头部
            return head;
        }
        // 如果当前 CPU 没有可用的内存块，释放锁并继续检查下一个 CPU
        release(&kmems[c].lock);
    }
    // 如果所有 CPU 都没有可用的内存块，则返回 NULL（表示没有偷到任何内存块）
    return 0;
}






// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;
  int c;
  push_off();
  c = cpuid();
  pop_off();

  acquire(&kmems[c].lock);
  r = kmems[c].freelist;
  if(r)
    kmems[c].freelist = r->next;
  release(&kmems[c].lock);

  if(!r && (r = steal(c))) {
    // 加锁修改当前CPU空闲物理页链表
    acquire(&kmems[c].lock);
    kmems[c].freelist = r->next;
    release(&kmems[c].lock);
  }

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}


