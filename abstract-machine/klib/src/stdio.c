#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

// return length of out
static int itoa_d(char *out, long a) {
  if (a == 0x80000000) {
    strcpy(out, "-2147483648");
    return 11;
  }
  if (a < 0) {
    out[0] = '-';
    return itoa_d(out + 1, abs(a)) + 1;
  }

  int digits = 1;
  int a_copy = a;

  // calculate digits of a
  while ((a_copy /= 10) != 0)
    ++digits;

  for (int i = digits-1; i >= 0; --i) {
    out[i] = '0' + a%10;
    a /= 10;
  }
  return digits;
}

// return length of out
static int itoa_x(char *out, unsigned long a){
  int digits = 1;
  unsigned a_copy = a;

  // calculate digits of a
  while ((a_copy /= 16) != 0)
    ++digits;

  for (int i = digits-1; i >= 0; --i) {
    out[i] = ((a%16 > 9) ? ('a' + a%16 - 10) : a%16 + '0');
    a /= 16;
  }
  return digits;
}

/*
 * support format:
 *  %[csxdp]
 *  %0[1-9][dx]
 */
int vsprintf(char *out, const char *fmt, va_list ap) {
  const char *fp = fmt;   // fmt ptr
  char *op = out;         // output ptr

  while (*fp) {
    if(*fp == '%') {
      ++fp;
      switch (*fp) {
        case 'c':
          *op = va_arg(ap, int);
          ++op; ++fp;
          break;
        case 's':
          strcpy(op, va_arg(ap, char*));
          op += strlen(op); ++fp;
          break;
        case 'x':
          op += itoa_x(op, va_arg(ap, unsigned long));
          ++fp;
          break;
        case 'd':
          op += itoa_d(op, va_arg(ap, int));
          ++fp;
          break;
        case 'p':
          strcpy(op, "0x"); op += 2;
          char ptr[2 * sizeof(void *)];
          int digit_num = itoa_x(ptr, va_arg(ap, unsigned long));
          for(int i = digit_num; i < 2 * sizeof(void *); ++i)
            *(op++) = '0';
          strncpy(op, ptr, digit_num); op += digit_num;
          ++fp;
          break;
        case '0':
          // check if it is correct format, otherwise goto default without break
          if (fp[1] > '0' && fp[1] <= '9' && (fp[2] == 'd' || fp[2] == 'x')) {
            int min_digits = fp[1] - '0';
            char num[12];
            int digits = 0;

            fp += 3;
            if (*fp == 'd') digits = itoa_d(num, va_arg(ap, int));
            if (*fp == 'x') digits = itoa_x(num, va_arg(ap, unsigned));
            if (digits < min_digits)
              while (digits < min_digits--)
                *(op++) = '0';
            strcpy(op,num);
            op += digits;
            break;
          }
        default:
          *(op++) = '%';
      }
    } else {
      *(op++) = *(fp++);
    }
  }
  *op = '\0';

  return (uintptr_t)op - (uintptr_t)out;
}

int sprintf(char *out, const char *fmt, ...) {
  va_list ap;
  int out_len;

  va_start(ap,fmt);
  out_len = vsprintf(out,fmt,ap);
  va_end(ap);
  return out_len;
}

int printf(const char *fmt, ...) {
  va_list ap;
  int out_len;
  char out[16384];

  va_start(ap,fmt);
  out_len = vsprintf(out,fmt,ap);
  panic_on(out_len >= 16384, "content to print is too large");
  va_end(ap);
  putstr(out);
  return out_len;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  panic("Not implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("Not implemented");
}

#endif
