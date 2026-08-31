#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "vm.h"

#include "shm.h"
#include "mbox.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  kexit(n);
  return 0; // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return kfork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return kwait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int t;
  int n;

  argint(0, &n);
  argint(1, &t);
  addr = myproc()->sz;

  if (t == SBRK_EAGER || n < 0)
  {
    if (growproc(n) < 0)
    {
      return -1;
    }
  }
  else
  {
    // Lazily allocate memory for this process: increase its memory
    // size but don't allocate memory. If the processes uses the
    // memory, vmfault() will allocate it.
    if (addr + n < addr)
      return -1;
    myproc()->sz += n;
  }
  return addr;
}

uint64
sys_pause(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if (n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n)
  {
    if (killed(myproc()))
    {
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

  argint(0, &pid);
  return kkill(pid);
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

//------------------------------------------------

uint64 sys_shm_create(void)
{
  int key;
  argint(0, &key);
  return shm_create(key);
}

uint64 sys_shm_get(void)
{
  int key;
  argint(0, &key);
  return shm_get(key);
}

uint64 sys_shm_close(void)
{
  int key;
  argint(0, &key);
  return shm_close(key);
}

uint64 sys_mbox_create(void)
{
  int key;
  argint(0, &key);
  return mbox_create(key);
}

uint64 sys_mbox_send(void)
{
  int id, msg;
  argint(0, &id);
  argint(1, &msg);
  return mbox_send(id, msg);
}

uint64 sys_mbox_recv(void)
{
  int id;
  uint64 addr;
  argint(0, &id);
  argaddr(1, &addr);
  int val;
  int r = mbox_recv(id, &val);
  if (r == 0)
  {
    if (copyout(myproc()->pagetable, addr, (char *)&val, sizeof(val)) < 0)
      return -1;
  }
  return r;
}
/*
// int getpagestat(int pid, struct pagestat *st)
uint64
sys_getpagestat(void)
{
  int pid;
  struct pagestat st;
  struct pagestat *ust;
  argint(0, &pid);
  argaddr(1, (uint64*)&ust);

  struct proc *p = find_proc_by_pid(pid); // implement or use existing helper; else scan proc table
  if(!p) return -1;

  // copy kernel struct to user-provided pointer
  st = p->pagestat;
  if(copyout(p->pagetable, (uint64)ust, (char*)&st, sizeof(st)) < 0) return -1;
  return 0;
}

// int dumpmru(void)
uint64
sys_dumpmru(void)
{
  dumpmru_to_console();
  return 0;
}
  */


