#include <am.h>
#include <nemu.h>
#include <klib.h>

static AddrSpace kas = {};
static void* (*pgalloc_usr)(int) = NULL;
static void (*pgfree_usr)(void*) = NULL;
static int vme_enable = 0;

static Area segments[] = {      // Kernel memory mappings
  NEMU_PADDR_SPACE
};

#define USER_SPACE RANGE(0x40000000, 0x80000000)

static inline void set_satp(void *pdir) {
  uintptr_t mode = 1ul << (__riscv_xlen - 1);
  asm volatile("csrw satp, %0" : : "r"(mode | ((uintptr_t)pdir >> 12)));
}

static inline uintptr_t get_satp() {
  uintptr_t satp;
  asm volatile("csrr %0, satp" : "=r"(satp));
  return satp << 12;
}

bool vme_init(void* (*pgalloc_f)(int), void (*pgfree_f)(void*)) {
  pgalloc_usr = pgalloc_f;
  pgfree_usr = pgfree_f;

  kas.ptr = pgalloc_f(PGSIZE);

  int i;
  for (i = 0; i < LENGTH(segments); i ++) {
    void *va = segments[i].start;
    for (; va < segments[i].end; va += PGSIZE) {
      map(&kas, va, va, 0);
    }
  }

  set_satp(kas.ptr);
  vme_enable = 1;

  return true;
}

void protect(AddrSpace *as) {
  PTE *updir = (PTE*)(pgalloc_usr(PGSIZE));
  as->ptr = updir;
  as->area = USER_SPACE;
  as->pgsize = PGSIZE;
  // map kernel space
  memcpy(updir, kas.ptr, PGSIZE);
}

void unprotect(AddrSpace *as) {
}

void __am_get_cur_as(Context *c) {
  c->pdir = (vme_enable ? (void *)get_satp() : NULL);
}

void __am_switch(Context *c) {
  if (vme_enable && c->pdir != NULL) {
    set_satp(c->pdir);
  }
}

// shift a physical address to the right place for a PTE.
#define PA2PTE(pa) ((((unsigned long)pa) >> 12) << 10)

#define PTE2PA(pte) (((pte) >> 10) << 12)

#define PTE_FLAGS(pte) ((pte) & 0x3FF)

// extract the three 9-bit page table indices from a virtual address.
#define PGSHIFT         12
#define PXMASK          0x1FF // 9 bits
#define PXSHIFT(level)  (PGSHIFT+(9*(level)))
#define PX(level, va) ((((unsigned long) (va)) >> PXSHIFT(level)) & PXMASK)

typedef unsigned long pte_t;
typedef unsigned long *pagetable_t; // 512 PTEs

static pte_t *walk(pagetable_t pagetable, unsigned long va, int alloc)
{
  for(int level = 2; level > 0; level--) {
    pte_t *pte = &pagetable[PX(level, va)];
    if(*pte & PTE_V) {
      pagetable = (pagetable_t)PTE2PA(*pte);
    } else {
      if(!alloc || (pagetable = (pagetable_t)pgalloc_usr(PGSIZE)) == 0)
        return 0;
      memset(pagetable, 0, PGSIZE);
      *pte = PA2PTE(pagetable) | PTE_V;
    }
  }
  return &pagetable[PX(0, va)];
}

void map(AddrSpace *as, void *va, void *pa, int prot) {
  /* in kernel aspace, va=pa, so as->ptr is also pagetable's va */
  assert(as->ptr);

  unsigned long vaddr = (unsigned long)va, paddr = (unsigned long)pa;
  pte_t *pte = NULL;

  if(((vaddr % PGSIZE) != 0) || ((paddr % PGSIZE) != 0))
    panic("map: va/pa not aligned");

  if((pte = walk((pagetable_t)as->ptr, vaddr, 1)) == 0)
    panic("walk error");
  if(*pte & PTE_V)
    panic("mappages: remap");
  *pte = PA2PTE(paddr) | prot | PTE_V;

  if (va < as->area.start)
    as->area.start = va;
  if (va + PGSIZE < as->area.end)
    as->area.end = va + PGSIZE;
}

Context *ucontext(AddrSpace *as, Area kstack, void *entry) {
  Context *context = (Context *)(kstack.end - sizeof(Context));
  context->mepc = (uintptr_t)entry;
  context->gpr[1] = (uintptr_t)NULL; // ra
  // context->gpr[2] = (uintptr_t)context;
  context->mcause = 0xa00001800; // corresponding to difftest
  context->GPRx = (uintptr_t)heap.end; // set stack
  return context;
}
