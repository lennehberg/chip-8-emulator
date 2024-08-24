#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

/*
 * struct representing then 8080 flags
 */
typedef struct ConditionCodes {
	uint8_t z:1;
	uint8_t s:1;
	uint8_t p:1;
	uint8_t cy:1;
	uint8_t ac:1;
	uint8_t pad:3;
}ConditionCodes;

/*
 * struct representing the 8080 "state"
 */
typedef struct State8080 {
	uint8_t a; // register A
	uint8_t b; // register B
	uint8_t c; // register C
	uint8_t d; // register D
	uint8_t e; // register E
	uint8_t h; // register H
	uint8_t l; // register L
	uint16_t sp; // stack pointer register
	uint16_t pc; // program counter register
	uint8_t *memory; // 8080 RAM, 0x0000 - 0xffff
	struct ConditionCodes cc; // flags and stuff
	uint8_t int_enable;
}State8080;

/*
 * raise an error for an instruction
 * which has yet to be implemented
 * @param state State8080 * for current state of the 8080
 * processor
 */
void UnimplementedInstruction(State8080 *state)
{
  printf("Error: Unimplemented Instruction!\n");
  exit(1);
}

/*
 * check if the parity of val is odd or even
 * @return 0 if parity is even, 1 if odd
 */
int checkParity(uint16_t val)
{
  // XOR all the bits of val to check for parity
  uint16_t ret = 0; // 0 won't affect the result at the beginning

  for (int i = 0; i < 16; ++i)
  {
	ret = ret ^ (val & 1); // extract the LSB of val and XOR with the
						   // previous LBS
  	val = val >> 1;        // right shift val to get the next bit
  }

  return ret;
}

/*
 * set the conditional flags of the current state according to value and
 * affected
 */
void setCC(ConditionCodes *state_cc, ConditionCodes *affected, uint16_t value)
{
  if (value == 0 && affected->z == 1)
  {
	state_cc->z = 1;
  }
  else
  {
	state_cc->z = 0;
  }
  if (value >= 0x80 && affected->s == 1)
  {
	state_cc->s = 1;
  }
  else
  {
	state_cc->s = 0;
  }
  if (checkParity (value) == 0 && affected->p == 1)
  {
	state_cc->p = 1;
  }
  else
  {
	state_cc->p = 0;
  }
  if (affected->cy == 1)
  {
	state_cc->cy = 1;
  }
  else
  {
	state_cc->cy = 0;
  }
  if (affected->ac == 1)
  {
	state_cc->ac = 1;
  }
  else
  {
	state_cc->ac = 0;
  }
}

/*
 * resolve the address from a register pair
 */
uint16_t resolveAddressInPair(uint8_t msb, uint8_t lsb)
{
  uint16_t ret = msb;
  ret = (ret << 8) | lsb;
  return ret;
}

/*
 * emulate the current instruction at the program counter
 * according to the 8080 instruction set
 */
int Emulate8080Op(State8080 *state)
{
  unsigned char *opcode = &state->memory[state->pc];

  switch(*opcode)
  {
	case 0x00: break;	// NOP
	case 0x01:			// LXI B,D16
	  state->c = opcode[1];
	  state->b = opcode[2];
	  state->pc += 2;
	  break;
	case 0x02: 			// STAX B
	  state->memory[resolveAddressInPair (state->b, state->c)] =
		  state->memory[state->a];
	  break;
	case 0x03:			// INX B
	  ++(state->memory[resolveAddressInPair (state->b, state->c)]);
	  break;
	case 0x04:			// INR B, affects Z,S,P,AC
	  ++(state->memory[state->b]);
	  if (state->memory[state->b] == 0)
	  {
		state->cc.z = 1;
	  }
	  if (state->memory[state->b] >= 0x80) // msb is 1
	  {
		state->cc.s = 1;
	  }
	  break;
	case 0x05:			// DCR B, affects Z,S,P,AC
	  --(state->memory[state->b]);
	case 0x06:UnimplementedInstruction(state); break;
	case 0x07:UnimplementedInstruction(state); break;
	case 0x09:UnimplementedInstruction(state); break;
	case 0x0a:UnimplementedInstruction(state); break;
	case 0x0b:UnimplementedInstruction(state); break;
	case 0x0c:UnimplementedInstruction(state); break;
	case 0x0d:UnimplementedInstruction(state); break;
	case 0x0e:UnimplementedInstruction(state); break;
	case 0x0f:UnimplementedInstruction(state); break;
	case 0x11:UnimplementedInstruction(state); break;
	case 0x12:UnimplementedInstruction(state); break;
	case 0x13:UnimplementedInstruction(state); break;
	case 0x14:UnimplementedInstruction(state); break;
	case 0x15:UnimplementedInstruction(state); break;
	case 0x16:UnimplementedInstruction(state); break;
	case 0x17:UnimplementedInstruction(state); break;
	case 0x19:UnimplementedInstruction(state); break;
	case 0x1a:UnimplementedInstruction(state); break;
	case 0x1b:UnimplementedInstruction(state); break;
	case 0x1c:UnimplementedInstruction(state); break;
	case 0x1d:UnimplementedInstruction(state); break;
	case 0x1e:UnimplementedInstruction(state); break;
	case 0x1f:UnimplementedInstruction(state); break;
	case 0x20:UnimplementedInstruction(state); break;
	case 0x21:UnimplementedInstruction(state); break;
	case 0x22:UnimplementedInstruction(state); break;
	case 0x23:UnimplementedInstruction(state); break;
	case 0x24:UnimplementedInstruction(state); break;
	case 0x25:UnimplementedInstruction(state); break;
	case 0x26:UnimplementedInstruction(state); break;
	case 0x27:UnimplementedInstruction(state); break;
	case 0x29:UnimplementedInstruction(state); break;
	case 0x2a:UnimplementedInstruction(state); break;
	case 0x2b:UnimplementedInstruction(state); break;
	case 0x2c:UnimplementedInstruction(state); break;
	case 0x2d:UnimplementedInstruction(state); break;
	case 0x2e:UnimplementedInstruction(state); break;
	case 0x2f:UnimplementedInstruction(state); break;
	case 0x30:UnimplementedInstruction(state); break;
	case 0x31:UnimplementedInstruction(state); break;
	case 0x32:UnimplementedInstruction(state); break;
	case 0x33:UnimplementedInstruction(state); break;
	case 0x34:UnimplementedInstruction(state); break;
	case 0x35:UnimplementedInstruction(state); break;
	case 0x36:UnimplementedInstruction(state); break;
	case 0x37:UnimplementedInstruction(state); break;
	case 0x39:UnimplementedInstruction(state); break;
	case 0x3a:UnimplementedInstruction(state); break;
	case 0x3b:UnimplementedInstruction(state); break;
	case 0x3c:UnimplementedInstruction(state); break;
	case 0x3d:UnimplementedInstruction(state); break;
	case 0x3e:UnimplementedInstruction(state); break;
	case 0x3f:UnimplementedInstruction(state); break;
	case 0x40:UnimplementedInstruction(state); break;
	case 0x41:UnimplementedInstruction(state); break;
	case 0x42:UnimplementedInstruction(state); break;
	case 0x43:UnimplementedInstruction(state); break;
	case 0x44:UnimplementedInstruction(state); break;
	case 0x45:UnimplementedInstruction(state); break;
	case 0x46:UnimplementedInstruction(state); break;
	case 0x47:UnimplementedInstruction(state); break;
	case 0x48:UnimplementedInstruction(state); break;
	case 0x49:UnimplementedInstruction(state); break;
	case 0x4a:UnimplementedInstruction(state); break;
	case 0x4b:UnimplementedInstruction(state); break;
	case 0x4c:UnimplementedInstruction(state); break;
	case 0x4d:UnimplementedInstruction(state); break;
	case 0x4e:UnimplementedInstruction(state); break;
	case 0x4f:UnimplementedInstruction(state); break;
	case 0x50:UnimplementedInstruction(state); break;
	case 0x51:UnimplementedInstruction(state); break;
	case 0x52:UnimplementedInstruction(state); break;
	case 0x53:UnimplementedInstruction(state); break;
	case 0x54:UnimplementedInstruction(state); break;
	case 0x55:UnimplementedInstruction(state); break;
	case 0x56:UnimplementedInstruction(state); break;
	case 0x57:UnimplementedInstruction(state); break;
	case 0x58:UnimplementedInstruction(state); break;
	case 0x59:UnimplementedInstruction(state); break;
	case 0x5a:UnimplementedInstruction(state); break;
	case 0x5b:UnimplementedInstruction(state); break;
	case 0x5c:UnimplementedInstruction(state); break;
	case 0x5d:UnimplementedInstruction(state); break;
	case 0x5e:UnimplementedInstruction(state); break;
	case 0x5f:UnimplementedInstruction(state); break;
	case 0x60:UnimplementedInstruction(state); break;
	case 0x61:UnimplementedInstruction(state); break;
	case 0x62:UnimplementedInstruction(state); break;
	case 0x63:UnimplementedInstruction(state); break;
	case 0x64:UnimplementedInstruction(state); break;
	case 0x65:UnimplementedInstruction(state); break;
	case 0x66:UnimplementedInstruction(state); break;
	case 0x67:UnimplementedInstruction(state); break;
	case 0x68:UnimplementedInstruction(state); break;
	case 0x69:UnimplementedInstruction(state); break;
	case 0x6a:UnimplementedInstruction(state); break;
	case 0x6b:UnimplementedInstruction(state); break;
	case 0x6c:UnimplementedInstruction(state); break;
	case 0x6d:UnimplementedInstruction(state); break;
	case 0x6e:UnimplementedInstruction(state); break;
	case 0x6f:UnimplementedInstruction(state); break;
	case 0x70:UnimplementedInstruction(state); break;
	case 0x71:UnimplementedInstruction(state); break;
	case 0x72:UnimplementedInstruction(state); break;
	case 0x73:UnimplementedInstruction(state); break;
	case 0x74:UnimplementedInstruction(state); break;
	case 0x75:UnimplementedInstruction(state); break;
	case 0x76:UnimplementedInstruction(state); break;
	case 0x77:UnimplementedInstruction(state); break;
	case 0x78:UnimplementedInstruction(state); break;
	case 0x79:UnimplementedInstruction(state); break;
	case 0x7a:UnimplementedInstruction(state); break;
	case 0x7b:UnimplementedInstruction(state); break;
	case 0x7c:UnimplementedInstruction(state); break;
	case 0x7d:UnimplementedInstruction(state); break;
	case 0x7e:UnimplementedInstruction(state); break;
	case 0x7f:UnimplementedInstruction(state); break;
	case 0x80:UnimplementedInstruction(state); break;
	case 0x81:UnimplementedInstruction(state); break;
	case 0x82:UnimplementedInstruction(state); break;
	case 0x83:UnimplementedInstruction(state); break;
	case 0x84:UnimplementedInstruction(state); break;
	case 0x85:UnimplementedInstruction(state); break;
	case 0x86:UnimplementedInstruction(state); break;
	case 0x87:UnimplementedInstruction(state); break;
	case 0x88:UnimplementedInstruction(state); break;
	case 0x89:UnimplementedInstruction(state); break;
	case 0x8a:UnimplementedInstruction(state); break;
	case 0x8b:UnimplementedInstruction(state); break;
	case 0x8c:UnimplementedInstruction(state); break;
	case 0x8d:UnimplementedInstruction(state); break;
	case 0x8e:UnimplementedInstruction(state); break;
	case 0x8f:UnimplementedInstruction(state); break;
	case 0x90:UnimplementedInstruction(state); break;
	case 0x91:UnimplementedInstruction(state); break;
	case 0x92:UnimplementedInstruction(state); break;
	case 0x93:UnimplementedInstruction(state); break;
	case 0x94:UnimplementedInstruction(state); break;
	case 0x95:UnimplementedInstruction(state); break;
	case 0x96:UnimplementedInstruction(state); break;
	case 0x97:UnimplementedInstruction(state); break;
	case 0x98:UnimplementedInstruction(state); break;
	case 0x99:UnimplementedInstruction(state); break;
	case 0x9a:UnimplementedInstruction(state); break;
	case 0x9b:UnimplementedInstruction(state); break;
	case 0x9c:UnimplementedInstruction(state); break;
	case 0x9d:UnimplementedInstruction(state); break;
	case 0x9e:UnimplementedInstruction(state); break;
	case 0x9f:UnimplementedInstruction(state); break;
	case 0xa0:UnimplementedInstruction(state); break;
	case 0xa1:UnimplementedInstruction(state); break;
	case 0xa2:UnimplementedInstruction(state); break;
	case 0xa3:UnimplementedInstruction(state); break;
	case 0xa4:UnimplementedInstruction(state); break;
	case 0xa5:UnimplementedInstruction(state); break;
	case 0xa6:UnimplementedInstruction(state); break;
	case 0xa7:UnimplementedInstruction(state); break;
	case 0xa8:UnimplementedInstruction(state); break;
	case 0xa9:UnimplementedInstruction(state); break;
	case 0xaa:UnimplementedInstruction(state); break;
	case 0xab:UnimplementedInstruction(state); break;
	case 0xac:UnimplementedInstruction(state); break;
	case 0xad:UnimplementedInstruction(state); break;
	case 0xae:UnimplementedInstruction(state); break;
	case 0xaf:UnimplementedInstruction(state); break;
	case 0xb0:UnimplementedInstruction(state); break;
	case 0xb1:UnimplementedInstruction(state); break;
	case 0xb2:UnimplementedInstruction(state); break;
	case 0xb3:UnimplementedInstruction(state); break;
	case 0xb4:UnimplementedInstruction(state); break;
	case 0xb5:UnimplementedInstruction(state); break;
	case 0xb6:UnimplementedInstruction(state); break;
	case 0xb7:UnimplementedInstruction(state); break;
	case 0xb8:UnimplementedInstruction(state); break;
	case 0xb9:UnimplementedInstruction(state); break;
	case 0xba:UnimplementedInstruction(state); break;
	case 0xbb:UnimplementedInstruction(state); break;
	case 0xbc:UnimplementedInstruction(state); break;
	case 0xbd:UnimplementedInstruction(state); break;
	case 0xbe:UnimplementedInstruction(state); break;
	case 0xbf:UnimplementedInstruction(state); break;
	case 0xc0:UnimplementedInstruction(state); break;
	case 0xc1:UnimplementedInstruction(state); break;
	case 0xc2:UnimplementedInstruction(state); break;
	case 0xc3:UnimplementedInstruction(state); break;
	case 0xc4:UnimplementedInstruction(state); break;
	case 0xc5:UnimplementedInstruction(state); break;
	case 0xc6:UnimplementedInstruction(state); break;
	case 0xc7:UnimplementedInstruction(state); break;
	case 0xc8:UnimplementedInstruction(state); break;
	case 0xc9:UnimplementedInstruction(state); break;
	case 0xca:UnimplementedInstruction(state); break;
	case 0xcc:UnimplementedInstruction(state); break;
	case 0xcd:UnimplementedInstruction(state); break;
	case 0xce:UnimplementedInstruction(state); break;
	case 0xcf:UnimplementedInstruction(state); break;
	case 0xd0:UnimplementedInstruction(state); break;
	case 0xd1:UnimplementedInstruction(state); break;
	case 0xd2:UnimplementedInstruction(state); break;
	case 0xd3:UnimplementedInstruction(state); break;
	case 0xd4:UnimplementedInstruction(state); break;
	case 0xd5:UnimplementedInstruction(state); break;
	case 0xd6:UnimplementedInstruction(state); break;
	case 0xd7:UnimplementedInstruction(state); break;
	case 0xd8:UnimplementedInstruction(state); break;
	case 0xda:UnimplementedInstruction(state); break;
	case 0xdb:UnimplementedInstruction(state); break;
	case 0xdc:UnimplementedInstruction(state); break;
	case 0xde:UnimplementedInstruction(state); break;
	case 0xdf:UnimplementedInstruction(state); break;
	case 0xe0:UnimplementedInstruction(state); break;
	case 0xe1:UnimplementedInstruction(state); break;
	case 0xe2:UnimplementedInstruction(state); break;
	case 0xe3:UnimplementedInstruction(state); break;
	case 0xe4:UnimplementedInstruction(state); break;
	case 0xe5:UnimplementedInstruction(state); break;
	case 0xe6:UnimplementedInstruction(state); break;
	case 0xe7:UnimplementedInstruction(state); break;
	case 0xe8:UnimplementedInstruction(state); break;
	case 0xe9:UnimplementedInstruction(state); break;
	case 0xea:UnimplementedInstruction(state); break;
	case 0xeb:UnimplementedInstruction(state); break;
	case 0xec:UnimplementedInstruction(state); break;
	case 0xee:UnimplementedInstruction(state); break;
	case 0xef:UnimplementedInstruction(state); break;
	case 0xf0:UnimplementedInstruction(state); break;
	case 0xf1:UnimplementedInstruction(state); break;
	case 0xf2:UnimplementedInstruction(state); break;
	case 0xf3:UnimplementedInstruction(state); break;
	case 0xf4:UnimplementedInstruction(state); break;
	case 0xf5:UnimplementedInstruction(state); break;
	case 0xf6:UnimplementedInstruction(state); break;
	case 0xf7:UnimplementedInstruction(state); break;
	case 0xf8:UnimplementedInstruction(state); break;
	case 0xf9:UnimplementedInstruction(state); break;
	case 0xfa:UnimplementedInstruction(state); break;
	case 0xfb:UnimplementedInstruction(state); break;
	case 0xfc:UnimplementedInstruction(state); break;
	case 0xfe:UnimplementedInstruction(state); break;
	case 0xff:UnimplementedInstruction(state); break;
  }

  ++state->pc;
}