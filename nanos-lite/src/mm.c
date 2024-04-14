#include <memory.h>
// we can't use am's malloc/free because of alignment
// am's malloc/free is used in am-kernels
static void *pf = NULL;

#ifdef HAS_VME
static void* pg_alloc(int n) {
  assert(pf);
  assert(n % PGSIZE == 0);

  void *ret = pf;

  pf += n;

  if (pf > heap.end) {
    panic("NOMEM!!");
  }
  return ret;
}
#endif

void* new_page(size_t nr_page) {
  return pg_alloc(nr_page * PGSIZE);
}

void free_page(void *p) {
}

/* The brk() system call handler. */
int mm_brk(uintptr_t brk) {
  return 0;
}

void init_mm() {
  pf = (void *)ROUNDUP(heap.start, PGSIZE);
  Log("free physical pages starting from %p", pf);

#ifdef HAS_VME
  vme_init(pg_alloc, free_page);
#endif
}
