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
} kmem;

struct {
  struct spinlock lock;
  int refcount[PHYSTOP / PGSIZE];
} ref;

void ref_inc(uint64 pa);
void ref_dec(uint64 pa);
int  ref_get(uint64 pa);

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&ref.lock, "ref");

  // Initialize refcounts to 0
  for (uint64 i = 0; i < (PHYSTOP / PGSIZE); i++) {
    ref.refcount[i] = 0;
  }

  // Add the physical memory range to the free list.
  // freerange will call kfree on each page; before calling kfree we
  // set refcount to 1 so that kfree's decrement logic works properly.
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE) {
    // Set refcount to 1 for this page so kfree() will decrement it to 0 and add to free list.
    uint64 pa = (uint64)p;
    uint64 idx = pa / PGSIZE;
    acquire(&ref.lock);
    ref.refcount[idx] = 1;
    release(&ref.lock);

    kfree(p);
  }
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  uint64 pageidx = (uint64)pa / PGSIZE;

  // Decrement reference count; only free when it reaches 0.
  acquire(&ref.lock);
  if(ref.refcount[pageidx] <= 0){
    panic("kfree: freeing page with non-positive refcount");
  }
  ref.refcount[pageidx] -= 1;
  int now = ref.refcount[pageidx];
  release(&ref.lock);

  if(now > 0){
    // still referenced somewhere else; don't add to free list
    return;
  }

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(!r)
    return 0;

  // fill with junk
  memset((char*)r, 5, PGSIZE);

  // initialize refcount to 1 for this page
  uint64 pa = (uint64)r;
  uint64 idx = pa / PGSIZE;
  acquire(&ref.lock);
  if(ref.refcount[idx] != 0){
    // Unexpected: page allocated was not at zero refcount.
    // We still set it to 1 to be safe.
  }
  ref.refcount[idx] = 1;
  release(&ref.lock);

  return (void*)r;
}

// Increment reference count for a physical page (pa must be page-aligned)
void
ref_inc(uint64 pa)
{
  if(pa % PGSIZE)
    panic("ref_inc: pa not page aligned");
  uint64 idx = pa / PGSIZE;
  acquire(&ref.lock);
  if(ref.refcount[idx] < 0)
    panic("ref_inc: negative refcount");
  ref.refcount[idx] += 1;
  release(&ref.lock);
}

// Decrement reference count for a physical page.
// If it reaches zero, the page is returned to the free list.
void
ref_dec(uint64 pa)
{
  if(pa % PGSIZE)
    panic("ref_dec: pa not page aligned");
  uint64 idx = pa / PGSIZE;

  acquire(&ref.lock);
  if(ref.refcount[idx] <= 0)
    panic("ref_dec: decrementing non-positive refcount");
  ref.refcount[idx] -= 1;
  int now = ref.refcount[idx];
  release(&ref.lock);

  if(now > 0)
    return;

  // Now free the page (same logic as in kfree when refcount reaches 0)
  // Fill with junk to catch dangling refs.
  memset((void*)pa, 1, PGSIZE);

  struct run *r = (struct run*)pa;
  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Return current refcount for a page (pa must be page-aligned)
int
ref_get(uint64 pa)
{
  if(pa % PGSIZE)
    panic("ref_get: pa not page aligned");
  uint64 idx = pa / PGSIZE;
  acquire(&ref.lock);
  int v = ref.refcount[idx];
  release(&ref.lock);
  return v;
}


// in kernel/kalloc.c (add near top with other helpers)
int
free_pages_available(void)
{
  return kmem.freelist != 0;
}
