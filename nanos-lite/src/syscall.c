#include <common.h>
#include "syscall.h"

/*
 * GPR1: syscall number
 * GPR2: 1st param
 * GPR3: 2nd param
 * GPR4: 3rd param
 * GPRx: syscall return val
 */
void do_syscall(Context *c) {
  /* avoid overlay syscall param when calling function ?? */
  uintptr_t a[4];
  a[0] = c->GPR1;
  a[1] = c->GPR2;
  a[2] = c->GPR3;
  a[3] = c->GPR4;

#ifdef CONFIG_STRACE
  Log("syscall %d with args 0x%x 0x%x 0x%x\n", a[0], a[1], a[2], a[3]);
#endif

  switch (a[0]) {
    case SYS_exit: halt(0); break;
    case SYS_yield: yield(); c->GPRx = 0; break;
    case SYS_write: char *cbuf = (char *)a[2]; for (uintptr_t i = 0; i < a[3]; i++) putch(*(cbuf + i)); c->GPRx = a[3]; break;
    case SYS_brk: c->GPRx = 0; break;
    default: panic("Unhandled syscall ID = %d", a[0]);
  }
}
