#include <am.h>
#include <riscv/riscv.h>
#include <klib.h>

/* according manual about MCAUSE csr */
#define ECALL_FROM_M_MODE 11
#define ECALL_FROM_M_MODE 11
#define INTR_YIELD -1
#define IRQ_TIMER 0x8000000000000007  // for riscv64

static Context* (*user_handler)(Event, Context*) = NULL;

void __am_get_cur_as(Context *c);
void __am_switch(Context *c);

static uintptr_t mscratch;
Context* __am_irq_handle(Context *c) {
  asm volatile("csrr %0, mscratch" : "=r"(mscratch));
  c->np = (mscratch == 0) ? KERNEL_MODE : USER_MODE;
  if (c->np == USER_MODE) {
    c->gpr[2] = mscratch - sizeof(Context);
  } else {
    c->gpr[2] = (uintptr_t)c;
  }

  __am_get_cur_as(c);

  if (user_handler) {
    Event ev = {0};
    switch (c->mcause) {
      case ECALL_FROM_M_MODE:
        switch(c->GPR1) {
          case INTR_YIELD:
            ev.event = EVENT_YIELD;
            break;
          default:
            ev.event = EVENT_SYSCALL;
            break;
        }
        c->mepc += 4; break;
      case IRQ_TIMER:
        ev.event = EVENT_IRQ_TIMER;
        break;
      default: ev.event = EVENT_ERROR; break;
    }

    c = user_handler(ev, c);
    assert(c != NULL);
  }

  __am_switch(c);

  if (c->np == USER_MODE) {
    mscratch = (uintptr_t)c; // let ksp be the new start of kernel stack
    asm volatile("csrw mscratch, %0" :: "r"(mscratch));
  }

  return c;
}

extern void __am_asm_trap(void);

bool cte_init(Context*(*handler)(Event, Context*)) {
  // initialize exception entry
  asm volatile("csrw mtvec, %0" : : "r"(__am_asm_trap));

  // register event handler
  user_handler = handler;

  return true;
}

Context *kcontext(Area kstack, void (*entry)(void *), void *arg) {
  Context *context = (Context *)(kstack.end - sizeof(Context));
  context->mepc = (uintptr_t)entry;
  context->gpr[10] = (uintptr_t)arg; // a0
  context->gpr[1] = (uintptr_t)NULL; // ra
  // context->gpr[2] = (uintptr_t)context;
  context->mcause = 0xa00001800; // corresponding to difftest
  context->mcause |= (1 << 7);   // set MPIE
  return context;
}

void yield() {
#ifdef __riscv_e
  asm volatile("li a5, -1; ecall");
#else
  asm volatile("li a7, -1; ecall");
#endif
}

bool ienabled() {
  return false;
}

void iset(bool enable) {
}
