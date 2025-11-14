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
	c->op = chip_no_op;
	c->I = 0;
	c->delay_timer = 60;
	c->sound_timer = 60;
}

void load_game(Chip* chip, FILE* stream){
	if(chip->sp < STACK_LEVELS){
		push(chip, chip->pc);
	} else{
		printf("Stack Overflow.\n");
		return;
	}
	int c;
	chip->pc = 0x200;
	while((c = getc(stream)) != EOF){
		if(chip->pc >= MEM_SIZE){
			ungetc(c, stream);
			break;
		}
		chip->memory[chip->pc++] = (uint8_t)c;
	}
	chip->pc = pop(chip);
}

void chip_low_op_fn(ChipArgs *args){
	Chip *chip = args->chip;
	uint16_t op = args->op;
	switch(op){
		case chip_clear:
			for(int i = 0; i < ROW; i++){ 
				memset(chip->display, 0, COL*(sizeof(*chip->display))); 
			}
			break;
		case chip_return:
			args->chip->pc = pop(args->chip);
			break;
		default:
			fprintf(stderr, "(%04x) Unrecognized opcode\n", op);
			break;
	}
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
	fprintf(stderr, "chip_draw_fn yet not implemented\n");
}

int get_key(){
	/* Blocking IO op */
	fprintf(stderr, "get_key yet not implemented\n");
	return 0;
}

void chip_high_op_fn(ChipArgs *args){
	Chip *chip = args->chip;
	uint16_t op = args->op;
	uint8_t x = get_x(op);
	switch(op&0xf0ff){
		case chip_get_delay:
			chip->v[x] = chip->delay_timer;
			break;
		case chip_get_key:
			chip->v[x] = get_key();
			break;
		case chip_set_delay:
			chip->delay_timer = chip->v[x];
			break;
		case chip_set_sound:
			chip->sound_timer = chip->v[x];
			break;
		case chip_add_addr:
			chip->I += chip->v[x];
			break;
		case chip_set_font:
			// chip->I = font[chip->v[x&0xf]];
			break;
		case chip_store_bcd:
			chip->memory[chip->I] = (chip->v[x]%1000 - chip->v[x]%100)/100; 
			chip->memory[chip->I+1] = (chip->v[x]%100 - chip->v[x]%10)/10; 
			chip->memory[chip->I+2] = chip->v[x]%10;
			break;
		case chip_dump:
			for(int i = 0; i <= x; i++){
				chip->memory[chip->I + i] = chip->v[i];
			}
			break;
		case chip_load:
			for(int i = 0; i <= x; i++){
				chip->v[i] = chip->memory[chip->I + i];
			}
			break;
		default:
			fprintf(stderr, "(%04x) Unrecognized opcode\n", op);
			break;
	}
}

void chip_keys_fn(ChipArgs *args){
	Chip *chip = args->chip;
	uint16_t op = args->op;
	uint8_t x = get_x(op);
	switch(op&0xf0ff){
		case chip_key_eq:
			if(chip->keys[chip->v[x]]) chip->pc+=2;
			break;
		case chip_key_neq:
			if(!chip->keys[chip->v[x]]) chip->pc+=2;
			break;
		default:
			fprintf(stderr, "(%04x) Unrecognized opcode\n", op);
			break;
	}
}

void chip_calc_fn(ChipArgs *args){
	Chip *chip = args->chip;
	uint16_t op = args->op;
	uint8_t x = get_x(op);
	uint8_t y = get_y(op);
	switch(op&0xf00f){
		case chip_set_vx:
			chip->v[x] = chip->v[y];
			break;
		case chip_or:
			chip->v[x] |= chip->v[y];
			break;
		case chip_and:
			chip->v[x] &= chip->v[y];
			break;
		case chip_xor:
			chip->v[x] ^= chip->v[y];
			break;
		case chip_add:
			// 0 not 1 overflow
			chip->v[0xf] = chip->v[x] > 0xff - chip->v[y]; 
			chip->v[x] += chip->v[y];
			break;
		case chip_x_sub_y:
			// 0 underflow 1 not
			chip->v[0xf] = chip->v[x] >= chip->v[y]; 
			chip->v[x] -= chip->v[y];
			break;
		case chip_y_sub_x:
			// 0 underflow 1 not
			chip->v[0xf] = chip->v[y] >= chip->v[x]; 
			chip->v[x] = chip->v[y] - chip->v[x];
			break;
		case chip_shift_r:
			chip->v[0xf] = chip->v[x]&0x1; // lsb
			chip->v[x] >>= 1;
			break;
		case chip_shift_l:
			chip->v[0xf] = chip->v[x]&0x80; // msb
			chip->v[x] <<= 1;
			break;
		default:
			fprintf(stderr, "(%04x) Unrecognized opcode\n", op);
			break;
	}
}
