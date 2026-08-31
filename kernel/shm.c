#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

#define SHM_MAX 16

struct
{
  struct spinlock lock;
  char *pages[SHM_MAX]; // Store the actual kalloc'd pages
  int keys[SHM_MAX];
  int refcnt[SHM_MAX];
} shm;

void shminit(void)
{
  initlock(&shm.lock, "shm");
  for (int i = 0; i < SHM_MAX; i++)
  {
    shm.pages[i] = 0;
    shm.keys[i] = -1;
    shm.refcnt[i] = 0;
  }
  //printf("shminit: done, SHM_MAX=%d\n", SHM_MAX);
}

int shm_create(int key)
{
  acquire(&shm.lock);

  // Check if key already exists
  for (int i = 0; i < SHM_MAX; i++)
  {
    if (shm.keys[i] == key && shm.pages[i] != 0)
    {
      // shm.refcnt[i]++;
      printf("shm_create: key=%d exists at idx=%d refcnt=%d\n", key, i, shm.refcnt[i]);
      release(&shm.lock);
      return i;
    }
  }

  // Find empty slot and allocate page
  for (int i = 0; i < SHM_MAX; i++)
  {
    if (shm.keys[i] == -1)
    {
      char *page = kalloc();
      if (page == 0)
      {
        printf("shm_create: kalloc failed for key=%d\n", key);
        release(&shm.lock);
        return -1;
      }
      memset(page, 0, PGSIZE);

      shm.pages[i] = page;
      shm.keys[i] = key;
      shm.refcnt[i] = 1;

      printf("shm_create: key=%d idx=%d page=%p\n", key, i, page);
      release(&shm.lock);
      return i;
    }
  }
  printf("shm_create: table full for key=%d\n", key);
  release(&shm.lock);
  return -1;
}

uint64
shm_get(int key)
{
  struct proc *p = myproc();
  if (p == 0)
  {
    printf("shm_get: no current process\n");
    return 0;
  }
  acquire(&shm.lock);
  for (int i = 0; i < SHM_MAX; i++)
  {
    if (shm.keys[i] == key && shm.pages[i] != 0)
    {
      char *kva = shm.pages[i]; // sanity checks
      if (((uint64)kva & (PGSIZE - 1)) != 0)
      {
        printf("shm_get: ERROR - kernel page not page-aligned %p\n", kva);

        release(&shm.lock);
        return 0;
      }
      uint64 pa = (uint64)kva; // or use KV2PA(kva)
      uint64 va = PGROUNDUP(p->sz);
      if (va < PGSIZE)
        va = PGSIZE; // avoid mapping at 0

      printf("shm_get: pid=%d key=%d idx=%d kva=%p pa=0x%lx va=0x%lx\n",
             p->pid, key, i, kva, pa, va);
      // check pa alignment
      if (pa & (PGSIZE - 1))
      {
        printf("shm_get: ERROR - pa not page-aligned pa=0x%lx\n", pa);
        release(&shm.lock);
        return 0;
      }

      int flags = PTE_V | PTE_R | PTE_W | PTE_U;
      if (mappages(p->pagetable, va, PGSIZE, pa, flags) < 0)
      {
        printf("shm_get: mappages failed for pid=%d va=0x%lx pa=0x%lx\n",
               p->pid, va, pa);
        release(&shm.lock);
        return 0;
      }

      // update process size only if we are extending it
      if (p->sz < va + PGSIZE)
        p->sz = va + PGSIZE;
      shm.refcnt[i]++; // if you want to count per-get
      printf("shm_get: success pid=%d va=0x%lx refcnt=%d\n",
             p->pid, va, shm.refcnt[i]);

      release(&shm.lock);
      return va;
    }
  }
  printf("shm_get: key=%d not found\n", key);
  release(&shm.lock);
  return 0;
}

int shm_close(int key)
{
  acquire(&shm.lock);

  for (int i = 0; i < SHM_MAX; i++)
  {
    if (shm.keys[i] == key && shm.pages[i] != 0)
    {
      shm.refcnt[i]--;
      printf("shm_close: key=%d refcnt now=%d\n", key, shm.refcnt[i]);

      if (shm.refcnt[i] <= 0)
      {
        printf("shm_close: freeing key=%d page=%p\n", key, shm.pages[i]);
        kfree(shm.pages[i]);
        shm.pages[i] = 0;
        shm.keys[i] = -1;
        shm.refcnt[i] = 0;
      }

      release(&shm.lock);
      return 0;
    }
  }
  printf("shm_close: key=%d not found\n", key);
  release(&shm.lock);
  return -1;
}