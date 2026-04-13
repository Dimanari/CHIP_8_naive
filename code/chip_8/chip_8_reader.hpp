#pragma once
#include <stddef.h>

#define MEM_SIZE 0x1000
#define DISPLAY_RESERVE 0xF00
#define SYS_MEM_END 0x200
#define STACK_START 0xEA0
#define STACK_END 0xEFF

#define REGISTERS_8BIT 16
#define KEYBOARD_KEYS 16
#define CARRY_REGISTER 0xf

#define TIMER_FREQ_HZ 60
#define CPU_FREQ_HZ 500

#define DISPLAY_RES_X 64
#define DISPLAY_RES_Y 32

// 1-byte
#define SPRITE_LINE 8

#define REG_BITS 8

#define SCREEN_OP ^

// NEEDS bitfield operation over char[] at random bit start

class CHIP8
{
public:
	int ReadRom(const char* path);

	const unsigned char* GET_RAM() const { return RAM; }
	const unsigned char* GET_REGS() const { return v_regs; }
	const unsigned char* KEYBOARD() const { return keyboard; }
	short GetPC() const { return program_counter; }

	short PushStack(short addr);
	short PopStack();


	int InstructionFromAddr(short addr) const;
	int ParseInstruction();
	int ALU(short instruction);
	int EXT(short instruction);
	int Return();
	int DispClear();
	int Call(short addr);
	int DrawAt(unsigned char x_pix, unsigned char y_pix, unsigned char lines);
	unsigned char RAND();
	int GetKeyPress(); // get last key event and consume it
	int Pressed(char key)
	{
		keyboard[key] = 2;
		return 0;
	}
	int UnPressed(char key)
	{
		keyboard[key] = 0;
		return 0;
	}
private:
	size_t clock;
	size_t tick;
	int _seed;
	short address_reg;
	short program_counter;
	short stack_pointer;
	unsigned char delay;
	unsigned char sound;
	unsigned char v_regs[REGISTERS_8BIT];
	unsigned char keyboard[KEYBOARD_KEYS]; // state of the keyboard
	unsigned char RAM[MEM_SIZE];
};