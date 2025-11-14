#include "chip8.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

void initialize(Chip* c){
	c->pc = 0x200;
	c->sp = 0;
	c->op = NO_OP;
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
