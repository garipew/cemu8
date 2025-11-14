#include "chip8.h"
#include <time.h>

Chip chip8 = {0};

Opcode fetch_instruction(Chip *c){
	if(c->pc + 1 < MEM_SIZE){
		Opcode op = c->memory[c->pc] << 8 | c->memory[c->pc+1];
		return op; 
	}
	return NO_OP;
}

void parse_instruction(int pc, Opcode op, FILE *stream){
	fprintf(stream, "0x%04x:\t%02x %02x\t", pc, (op>>8)&0xff, op&0xff);
	switch(op&0xF000){
		case LOW_OP:
			switch(op&0xFF){
				case CLEAR:
					fprintf(stream, "screen_clear(chip);\n");
					break;
				case RETURN:
					fprintf(stream, "chip->pc = pop(chip); // return\n");
					break;
				default:
					fprintf(stream, "push(chip, chip->pc); chip->pc = 0x%04x;\n", op&0xfff);
					break;
			}
			break;
		case JP:
			fprintf(stream, "chip->pc = 0x%04x;\n", op&0xfff);
			break;
		case FUNCTION_CALL:
			fprintf(stream, "push(chip, chip->pc); chip->pc = 0x%04x;\n", op&0xfff);
			break;
		case SKIP_EQ:
			fprintf(stream, "if(chip->v[%d] == 0x%x) chip->pc+=2;\n", get_x(op), op&0xff);
			break;
		case SKIP_NE:
			fprintf(stream, "if(chip->v[%d] != 0x%x) chip->pc+=2;\n", get_x(op), op&0xff);
			break;
		case JEI:
			fprintf(stream, "if(chip->v[%d] == chip->v[%d]) chip->pc+=2;\n", get_x(op), get_y(op));
			break;
		case SET_REG:
			fprintf(stream, "chip->v[%d] = 0x%02x;\n", get_x(op), op&0xff);
			break;
		case ADDI:
			fprintf(stream, "chip->v[%d] += 0x%x;\n", get_x(op), op&0xff);
			break;
		case CALC:
			switch(op&0xF00F){
				case SET_VX:
					fprintf(stream, "chip->v[%d] = chip->v[%d];\n", get_x(op), get_y(op));
					break;
				case OR:
					fprintf(stream, "chip->v[%d] |= chip->v[%d];\n", get_x(op), get_y(op));
					break;
				case AND:
					fprintf(stream, "chip->v[%d] &= chip->v[%d];\n", get_x(op), get_y(op));
					break;
				case XOR:
					fprintf(stream, "chip->v[%d] ^= chip->v[%d];\n", get_x(op), get_y(op));
					break;
				case ADD:
					fprintf(stream, "chip->v[%d] += chip->v[%d];\n", get_x(op), get_y(op));
					break;
				case X_SUB_Y:
					fprintf(stream, "chip->v[%d] -= chip->v[%d];\n", get_x(op), get_y(op));
					break;
				case Y_SUB_X:
					fprintf(stream, "chip->v[%d] = chip->v[%d] - chip->v[%d];\n", get_x(op), get_y(op), get_x(op));
					break;
				case SHIFT_R:
					fprintf(stream, "chip->v[0xf] = chip->v[%d]&0x1; chip->v[%d] >>= 1;\n", get_x(op), get_x(op));
					break;
				case SHIFT_L:
					fprintf(stream, "chip->v[0xf] = chip->v[%d]&0x80; chip->v[%d] >>= 1;\n", get_x(op), get_x(op));
					break;
				default:
					fprintf(stream, "// (0x%04x) Calc op not supported\n", op);
					break;
			}
			break;
		case JNI:
			fprintf(stream, "if(chip->v[%d] != chip->v[%d]) chip->pc+=2;\n", get_x(op), get_y(op));
			break;
		case SET_I:
			fprintf(stream, "chip->I = 0x%03x;\n", op&0xfff);
			break;
		case JP_OFFSET:
			fprintf(stream, "chip->pc = 0x%03x + chip->v[0];\n", op&0xfff);
			break;
		case RAND:
			fprintf(stream, "chip->v[%d] = rand() %% 0x%x;\n", get_x(op), op&0xff);
			break;
		case DRAW:
			fprintf(stream, "draw(chip, chip->v[%d], chip->v[%d], 0x%x);\n", get_x(op), get_y(op), op&0xf);
			break;
		case KEYS:
			switch(op&0xE0FF){
				case KEY_EQ:
					fprintf(stream, "if(is_pressed(chip, chip->keys[chip->v[%d]])) chip->pc+=2;\n", get_x(op));
					break;
				case KEY_NEQ:
					fprintf(stream, "if(!is_pressed(chip, chip->keys[chip->v[%d]])) chip->pc+=2;\n", get_x(op));
					break;
				default:
					fprintf(stream, "// (0x%04x) Unrecognized instruction\n", op);
					break;
			}
			break;
		case EXTRA:
			switch(op&0xF0FF){
				case GET_DELAY:
					fprintf(stream, "chip->v[%d] = get_delay(chip);\n", get_x(op));
					break;
				case GET_KEY:
					fprintf(stream, "chip->v[%d] = get_key();\n", get_x(op));
					break;
				case SET_DELAY:
					fprintf(stream, "set_delay(chip, chip->v[%d]);\n", get_x(op));
					break;
				case SET_SOUND:
					fprintf(stream, "set_sound(chip, chip->v[%d]);\n", get_x(op));
					break;
				case ADD_ADDR:
					fprintf(stream, "chip->I += chip->v[%d];\n", get_x(op));
					break;
				case SET_FONT:
					fprintf(stream, "chip->I = chip->memory[FONT_START+chip->v[%d]*FONT_LEN];\n", get_x(op));
					break;
				case WRITE_VX:
					fprintf(stream, "store_bcd(chip, %d);\n", get_x(op));
					break;
				case DUMP:
					fprintf(stream, "for(int i = 0; i <= %d; i++){ chip->memory[chip->I + i] = chip->v[i]; }\n", get_x(op));
					break;
				case LOAD:
					fprintf(stream, "for(int i = 0; i <= %d; i++){ chip->v[i] = chip->memory[chip->I + i]; }\n", get_x(op));
					break;
				default:
					fprintf(stream, "// (0x%04x) not an operation\n", op);
					break;
			}
			break;
		default:
			fprintf(stream, "// (0x%04x) not an operation\n", op);
			break;
	}
}

int main(int argc, char **argv){
	if(argc < 2){
		fprintf(stderr, "%s: Usage %s <rom-path>\n", argv[0], argv[0]);
		return 1;
	}
	srand(time(NULL));
	initialize(&chip8);

	FILE *rom_file = fopen(argv[1], "r");
	if(!rom_file){
		fprintf(stderr, "%s: Couldn't open %s\n", argv[0], argv[1]);
		return 1;
	}
	load_game(&chip8, rom_file);
	fclose(rom_file);

	fprintf(stdout, "/* %s */\n\n", argv[1]);
	for(Opcode op; (op=fetch_instruction(&chip8)) != NO_OP; chip8.pc+=2){
		parse_instruction(chip8.pc, op, stdout);
	}
	return 0;
}
