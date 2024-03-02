#include <common.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <elf.h>
#include <gelf.h>
#include <libelf.h>
#include <isa.h>

typedef struct func func;

struct func {
  char *func_name;
  word_t func_addr;
  word_t func_size;
  func *next;
};

static func *funcs;

void init_ftrace(char *elf_file) {
  int fd;
  Elf *elf;
  Elf_Scn *section = NULL;
  GElf_Shdr shdr;
  Elf_Data *data;

  Log("The elf file reference to ftrace is %s", elf_file);

  assert(elf_version(EV_CURRENT) != EV_NONE);
  assert((fd = open(elf_file, O_RDONLY)) > 0);
  assert((elf = elf_begin(fd, ELF_C_READ, NULL)) != NULL);

  while ((section = elf_nextscn(elf, section)) != NULL) {
    /* get section header info */
    assert(gelf_getshdr(section, &shdr) == &shdr);

    /* find symtab */
    if (shdr.sh_type == SHT_SYMTAB) {
      assert((data = elf_getdata(section, NULL)) != NULL);

      size_t num_symbols = shdr.sh_size / shdr.sh_entsize;

      GElf_Sym sym;
      for (size_t i = 0; i < num_symbols; i++) {
        assert(gelf_getsym(data, i, &sym) == &sym);

        /* find FUNC symbol */
        if (GELF_ST_TYPE(sym.st_info) == STT_FUNC) {
          func *new_func = malloc(sizeof(func));
          char *func_name = elf_strptr(elf, shdr.sh_link, sym.st_name);
          char *name = malloc(strlen(func_name));
          assert((name != NULL) && (new_func != NULL));

          /* get FUNC name */
          sprintf(name, "%s", func_name);
          new_func->func_name = name;
          new_func->next = funcs;
          new_func->func_addr = sym.st_value;
          new_func->func_size = sym.st_size;
          funcs = new_func;
        }
      }
    }
  }

#define DEBUG_FTRACE 0
#if DEBUG_FTRACE
  func *iter = funcs;
  while (iter)
  {
    printf("func: %s [" FMT_WORD "-" FMT_WORD "]\n",
      iter->func_name, iter->func_addr, iter->func_addr + iter->func_size);
    iter = iter->next;
  }
#endif

  elf_end(elf);
  close(fd);
}

static func* find_entry(word_t addr){
  func *iter = funcs;
  while (iter) {
    if (addr >= iter->func_addr && addr < iter->func_addr + iter->func_size)
      return iter;
    iter = iter->next;
  }

  return NULL;
}

static int tabs = 0;    // for output format
static int last_op = RET;   // for deciding tabs

void print_ftrace(int op_type, word_t jump_to) {
  if (op_type == RET && last_op == RET) --tabs;
  if (op_type == CALL && last_op == CALL) ++tabs;

  printf(FMT_WORD ":", cpu.pc);
  for (int i = 0; i < tabs; ++i)
    printf(" ");

  func *jumpto_ent = find_entry(jump_to);
  func *now_ent = find_entry(cpu.pc);
  char *jumpto_func = jumpto_ent ? jumpto_ent->func_name : "???";
  char *now_func = now_ent ? now_ent->func_name : "???";

  switch (op_type){
    case CALL:
      printf("call %s in %s\n", jumpto_func, now_func);
      last_op = CALL;
      break;
    case RET:
      printf("ret to %s from %s\n", jumpto_func, now_func);
      last_op = RET;
      break;
  }
}
