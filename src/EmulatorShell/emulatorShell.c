#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define INT_8 8
#define INT_16 16

#define BITWISE_OVRR(dest, src) (dest & src) | (~dest & src)
#define MOV_(dest, src) \
{ \
  dest = src; \
}

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

void LXI_(uint8_t *rh, uint8_t *rl, const uint8_t *state_memory,
          const unsigned char *opcode, uint16_t *pc)
{
  *rh = state_memory[opcode[2]];
  *rl = state_memory[opcode[1]];
  *pc += 2;
}

void STAX_(uint8_t rh, uint8_t rl, uint8_t *state_memory, uint8_t state_a)
{
  state_memory[concBytes(rh, rl)] = state_a;
}

void INX_(uint8_t *rh, uint8_t *rl, ConditionCodes *state_cc, ConditionCodes *affected)
{
  // increment low register, check if carry, increment high register if necessary
  *rl = addAndCarries(*rl, 1, INT_8,
                      &state_cc->cy, &state_cc->ac, affected);
  if (state_cc->cy)
  {
    *rh = addAndCarries(*rh, 1, INT_8, &
                        state_cc->cy, &state_cc->ac, affected);
  }
}

void INR_(uint8_t *reg_ptr, ConditionCodes *state_cc, ConditionCodes *affected)
{
  // increment register content
  affected->z = 1; affected->s = 1; affected->p = 1; affected->ac = 1;
  *reg_ptr = addAndCarries(*reg_ptr, 1, INT_8,
                          &state_cc->cy, &state_cc->ac, affected);
  setCC(state_cc, affected, *reg_ptr);
}

void INR16_(uint16_t *reg_ptr, ConditionCodes *state_cc, ConditionCodes *affected)
{
  // increment register content
  affected->z = 1; affected->s = 1; affected->p = 1; affected->ac = 1;
  *reg_ptr = addAndCarries(*reg_ptr, 1, INT_16,
                          &state_cc->cy, &state_cc->ac, affected);
  setCC(state_cc, affected, *reg_ptr);
}

void DCR_(uint8_t *reg_ptr, ConditionCodes *state_cc, ConditionCodes *affected)
{
  // decrement register content, using 2's complement
  affected->z = 1; affected->s = 1; affected->p = 1; affected->ac = 1;
  *reg_ptr = addAndCarries(*reg_ptr, (~1) + 1, INT_8,
                          &state_cc->cy, &state_cc->ac, affected);
  setCC(state_cc, affected, *reg_ptr);
}

void DCR16_(uint16_t *reg_ptr, ConditionCodes *state_cc, ConditionCodes *affected)
{
  // decrement register content, using 2's complement
  affected->z = 1; affected->s = 1; affected->p = 1; affected->ac = 1;
  *reg_ptr = addAndCarries(*reg_ptr, (~1) + 1, INT_16,
                          &state_cc->cy, &state_cc->ac, affected);
  setCC(state_cc, affected, *reg_ptr);
}

void MVI_(uint8_t *reg_ptr, uint8_t *state_memory, const unsigned char *opcode, uint16_t *state_pc)
{
  *reg_ptr = state_memory[opcode[1]];
  ++(*state_pc);
}

void DAD_(uint8_t *state_h, uint8_t *state_l, uint8_t *rh, uint8_t *rl,
          ConditionCodes *state_cc, ConditionCodes *affected)
{
  // concatinate HL and (rh)(rl)
  uint16_t hl = concBytes(*state_h, *state_l);
  uint16_t bc = concBytes(*rh, *rl);
  // add (rh)(rl) to HL and save in HL
  hl = addAndCarries(hl, bc, INT_16,
    &state_cc->cy, &state_cc->ac, affected);
  // seperate the hl "16-bit" byte into 8-bit bytes and store in h and l,
  // respectively
  sepByte(hl, state_h, state_l);
  // set the condition flags for CY
  // set the affected flag to CY
  affected->cy = state_cc->cy;
  setCC(state_cc, affected, 0);
}

void LDAX_(uint8_t *state_a, uint8_t *state_memory, uint8_t rh, uint8_t rl)
{
  *state_a = state_memory[concBytes(rh, rl)];
}

void DCX_(uint8_t *rh, uint8_t *rl, ConditionCodes *state_cc, ConditionCodes *affected)
{
  uint16_t bc = concBytes(*rh, *rl);
  bc = addAndCarries(bc, (~1) + 1, INT_16,
    &state_cc->cy, &state_cc->ac, affected);
  sepByte(bc, rh, rl);
}

uint8_t MUX_(uint8_t a, uint8_t b, uint8_t sel)
{
  uint8_t out = (~sel) & a | sel & b;
  return out;
}

void shiftLeft(const uint8_t a, uint8_t *out)
{
  uint8_t mask = 1;
  for (int i = 0; i < INT_8; ++i)
  {
    *out = *out | (MUX_(0, a & mask, mask) << 1);
    mask = mask << 1;
  }
}

void shiftRight(const uint8_t a, uint8_t *out)
{
  uint8_t mask = 0x80;
  for (int i = INT_8; i > 0; --i)
  {
    *out = *out | (MUX_(0, a & mask, mask) >> 1);
    mask = mask >> 1;
  }
}

uint8_t RLC_(const uint8_t a, uint8_t *cy)
{
  uint8_t out = 0;
  // bit 0 = prev bit 7, cy = prev bit 0
  uint8_t mask = 0x80;
  // mask = prev bit 7
  mask = mask & a >> (INT_8 - 1);
  out = mask;
  *cy = mask;
  shiftLeft(a, &out);

  return out;
}

uint8_t RRC_(const uint8_t a, uint8_t *cy)
{
  uint8_t out = 0;
  // bit 7 = prev bit 0, cy = prev bit 0
  uint8_t mask = 1;
  *cy = mask & a;
  out = (mask & a) << (INT_8 - 1);
  shiftRight(a, &out);

  return out;
}

uint8_t RAL_(const uint8_t a, uint8_t *cy)
{
  uint8_t out = 0;
  // bit 0 = prev cy, cy = prev bit 7
  out = *cy;
  *cy = 0x80 & a >> (INT_8 - 1);
  shiftLeft(a, &out);
  return out;
}

uint8_t RAR_(const uint8_t a, uint8_t *cy)
{
  uint8_t out = 0;
  // bit 7 = prev bit 7, cy = prev bit 0
  out = 0x80 & a;
  *cy = 0x01 & a;
  shiftRight(a, &out);
  return out;
}



/*
 * emulate the current instruction at the program counter
 * according to the 8080 instruction set
 */
void Emulate8080Op(State8080 *state) {
  unsigned char *opcode = &state->memory[state->pc];
  ConditionCodes affected = {};

  switch (*opcode) {
  case 0x00:
    break;   // NOP
  case 0x01: // LXI B,D16 | B = hdata, C = ldata
    LXI_(&state->b, &state->c, state->memory, opcode, &state->pc);
    break;
  case 0x02: // STAX B | ((BC)) = (A)
    STAX_(state->b, state->c, state->memory, state->a);
    break;
  case 0x03: // INX B | (BC) = (BC) + 1
    // increment register C, check if carry, increment B if necessary
    INX_(&state->b, &state->c, &state->cc, &affected);
    break;
  case 0x04: // INR B | (B) = (B) + 1, affects Z,S,P,AC
    INR_(&state->b, &state->cc, &affected);
    break;
  case 0x05: // DCR B | (B) = (B) - 1, affects Z,S,P,AC
    DCR_(&state->b, &state->cc, &affected);
  case 0x06: // MVI B,D8 | (B) = byte 2
    MVI_(&state->b, state->memory, opcode, &state->pc);
    break;
  case 0x07: // RLC | shift left A; A0, CY = A7
    state->a = RLC_(state->a, &state->cc.cy);
    break;
  case 0x09: // DAD B | (HL) = (HL) + (BC), affects CY
    DAD_(&state->h, &state->l, &state->b, &state->c, &state->cc, &affected);
    break;
  case 0x0a: // LDAX B | (A) <- ((BC))
    LDAX_(&state->a, state->memory, state->b, state->c);
    break;
  case 0x0b: // DCX B | (BC) = (BC) - 1
    DCX_(&state->b, &state->c, &state->cc, &affected);
    break;
  case 0x0c: // INR C | (C) = (C) + 1, affects Z,S,P,AC
    INR_(&state->c, &state->cc, &affected);
    break;
  case 0x0d: // DCR C | (C) = (C) - 1, affects Z,S,P,AC
    DCR_(&state->c, &state->cc, &affected);
    break;
  case 0x0e: // MVI C,D8 | (C) = byte 2
    MVI_(&state->c, state->memory, opcode, &state->pc);
    break;
  case 0x0f: // RRC | CY, A7 = A0
    state->a = RRC_(state->a, &state->cc.cy);
    break;
  case 0x11: // LXI D, D16 | D = BYTE3, E = BYTE2
    LXI_(&state->d, &state->e, state->memory, opcode, &state->pc);
    break;
  case 0x12: // STAX D
    STAX_(state->d, state->e, state->memory, state->a);
    break;
  case 0x13: // INX D
    INX_(&state->d, &state->e, &state->cc, &affected);
    break;
  case 0x14: // INR D
    INR_(&state->d, &state->cc, &affected);
    break;
  case 0x15: // DCR D
    DCR_(&state->d, &state->cc, &affected);
    break;
  case 0x16: // MVI D,D8
    MVI_(&state->d, state->memory, opcode, &state->pc);
    break;
  case 0x17: // RAL | A = A<<1; bit 0 = prev CY; CY = prev bit 7
    state->a = RAL_(state->a, &state->cc.cy);
    break;
  case 0x19: // DAD D
    DAD_(&state->h, &state->l, &state->d, &state->e, &state->cc, &affected);
    break;
  case 0x1a: // LDAX D
    LDAX_(&state->a, state->memory, state->d, state->e);
    break;
  case 0x1b: // DCX D
    DCX_(&state->d, &state->e, &state->cc, &affected);
    break;
  case 0x1c: // INR E
    INR_(&state->e, &state->cc, &affected);
    break;
  case 0x1d: // DCR E
    DCR_(&state->e, &state->cc, &affected);
    break;
  case 0x1e: // MVI E,D8
    MVI_(&state->e, state->memory, opcode, &state->pc);
    break;
  case 0x1f: // RAR | A = A >> 1; bit 7 = prev bit 7; CY = prev bit 0
    state->a = RAR_(state->a, &state->cc.cy);
    break;
  case 0x20: // RIM
    UnimplementedInstruction(state);
    break;
  case 0x21: // LXI H,16
    LXI_(&state->h, &state->l, state->memory, opcode, &state->pc);
    break;
  case 0x22: // SHLD D16
    state->memory[opcode[1]] = state->l;
    state->memory[opcode[1] + 1] = state->h;
    ++state->pc;
    break;
  case 0x23: // INX H
    INX_(&state->h, &state->l, &state->cc, &affected);
    break;
  case 0x24: // INR H
    INR_(&state->h, &state->cc, &affected);
    break;
  case 0x25: // DCR H
    DCR_(&state->h, &state->cc, &affected);
    break;
  case 0x26: // MVI H,D8
    MVI_(&state->h, state->memory, opcode, &state->pc);
    break;
  case 0x27: // DAA
    UnimplementedInstruction(state);
    break;
  case 0x29: // DAD H
    DAD_(&state->h, &state->l, &state->h, &state->l, &state->cc, &affected);
    break;
  case 0x2a: // LHLD D16
    state->l = state->memory[opcode[1]];
    state->h = state->memory[opcode[1] + 1];
    ++state->pc;
    break;
  case 0x2b: // DCX H
    DCX_(&state->h, &state->l, &state->cc, &affected);
    break;
  case 0x2c: // INR L
    INR_(&state->l, &state->cc, &affected);
    break;
  case 0x2d: // DCR L
    DCR_(&state->l, &state->cc, &affected);
    break;
  case 0x2e: // MVI L.D8
    MVI_(&state->l, state->memory, opcode, &state->pc);
    break;
  case 0x2f: // CMA
    state->a = ~state->a;
    break;
  case 0x30: // SIM
    UnimplementedInstruction(state);
    break;
  case 0x31: // LXI SP,D16
    uint8_t spHi, spLow;
    LXI_(&spHi, &spLow, state->memory, opcode, &state->pc);
    state->sp = concBytes(spHi, spLow);
    break;
  case 0x32: // STA D16
    state->memory[opcode[1]] = state->a;
    ++state->pc;
    break;
  case 0x33: // INX SP
    ++state->sp;
    break;
  case 0x34: // INR M
    uint16_t hl = concBytes(state->h, state->l);
    INR_(&state->memory[hl], &state->cc, &affected);
    // sepByte(hl, &state->h, &state->l);
    break;
  case 0x35: // DCR M
    hl = concBytes(state->h, state->l);
    DCR_(&state->memory[hl], &state->cc, &affected);
    // sepByte(hl, &state->h, &state->l);
    break;
  case 0x36: // MVI M,D8
    hl = concBytes(state->h, state->l);
    MVI_(&state->memory[hl], state->memory, opcode, &state->pc);
    break;
  case 0x37: // STC
    state->cc.cy = 1;
    break;
  case 0x39: // DAD SP
    DAD_(&state->h, &state->l, &spHi, &spLow,
      &state->cc, &affected);
    state->sp = concBytes(spHi, spLow);
    break;
  case 0x3a: // LDA D8
    state->a = opcode[1];
    ++state->pc;
    break;
  case 0x3b: // DCX SP
    DCX_(&spHi, &spLow, &state->cc, &affected);
    state->sp = concBytes(spHi, spLow);
    break;
  case 0x3c: // INR A
    INR_(&state->a, &state->cc, &affected);
    break;
  case 0x3d: // DCR A
    DCR_(&state->a, &state->cc, &affected);
    break;
  case 0x3e: // MVI A,D16
    MVI_(&state->a, state->memory, opcode, &state->pc);
    break;
  case 0x3f: // CMC
    state->cc.cy = ~state->cc.cy;
    break;
  case 0x40: // MOV B,B
    MOV_(state->b, state->b);
    break;
  case 0x41: // MOV B,C
    MOV_(state->b, state->c);
    break;
  case 0x42: // MOV B,D
    MOV_(state->b, state->d);
    break;
  case 0x43: // MOV B,E
    MOV_(state->b, state->e);
    break;
  case 0x44: // MOV B,H
    MOV_(state->b, state->h);
    break;
  case 0x45: // MOV B,L
    MOV_(state->b, state->l);
    break;
  case 0x46: // MOV B,M
    hl = concBytes(state->h, state->l);
    MOV_(state->b, state->memory[hl]);
    break;
  case 0x47: // MOV B,A
    MOV_(state->b, state->a);
    break;
  case 0x48:
    MOV_(state->c, state->b);
    break;
  case 0x49:
    MOV_(state->c, state->c);
    break;
  case 0x4a:
    MOV_(state->c, state->d);
    break;
  case 0x4b:
    MOV_(state->c, state->e);
    break;
  case 0x4c:
    MOV_(state->c, state->h);
    break;
  case 0x4d:
    MOV_(state->c, state->l);
    break;
  case 0x4e:
    hl = concBytes(state->h, state->l);
    MOV_(state->c, state->memory[hl]);
    break;
  case 0x4f:
    MOV_(state->c, state->a);
    break;
  case 0x50:
    MOV_(state->d, state->b);
    break;
  case 0x51:
    MOV_(state->d, state->c);
    break;
  case 0x52:
    MOV_(state->d, state->d);
    break;
  case 0x53:
    MOV_(state->d, state->e);
    break;
  case 0x54:
    MOV_(state->d, state->h);
    break;
  case 0x55:
    MOV_(state->d, state->l);
    break;
  case 0x56:
    hl = concBytes(state->h, state->l);
    MOV_(state->d, state->memory[hl]);
    break;
  case 0x57:
    MOV_(state->d, state->a);
    break;
  case 0x58:
    MOV_(state->e, state->b);
    break;
  case 0x59:
    MOV_(state->e, state->c);
    break;
  case 0x5a:
    MOV_(state->e, state->d);
    break;
  case 0x5b:
    MOV_(state->e, state->e);
    break;
  case 0x5c:
    MOV_(state->e, state->h);
    break;
  case 0x5d:
    MOV_(state->e, state->l);
    break;
  case 0x5e:
    hl = concBytes(state->h, state->l);
    MOV_(state->e, state->memory[hl]);
    break;
  case 0x5f:
    MOV_(state->e, state->a);
    break;
  case 0x60:
    MOV_(state->h, state->b);
    break;
  case 0x61:
    MOV_(state->h, state->c);
    break;
  case 0x62:
    MOV_(state->h, state->d);
    break;
  case 0x63:
    MOV_(state->h, state->e);
    break;
  case 0x64:
    MOV_(state->h, state->h);
    break;
  case 0x65:
    MOV_(state->h, state->l);
    break;
  case 0x66:
    hl = concBytes(state->h, state->l);
    MOV_(state->h, state->memory[hl]);
    break;
  case 0x67:
    MOV_(state->h, state->a);
    break;
  case 0x68:
    MOV_(state->l, state->b);
    break;
  case 0x69:
    MOV_(state->l, state->c);
    break;
  case 0x6a:
    MOV_(state->l, state->d);
    break;
  case 0x6b:
    MOV_(state->l, state->e);
    break;
  case 0x6c:
    MOV_(state->l, state->h);
    break;
  case 0x6d:
    MOV_(state->l, state->l);
    break;
  case 0x6e:
    hl = concBytes(state->h, state->l);
    MOV_(state->l, state->memory[hl]);
    break;
  case 0x6f:
    MOV_(state->l, state->a);
    break;
  case 0x70:
    hl = concBytes(state->h, state->l);
    MOV_(state->memory[hl], state->b);
    break;
  case 0x71:
    hl = concBytes(state->h, state->l);
    MOV_(state->memory[hl], state->c);
    break;
  case 0x72:
    hl = concBytes(state->h, state->l);
    MOV_(state->memory[hl], state->d);
    break;
  case 0x73:
    hl = concBytes(state->h, state->l);
    MOV_(state->memory[hl], state->e);
    break;
  case 0x74:
    hl = concBytes(state->h, state->l);
    MOV_(state->memory[hl], state->h);
    break;
  case 0x75:
    hl = concBytes(state->h, state->l);
    MOV_(state->memory[hl], state->l);
    break;
  case 0x76: //HLT
    UnimplementedInstruction(state);
    break;
  case 0x77:
    hl = concBytes(state->h, state->l);
    MOV_(state->memory[hl], state->a);
    break;
  case 0x78:
    MOV_(state->a, state->b);
    break;
  case 0x79:
    MOV_(state->a, state->c);
    break;
  case 0x7a:
    MOV_(state->a, state->d);
    break;
  case 0x7b:
    MOV_(state->a, state->e);
    break;
  case 0x7c:
    MOV_(state->a, state->h);
    break;
  case 0x7d:
    MOV_(state->a, state->l);
    break;
  case 0x7e:
    hl = concBytes(state->h, state->l);
    MOV_(state->a, state->memory[hl]);
    break;
  case 0x7f:
    MOV_(state->a, state->a);
    break;
  case 0x80: // ADD B
    state->a = addAndCarries(state->a, state->b, INT_8,
                            &state->cc.cy, &state->cc.ac, &affected);
    break;
  case 0x81: // ADD C
    state->a = addAndCarries(state->a, state->c, INT_8,
                            &state->cc.cy, &state->cc.ac, &affected);
    break;
  case 0x82: // ADD D
    state->a = addAndCarries(state->a, state->d, INT_8,
                            &state->cc.cy, &state->cc.ac, &affected);
    break;
  case 0x83: // ADD E
    state->a = addAndCarries(state->a, state->e, INT_8,
                            &state->cc.cy, &state->cc.ac, &affected);
    break;
  case 0x84: // ADD H
    state->a = addAndCarries(state->a, state->h, INT_8,
                            &state->cc.cy, &state->cc.ac, &affected);
    break;
  case 0x85: // ADD L
    state->a = addAndCarries(state->a, state->l, INT_8,
                            &state->cc.cy, &state->cc.ac, &affected);
    break;
  case 0x86: // ADD M
    hl = concBytes(state->h, state->l);
    state->a = addAndCarries(state->a, state->memory[hl], INT_8,
                            &state->cc.cy, &state->cc.ac, &affected);
    break;
  case 0x87: // ADD A
    state->a = addAndCarries(state->a, state->a, INT_8,
                            &state->cc.cy, &state->cc.ac, &affected);
    break;
  case 0x88:// ADC B
    state->a = addAndCarries(state->a, state->b, INT_8,
                            &state->cc.cy, &state->cc.ac, &affected);
    state->a = addAndCarries(state->a, state->cc.cy, INT_8,
                              &state->cc.cy, &state->cc.ac, &affected);
    break;
  case 0x89:// ADC C
    state->a = addAndCarries(state->a, state->c, INT_8,
                            &state->cc.cy, &state->cc.ac, &affected);
    state->a = addAndCarries(state->a, state->cc.cy, INT_8,
                              &state->cc.cy, &state->cc.ac, &affected);
    break;
  case 0x8a:// ADC D
    state->a = addAndCarries(state->a, state->d, INT_8,
                            &state->cc.cy, &state->cc.ac, &affected);
    state->a = addAndCarries(state->a, state->cc.cy, INT_8,
                              &state->cc.cy, &state->cc.ac, &affected);
    break;
  case 0x8b:// ADC E
    state->a = addAndCarries(state->a, state->e, INT_8,
                            &state->cc.cy, &state->cc.ac, &affected);
    state->a = addAndCarries(state->a, state->cc.cy, INT_8,
                              &state->cc.cy, &state->cc.ac, &affected);
    break;
  case 0x8c:// ADC H
    state->a = addAndCarries(state->a, state->h, INT_8,
                            &state->cc.cy, &state->cc.ac, &affected);
    state->a = addAndCarries(state->a, state->cc.cy, INT_8,
                              &state->cc.cy, &state->cc.ac, &affected);
    break;
  case 0x8d:// ADC L
    state->a = addAndCarries(state->a, state->l, INT_8,
                            &state->cc.cy, &state->cc.ac, &affected);
    state->a = addAndCarries(state->a, state->cc.cy, INT_8,
                              &state->cc.cy, &state->cc.ac, &affected);
    break;
  case 0x8e:
    hl = concBytes(state->h, state->l);
    state->a = addAndCarries(state->a, state->memory[hl], INT_8,
                            &state->cc.cy, &state->cc.ac, &affected);
    state->a = addAndCarries(state->a, state->cc.cy, INT_8,
                              &state->cc.cy, &state->cc.ac, &affected);
    break;
  case 0x8f:// ADC A
    state->a = addAndCarries(state->a, state->a, INT_8,
                            &state->cc.cy, &state->cc.ac, &affected);
    state->a = addAndCarries(state->a, state->cc.cy, INT_8,
                              &state->cc.cy, &state->cc.ac, &affected);
    break;
  case 0x90:
    UnimplementedInstruction(state);
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
