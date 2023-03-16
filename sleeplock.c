// Sleeping locks

#include "types.h"
#include "defs.h"
#include "param.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"
#include "sleeplock.h"
// 由于睡眠锁不会关中断，因此不能用在中断处理程序中！

void
initsleeplock(struct sleeplock *lk, char *name)
{
  initlock(&lk->lk, "sleep lock");
  lk->name = name;
  lk->locked = 0;
  lk->pid = 0;
}

// 获取睡眠锁
void
acquiresleep(struct sleeplock *lk)
{
  acquire(&lk->lk); // 首先获取睡眠锁中的自旋锁
  while (lk->locked) { // 此时对locked字段的存取操作就是原子的了
    sleep(lk, &lk->lk); // 如果已经被其他进程获取，则本线程投入睡眠。 sleep函数，在将进程投入睡眠前，会对自旋锁解锁
  }
  lk->locked = 1;  // 获取睡眠锁
  lk->pid = myproc()->pid; // 用于debug
  release(&lk->lk); // 释放自旋锁
}

// 释放睡眠锁
void
releasesleep(struct sleeplock *lk)
{
  acquire(&lk->lk); // 首先获取睡眠锁中的自旋锁
  lk->locked = 0;   // 释放睡眠锁
  lk->pid = 0;
  wakeup(lk);       // 唤醒等待lk的进程
  release(&lk->lk); // 释放自旋锁
} 

int
holdingsleep(struct sleeplock *lk)
{
  int r;
  
  acquire(&lk->lk);
  r = lk->locked && (lk->pid == myproc()->pid);
  release(&lk->lk);
  return r;
}



