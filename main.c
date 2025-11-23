#define _GNU_SOURCE

#include "chip8.h"
#include <raylib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>
#include <snorkel.h>

#define FPS 60
#define CLOCK_HZ 60
// TODO(garipew): There isn't a single defined value for the CPU frequency and
// the required frequency varies across roms, this should be set at execution
// instead of at compilation.
static long CPU_HZ = 500;

#define SEC_AS_NSEC 1000000000L

typedef struct {
	struct timespec prev;
	struct timespec now;
	struct timespec elapsed;
} Clock;

#define clock_get_time(c) \
	(((c).now.tv_sec-(c).prev.tv_sec)*SEC_AS_NSEC)+(c).now.tv_nsec-(c).prev.tv_nsec

Clock cpu_clock = {0};
Clock delay_clock = {0};
Chip chip8 = {0};
ChipArgs args = {0};
int is_game = 1;

Opcode fetch_instruction(Chip *c){
	if(c->pc + 1 < MEM_SIZE){
		Opcode op = c->memory[c->pc] << 8 | c->memory[c->pc+1];
		return op; 
	}
	return chip_no_op;
}

void parse_instruction(Opcode op, FILE *stream, ChipArgs *args){
	args->op = op;
#ifdef PARSER
	switch(op&0xf000){
#define FUNC(arg) \
		case arg: \
			fprintf(stream, #arg); \
			fprintf(stream, "\n"); \
			return;	
		OPCODE_LIST
#undef FUNC
	}
	fprintf(stderr, "(%04x) Unrecognized opcode\n", op);
#endif
}

int run_cycle(){
	chipfunc_t f;
	Opcode op = fetch_instruction(&chip8);
	if(op == chip_no_op){
		return 0;
	}
#ifdef PARSER
	fprintf(stdout, "0x%x:\t", chip8.pc);
	fprintf(stdout, "%.2x %.2x\t", (op>>8), op&0xff);
#endif
	parse_instruction(op, stdout, &args);
	if(chip8.status == RUN){
		chip8.pc+=2;
	}
	f = fn_table[idx_from_opcode(op)];
	f(&args);
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

void fix_schedule(const struct timespec *period, Clock *clock){
	struct timespec sleep_time = {0};
	clock->elapsed.tv_nsec = clock_get_time(*clock);
	if(clock->elapsed.tv_nsec < period->tv_nsec){
		sleep_time.tv_nsec = period->tv_nsec;
		sleep_time.tv_nsec -= clock->elapsed.tv_nsec;
		nanosleep(&sleep_time, NULL);
	}
}

void co_screen(){
	const struct timespec period = {0,  SEC_AS_NSEC/FPS};
	Clock clock = {0};
	clock_gettime(CLOCK_MONOTONIC, &clock.prev);

	InitWindow(COL<<4, ROW<<4, "cemu8");
	for(; !WindowShouldClose() && is_game; ){
		/* FPS guard */
		clock_gettime(CLOCK_MONOTONIC, &clock.now);
		clock.elapsed.tv_nsec = clock_get_time(clock);
		while(clock.elapsed.tv_nsec < period.tv_nsec){
			fprintf(stderr, "still here\n");
			yield;
			clock_gettime(CLOCK_MONOTONIC, &clock.now);
			clock.elapsed.tv_nsec = clock_get_time(clock);
		}
		clock_gettime(CLOCK_MONOTONIC, &clock.prev);
		fprintf(stderr, "draw\n");

		BeginDrawing();
		ClearBackground(BLACK);
		for(int x, y, pixel = 0; pixel < ROW*COL; pixel++){
			x = pixel % COL;
			y = pixel / COL;
			if(!chip8.display[y][x]){
				continue;
			}
			DrawRectangle(x<<4, y<<4, PIXEL, PIXEL, WHITE);
		}
		EndDrawing();
	}
	CloseWindow();
	is_game = 0;
}

void co_cpu(){
	const struct timespec clock_period = {0,  SEC_AS_NSEC/CLOCK_HZ};
	const struct timespec cpu_period = {0, SEC_AS_NSEC/CPU_HZ};

	clock_gettime(CLOCK_MONOTONIC, &delay_clock.prev);
	cpu_clock.prev = delay_clock.prev;

	for(; is_game ;){
		clock_gettime(CLOCK_MONOTONIC, &delay_clock.now);
		delay_clock.elapsed.tv_nsec = clock_get_time(delay_clock);
		if(delay_clock.elapsed.tv_nsec > clock_period.tv_nsec){
			clock_tick();
			/*fprintf(stderr, "\t\t\ttack\n");*/
			delay_clock.prev = delay_clock.now;
		}

		cpu_clock.now = delay_clock.now;
		cpu_clock.elapsed.tv_nsec = clock_get_time(cpu_clock);
		fix_schedule(&cpu_period, &cpu_clock);
		run_cycle();
		yield;
		/*fprintf(stderr, "tick\n");*/
		clock_gettime(CLOCK_MONOTONIC, &cpu_clock.prev);
	}
}

void co_input(){
	for(; is_game; ){
		get_key(&chip8);
		yield;
	}
}

int main(int argc, char **argv){
	struct option options[2] = {0};
	options[0].name = "cpu";
	options[0].has_arg = required_argument;
	options[0].val = 'c';
	int opt;

	while((opt=getopt_long(argc, argv, "c:", options, NULL)) != -1){
		switch(opt){
			case 'c':
				CPU_HZ = strtol(optarg, NULL, 10);
				break;
			default:
				fprintf(stderr, "%s: -%c not an option\n",
						argv[0], opt);
				return -1;
		}
	}

	if(optind >= argc){
		fprintf(stderr, "%s: Usage %s <rom-path>\n", argv[0], argv[0]);
		return 1;
	}
	srand(time(NULL));
	load_fn_table();
	initialize(&chip8);
	args.chip = &chip8;

	FILE *rom_file = fopen(argv[optind], "r");
	if(!rom_file){
		fprintf(stderr, "%s: Couldn't open %s\n", argv[0], argv[optind]);
		return 1;
	}
	load_game(&chip8, rom_file);
	fclose(rom_file);

	fprintf(stdout, "\n/* %s */\n\n\n", argv[optind]);

	coroutine_create(co_cpu);
	coroutine_create(co_screen);
	coroutine_create(co_input);
	coroutine_start();

	fprintf(stdout, "\n\n/* %s */\n\n", argv[optind]);
	return 0;
}
