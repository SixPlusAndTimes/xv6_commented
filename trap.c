#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"

// Interrupt descriptor table (shared by all CPUs).
struct gatedesc idt[256];
extern uint vectors[];  // in vectors.S: array of 256 entry pointers
struct spinlock tickslock;
uint ticks;
int mappages(pde_t *pgdir, void *va, uint size, uint pa, int perm);
void
tvinit(void)
{
  int i;

  for(i = 0; i < 256; i++)
    SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0); // 设置为中断门，在进入内核时关中断。
  SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);// 只有系统调用的DPL被设置成为 DPL_USER； 
  //通过陷进门实现系统调用，进入内核时不关闭中断，这直接导致了与JOS的一个重要不同 ： xv6在执行进程切换时，同时切换内核栈；但是JOS不需要，因为JOS保证一个进程处于内核态时它的IF标志为0，不能被打断

  initlock(&tickslock, "time");
}

void
idtinit(void)
{
  lidt(idt, sizeof(idt));
}

//PAGEBREAK: 41
void
trap(struct trapframe *tf)
{
  //检测traono看看，程序为什么会trap

  // 程序因系统调用而trap
  if(tf->trapno == T_SYSCALL){
    if(myproc()->killed)
      exit();
    myproc()->tf = tf; // 设置trapframe
    syscall();
    if(myproc()->killed)
      exit();// 不会return
    return;
  }

  switch(tf->trapno){
  case T_IRQ0 + IRQ_TIMER:
  // 时钟中断处理程序
    if(cpuid() == 0){
      acquire(&tickslock);
      ticks++;
      wakeup(&ticks); // 这可能导致中断返回至另一个进程
      release(&tickslock);
    }
    //hw cpu alarm 
    if(myproc() != 0 && (tf->cs & 3) == 3 && (myproc()->alarmticks != -1)) {
        myproc()->alarmticksLeft--;
      // cprintf("ticks interrupt alarmticksLeft--, = %d\n", myproc()->alarmticksLeft);

        if (myproc()->alarmticksLeft == 0) {
          
          // cprintf("myproc()->alarmticksLeft == 0\n");
          // 下面修改trapframe，中断返回时能够返回到 alarmhandler中；alarmhandler执行完后又返回原用户程序继续执行
          // myproc()->tf->esp -= 4; // 错误 proc中没有保存此次的trapframe，但是JOS在trap()中有这样的操作
          // 伪造用户程序call hander的假象
          tf->esp -= 4; // 压栈
          *(uint*)(tf->esp) = tf->eip; // handler()的返回地址是原用户程序的返回地址
          tf->eip = (uint)(myproc()->alarmhandler); // 中断程序的返回地址修改为，hanler()的入口
          myproc()->alarmticksLeft = myproc()->alarmticks; // 重置ticks
        }
    }
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE:
    ideintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE+1:
    // Bochs generates spurious IDE1 interrupts.
    break;
  case T_IRQ0 + IRQ_KBD:
    kbdintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_COM1:
    uartintr();
    lapiceoi();
    break;
  case T_IRQ0 + 7:
  case T_IRQ0 + IRQ_SPURIOUS:
    cprintf("cpu%d: spurious interrupt at %x:%x\n",
            cpuid(), tf->cs, tf->eip);
    lapiceoi();
    break;

  //PAGEBREAK: 13
  default:
    if(myproc() == 0 || (tf->cs&3) == 0){
      // In kernel, it must be our mistake.
      cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
              tf->trapno, cpuid(), tf->eip, rcr2());
      panic("trap");
    }
    // In user space, assume process misbehaved.
    // 处理pagefault
    if (tf->trapno == T_PGFLT) {
      struct proc *curproc = myproc();
      uint addr = PGROUNDDOWN(rcr2());
      // uint sz = curproc->sz;
      char* mem = kalloc(); // 分配物理页面
      if(mem == 0){
        cprintf("allocuvm out of memory\n");
        kfree(mem);
        myproc()->killed = 1;
      } else {
        memset(mem, 0, PGSIZE);
        if(mappages(curproc->pgdir, (char*)addr, PGSIZE, V2P(mem), PTE_W|PTE_U) < 0){
          cprintf("allocuvm out of memory (2)\n");
          kfree(mem);
          myproc()->killed = 1;
        }
        return; // 从异常返回时再执行一遍引起异常的指令
      }
    }
    cprintf("pid %d %s: trap %d err %d on cpu %d "
            "eip 0x%x addr 0x%x--kill proc\n",
            myproc()->pid, myproc()->name, tf->trapno,
            tf->err, cpuid(), tf->eip, rcr2());
    myproc()->killed = 1;
  }

  // Force process exit if it has been killed and is in user space.
  // (If it is still executing in the kernel, let it keep running
  // until it gets to the regular system call return.)
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();

  // Force process to give up CPU on clock tick.
  // If interrupts were on while locks held, would need to check nlock.
  if(myproc() && myproc()->state == RUNNING &&
     tf->trapno == T_IRQ0+IRQ_TIMER)
    yield();

  // Check if the process has been killed since we yielded
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();
}
