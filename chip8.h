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

typedef struct {
	uint16_t op;
	Chip *chip;
} ChipArgs;

typedef enum  {
	chip_no_op,
	chip_low_op = 0x0000,
	chip_asm_call = 0x0000,
	chip_clear = 0xe0,
	chip_return = 0xee,

	chip_jp = 0x1000,
	chip_call = 0x2000,
	chip_skip_eq = 0x3000,
	chip_skip_ne = 0x4000,
	chip_jei = 0x5000,
	chip_set_v = 0x6000,
	chip_addi = 0x7000,

	chip_calc = 0x8000,
	chip_set_vx = 0x8000,
	chip_or = 0x8001,
	chip_and = 0x8002,
	chip_xor = 0x8003,
	chip_add = 0x8004,
	chip_x_sub_y = 0x8005,
	chip_shift_r = 0x8006,
	chip_y_sub_x = 0x8007,
	chip_shift_l = 0x800e,

	chip_jni = 0x9000,
	chip_set_i = 0xa000,
	chip_jp_offset = 0xb000,
	chip_rand = 0xc000,
	chip_draw = 0xd000,

	chip_keys = 0xe000,
	chip_key_eq = 0xe09e,
	chip_key_neq = 0xe0a1,

	chip_high_op = 0xf000,
	chip_get_delay = 0xf007,
	chip_get_key = 0xf00a,
	chip_set_delay = 0xf015,
	chip_set_sound = 0xf018,
	chip_add_addr = 0xf01e,
	chip_set_font = 0xf029,
	chip_store_bcd = 0xf033,
	chip_dump = 0xf055,
	chip_load = 0xf065
} Opcode;

// NOTE(garipew): This list does not include every opcode. It only contains
// opcodes with unique most significant NIBBLE. That is, only the hex(X000).
#define OPCODE_LIST \
	FUNC(chip_low_op) \
	FUNC(chip_jp) \
	FUNC(chip_call) \
	FUNC(chip_skip_eq) \
	FUNC(chip_skip_ne) \
	FUNC(chip_jei) \
	FUNC(chip_set_v) \
	FUNC(chip_addi) \
	FUNC(chip_calc) \
	FUNC(chip_jni) \
	FUNC(chip_set_i) \
	FUNC(chip_jp_offset) \
	FUNC(chip_rand) \
	FUNC(chip_draw) \
	FUNC(chip_keys) \
	FUNC(chip_high_op)

#define FUNC(arg) \
	void arg##_fn(ChipArgs*);
OPCODE_LIST
#undef FUNC
// chip functions
typedef void (*chipfunc_t)(ChipArgs*);

#define idx_from_opcode(op) \
	(op>>12)&0xf
extern chipfunc_t fn_table[0x10];
void load_fn_table();
#endif
