#define _GNU_SOURCE

#include "chip8.h"
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#define CLOCK_HZ 60
// TODO(garipew): There isn't a single defined value for the CPU frequency and
// the required frequency varies across roms, this should be set at execution
// instead of at compilation.
#define CPU_HZ 700


// NOTE(garipew): Here lies my first sorcery, unfortunately it could not
// survive dealing with multiple frequencies without becoming too troublesome.
/*
#define tick(till, T) \
	for(	clock_t tick_t=clock(); \
		till; \
		tick_t=((double)(clock()-tick_t)/CLOCKS_PER_SEC)*1000000L,\
		T > tick_t ? \
		(usleep(T-tick_t) && (tick_t=clock())) :\
		(tick_t=clock()))
*/
// Gone but not forgotten. :(

#define SEC_IN_NSEC 1000000000L

typedef struct {
	struct timespec prev;
	struct timespec now;
	struct timespec elapsed;
} Clock;

#define clock_get_time(c) \
	((c.now.tv_sec-c.prev.tv_sec)*SEC_IN_NSEC)+c.now.tv_nsec-c.prev.tv_nsec

Clock cpu_clock = {0};
Clock delay_clock = {0};
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

void clock_tick(){
	if(chip8.delay_timer){
		chip8.delay_timer--;
	}
	if(chip8.sound_timer){
		chip8.sound_timer--;
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

	fprintf(stdout, "/* %s */\n\n", argv[1]);

	const struct timespec clock_period = {0,  SEC_IN_NSEC/CLOCK_HZ};
	const struct timespec cpu_period = {0, SEC_IN_NSEC/CPU_HZ};

	clock_gettime(CLOCK_MONOTONIC, &delay_clock.prev);
	cpu_clock.prev = delay_clock.prev;

	struct timespec sleep_time = {0};
	for(; ;){
		clock_gettime(CLOCK_MONOTONIC, &delay_clock.now);
		delay_clock.elapsed.tv_nsec = clock_get_time(delay_clock);
		if(delay_clock.elapsed.tv_nsec > clock_period.tv_nsec){
			clock_tick();
			//fprintf(stderr, "\t\t\ttack\n");
			delay_clock.prev = delay_clock.now;
		}

		cpu_clock.now = delay_clock.now;
		cpu_clock.elapsed.tv_nsec = clock_get_time(cpu_clock);
		if(cpu_clock.elapsed.tv_nsec < cpu_period.tv_nsec){
			sleep_time.tv_nsec = cpu_period.tv_nsec;
			sleep_time.tv_nsec -= cpu_clock.elapsed.tv_nsec;
			nanosleep(&sleep_time, NULL);
		}
		run_cycle();
		//fprintf(stderr, "tick\n");
		clock_gettime(CLOCK_MONOTONIC, &cpu_clock.prev);
	}

	return 0;
}
