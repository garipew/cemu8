#define _GNU_SOURCE

#include "chip8.h"
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#define CLOCK_HZ 60

#define tick(T) \
	for(	clock_t die=1,diff,end,st=clock(); \
		die; \
		end=clock(),diff=(1000000L*((double)(end-st)/CLOCKS_PER_SEC)),\
		die=T > diff ? (usleep(T-diff) && 0) : 0) \

Chip chip8 = {0};
ChipArgs args = {0};

Opcode fetch_instruction(Chip *c){
	if(c->pc + 1 < MEM_SIZE){
		Opcode op = c->memory[c->pc] << 8 | c->memory[c->pc+1];
		return op; 
	}
	return chip_no_op;
}

void parse_instruction(Opcode op, FILE *stream, ChipArgs *args){
	switch(op&0xf000){
#define FUNC(arg) \
		case arg: \
			fprintf(stream, #arg); \
			fprintf(stream, "\n"); \
			args->op = op; \
			return;	
		OPCODE_LIST
#undef FUNC
	}
	fprintf(stderr, "(%04x) Unrecognized opcode\n", op);
}

int run_cycle(){
#ifndef PARSER
	chipfunc_t f;
#endif
	Opcode op = fetch_instruction(&chip8);
	if(op == chip_no_op){
		return 0;
	}
	fprintf(stdout, "0x%x:\t", chip8.pc);
	fprintf(stdout, "%.2x %.2x\t", (op>>8), op&0xff);
	parse_instruction(op, stdout, &args);
	chip8.pc+=2;
#ifndef PARSER
	f = fn_table[idx_from_opcode(op)];
	f(&args);
#endif
	return 1;
}

int main(int argc, char **argv){
	if(argc < 2){
		fprintf(stderr, "%s: Usage %s <rom-path>\n", argv[0], argv[0]);
		return 1;
	}
	srand(time(NULL));
	load_fn_table();
	initialize(&chip8);
	args.chip = &chip8;

	FILE *rom_file = fopen(argv[1], "r");
	if(!rom_file){
		fprintf(stderr, "%s: Couldn't open %s\n", argv[0], argv[1]);
		return 1;
	}
	load_game(&chip8, rom_file);
	fclose(rom_file);

	const useconds_t period = 1000000L/CLOCK_HZ;
	fprintf(stdout, "/* %s */\n\n", argv[1]);
	for(int game = 1;  game;){
		tick(period){
			//printf("tick tack\n");
			game = run_cycle();
		}
	}

	return 0;
}
