#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

struct spinlock tickslock;
uint ticks;

extern char trampoline[], uservec[];

// in kernelvec.S, calls kerneltrap().
void kernelvec();

extern int devintr();

void
trapinit(void)
{
  initlock(&tickslock, "time");
}

// set up to take exceptions and traps while in the kernel.
void
trapinithart(void)
{
  w_stvec((uint64)kernelvec);
}

//
// handle an interrupt, exception, or system call from user space.
// called from, and returns to, trampoline.S
// return value is user satp for trampoline.S to switch to.
//
/*uint64
usertrap(void)
{
  int which_dev = 0;

  if((r_sstatus() & SSTATUS_SPP) != 0)
    panic("usertrap: not from user mode");

  // send interrupts and exceptions to kerneltrap(),
  // since we're now in the kernel.
  w_stvec((uint64)kernelvec);

  struct proc *p = myproc();

  // save user program counter.
  p->trapframe->epc = r_sepc();

  if(r_scause() == 8){
    // system call

    if(killed(p))
      kexit(-1);

    // sepc points to the ecall instruction,
    // but we want to return to the next instruction.
    p->trapframe->epc += 4;

    // an interrupt will change sepc, scause, and sstatus,
    // so enable only now that we're done with those registers.
    intr_on();

    syscall();
  } else if((which_dev = devintr()) != 0){
    // handled device interrupt
  } else if(r_scause() == 15 || r_scause() == 13){
    // store (15) or load (13) page fault
    uint64 va = r_stval();
    int write = (r_scause() == 15); // write=true for store faults

    if(vmfault(p->pagetable, va, write) == 0){
      // vmfault failed to handle the fault -> kill process
      printf("usertrap(): vmfault failed (scause=0x%lx) pid=%d va=0x%lx\n",
             r_scause(), p->pid, va);
      setkilled(p);
    }
    // else vmfault handled it successfully
  } else {
    printf("usertrap(): unexpected scause 0x%lx pid=%d\n", r_scause(), p->pid);
    printf("            sepc=0x%lx stval=0x%lx\n", r_sepc(), r_stval());
    setkilled(p);
  }

  if(killed(p))
    kexit(-1);

  // give up the CPU if this is a timer interrupt.
  if(which_dev == 2)
    yield();

  prepare_return();

  // the user page table to switch to, for trampoline.S
  uint64 satp = MAKE_SATP(p->pagetable);

  // return to trampoline.S; satp value in a0.
  return satp;
}*/

/*uint64
usertrap(void)
{
  int which_dev = 0;

  if((r_sstatus() & SSTATUS_SPP) != 0)
    panic("usertrap: not from user mode");
  w_stvec((uint64)kernelvec);

  struct proc *p = myproc();

  p->trapframe->epc = r_sepc();

  if(r_scause() == 8){


    if(killed(p))
      kexit(-1);

    // sepc points to the ecall instruction,
    // but we want to return to the next instruction.
    p->trapframe->epc += 4;

    // an interrupt will change sepc, scause, and sstatus,
    // so enable only now that we're done with those registers.
    intr_on();

    syscall();
  } else if((which_dev = devintr()) != 0){
    // handled device interrupt
  } else if(r_scause() == 15 || r_scause() == 13){
    // store (15) or load (13) page fault
    uint64 va = r_stval();
    uint64 va_aligned = PGROUNDDOWN(va);
    int write = (r_scause() == 15); // write=true for store faults

    // count the page fault
    p->pagestat.page_faults += 1;

    // Call the existing vmfault handler. Keep exact semantics: your code
    // considered vmfault()==0 as failure (see original). We preserve that.
    if(vmfault(p->pagetable, va, write) == 0){
      // vmfault failed to handle the fault -> kill process
      printf("usertrap(): vmfault failed (scause=0x%lx) pid=%d va=0x%lx\n",
             r_scause(), p->pid, va);
      setkilled(p);
    } else {
      // vmfault succeeded and created/updated a mapping.
      // Update MRU *only* if the VA is now mapped to a user page.
      // This is non-invasive wrt COW: if COW handling happened inside vmfault,
      // the new mapping will be visible here and we'll mru_touch it.
      pte_t *pte = walk(p->pagetable, va_aligned, 0);
      if(pte && (*pte & PTE_V) && (*pte & PTE_U)) {
        // mru_touch is a no-op if we cannot allocate an MRU node.
        mru_touch(p, va_aligned);
      }
    }
    // else vmfault handled it successfully (or we marked killed)
  } else {
    printf("usertrap(): unexpected scause 0x%lx pid=%d\n", r_scause(), p->pid);
    printf("            sepc=0x%lx stval=0x%lx\n", r_sepc(), r_stval());
    setkilled(p);
  }

  if(killed(p))
    kexit(-1);

  // give up the CPU if this is a timer interrupt.
  if(which_dev == 2)
    yield();

  prepare_return();

  // the user page table to switch to, for trampoline.S
  uint64 satp = MAKE_SATP(p->pagetable);

  // return to trampoline.S; satp value in a0.
  return satp;
}
*/

uint64
usertrap(void)
{
  int which_dev = 0;

  if((r_sstatus() & SSTATUS_SPP) != 0)
    panic("usertrap: not from user mode");

  // switch traps to kernelvec
  w_stvec((uint64)kernelvec);

  struct proc *p = myproc();
  // save user program counter
  p->trapframe->epc = r_sepc();

  if(r_scause() == 8){
    // system call
    if(killed(p))
      kexit(-1);

    p->trapframe->epc += 4; // next instruction
    intr_on();
    syscall();
  } else if((which_dev = devintr()) != 0){
    // device interrupt handled
  } else if(r_scause() == 15 || r_scause() == 13){
    // store/load page fault
    uint64 va = r_stval();
    uint64 va_aligned = PGROUNDDOWN(va);
    int write = (r_scause() == 15); // store -> write=true

    // increment page fault count
    p->pagestat.page_faults += 1;

    // call vmfault to handle COW or lazy allocation
    uint64 pa = vmfault(p->pagetable, va, write);
    if(pa == 0){
      // page fault could not be handled -> kill process
      printf("usertrap(): vmfault failed (scause=0x%lx) pid=%d va=0x%lx\n",
             r_scause(), p->pid, va);
      setkilled(p);
    } else {
      // if mapping succeeded, optionally update MRU
      pte_t *pte = walk(p->pagetable, va_aligned, 0);
      if(pte && (*pte & PTE_V) && (*pte & PTE_U)){
        //mru_touch(p, va_aligned);
      }
    }
  } else {
    printf("usertrap(): unexpected scause 0x%lx pid=%d\n", r_scause(), p->pid);
    printf("            sepc=0x%lx stval=0x%lx\n", r_sepc(), r_stval());
    setkilled(p);
  }

  if(killed(p))
    kexit(-1);

  // yield on timer interrupt
  if(which_dev == 2)
    yield();

  prepare_return();

  return MAKE_SATP(p->pagetable);
}



//
// set up trapframe and control registers for a return to user space
//
void
prepare_return(void)
{
  struct proc *p = myproc();

  intr_off();

  uint64 trampoline_uservec = TRAMPOLINE + (uservec - trampoline);
  w_stvec(trampoline_uservec);

  p->trapframe->kernel_satp = r_satp();         // kernel page table
  p->trapframe->kernel_sp = p->kstack + PGSIZE; // process's kernel stack
  p->trapframe->kernel_trap = (uint64)usertrap;
  p->trapframe->kernel_hartid = r_tp();         // hartid for cpuid()

  // set S Previous Privilege mode to User.
  unsigned long x = r_sstatus();
  x &= ~SSTATUS_SPP; // clear SPP for user mode
  x |= SSTATUS_SPIE; // enable interrupts in user mode
  w_sstatus(x);

  // set user program counter.
  w_sepc(p->trapframe->epc);
}

void 
kerneltrap()
{
  int which_dev = 0;
  uint64 sepc = r_sepc();
  uint64 sstatus = r_sstatus();
  uint64 scause = r_scause();

  if((sstatus & SSTATUS_SPP) == 0)
    panic("kerneltrap: not from supervisor mode");
  if(intr_get() != 0)
    panic("kerneltrap: interrupts enabled");

  if((which_dev = devintr()) == 0){
    printf("scause=0x%lx sepc=0x%lx stval=0x%lx\n", scause, r_sepc(), r_stval());
    panic("kerneltrap");
  }

  if(which_dev == 2 && myproc() != 0)
    yield();

  w_sepc(sepc);
  w_sstatus(sstatus);
}

void
clockintr()
{
  if(cpuid() == 0){
    acquire(&tickslock);
    ticks++;
    wakeup(&ticks);
    release(&tickslock);
  }
  w_stimecmp(r_time() + 1000000);
}

int
devintr()
{
  uint64 scause = r_scause();

  if(scause == 0x8000000000000009L){
    int irq = plic_claim();
    if(irq == UART0_IRQ){
      uartintr();
    } else if(irq == VIRTIO0_IRQ){
      virtio_disk_intr();
    } else if(irq){
      printf("unexpected interrupt irq=%d\n", irq);
    }
    if(irq)
      plic_complete(irq);
    return 1;
  } else if(scause == 0x8000000000000005L){
    clockintr();
    return 2;
  } else {
    return 0;
  }
}
