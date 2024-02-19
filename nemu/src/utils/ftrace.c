#include <common.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <elf.h>
#include <gelf.h>
#include <libelf.h>

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

    elf_end(elf);
    close(fd);
}
