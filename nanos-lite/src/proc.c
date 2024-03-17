#include <proc.h>
#include <loader.h>

#define MAX_NR_PROC 4

static PCB pcb[MAX_NR_PROC] __attribute__((used)) = {};
static PCB pcb_boot = {};
static int current_pcb_id; // -1 is pcb_boot
PCB *current = NULL;

void switch_boot_pcb() {
  current_pcb_id = -1;
  current = &pcb_boot;
}

void hello_fun(void *arg) {
  int j = 1;
  while (1) {
    Log("Hello World from Nanos-lite with arg '%s' for the %dth time!", (char *)arg, j);
    j ++;
    yield();
  }
}

void init_proc() {
  Log("Initializing processes...");
  context_kload(&pcb[0], hello_fun, "kernel");
  context_uload(&pcb[1], "/bin/hello");
  switch_boot_pcb();
}

Context* schedule(Context *prev) {
  int prev_pcb_id = current_pcb_id;

  // pcb_boot is kernel context and can be saved here when schedule for the first time.
  current->cp = prev;

  for (current_pcb_id++; current_pcb_id < MAX_NR_PROC; current_pcb_id++) {
    if (pcb[current_pcb_id].cp != NULL) {
      current = &pcb[current_pcb_id];
      return pcb[current_pcb_id].cp;
    }
  }

  for (current_pcb_id = 0; current_pcb_id < prev_pcb_id; current_pcb_id++) {
    if (pcb[current_pcb_id].cp != NULL) {
      current = &pcb[current_pcb_id];
      return pcb[current_pcb_id].cp;
    }
  }
  return NULL;
}
