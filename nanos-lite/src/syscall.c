#include <common.h>
#include <fs.h>
#include "syscall.h"

struct timeval{
  long tv_sec;
  long tv_usec;
};

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
  Log("syscall %d with args 0x%x 0x%x 0x%x", a[0], a[1], a[2], a[3]);

  switch (a[0]) {
    case SYS_open: Log("opening %s", (char *)a[1]); break;
    case SYS_read: Log("read file %s %d bytes, from offset %d", fd2fname((int)a[1]), (size_t)a[3], fd2foff((int)a[1])); break;
    case SYS_write: Log("write file %s %d bytes, from offset %d", fd2fname((int)a[1]), (size_t)a[3], fd2foff((int)a[1])); break;
    case SYS_lseek: Log("change file offset %s", fd2fname((int)a[1])); break;
    case SYS_close: Log("close file %s", fd2fname((int)a[1])); break;
  }
#endif

  switch (a[0]) {
    case SYS_exit: halt(0); break;
    case SYS_yield: yield(); c->GPRx = 0; break;
    case SYS_open: c->GPRx = fs_open((char *)a[1], (int)a[2], (int)a[3]); break;
    case SYS_read: c->GPRx = fs_read((int)a[1], (void *)a[2], (size_t)a[3]); break;
    case SYS_write: c->GPRx = fs_write((int)a[1], (void *)a[2], (size_t)a[3]); break;
    case SYS_lseek: c->GPRx = fs_lseek((int)a[1], (size_t)a[2], (int)a[3]); break;
    case SYS_close: c->GPRx = fs_close((int)a[1]); break;
    case SYS_brk: c->GPRx = 0; break;
    case SYS_gettimeofday:
      ((struct timeval *)c->GPR2)->tv_sec = (io_read(AM_TIMER_UPTIME).us / 1000000);
      ((struct timeval *)c->GPR2)->tv_usec = (io_read(AM_TIMER_UPTIME).us % 1000000);
      c->GPRx = 0; break;
    default: panic("Unhandled syscall ID = %d", a[0]);
  }
}
