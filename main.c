#include "chip8.h"
#include <time.h>
#include <stdio.h>

Chip chip8 = {0};
ChipArgs args = {0};

Opcode fetch_instruction(Chip *c){
	if(c->pc + 1 < MEM_SIZE){
		Opcode op = c->memory[c->pc] << 8 | c->memory[c->pc+1];
		return op; 
	}
	return chip_no_op;
}

/* chipfunc_t parse_instruction(int, Opcode, FILE*, ChipArgs*)
 * This function writes the next instruction to FILE* and
 * fill ChipArgs*
 * return a function pointer equivalent to the parsed instruction.
 */
void parse_instruction(Opcode op, FILE *stream, ChipArgs *args){
	switch(op&0xf000){
#define FUNC(arg) \
		case arg: \
			fprintf(stream, #arg); \
			fprintf(stream, " nnn = 0x%x\n", op&0xfff); \
			args->op = op; \
			break;
		OPCODE_LIST
#undef FUNC
		default:
			fprintf(stderr, "(%04x) Unrecognized opcode\n", op);
			break;
	}
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

	chipfunc_t f;
	fprintf(stdout, "/* %s */\n\n", argv[1]);
	for(Opcode op; (op=fetch_instruction(&chip8)) != chip_no_op; ){
		fprintf(stdout, "0x%04x:\t%02x %02x\t", chip8.pc, (op>>8)&0xff, op&0xff);
		chip8.pc+=2;
		parse_instruction(op, stdout, &args);
#ifdef RUN
		f = fn_table[idx_from_opcode(op)];
		f(&args);
#endif
	}
	return 0;
}
