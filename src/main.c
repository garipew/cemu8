#define _GNU_SOURCE

#include "chip8.h"
#include <raylib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>

#define SNORKEL_IMPLEMENTATION
#include "snorkel/snorkel_co.h"

#define FPS 60
#define CLOCK_HZ 60
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

#ifdef PARSER
void parse_instruction(Opcode op, FILE *stream, ChipArgs *args){
#else
void parse_instruction(Opcode op, ChipArgs *args){
#endif
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
	void (*f)(ChipArgs*);
	Opcode op = fetch_instruction(&chip8);
	if(op == chip_no_op){
		return 0;
	}
#ifdef PARSER
	fprintf(stdout, "0x%x:\t", chip8.pc);
	fprintf(stdout, "%.2x %.2x\t", (op>>8), op&0xff);
	parse_instruction(op, stdout, &args);
#else
	parse_instruction(op, &args);
#endif
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

void* co_screen(){
	const struct timespec period = {0,  SEC_AS_NSEC/FPS};
	Clock clock = {0};
	clock_gettime(CLOCK_MONOTONIC, &clock.prev);

	for(; !WindowShouldClose() && is_game; ){
		/* FPS guard */
		clock.elapsed.tv_nsec = clock_get_time(clock);
		for(; clock.elapsed.tv_nsec < period.tv_nsec ;){
			yield(NULL);
			clock_gettime(CLOCK_MONOTONIC, &clock.now);
			clock.elapsed.tv_nsec = clock_get_time(clock);
		}

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
		clock_gettime(CLOCK_MONOTONIC, &clock.prev);
	}
	is_game = 0;
	return NULL;
}

void* co_cpu(){
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
		yield(NULL);
		/*fprintf(stderr, "tick\n");*/
		clock_gettime(CLOCK_MONOTONIC, &cpu_clock.prev);
	}
	return NULL;
}

void* co_input(){
	for(; is_game; ){
		get_key(&chip8);
		yield(NULL);
	}
	return NULL;
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

	InitWindow(COL<<4, ROW<<4, "cemu8");

	coroutine_create(co_cpu, NULL);
	coroutine_create(co_screen, NULL);
	coroutine_create(co_input, NULL);

	coroutine_start();

	CloseWindow();

	fprintf(stdout, "\n\n/* %s */\n\n", argv[optind]);
	return 0;
}
