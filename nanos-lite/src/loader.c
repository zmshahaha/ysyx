#include <proc.h>
#include <elf.h>

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

extern size_t ramdisk_read(void *buf, size_t offset, size_t len);

static uintptr_t loader(PCB *pcb, const char *filename) {
  // read elf header
  Elf_Ehdr eh;
  ramdisk_read(&eh, 0, sizeof(Elf_Ehdr));
  assert(*(uint32_t *)(eh.e_ident) == 0x464c457f); // convert to uint32 is little endian
  assert(eh.e_machine == EXPECT_TYPE);

  // load elf
  Elf_Phdr ph;
  for (int i = 0; i < eh.e_phnum; ++i) {
    ramdisk_read(&ph, eh.e_phoff + i * eh.e_phentsize, eh.e_phentsize);
    if (ph.p_type == PT_LOAD) {
      ramdisk_read((void *)ph.p_vaddr, ph.p_offset, ph.p_filesz);
      memset((void *)(ph.p_vaddr + ph.p_filesz), 0, ph.p_memsz - ph.p_filesz);
    }
  }

  // return to begin addr (.text)
  return eh.e_entry;
}

void naive_uload(PCB *pcb, const char *filename) {
  uintptr_t entry = loader(pcb, filename);
  Log("Jump to entry = %p", entry);
  ((void(*)())entry) ();
}

