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

#include "sdb.h"

#define NR_WP 32
#define MAX_EXPR_SIZE 128

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */
  char exp[MAX_EXPR_SIZE];
  word_t val;
  int hit_time;
} WP;

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;              // NO is always equal to the subscript
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

void new_wp(char *expression){
  // first need to verify the legality of expr
  bool success;
  word_t value = expr(expression, &success);
  if (success == false) {
    printf("error: expression can't be evaluated\n");
    return;
  }

  // and then add watchpoint
  WP *new_watchpoint;
  if (free_ == NULL) {
    printf("no free watchpoint space\n");
    return;
  }

  new_watchpoint = free_;
  free_ = free_->next;
  new_watchpoint->next = head;
  head = new_watchpoint;

  new_watchpoint->hit_time = 0;
  new_watchpoint->val = value;
  strcpy(new_watchpoint->exp,expression);
  printf("new watchpoint: %d\n",new_watchpoint->NO);
  printf("its init value is: " FMT_WORD "\t" FMT_WORD_D "\n", new_watchpoint->val, new_watchpoint->val);
}

void free_wp(int no){
  // no is based on position, so we can find the wp by no
  // but still need to check if wp is alloced
  WP *iter = head, *prev_iter = head, *wp;

  if (no < 0 || no > NR_WP) {
    printf("invalid wp number\n");
    return;
  }

  wp = &wp_pool[no];

  if (head == NULL) {
    printf("no such watchpoint\n");
    return;
  }

  if (head == wp) {
    head = head->next;
    wp->next = free_;
    free_ = wp;
    printf("watchpoint %d has freed\n", wp->NO);
    return;
  }

  if (head->next)
    iter=iter->next;

  while (iter!=NULL) {
    if (iter == wp) {
      prev_iter->next = iter->next;
      iter->next = free_;
      free_ = iter;
      printf("watchpoint %d has freed\n", wp->NO);
      return;
    }
    prev_iter = iter;
    iter = iter->next;
  }

  printf("no such watchpoint\n");
}

bool check_all_wp(){
  WP *iter = head;
  bool changed = false;
  word_t new_val;
  bool success; // for filling expr's param
  while (iter != NULL) {
    new_val = expr(iter->exp, &success);
    if(new_val != iter->val){
      changed = true;
      printf("watchpoint %d: %s\n", iter->NO, iter->exp);
      printf("Old value = " FMT_WORD "\t" FMT_WORD_D "\n", iter->val, iter->val);
      printf("New value = " FMT_WORD "\t" FMT_WORD_D "\n\n", new_val, new_val);
      iter->val = new_val;
      ++iter->hit_time;
    }
    iter = iter->next;
  }
  return changed;
}

void print_all_wp(){
  WP *iter = head;
  if (iter == NULL) {
    printf("No watchpoints\n");
    return;
  }

  printf("Num  Hits        Value           What\n");
  while (iter != NULL) {
    printf("%2d\t%d\t" FMT_WORD "\t%s\n", iter->NO, iter->hit_time, iter->val, iter->exp);
    iter = iter->next;
  }
}
