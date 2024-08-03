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

word_t isa_raise_intr(word_t NO, vaddr_t epc) {
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * Then return the address of the interrupt/exception vector.
   */

#ifdef CONFIG_ETRACE
  printf("exception " FMT_WORD " occurs at " FMT_WORD "\n", NO, epc);
#endif

  csr(MEPC) = epc;
  csr(MCAUSE) = NO;
  return csr(MTVEC);
}

#ifdef CONFIG_RV32
#define IRQ_TIMER 0x80000007  // for riscv32
#endif
#ifdef CONFIG_RV64
#define IRQ_TIMER 0x8000000000000007  // for riscv64
#endif

#define MIE (1 << 3)
#define MPIE (1 << 7)

word_t isa_query_intr() {
  if ((csr(MSTATUS) & MIE) && (cpu.INTR == true)) {
    cpu.INTR = false;
    csr(MSTATUS) |= MPIE;
    return IRQ_TIMER;
  }
  return INTR_EMPTY;
}
