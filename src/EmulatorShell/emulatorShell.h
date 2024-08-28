//
// Created by ubuntu on 8/24/24.
//

#ifndef _EMULATORSHELL_H_
#define _EMULATORSHELL_H_
#include <stdint.h>
#include <stdio.h>
/*
 * struct representing then 8080 flags
 */
typedef struct ConditionCodes {
    uint8_t z : 1;
    uint8_t s : 1;
    uint8_t p : 1;
    uint8_t cy : 1;
    uint8_t ac : 1;
    uint8_t pad : 3;
} ConditionCodes;

/*
 * struct representing the 8080 "state"
 */
typedef struct State8080 {
    uint8_t a;                // register A
    uint8_t b;                // register B
    uint8_t c;                // register C
    uint8_t d;                // register D
    uint8_t e;                // register E
    uint8_t h;                // register H
    uint8_t l;                // register L
    uint16_t sp;              // stack pointer register
    uint16_t pc;              // program counter register
    uint8_t *memory;          // 8080 RAM, 0x0000 - 0xffff
    struct ConditionCodes cc; // flags and stuff
    uint8_t int_enable;
} State8080;

uint16_t addAndCarries(uint16_t byte1, uint16_t byte2, size_t size,
                       uint8_t *carry, uint8_t *auxcarry,
                       ConditionCodes *affected);
#endif //_EMULATORSHELL_H_
