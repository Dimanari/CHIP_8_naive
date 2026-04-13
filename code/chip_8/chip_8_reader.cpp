#include <memory.h>
#include <stdio.h>

#include "chip_8_reader.hpp"

static int ReadFileUpTo(const char* path, unsigned char* memory, size_t size_data);

int CHIP8::ReadRom(const char* path)
{
	address_reg = 0;
	program_counter = SYS_MEM_END;
	stack_pointer = STACK_START;
	clock = 0;
	tick = 0;
	// sanity
	memset(v_regs, 0, REGISTERS_8BIT);
	memset(keyboard, 0, KEYBOARD_KEYS);
	memset(RAM, 0, MEM_SIZE);

	return ReadFileUpTo(path, RAM + SYS_MEM_END, STACK_START - SYS_MEM_END);
}

int CHIP8::InstructionFromAddr(short addr) const
{
	return (RAM[addr] << 8) + (RAM[addr + 1]);
}

int CHIP8::ParseInstruction()
{
	unsigned short instruction = InstructionFromAddr(program_counter);
	program_counter += 2;
	clock++;

	size_t new_timer_tick = (clock * TIMER_FREQ_HZ) / CPU_FREQ_HZ;
	if(tick != new_timer_tick)
	{
		tick = new_timer_tick;
		// do timer ticks
		if(0 != sound) sound--;
		if(0 != delay) delay--;
	}

	switch ((instruction & 0xF000) >> 12)
	{
	case 0:
	{
		if(0x00E0 == instruction)
		{
			DispClear();
			return 0;
		}
		if(0x00EE == instruction)
		{
			return Return();
		}
		// not implementing 0NNN instructions for now
		// return Call(instruction & 0x0FFF);
	}
		break;
	case 0x01:
		// goto NNN; of PC = NNN
		program_counter = instruction & 0x0FFF;
		break;
	case 0x02:
		// *(0xNNN)()
		return Call(instruction & 0x0FFF);
		break;
	case 0x03:
	{
		// if (Vx == NN)
		int reg = (instruction & 0x0F00) >> 8;
		unsigned char val = (instruction & 0x00FF);
		if(v_regs[reg] == val)
			program_counter += 2;
	}
		break;
	case 0x04:
	{
		// if (Vx != NN)
		int reg = (instruction & 0x0F00) >> 8;
		unsigned char val = (instruction & 0x00FF);
		if(v_regs[reg] != val)
			program_counter += 2;
	}
		break;
	case 0x05:
	{
		// if (Vx == Vy)
		int reg = (instruction & 0x0F00) >> 8;
		int reg2 = (instruction & 0x00F0) >> 4;
		if(v_regs[reg] == v_regs[reg2])
			program_counter += 2;
	}
		break;
	case 0x06:
	{
		// Vx = NN
		int reg = (instruction & 0x0F00) >> 8;
		unsigned char val = (instruction & 0x00FF);
		v_regs[reg] = val;
	}
		break;
	case 0x07:
	{
		// Vx += NN
		int reg = (instruction & 0x0F00) >> 8;
		unsigned char val = (instruction & 0x00FF);
		v_regs[reg] += val;
	}
		break;
	case 0x08:
	{
		ALU(instruction & 0x0FFF);
	}
		break;
	case 0x09:
	{
		// if (Vx != Vy)
		int reg = (instruction & 0x0F00) >> 8;
		int reg2 = (instruction & 0x00F0) >> 4;
		if(v_regs[reg] != v_regs[reg2])
			program_counter += 2;
	}
		break;
	case 0x0a:
	{
		// I = NNN
		address_reg = instruction & 0x0FFF;
	}
		break;
	case 0x0b:
	{
		// PC = V0 + NNN
		program_counter = (instruction & 0x0FFF) + v_regs[0];
	}
		break;
	case 0x0c:
	{
		// Vx = rand() & NN
		int reg = (instruction & 0x0F00) >> 8;
		unsigned char val = (instruction & 0x00FF);
		v_regs[reg] = RAND() && val;
	}
		break;
	case 0x0d:
	{
		// draw(Vx, Vy, N) read data from location I
		int reg = (instruction & 0x0F00) >> 8;
		int reg2 = (instruction & 0x00F0) >> 4;
		int lines = (instruction & 0x000F);

		DrawAt(v_regs[reg], v_regs[reg2], lines);
	}
		break;
	case 0x0e:
	{
		if((instruction & 0x00FF) == 0x009E)
		{
			//if (key() == Vx)
			int reg = (instruction & 0x0F00) >> 8;
			int Key = v_regs[reg] & 0x0F;
			if(keyboard[Key])
				program_counter += 2;
		}
		if((instruction & 0x00FF) == 0x00A1)
		{
			//if (key() != Vx)
			int reg = (instruction & 0x0F00) >> 8;
			int Key = v_regs[reg] & 0x0F;
			if(!keyboard[Key])
				program_counter += 2;
		}
	}
		break;
	case 0x0f:
	{
		EXT(instruction & 0x0FFF);
	}
		break;
	}


	for(int key = 0; key < KEYBOARD_KEYS; ++key)
		if(keyboard[key]) keyboard[key] = 1;
	
	return 0;
}

int CHIP8::EXT(short instruction)
{
	int reg = (instruction & 0x0F00) >> 8;

	switch (instruction & 0xFF)
	{
	case 0x07:
		//Vx = get_delay()
		v_regs[reg] = delay;
		break;
	case 0x0A:
		{
			//Vx = get_key()
			int key_value = GetKeyPress();
			if(-1 == key_value)
			{
				program_counter -= 2;
			}
			else
			{
				v_regs[reg] = key_value;
			}
		}
		break;
	case 0x15:
		//delay_timer(Vx)
		delay = v_regs[reg];
		break;
	case 0x18:
		//sound_timer(Vx)
		sound = v_regs[reg];
		break;
	case 0x1E:
		//I += Vx
		address_reg += v_regs[reg];
		break;
	case 0x29:
		//I = sprite_addr[Vx]
		break;
	case 0x33:
		//set_BCD(Vx) {*(I+0) = BCD(3); *(I+1) = BCD(2); *(I+2) = BCD(1);}
		{
			RAM[address_reg] = v_regs[reg] / 100;
			RAM[address_reg + 1] = (v_regs[reg] / 10) % 10;
			RAM[address_reg + 2] = v_regs[reg] % 10;
		}
		break;
	case 0x55:
		//reg_dump(Vx, &I)
		{
			for(int i = 0; i <= reg; ++i)
			{
				RAM[address_reg + i] = v_regs[i];
			}
		}
		break;
	case 0x65:
		//reg_load(Vx, &I)
		{
			for(int i = 0; i <= reg; ++i)
			{
				v_regs[i] = RAM[address_reg + i];
			}
		}
		break;
	}
	return 0;
}

int CHIP8::ALU(short instruction)
{
	int reg = (instruction & 0x0F00) >> 8;
	int reg2 = (instruction & 0x00F0) >> 4;
	switch(instruction & 0x000F)
	{
	case 0x00:
	{
		// Vx = Vy
		v_regs[reg] = v_regs[reg2];
	}
		break;
	case 0x01:
	{
		// Vx |= Vy
		v_regs[reg] |= v_regs[reg2];
	}
		break;
	case 0x02:
	{
		// Vx &= Vy
		v_regs[reg] &= v_regs[reg2];
	}
		break;
	case 0x03:
	{
		// Vx ^= Vy
		v_regs[reg] ^= v_regs[reg2];
	}
		break;
	case 0x04:
	{
		// Vx += Vy

		int value = (int)v_regs[reg] + (int)v_regs[reg2];

		// overflow check
		v_regs[reg] = value & 0xFF;
		v_regs[0xf] = (value > 0xFF);

	}
		break;
	case 0x05:
	{
		// Vx -= Vy
		
		// underflow check
		int value = (int)v_regs[reg] - (int)v_regs[reg2];
		v_regs[reg] = value & 0xFF;
		v_regs[0xf] = value >= 0;
	}
		break;
	case 0x06:
	{
		// Vx >>= 1
		
	#ifdef ORG_INTERP
		unsigned char reg_read = v_regs2[reg];
	#else
		unsigned char reg_read = v_regs[reg];
	#endif
		// underflow check
		v_regs[reg] = reg_read >> 1;
		v_regs[0xf] = reg_read & 1;
	}
		break;
	case 0x07:
	{
		// Vx = Vy - Vx


		// underflow check
		int value = v_regs[reg2] - (int)v_regs[reg];
		v_regs[reg] = value;
		v_regs[0xf] = value >= 0;
	}
		break;
	case 0x0E:
	{
		// Vx <<= 1
		
	#ifdef ORG_INTERP
		unsigned char reg_read = v_regs2[reg];
	#else
		unsigned char reg_read = v_regs[reg];
	#endif
		// underflow check
		v_regs[reg] = reg_read << 1;
		v_regs[0xf] = reg_read >> (REG_BITS - 1);
	}
		break;
	}
	return 0;
}

short CHIP8::PushStack(short addr)
{
	RAM[stack_pointer] = addr >> 8;
	RAM[stack_pointer + 1] = addr & 0xFF;

	stack_pointer += 2;
	return stack_pointer;
}
short CHIP8::PopStack()
{
	if(stack_pointer == STACK_START)
		return -1;
	stack_pointer -= 2;
	unsigned short addr = InstructionFromAddr(stack_pointer);
	return addr;
}

int CHIP8::Return()
{
	short addr = PopStack();
	if(-1 == addr)
		// EXIT
		return 1;
	
	program_counter = addr;
	return 0;
}

int CHIP8::Call(short addr)
{
	short stack = PushStack(program_counter);
	if(stack == STACK_END)
		// STACK OVERFLOW
		return 2;
	program_counter = addr;
	return 0;
}

int CHIP8::DispClear()
{
	unsigned char* display = RAM + DISPLAY_RESERVE;
	memset(display, 0, (MEM_SIZE - DISPLAY_RESERVE));

	return 0;
}
int CHIP8::DrawAt(unsigned char x_pix, unsigned char y_pix, unsigned char lines)
{
	unsigned char* display = RAM + DISPLAY_RESERVE;
	unsigned char* sprite = RAM + address_reg;

	v_regs[0x0F] = 0;

	for(int y = 0; y < lines; ++y)
	{
		int real_y = y + y_pix;
		if(real_y >= DISPLAY_RES_Y)
			break;

		int real_x_byte = x_pix / 8;
		int real_x_bit = x_pix & 7;

		if(0 == real_x_bit)
		{
			// aligned blitting - faster
			unsigned char old = display[real_y * DISPLAY_RES_X / 8 + real_x_byte];
			display[real_y * DISPLAY_RES_X / 8 + real_x_byte] ^= sprite[y];

			if(old != (old & display[real_y * DISPLAY_RES_X / 8 + real_x_byte]))
			{
				v_regs[0x0F] = 1;
			}
		}
		else
		{
			// unaligned blitting
			unsigned char old = display[real_y * DISPLAY_RES_X / 8 + real_x_byte];
			display[real_y * DISPLAY_RES_X / 8 + real_x_byte] ^= sprite[y] >> real_x_bit;

			if(old != (old & display[real_y * DISPLAY_RES_X / 8 + real_x_byte]))
			{
				v_regs[0x0F] = 1;
			}

			if(real_x_byte + 1 < DISPLAY_RES_X)
			{
				unsigned char old = display[real_y * DISPLAY_RES_X / 8 + real_x_byte + 1];
				display[real_y * DISPLAY_RES_X / 8 + real_x_byte + 1] ^= sprite[y] << (8 - real_x_bit);

				if(old != (old & display[real_y * DISPLAY_RES_X / 8 + real_x_byte + 1]))
				{
					v_regs[0x0F] = 1;
				}
			}
		}
	}

	return 0;
}

unsigned char CHIP8::RAND()  { return 0; }
int CHIP8::GetKeyPress()
{
	for(int key = 0; key < KEYBOARD_KEYS; ++key)
		if(2 == keyboard[key])
		{
			return key + 1;
		}
	return 0;
} // get last key event and consume it

int ReadFileUpTo(const char* path, unsigned char* memory, size_t size_data)
{
	if(NULL == path)
		return -2;
	if(NULL == memory)
		return -2;
	FILE* pfile = fopen(path, "rb");
	if(NULL == pfile)
		return -1;
	size_t read_bytes = 0;
	while(size_data > read_bytes)
	{
		int byte = fgetc(pfile);
		if(EOF == byte)
			break;
		*memory++ = byte;
		read_bytes++;
	}
	
	fclose(pfile);

	return read_bytes;
}