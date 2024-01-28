#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
  size_t len;
  for (len = 0; s[len] != '\0'; ++len);
  return len;
}

// it's ub in overlap
char *strcpy(char *dst, const char *src) {
  memmove(dst, src, strlen(src) + 1);
  return dst;
}

char *strncpy(char *dst, const char *src, size_t n) {
  size_t len = strlen(src);
  if(n >= len) {
    strcpy(dst, src);
  } else {
    memmove(dst, src, n);
    dst[n] = '\0';
  }
  return dst;
}

char *strcat(char *dst, const char *src) {
  strcpy(dst + strlen(dst), src);
  return dst;
}

int strcmp(const char *s1, const char *s2) {
  size_t len = (strlen(s1) < strlen(s2)) ? strlen(s1) : strlen(s2);
  for(size_t i = 0; i <= len; ++i){
    if(s1[i] < s2[i]) return -1;
    if(s1[i] > s2[i]) return 1;
  }
  return 0;
}

int strncmp(const char *s1, const char *s2, size_t n) {
  size_t len = (strlen(s1) < strlen(s2)) ? strlen(s1) : strlen(s2);
  len = (len < n) ? len : (n-1);
  for(size_t i = 0; i <= len; ++i){
    if(s1[i] < s2[i]) return -1;
    if(s1[i] > s2[i]) return 1;
  }
  return 0;
}

void *memset(void *s, int c, size_t n) {
  char *c_s = (char *)s;
  for(size_t i = 0; i < n; ++i)
    c_s[i] = c;
  return s;
}

void *memmove(void *dst, const void *src, size_t n) {
  if ((uintptr_t)src < (uintptr_t)dst) {
    char *c_dst = (char *)dst, *c_src = (char *)src;
    for(size_t i = n-1; i < n; --i) // if i >= n, overflow.(size_t is unsigned)
      c_dst[i] = c_src[i];
  } else if ((uintptr_t)dst < (uintptr_t)src) {
    memcpy(dst, src, n);
  }
  return dst;
}

void *memcpy(void *out, const void *in, size_t n) {
  char *c_out = (char *)out, *c_in = (char *)in;
  for(size_t i = 0; i < n; ++i)
    c_out[i] = c_in[i];
  return out;
}

int memcmp(const void *s1, const void *s2, size_t n) {
  unsigned char *uc_s1 = (unsigned char *)s1, *uc_s2 = (unsigned char *)s2;
  for(size_t i = 0; i < n; ++i){
    if(uc_s1[i] < uc_s2[i]) return -1;
    if(uc_s1[i] > uc_s2[i]) return 1;
  }
  return 0;
}

#endif
