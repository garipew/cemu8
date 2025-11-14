#ifndef CHIP8_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define COL 64
#define ROW 32

#define MEM_SIZE 0x1000
#define STACK_LEVELS 12

#define get_x(opcode) \
	((opcode)>>8)&0xf

#define get_y(opcode) \
	((opcode)>>4)&0xf

#define push(c, data) \
	c->stack[c->sp++] = data

#define pop(c) \
	c->stack[--c->sp]

typedef enum  {
	NO_OP,
	LOW_OP = 0x0000,
	ASM_CALL = 0x0000,
	CLEAR = 0xE0,
	RETURN = 0xEE,

	JP = 0x1000,
	FUNCTION_CALL = 0x2000,
	SKIP_EQ = 0x3000,
	SKIP_NE = 0x4000,
	JEI = 0x5000,
	SET_REG = 0x6000,
	ADDI = 0x7000,

	CALC = 0x8000,
	SET_VX = 0x8000,
	OR = 0x8001,
	AND = 0x8002,
	XOR = 0x8003,
	ADD = 0x8004,
	X_SUB_Y = 0x8005,
	SHIFT_R = 0x8006,
	Y_SUB_X = 0x8007,
	SHIFT_L = 0x800E,

	JNI = 0x9000,
	SET_I = 0xA000,
	JP_OFFSET = 0xB000,
	RAND = 0xC000,
	DRAW = 0xD000,

	KEYS = 0xE000,
	KEY_EQ = 0xE09E,
	KEY_NEQ = 0xE0A1,

	EXTRA = 0xF000,
	GET_DELAY = 0xF007,
	GET_KEY = 0xF00A,
	SET_DELAY = 0xF015,
	SET_SOUND = 0xF018,
	ADD_ADDR = 0xF01E,
	SET_FONT = 0xF029,
	WRITE_VX = 0xF033,
	DUMP = 0xF055,
	LOAD = 0xF065
} Opcode;

typedef struct chip{
	/* wider type first to force alignment, then no gaps? */
	uint32_t stack[STACK_LEVELS]; 

	uint16_t I; 
	uint16_t pc;

	uint8_t memory[MEM_SIZE]; 
	uint8_t v[0x10]; 
	uint8_t delay_timer;
	uint8_t sound_timer;

	uint16_t op;
	uint8_t sp; 
	uint8_t keys[16]; 

	uint8_t display[COL][ROW]; 
} Chip;

void initialize(Chip*);
void load_game(Chip*, FILE*);
#endif
