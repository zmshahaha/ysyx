#include <am.h>
#include <nemu.h>

#define KEYDOWN_MASK 0x8000

void __am_input_keybrd(AM_INPUT_KEYBRD_T *kbd) {
  uint32_t key_mem = inl(KBD_ADDR);
  kbd->keycode = key_mem & ~KEYDOWN_MASK;     // without key_mask
  kbd->keydown = (key_mem & KEYDOWN_MASK) ? false :true;  // false: release, true: press down
}
