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
  uintptr_t a[4];
  a[0] = c->GPR1;

  switch (a[0]) {
    case SYS_exit: halt(0); break;
    case SYS_yield: yield(); c->GPRx = 0; break;
    default: panic("Unhandled syscall ID = %d", a[0]);
  }
}
