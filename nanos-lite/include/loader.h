#ifndef __LOADER_H__
#define __LOADER_H__

#include <common.h>
#include <proc.h>

void naive_uload(PCB *pcb, const char *filename);
void context_kload(PCB *pcb, void (*entry)(void *), void *arg);

#endif
