/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"
#include <memory/vaddr.h>

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}


static int cmd_q(char *args) {
  extern NEMUState nemu_state;
  nemu_state.state = NEMU_QUIT;
  return -1;
}

static int cmd_si(char *args) {
  uint64_t step;

  if (args == NULL)
    step = 1;
  else
    step = (uint64_t)atoll(args);

  cpu_exec(step);
  return 0;
}

static int cmd_info(char *args) {
  // skip leading blanks
  args = strtok(args, " ");
  if (args == NULL) {
    printf("Use \"info r\" to print reg state\n");
    return 0;
  }

  if (strcmp(args, "r") == 0)
    isa_reg_display();
  else
    printf("unknown option %s\n", args);
  return 0;
}

static int cmd_x(char *args) {
  int count;
  vaddr_t addr;

  // skip leading blanks
  args = strtok(args, " ");

  if (args == NULL) {
    printf("Use \"x N EXPR\" to output N consecutive 4 bytes in hex, EXPR is the starting memory addr\n");
    return 0;
  }

  count = atoi(args);

  args = strtok(NULL, " ");

  if (args == NULL) {
    printf("Use \"x N EXPR\" to output N consecutive 4 bytes in hex, EXPR is the starting memory addr\n");
    return 0;
  }
  addr = (vaddr_t)strtol(args, NULL, 16);

  for (int i = 0; i < count; i++) {
    if (i%4 == 0)
      printf(FMT_PADDR ":\t", addr + i*4);
    printf("0x%08x\t", vaddr_read(addr + i*4, 4));
    if (i%4 == 3)
      printf("\n");
  }

  if (count%4 != 0)
    printf("\n");
  return 0;
}

static int cmd_p(char *args) {
  if (args == NULL)
    return 0;

  bool success;
  word_t val = expr(args, &success);

  if (success)
    printf(FMT_WORD "\t" FMT_WORD_D "\n", val, val);
  else
    printf("calculate failed\n");

  return 0;
}

static int cmd_help(char *args);

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },

  /* TODO: Add more commands */
  { "si", "Executing N instructions. Default N is 1.", cmd_si },
  { "info", "Use \"info r\" to print reg state", cmd_info },
  { "x", "Use \"x N EXPR\" to output N consecutive 4 bytes in hex, EXPR is the starting memory addr", cmd_x },
  { "p", "Use \"p EXPR\" to output value of EXPR", cmd_p },
};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
