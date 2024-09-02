#include "../src/EmulatorShell/emulatorShell.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define DEBUG_BINARY_FORMAT "%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d\n"
// 0000 0000 0000 0000
#define DEBUG_BYTE_TO_BINARY(byte)                                             \
      ((byte) & 0x8000 ? 1 : 0),                                                                \
      ((byte) & 0x4000 ? 1: 0),                                                        \
                                                                   \
      ((byte) & 0x2000 ?  1 : 0),                                                        \
                                                                   \
      ((byte) & 0x1000 ? 1 : 0),                                                      \
                                                                    \
      ((byte) & 0x0800 ? 1 : 0),                                                      \
                                                                    \
      ((byte) & 0x0400 ? 1 : 0),                                                       \
                                                                   \
      ((byte) & 0x0200 ? 1 : 0),                                                      \
                                                                    \
      ((byte) & 0x0100 ? 1 : 0),                                                      \
                                                                    \
      ((byte) & 0x0080 ? 1 : 0),                                                      \
                                                                    \
      ((byte) & 0x0040 ? 1 : 0),                                                      \
                                                                    \
      ((byte) & 0x0020 ? 1 : 0),                                                      \
                                                                    \
      ((byte) & 0x0010 ? 1 : 0),                                                      \
                                                                    \
      ((byte) & 0x0008 ? 1 : 0),                                                       \
                                                                   \
      ((byte) & 0x0004 ? 1 : 0),                                                       \
                                                                   \
      ((byte) & 0x0002 ? 1 : 0),                                                       \
                                                                   \
      ((byte) & 0x0001 ? 1 : 0)


int test_adder();

int main() {
  assert(test_adder() == 0);
  exit(0);
}

int test_adder() {
  uint16_t byte1 = 0b011111, byte2 = 0b011100;
  uint8_t carry = 0, auxcarry = 0;
  ConditionCodes affected = {};
    affected.ac = 1;

  printf("byte1 = "DEBUG_BINARY_FORMAT, DEBUG_BYTE_TO_BINARY(byte1));
  printf("byte2 = "DEBUG_BINARY_FORMAT, DEBUG_BYTE_TO_BINARY(byte2));

  uint16_t res = addAndCarries(byte1, byte2, 16, &carry, &auxcarry, &affected);
  printf("res   = " DEBUG_BINARY_FORMAT, DEBUG_BYTE_TO_BINARY(res));
  printf(" = %d\n", res);
  if (res == byte1 + byte2)
  {
      return 0;
  }
  return 1;
}
