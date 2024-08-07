#include <proc.h>
#include <elf.h>
#include <ramdisk.h>
#include <fs.h>

#ifdef __LP64__
# define Elf_Ehdr Elf64_Ehdr
# define Elf_Phdr Elf64_Phdr
#else
# define Elf_Ehdr Elf32_Ehdr
# define Elf_Phdr Elf32_Phdr
#endif

#if defined(__ISA_AM_NATIVE__)
# define EXPECT_TYPE EM_X86_64
#elif defined(__ISA_X86__)
# define EXPECT_TYPE EM_X86_64  // see /usr/include/elf.h to get the right type
#elif defined(__ISA_MIPS32__)
# define EXPECT_TYPE EM_MIPS
#elif defined(__ISA_RISCV64__) || defined(__ISA_RISCV32__)
# define EXPECT_TYPE EM_RISCV
#else
# error Unsupported ISA
#endif

static uintptr_t loader(PCB *pcb, const char *filename) {
  // read elf header
  int fd = fs_open(filename, 0, 0);
  Elf_Ehdr eh;

  assert(fd >= 0);
  fs_read(fd, &eh, sizeof(Elf_Ehdr));
  assert(*(uint32_t *)(eh.e_ident) == 0x464c457f); // convert to uint32 is little endian
  assert(eh.e_machine == EXPECT_TYPE);

  // load elf
  Elf_Phdr ph;
  for (int i = 0; i < eh.e_phnum; ++i) {
    fs_lseek(fd, eh.e_phoff + i * eh.e_phentsize, SEEK_SET);
    fs_read(fd, &ph, eh.e_phentsize);
    if (ph.p_type == PT_LOAD) {
      fs_lseek(fd, ph.p_offset, SEEK_SET);
      void *load_addr = NULL;
#ifdef HAS_VME
      void *start = (void *)(ph.p_vaddr / PGSIZE * PGSIZE);
      void *end = (void *)((ph.p_vaddr + ph.p_filesz + PGSIZE - 1) / PGSIZE * PGSIZE);
      int pages = (end - start) / PGSIZE;
      void *map_addr = new_page(pages);
      load_addr = map_addr + (ph.p_vaddr - (uintptr_t)start);
      for (int i = 0; i < pages; i++) {
        map(&pcb->as, start + i*PGSIZE, map_addr + i*PGSIZE, MMAP_WRITE | MMAP_READ | MMAP_EXECUTE);
      }
#else
      load_addr = (void *)ph.p_vaddr;
#endif
      fs_read(fd, load_addr, ph.p_filesz);
      memset(load_addr + ph.p_filesz, 0, ph.p_memsz - ph.p_filesz);
    }
  }

  fs_lseek(fd, 0, SEEK_SET);
  fs_close(fd);
  // return to begin addr (.text)
  return eh.e_entry;
}

void naive_uload(PCB *pcb, const char *filename) {
  uintptr_t entry = loader(pcb, filename);
  Log("Jump to entry = %p", entry);
  ((void(*)())entry) ();
}

void context_kload(PCB *pcb, void (*entry)(void *), void *arg) {
  Area kstack;
  kstack.start = pcb->stack;
  kstack.end = pcb->stack + STACK_SIZE;
  pcb->cp = kcontext(kstack, entry, arg);
}

void context_uload(PCB *pcb, char *file_name) {
  Area kstack;
  uintptr_t entry;

  protect(&pcb->as);
  kstack.start = pcb->stack;
  kstack.end = pcb->stack + STACK_SIZE;
  entry = loader(pcb, file_name);
  pcb->cp = ucontext(&pcb->as, kstack, (void *)entry);
}
