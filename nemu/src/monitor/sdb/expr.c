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

#ifndef TEST_EXPR
#include <isa.h>
#include <memory/vaddr.h>
#else
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
typedef uint32_t word_t;
#define ARRLEN(arr) (int)(sizeof(arr) / sizeof(arr[0]))
#define panic(...) assert(0)
#define Log(...)
#endif

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include <memory.h>
#include <stdlib.h>

enum {
  TK_NOTYPE = 256, TK_EQ,

  /* TODO: Add more token types */
  TK_NEQ, TK_AND, TK_DEM, TK_HEX, TK_REG, TK_DEREF,
};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */
  {" +", TK_NOTYPE},          // spaces
  {"==", TK_EQ},              // 257
  {"!=", TK_NEQ},            // 258
  {"&&", TK_AND},             // 259
  {"\\+", '+'},               // 40
  {"\\-", '-'},               // 45
  {"\\*", '*'},               // 42
  {"\\/", '/'},               // 47
  {"\\(", '('},               // 40
  {"\\)", ')'},               // 41
  {"0x[0-9a-f]+", TK_HEX},    // 261
  {"[0-9]+", TK_DEM},         // 260
  {"\\$[$a-z0-9]+", TK_REG},  // 262
  {"\\*", TK_DEREF},          // 263 it won't be evaled, just for notion
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */
        if (rules[i].token_type != TK_NOTYPE && nr_token >= ARRLEN(tokens)) {
          printf("can't contain more tokens\n");
          return false;
        }
        switch (rules[i].token_type) {
          case TK_NOTYPE: break;
          case TK_DEM: case TK_HEX: case TK_REG:
            if (substr_len > 20) {
              printf("too big number from %s\n", substr_start);
              return false;
            }
            memcpy(tokens[nr_token].str, substr_start, substr_len);
            tokens[nr_token].str[substr_len] = '\0';
          default: tokens[nr_token].type = rules[i].token_type; nr_token++;
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

static bool eval_success;

static inline bool is_parentheses_valid() {
  int stack = 0;

  for (int i = 0; i < nr_token; i++) {
    if (tokens[i].type == '(') {
      stack ++;
    } else if (tokens[i].type == ')') {
      stack --;
    }

    if (stack < 0) {
      return false;
    }
  }

  if (stack != 0){
    return false;
  }

  return true;
}

// assume that: 1. begin < end 2. parentheses valid
static inline bool check_parentheses(int begin, int end) {
  int stack = 0;

  if ((tokens[begin].type == '(') && (tokens[end].type == ')')) {
    for (int i = begin + 1; i < end; i++) {
      if (tokens[i].type == '(')
        stack ++;
      else if (tokens[i].type == ')')
        stack --;
      if (stack < 0)
        return false;
    }

    return true;
  }

  return false;
}

static int find_main_op(int begin, int end) {
  int stack = 0, eq_neq = -1, add_min = -1, mul_div = -1, deref = -1;

  for (int i = begin; i < end; i++) {
    if (tokens[i].type == '(') {
      stack ++;
      continue;
    } else if (tokens[i].type == ')') {
      stack --;
      continue;
    }

    if (stack == 0) {
      switch (tokens[i].type)
      {
      // priority low to high
      case TK_AND:
        return i;
      case TK_EQ: case TK_NEQ:
        if (eq_neq == -1)
          eq_neq = i;
      case '+':case '-':
        if (add_min == -1)
          add_min = i;
      case '*':case '/':
        if (mul_div == -1)
          mul_div = i;
      case TK_DEREF:
        if (deref == -1)
          deref = i;
      }
    }
  }

  if (eq_neq != -1)
    return eq_neq;

  if (add_min != -1)
    return add_min;

  if (mul_div != -1)
    return mul_div;

  return deref;
}

static word_t eval(int begin, int end) {
  if (begin > end) {
    /* Bad expression */
    goto _error;
  }
  else if (begin == end) {
    /* Single token.
     * For now this token should be a number.
     * Return the value of the number.
     */
    if (tokens[begin].type == TK_DEM)
      return (word_t)strtol(tokens[begin].str, NULL, 10);
    if (tokens[begin].type == TK_HEX)
      return (word_t)strtol(tokens[begin].str, NULL, 16);
#ifndef TEST_EXPR
    if (tokens[begin].type == TK_REG) {
      bool reg_valid;
      word_t ret;
      ret = isa_reg_str2val(&tokens[begin].str[1], &reg_valid);
      if (reg_valid)
        return ret;
      else
        goto _error;
    }
#endif
  }
  else if (check_parentheses(begin, end) == true) {
    /* The expression is surrounded by a matched pair of parentheses.
     * If that is the case, just throw away the parentheses.
     */
    return eval(begin + 1, end - 1);
  }
  else {
    /* We should do more things here. */
    int main_op = find_main_op(begin, end);
    if (main_op < 0)
      goto _error;

    switch (tokens[main_op].type) {
      case '+': return eval(begin, main_op - 1) + eval(main_op + 1, end);
      case '-': return eval(begin, main_op - 1) - eval(main_op + 1, end);
      case '*': return eval(begin, main_op - 1) * eval(main_op + 1, end);
      case '/':
        if (eval(main_op + 1, end) == 0) {
          printf("invalid expr: divide 0\n");
          goto _error;
        }
        return eval(begin, main_op - 1) / eval(main_op + 1, end);
      case TK_AND: return eval(begin, main_op - 1) && eval(main_op + 1, end);
      case TK_EQ: return eval(begin, main_op - 1) == eval(main_op + 1, end);
      case TK_NEQ: return eval(begin, main_op - 1) != eval(main_op + 1, end);
#ifndef TEST_EXPR
      case TK_DEREF: return vaddr_read(eval(main_op + 1, end), sizeof(word_t));
#endif
      default: printf("invalid expr: can't find main op\n"); goto _error;
    }
  }

_error:
  eval_success = false;
  return 0;
}

word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  eval_success = true;
  if (is_parentheses_valid() == false) {
    printf("invalid parentheses\n");
    *success = false;
    return 0;
  }

  for (int i = 0; i < nr_token; i ++) {
    if (tokens[i].type == '*' && (i == 0 ||
        (tokens[i - 1].type != ')' && tokens[i - 1].type != TK_DEM && tokens[i - 1].type != TK_HEX && tokens[i - 1].type != TK_REG)))
      tokens[i].type = TK_DEREF;
  }

  word_t val = eval(0, nr_token - 1);
  *success = eval_success;

  return val;
}
