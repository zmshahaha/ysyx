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
#include "../local-include/reg.h"
#include <memory/vaddr.h>
#include <memory/paddr.h>

int isa_mmu_check(vaddr_t vaddr, int len, int type) {
  if (csr(SATP) & (~(word_t)0)) {
    return MMU_TRANSLATE;
  } else {
    return MMU_DIRECT;
  }
}

typedef word_t pte_t;
typedef word_t satp_t;

#ifdef CONFIG_RV64
union rv64_satp
{
  satp_t satp;
  struct {
    satp_t ppn      : 44;
    satp_t asid     : 16;
    satp_t mode     : 4;
  } rv64_satp;
};

union sv39_va
{
  vaddr_t va;
  struct {
    vaddr_t pg_offset: 12;
    vaddr_t vpn0     : 9; // virtual page number
    vaddr_t vpn1     : 9;
    vaddr_t vpn2     : 9;
    vaddr_t reserved : 25;
  } sv39_va;
};

union sv39_pa
{
  paddr_t pa;
  struct {
    paddr_t pg_offset: 12;
    paddr_t ppn0     : 9; // physical page number
    paddr_t ppn1     : 9;
    paddr_t ppn2     : 26;
    paddr_t reserved : 8;
  } sv39_pa;
};

union sv39_pte {
  pte_t pte;
  struct {
    pte_t v        : 1; // valid
    pte_t r        : 1; // read
    pte_t w        : 1; // write
    pte_t x        : 1; // execute
    pte_t u        : 1; // user
    pte_t g        : 1; // global
    pte_t a        : 1; // accessed
    pte_t d        : 1; // dirty
    pte_t reserved : 2;
    pte_t ppn0     : 9; // physical page number
    pte_t ppn1     : 9;
    pte_t ppn2     : 26;
    pte_t reserved2: 7;
    pte_t pmbt     : 2; // Page-Based Memory Type
    pte_t n        : 1; // no-execute
  } sv39_pte;
};
#endif
/*
 * only support Sv39 and 4KB page here.
 * all according to: https://five-embeddev.com/riscv-priv-isa-manual/Priv-v1.12/supervisor.html#sv32algorithm
 * not check priviledge
 * not check param: type
 */
paddr_t isa_mmu_translate(vaddr_t vaddr, int len, int type) {
  if (isa_mmu_check(vaddr, len, type) == MMU_DIRECT) {
    return vaddr;
  }

  word_t a;
  int levels, pte_size = sizeof(word_t);
#ifdef CONFIG_RV64
  union rv64_satp satp;
  union sv39_va va;
  union sv39_pa pa;
  union sv39_pte pte;

  satp.satp = csr(SATP);
  va.va = vaddr;
  pa.pa = 0;
  levels = 3;

  assert(satp.rv64_satp.mode == 8); // Sv39
  assert(satp.rv64_satp.asid == 0); // not support

  a = satp.rv64_satp.ppn * PAGE_SIZE;
  int i = levels - 1;

  /* find leaf pte */
  for(; ; i--) {
    switch (i) {
      case 0: pte.pte = paddr_read(a + va.sv39_va.vpn0 * pte_size, sizeof(word_t)); break;
      case 1: pte.pte = paddr_read(a + va.sv39_va.vpn1 * pte_size, sizeof(word_t)); break;
      case 2: pte.pte = paddr_read(a + va.sv39_va.vpn2 * pte_size, sizeof(word_t)); break;
      default: panic("invalid level"); break;
    }
    if ((pte.sv39_pte.v == 0) || ((pte.sv39_pte.r == 0) && (pte.sv39_pte.w == 1))) {
      panic("pte format error");   // page fault is not supported
    }

    if ((pte.sv39_pte.r == 1)) {
      break;
    }

    a = pte.sv39_pte.ppn0 + (pte.sv39_pte.ppn1 << 9) + (pte.sv39_pte.ppn2 << 18);
  }

  /* leaf found. fill pa */
  pa.sv39_pa.pg_offset = va.sv39_va.pg_offset;
  for (int j = 0; j < i; j++) {
    switch (j) {
      case 0: pa.sv39_pa.ppn0 = va.sv39_va.vpn0; break;
      case 1: pa.sv39_pa.ppn1 = va.sv39_va.vpn1; break;
      default: panic("invalid leaf page table level"); break;
    }
  }

  for (int j = i; j < levels; j++) {
    switch (j) {
      case 0: pa.sv39_pa.ppn0 = pte.sv39_pte.ppn0; break;
      case 1: pa.sv39_pa.ppn1 = pte.sv39_pte.ppn1; break;
      case 2: pa.sv39_pa.ppn2 = pte.sv39_pte.ppn2; break;
      default: panic("should never reach here"); break;
    }
  }

  return pa.pa;
#endif
#ifdef CONFIG_RV32
#endif
}
