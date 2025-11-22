#include "chip8.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

chipfunc_t fn_table[0x10];

void load_fn_table(){
#define FUNC(arg) \
	fn_table[(arg>>12)&0xf] = arg##_fn; 
	OPCODE_LIST
#undef FUNC
}

void initialize(Chip* c){
	c->pc = 0x200;
	c->sp = 0;
	c->I = 0;
	c->delay_timer = 60;
	c->sound_timer = 60;
}

void load_game(Chip* chip, FILE* stream){
	if(chip->sp >= STACK_LEVELS){
		fprintf(stderr, "Stack Overflow.\n");
		return;
	} 
	push(chip, chip->pc);
	int c;
	chip->pc = 0x200;
	while(chip->pc < MEM_SIZE){
		if((c = getc(stream)) == EOF){
			break;
		}
		chip->memory[chip->pc++] = (uint8_t)c;
	}
	chip->pc = pop(chip);
}

void chip_clear_fn(Chip *chip){
	for(int i = 0; i < ROW; i++){ 
		memset(chip->display[i], 0, COL); 
	}
}

void chip_low_op_fn(ChipArgs *args){
	Chip *chip = args->chip;
	uint16_t op = args->op;
	switch(op){
		case chip_clear:
			chip_clear_fn(chip);
			return;
		case chip_return:
			args->chip->pc = pop(args->chip);
			return;
	}
	fprintf(stderr, "(%04x) Unrecognized opcode\n", op);
}

void chip_jp_fn(ChipArgs *args){
	args->chip->pc = args->op&0xfff;
}

void chip_call_fn(ChipArgs *args){
	Chip *chip = args->chip;
	push(chip, chip->pc);
	chip->pc = args->op&0xfff;
}

void chip_skip_eq_fn(ChipArgs *args){
	Chip *chip = args->chip;
	uint16_t op = args->op;
	uint8_t x = get_x(op);
	if(chip->v[x] == (op&0xff)) chip->pc+=2;
}

void chip_skip_ne_fn(ChipArgs *args){
	Chip *chip = args->chip;
	uint16_t op = args->op;
	uint8_t x = get_x(op);
	if(chip->v[x] == (op&0xff)) chip->pc+=2;
}

void chip_jei_fn(ChipArgs *args){
	Chip *chip = args->chip;
	uint16_t op = args->op;
	uint8_t x = get_x(op);
	uint8_t y = get_y(op);
	if(chip->v[x] == chip->v[y]) chip->pc+=2;
}

void chip_set_v_fn(ChipArgs *args){
	Chip *chip = args->chip;
	uint16_t op = args->op;
	uint8_t x = get_x(op);
	chip->v[x] = op&0xff;;
}

void chip_addi_fn(ChipArgs *args){
	Chip *chip = args->chip;
	uint16_t op = args->op;
	uint8_t x = get_x(op);
	chip->v[x] += op&0xff;
}

void chip_jni_fn(ChipArgs *args){
	Chip *chip = args->chip;
	uint16_t op = args->op;
	uint8_t x = get_x(op);
	uint8_t y = get_y(op);
	if(chip->v[x] != chip->v[y]) chip->pc+=2;
}

void chip_set_i_fn(ChipArgs *args){
	Chip *chip = args->chip;
	uint16_t op = args->op;
	chip->I = op&0xfff;
}

void chip_jp_offset_fn(ChipArgs *args){
	Chip *chip = args->chip;
	uint16_t op = args->op;
	chip->pc = (op&0xfff) + chip->v[0];
}

void chip_rand_fn(ChipArgs *args){
	Chip *chip = args->chip;
	uint16_t op = args->op;
	uint8_t x = get_x(op);
	chip->v[x] = rand() % (op&0xff);
}

void chip_draw_fn(ChipArgs *args){
	Chip *chip = args->chip;
	uint16_t op = args->op;
	uint8_t x = get_x(op);
	uint8_t y = get_y(op);
	uint8_t height = op & 0xf;
	uint8_t loaded_byte, bit, draw_x, draw_y;
	chip->v[0xf] = 0;
	for(int i = 0; i < height; i++){
		loaded_byte = chip->memory[chip->I+i];
		draw_y = (chip->v[y]+i) % ROW;
		for(int j = 0; j < 8; j++){
			bit = (loaded_byte>>(7-j))&0x1;
			draw_x = (chip->v[x] + j) % COL;
			if(chip->display[draw_y][draw_x] && bit){
				chip->v[0xf] = 1;
			}
			chip->display[draw_y][draw_x] ^= bit;
		}
	}
}

int get_key(){
	/* Blocking IO op */
	fprintf(stderr, "get_key yet not implemented\n");
	getchar();
	return 0;
}

void chip_store_bcd_fn(ChipArgs *args){
	Chip *chip = args->chip;
	uint16_t op = args->op;
	uint8_t x = get_x(op);
	chip->memory[chip->I] = (chip->v[x]%1000 - chip->v[x]%100)/100; 
	chip->memory[chip->I+1] = (chip->v[x]%100 - chip->v[x]%10)/10; 
	chip->memory[chip->I+2] = chip->v[x]%10;
}

void chip_dump_fn(ChipArgs *args){
	Chip *chip = args->chip;
	uint16_t op = args->op;
	uint8_t x = get_x(op);
	for(int i = 0; i <= x; i++){
		chip->memory[chip->I + i] = chip->v[i];
	}
}

void chip_load_fn(ChipArgs *args){
	Chip *chip = args->chip;
	uint16_t op = args->op;
	uint8_t x = get_x(op);
	for(int i = 0; i <= x; i++){
		chip->v[i] = chip->memory[chip->I + i];
	}
}

void chip_high_op_fn(ChipArgs *args){
	Chip *chip = args->chip;
	uint16_t op = args->op;
	uint8_t x = get_x(op);
	switch(op&0xf0ff){
		case chip_get_delay:
			chip->v[x] = chip->delay_timer;
			return;
		case chip_get_key:
			chip->v[x] = get_key();
			return;
		case chip_set_delay:
			chip->delay_timer = chip->v[x];
			return;
		case chip_set_sound:
			chip->sound_timer = chip->v[x];
			return;
		case chip_add_addr:
			chip->I += chip->v[x];
			return;
		case chip_set_font:
			chip->I = chip->memory[chip->v[x&0xf]*FONT_WID];
			return;
		case chip_store_bcd:
			chip_store_bcd_fn(args);
			return;
		case chip_dump:
			chip_dump_fn(args);
			return;
		case chip_load:
			chip_load_fn(args);
			return;
	}
	fprintf(stderr, "(%04x) Unrecognized opcode\n", op);
}

void chip_keys_fn(ChipArgs *args){
	Chip *chip = args->chip;
	uint16_t op = args->op;
	uint8_t x = get_x(op);
	uint8_t key_idx = chip->v[x] & 0xf;
	switch(op&0xf0ff){
		case chip_key_eq:
			if(chip->keys[key_idx]) chip->pc+=2;
			return;
		case chip_key_neq:
			if(!chip->keys[key_idx]) chip->pc+=2;
			return;
	}
	fprintf(stderr, "(%04x) Unrecognized opcode\n", op);
}

void chip_calc_fn(ChipArgs *args){
	Chip *chip = args->chip;
	uint16_t op = args->op;
	uint8_t x = get_x(op);
	uint8_t y = get_y(op);
	switch(op&0xf00f){
		case chip_set_vx:
			chip->v[x] = chip->v[y];
			return;
		case chip_or:
			chip->v[x] |= chip->v[y];
			return;
		case chip_and:
			chip->v[x] &= chip->v[y];
			return;
		case chip_xor:
			chip->v[x] ^= chip->v[y];
			return;
		case chip_add:
			// 0 not 1 overflow
			chip->v[0xf] = chip->v[x] > 0xff - chip->v[y]; 
			chip->v[x] += chip->v[y];
			return;
		case chip_x_sub_y:
			// 0 underflow 1 not
			chip->v[0xf] = chip->v[x] >= chip->v[y]; 
			chip->v[x] -= chip->v[y];
			return;
		case chip_y_sub_x:
			// 0 underflow 1 not
			chip->v[0xf] = chip->v[y] >= chip->v[x]; 
			chip->v[x] = chip->v[y] - chip->v[x];
			return;
		case chip_shift_r:
			chip->v[0xf] = chip->v[x]&0x1; // lsb
			chip->v[x] >>= 1;
			return;
		case chip_shift_l:
			chip->v[0xf] = chip->v[x]&0x80; // msb
			chip->v[x] <<= 1;
			return;
	}
	fprintf(stderr, "(%04x) Unrecognized opcode\n", op);
}
