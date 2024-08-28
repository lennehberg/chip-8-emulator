#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define INT_8 8
#define INT_16 16
#define DEBUG
#ifndef NDEBUG
#include "emulatorShell.h"
#endif //NDEBUG




/*
 * raise an error for an instruction
 * which has yet to be implemented
 * @param state State8080 * for current state of the 8080
 * processor
 */
void UnimplementedInstruction(State8080 *state) {
  printf("Error: Unimplemented Instruction!\n");
  exit(1);
}

/*
 * check if the parity of val is odd or even
 * @return 0 if parity is even, 1 if odd
 */
int checkParity(uint16_t val) {
  // XOR all the bits of val to check for parity
  uint16_t ret = 0; // 0 won't affect the result at the beginning

  for (int i = 0; i < 16; ++i) {
    ret = ret ^ (val & 1); // extract the LSB of val and XOR with the
                           // previous LBS
    val = val >> 1;        // right shift val to get the next bit
  }

  return ret;
}

/*
 * set the conditional flags of the current state according to value and
 * affected
 * @param needed for zero, sign and parity
 */
void setCC(ConditionCodes *state_cc, ConditionCodes *affected, uint16_t value) {
  if (value == 0 && affected->z == 1) {
    state_cc->z = 1;
  } else {
    state_cc->z = 0;
  }
  if (value >= 0x80 && affected->s == 1) {
    state_cc->s = 1;
  } else {
    state_cc->s = 0;
  }
  if (checkParity(value) == 0 && affected->p == 1) {
    state_cc->p = 1;
  } else {
    state_cc->p = 0;
  }
  if (affected->cy == 1) {
    state_cc->cy = 1;
  } else {
    state_cc->cy = 0;
  }
  if (affected->ac == 1) {
    state_cc->ac = 1;
  } else {
    state_cc->ac = 0;
  }
}

/*
 * concatinate from a register pair
 */
uint16_t concBytes(uint8_t msb, uint8_t lsb) {
  uint16_t ret = msb;
  ret = (ret << 8) | lsb;
  return ret;
}

/*
 * seperate byte into msb and lsb
 */
void sepByte(uint16_t byte, uint8_t *msb, uint8_t *lsb)
{
  uint16_t mask = 0xff00;
  *msb = (mask & byte) >> INT_8;
  mask = 0xff;
  *lsb = (mask & byte);
}

uint16_t halfAdd(uint16_t byte1, uint16_t byte2, uint8_t *carry) {
  uint16_t m_byte1 = 1, m_byte2 = 1, res = 0;
  // HALF ADD
  // m_byte1 = 0000 0001 m_byte2 = 0000 0001
  m_byte1 = m_byte1 & byte1;
  m_byte2 = m_byte2 & byte2;
  // m_byte1 = 0000 0001 m_byte2 = 0000 0001
  res = m_byte1 ^ m_byte2;
  *carry = m_byte1 & m_byte2;

  return res;
}

uint16_t fullAdd(uint16_t m_byte1, uint16_t m_byte2, uint8_t *carry) {
  uint16_t res = 0;
  uint8_t temp_carry1 = 0, temp_carry2 = 0;
  m_byte1 = halfAdd(m_byte1, m_byte2, &temp_carry1);
  res = halfAdd(m_byte1, *carry, &temp_carry2);
  *carry = temp_carry1 | temp_carry2;

  return res;
}

/*
 * bitwise adder storing carries
 * @param byte1
 * @param byte2
 * @param size size of byte1 and byte2 (8 or 16)
 * @param carry int* to carry bit
 * @param auxcarry int* to auxcarry bit
 * @param affected ConditionCodes* if c and ac
 *                0 | 0 | 0      0
 *                ---------
 *                0 | 1 | 1      0
 *                ---------
 *                1 | 0 | 1      0
 *                ---------
 *                1 | 1 | 0      1
 *
 */
uint16_t addAndCarries(uint16_t byte1, uint16_t byte2, size_t size,
                       uint8_t *carry, uint8_t *auxcarry,
                       ConditionCodes *affected) {
  // byte1 = 1010 0011 byte2 = 1001 1011
  uint16_t res = halfAdd(byte1, byte2, carry);
  uint16_t m_byte1 = 1, m_byte2 = 1, f_byte = 0;
  for (size_t i = 1; i < size; ++i) {
    m_byte1 = 1;
    m_byte2 = 1;
    // if aucillary carry flag is set,
    // check if a carry happened from bit 3 to bit 4
    if (i == 4) {
      if (*carry && affected->ac) {
        *auxcarry = 1;
      }
    }
    // i.e if i == 4:
    // m_byte1 = 0001 0000 => 0000 0000
    m_byte1 = (m_byte1 << i) & byte1;
    // m_byte1 = 0001 0000 => 0001 0000
    m_byte2 = (m_byte2 << i) & byte2;
    // f_byte = 0001 0000 *carry = 0000 0000
    m_byte1 = m_byte1 >> i;
    m_byte2 = m_byte2 >> i;
    f_byte = fullAdd(m_byte1, m_byte2, carry);
    res = res | (f_byte << i);
  }
  return res;
}

/*
 * emulate the current instruction at the program counter
 * according to the 8080 instruction set
 */
void Emulate8080Op(State8080 *state) {
  unsigned char *opcode = &state->memory[state->pc];
  ConditionCodes affected = {};
  uint8_t carry = 0, auxcarry = 0;

  switch (*opcode) {
  case 0x00:
    break;   // NOP
  case 0x01: // LXI B,D16 | B = hdata, C = ldata
    state->c = opcode[1];
    state->b = opcode[2];
    state->pc += 2;
    break;
  case 0x02: // STAX B | ((BC)) = (A)
    state->memory[concBytes(state->b, state->c)] =
        state->a;
    break;
  case 0x03: // INX B | (BC) = (BC) + 1
    // increment register C, check if carry, increment B if necessary
    state->c = addAndCarries(state->c, 1, INT_8,
      &state->cc.cy, &state->cc.ac, &affected);
    if (&state->cc.cy)
    {
      state->b = addAndCarries(state->b, 1, INT_8,
        &state->cc.cy, &state->cc.ac, &affected);
    }
    break;
  case 0x04: // INR B | (B) = (B) + 1, affects Z,S,P,AC
    affected.z = 1; affected.s = 1; affected.p = 1; affected.ac = 1;
    state->b = addAndCarries(state->b, 1, INT_8,
      &state->cc.cy, &state->cc.ac, &affected);
    setCC(&state->cc, &affected, state->b);
    break;
  case 0x05: // DCR B | (B) = (B) - 1, affects Z,S,P,AC
    state->b = addAndCarries(state->b, ~1, INT_8,
      &state->cc.cy, &state->cc.ac, &affected);
  case 0x06: // MVI B,D8 | (B) = byte 2
    state->b = opcode[1];
    ++(state->pc);
    break;
  case 0x07: // RLC | shift left A; A0, CY = A7
    state->cc.cy = state->a & 0x80;
    state->a = state->a << 1;
    state->a = state->a | state->cc.cy;
    break;
  case 0x09: // DAD B | (HL) = (HL) + (BC), affects CY
    // concatinate HL and BC
    uint16_t hl = concBytes(state->h, state->l);
    uint16_t bc = concBytes(state->b, state->c);
    // add BC to HL and save in HL
    hl = addAndCarries(hl, bc, INT_16,
      &state->cc.cy, &state->cc.ac, &affected);
    // seperate the hl "16-bit" byte into 8-bit bytes and store in h and l,
    // respectively
    sepByte(hl, &state->h, &state->l);
    // set the condition flags for CY
    // set the affected flag to CY
    affected.cy = state->cc.cy;
    setCC(&state->cc, &affected, 0);
    break;
  case 0x0a:
    UnimplementedInstruction(state);
    break;
  case 0x0b:
    UnimplementedInstruction(state);
    break;
  case 0x0c:
    UnimplementedInstruction(state);
    break;
  case 0x0d:
    UnimplementedInstruction(state);
    break;
  case 0x0e:
    UnimplementedInstruction(state);
    break;
  case 0x0f:
    UnimplementedInstruction(state);
    break;
  case 0x11:
    UnimplementedInstruction(state);
    break;
  case 0x12:
    UnimplementedInstruction(state);
    break;
  case 0x13:
    UnimplementedInstruction(state);
    break;
  case 0x14:
    UnimplementedInstruction(state);
    break;
  case 0x15:
    UnimplementedInstruction(state);
    break;
  case 0x16:
    UnimplementedInstruction(state);
    break;
  case 0x17:
    UnimplementedInstruction(state);
    break;
  case 0x19:
    UnimplementedInstruction(state);
    break;
  case 0x1a:
    UnimplementedInstruction(state);
    break;
  case 0x1b:
    UnimplementedInstruction(state);
    break;
  case 0x1c:
    UnimplementedInstruction(state);
    break;
  case 0x1d:
    UnimplementedInstruction(state);
    break;
  case 0x1e:
    UnimplementedInstruction(state);
    break;
  case 0x1f:
    UnimplementedInstruction(state);
    break;
  case 0x20:
    UnimplementedInstruction(state);
    break;
  case 0x21:
    UnimplementedInstruction(state);
    break;
  case 0x22:
    UnimplementedInstruction(state);
    break;
  case 0x23:
    UnimplementedInstruction(state);
    break;
  case 0x24:
    UnimplementedInstruction(state);
    break;
  case 0x25:
    UnimplementedInstruction(state);
    break;
  case 0x26:
    UnimplementedInstruction(state);
    break;
  case 0x27:
    UnimplementedInstruction(state);
    break;
  case 0x29:
    UnimplementedInstruction(state);
    break;
  case 0x2a:
    UnimplementedInstruction(state);
    break;
  case 0x2b:
    UnimplementedInstruction(state);
    break;
  case 0x2c:
    UnimplementedInstruction(state);
    break;
  case 0x2d:
    UnimplementedInstruction(state);
    break;
  case 0x2e:
    UnimplementedInstruction(state);
    break;
  case 0x2f:
    UnimplementedInstruction(state);
    break;
  case 0x30:
    UnimplementedInstruction(state);
    break;
  case 0x31:
    UnimplementedInstruction(state);
    break;
  case 0x32:
    UnimplementedInstruction(state);
    break;
  case 0x33:
    UnimplementedInstruction(state);
    break;
  case 0x34:
    UnimplementedInstruction(state);
    break;
  case 0x35:
    UnimplementedInstruction(state);
    break;
  case 0x36:
    UnimplementedInstruction(state);
    break;
  case 0x37:
    UnimplementedInstruction(state);
    break;
  case 0x39:
    UnimplementedInstruction(state);
    break;
  case 0x3a:
    UnimplementedInstruction(state);
    break;
  case 0x3b:
    UnimplementedInstruction(state);
    break;
  case 0x3c:
    UnimplementedInstruction(state);
    break;
  case 0x3d:
    UnimplementedInstruction(state);
    break;
  case 0x3e:
    UnimplementedInstruction(state);
    break;
  case 0x3f:
    UnimplementedInstruction(state);
    break;
  case 0x40:
    UnimplementedInstruction(state);
    break;
  case 0x41:
    UnimplementedInstruction(state);
    break;
  case 0x42:
    UnimplementedInstruction(state);
    break;
  case 0x43:
    UnimplementedInstruction(state);
    break;
  case 0x44:
    UnimplementedInstruction(state);
    break;
  case 0x45:
    UnimplementedInstruction(state);
    break;
  case 0x46:
    UnimplementedInstruction(state);
    break;
  case 0x47:
    UnimplementedInstruction(state);
    break;
  case 0x48:
    UnimplementedInstruction(state);
    break;
  case 0x49:
    UnimplementedInstruction(state);
    break;
  case 0x4a:
    UnimplementedInstruction(state);
    break;
  case 0x4b:
    UnimplementedInstruction(state);
    break;
  case 0x4c:
    UnimplementedInstruction(state);
    break;
  case 0x4d:
    UnimplementedInstruction(state);
    break;
  case 0x4e:
    UnimplementedInstruction(state);
    break;
  case 0x4f:
    UnimplementedInstruction(state);
    break;
  case 0x50:
    UnimplementedInstruction(state);
    break;
  case 0x51:
    UnimplementedInstruction(state);
    break;
  case 0x52:
    UnimplementedInstruction(state);
    break;
  case 0x53:
    UnimplementedInstruction(state);
    break;
  case 0x54:
    UnimplementedInstruction(state);
    break;
  case 0x55:
    UnimplementedInstruction(state);
    break;
  case 0x56:
    UnimplementedInstruction(state);
    break;
  case 0x57:
    UnimplementedInstruction(state);
    break;
  case 0x58:
    UnimplementedInstruction(state);
    break;
  case 0x59:
    UnimplementedInstruction(state);
    break;
  case 0x5a:
    UnimplementedInstruction(state);
    break;
  case 0x5b:
    UnimplementedInstruction(state);
    break;
  case 0x5c:
    UnimplementedInstruction(state);
    break;
  case 0x5d:
    UnimplementedInstruction(state);
    break;
  case 0x5e:
    UnimplementedInstruction(state);
    break;
  case 0x5f:
    UnimplementedInstruction(state);
    break;
  case 0x60:
    UnimplementedInstruction(state);
    break;
  case 0x61:
    UnimplementedInstruction(state);
    break;
  case 0x62:
    UnimplementedInstruction(state);
    break;
  case 0x63:
    UnimplementedInstruction(state);
    break;
  case 0x64:
    UnimplementedInstruction(state);
    break;
  case 0x65:
    UnimplementedInstruction(state);
    break;
  case 0x66:
    UnimplementedInstruction(state);
    break;
  case 0x67:
    UnimplementedInstruction(state);
    break;
  case 0x68:
    UnimplementedInstruction(state);
    break;
  case 0x69:
    UnimplementedInstruction(state);
    break;
  case 0x6a:
    UnimplementedInstruction(state);
    break;
  case 0x6b:
    UnimplementedInstruction(state);
    break;
  case 0x6c:
    UnimplementedInstruction(state);
    break;
  case 0x6d:
    UnimplementedInstruction(state);
    break;
  case 0x6e:
    UnimplementedInstruction(state);
    break;
  case 0x6f:
    UnimplementedInstruction(state);
    break;
  case 0x70:
    UnimplementedInstruction(state);
    break;
  case 0x71:
    UnimplementedInstruction(state);
    break;
  case 0x72:
    UnimplementedInstruction(state);
    break;
  case 0x73:
    UnimplementedInstruction(state);
    break;
  case 0x74:
    UnimplementedInstruction(state);
    break;
  case 0x75:
    UnimplementedInstruction(state);
    break;
  case 0x76:
    UnimplementedInstruction(state);
    break;
  case 0x77:
    UnimplementedInstruction(state);
    break;
  case 0x78:
    UnimplementedInstruction(state);
    break;
  case 0x79:
    UnimplementedInstruction(state);
    break;
  case 0x7a:
    UnimplementedInstruction(state);
    break;
  case 0x7b:
    UnimplementedInstruction(state);
    break;
  case 0x7c:
    UnimplementedInstruction(state);
    break;
  case 0x7d:
    UnimplementedInstruction(state);
    break;
  case 0x7e:
    UnimplementedInstruction(state);
    break;
  case 0x7f:
    UnimplementedInstruction(state);
    break;
  case 0x80:
    UnimplementedInstruction(state);
    break;
  case 0x81:
    UnimplementedInstruction(state);
    break;
  case 0x82:
    UnimplementedInstruction(state);
    break;
  case 0x83:
    UnimplementedInstruction(state);
    break;
  case 0x84:
    UnimplementedInstruction(state);
    break;
  case 0x85:
    UnimplementedInstruction(state);
    break;
  case 0x86:
    UnimplementedInstruction(state);
    break;
  case 0x87:
    UnimplementedInstruction(state);
    break;
  case 0x88:
    UnimplementedInstruction(state);
    break;
  case 0x89:
    UnimplementedInstruction(state);
    break;
  case 0x8a:
    UnimplementedInstruction(state);
    break;
  case 0x8b:
    UnimplementedInstruction(state);
    break;
  case 0x8c:
    UnimplementedInstruction(state);
    break;
  case 0x8d:
    UnimplementedInstruction(state);
    break;
  case 0x8e:
    UnimplementedInstruction(state);
    break;
  case 0x8f:
    UnimplementedInstruction(state);
    break;
  case 0x90:
    UnimplementedInstruction(state);
    break;
  case 0x91:
    UnimplementedInstruction(state);
    break;
  case 0x92:
    UnimplementedInstruction(state);
    break;
  case 0x93:
    UnimplementedInstruction(state);
    break;
  case 0x94:
    UnimplementedInstruction(state);
    break;
  case 0x95:
    UnimplementedInstruction(state);
    break;
  case 0x96:
    UnimplementedInstruction(state);
    break;
  case 0x97:
    UnimplementedInstruction(state);
    break;
  case 0x98:
    UnimplementedInstruction(state);
    break;
  case 0x99:
    UnimplementedInstruction(state);
    break;
  case 0x9a:
    UnimplementedInstruction(state);
    break;
  case 0x9b:
    UnimplementedInstruction(state);
    break;
  case 0x9c:
    UnimplementedInstruction(state);
    break;
  case 0x9d:
    UnimplementedInstruction(state);
    break;
  case 0x9e:
    UnimplementedInstruction(state);
    break;
  case 0x9f:
    UnimplementedInstruction(state);
    break;
  case 0xa0:
    UnimplementedInstruction(state);
    break;
  case 0xa1:
    UnimplementedInstruction(state);
    break;
  case 0xa2:
    UnimplementedInstruction(state);
    break;
  case 0xa3:
    UnimplementedInstruction(state);
    break;
  case 0xa4:
    UnimplementedInstruction(state);
    break;
  case 0xa5:
    UnimplementedInstruction(state);
    break;
  case 0xa6:
    UnimplementedInstruction(state);
    break;
  case 0xa7:
    UnimplementedInstruction(state);
    break;
  case 0xa8:
    UnimplementedInstruction(state);
    break;
  case 0xa9:
    UnimplementedInstruction(state);
    break;
  case 0xaa:
    UnimplementedInstruction(state);
    break;
  case 0xab:
    UnimplementedInstruction(state);
    break;
  case 0xac:
    UnimplementedInstruction(state);
    break;
  case 0xad:
    UnimplementedInstruction(state);
    break;
  case 0xae:
    UnimplementedInstruction(state);
    break;
  case 0xaf:
    UnimplementedInstruction(state);
    break;
  case 0xb0:
    UnimplementedInstruction(state);
    break;
  case 0xb1:
    UnimplementedInstruction(state);
    break;
  case 0xb2:
    UnimplementedInstruction(state);
    break;
  case 0xb3:
    UnimplementedInstruction(state);
    break;
  case 0xb4:
    UnimplementedInstruction(state);
    break;
  case 0xb5:
    UnimplementedInstruction(state);
    break;
  case 0xb6:
    UnimplementedInstruction(state);
    break;
  case 0xb7:
    UnimplementedInstruction(state);
    break;
  case 0xb8:
    UnimplementedInstruction(state);
    break;
  case 0xb9:
    UnimplementedInstruction(state);
    break;
  case 0xba:
    UnimplementedInstruction(state);
    break;
  case 0xbb:
    UnimplementedInstruction(state);
    break;
  case 0xbc:
    UnimplementedInstruction(state);
    break;
  case 0xbd:
    UnimplementedInstruction(state);
    break;
  case 0xbe:
    UnimplementedInstruction(state);
    break;
  case 0xbf:
    UnimplementedInstruction(state);
    break;
  case 0xc0:
    UnimplementedInstruction(state);
    break;
  case 0xc1:
    UnimplementedInstruction(state);
    break;
  case 0xc2:
    UnimplementedInstruction(state);
    break;
  case 0xc3:
    UnimplementedInstruction(state);
    break;
  case 0xc4:
    UnimplementedInstruction(state);
    break;
  case 0xc5:
    UnimplementedInstruction(state);
    break;
  case 0xc6:
    UnimplementedInstruction(state);
    break;
  case 0xc7:
    UnimplementedInstruction(state);
    break;
  case 0xc8:
    UnimplementedInstruction(state);
    break;
  case 0xc9:
    UnimplementedInstruction(state);
    break;
  case 0xca:
    UnimplementedInstruction(state);
    break;
  case 0xcc:
    UnimplementedInstruction(state);
    break;
  case 0xcd:
    UnimplementedInstruction(state);
    break;
  case 0xce:
    UnimplementedInstruction(state);
    break;
  case 0xcf:
    UnimplementedInstruction(state);
    break;
  case 0xd0:
    UnimplementedInstruction(state);
    break;
  case 0xd1:
    UnimplementedInstruction(state);
    break;
  case 0xd2:
    UnimplementedInstruction(state);
    break;
  case 0xd3:
    UnimplementedInstruction(state);
    break;
  case 0xd4:
    UnimplementedInstruction(state);
    break;
  case 0xd5:
    UnimplementedInstruction(state);
    break;
  case 0xd6:
    UnimplementedInstruction(state);
    break;
  case 0xd7:
    UnimplementedInstruction(state);
    break;
  case 0xd8:
    UnimplementedInstruction(state);
    break;
  case 0xda:
    UnimplementedInstruction(state);
    break;
  case 0xdb:
    UnimplementedInstruction(state);
    break;
  case 0xdc:
    UnimplementedInstruction(state);
    break;
  case 0xde:
    UnimplementedInstruction(state);
    break;
  case 0xdf:
    UnimplementedInstruction(state);
    break;
  case 0xe0:
    UnimplementedInstruction(state);
    break;
  case 0xe1:
    UnimplementedInstruction(state);
    break;
  case 0xe2:
    UnimplementedInstruction(state);
    break;
  case 0xe3:
    UnimplementedInstruction(state);
    break;
  case 0xe4:
    UnimplementedInstruction(state);
    break;
  case 0xe5:
    UnimplementedInstruction(state);
    break;
  case 0xe6:
    UnimplementedInstruction(state);
    break;
  case 0xe7:
    UnimplementedInstruction(state);
    break;
  case 0xe8:
    UnimplementedInstruction(state);
    break;
  case 0xe9:
    UnimplementedInstruction(state);
    break;
  case 0xea:
    UnimplementedInstruction(state);
    break;
  case 0xeb:
    UnimplementedInstruction(state);
    break;
  case 0xec:
    UnimplementedInstruction(state);
    break;
  case 0xee:
    UnimplementedInstruction(state);
    break;
  case 0xef:
    UnimplementedInstruction(state);
    break;
  case 0xf0:
    UnimplementedInstruction(state);
    break;
  case 0xf1:
    UnimplementedInstruction(state);
    break;
  case 0xf2:
    UnimplementedInstruction(state);
    break;
  case 0xf3:
    UnimplementedInstruction(state);
    break;
  case 0xf4:
    UnimplementedInstruction(state);
    break;
  case 0xf5:
    UnimplementedInstruction(state);
    break;
  case 0xf6:
    UnimplementedInstruction(state);
    break;
  case 0xf7:
    UnimplementedInstruction(state);
    break;
  case 0xf8:
    UnimplementedInstruction(state);
    break;
  case 0xf9:
    UnimplementedInstruction(state);
    break;
  case 0xfa:
    UnimplementedInstruction(state);
    break;
  case 0xfb:
    UnimplementedInstruction(state);
    break;
  case 0xfc:
    UnimplementedInstruction(state);
    break;
  case 0xfe:
    UnimplementedInstruction(state);
    break;
  case 0xff:
    UnimplementedInstruction(state);
    break;
  }

  ++(state->pc);
}
