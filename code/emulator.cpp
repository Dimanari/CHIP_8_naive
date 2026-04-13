#include <memory.h>
#include <stdio.h>
#include <SDL.h>

#include <chip_8/chip_8_reader.hpp>

void printRam(const unsigned char* mem, size_t size);
int GetCycleCount(Uint64 elapsed, int cpu_freq);
void DrawFrom(const unsigned char* display, int res_x, int res_y);

int key_map[KEYBOARD_KEYS] = 
{
	SDLK_KP_1,
	SDLK_KP_2,
	SDLK_KP_3,
	SDLK_KP_4,
	SDLK_KP_5,
	SDLK_KP_6,
	SDLK_KP_7,
	SDLK_KP_8,
	SDLK_KP_9,
	SDLK_KP_0,
	SDLK_RETURN,
	SDLK_SPACE
};

int main(int argc, char* argv[])
{
	CHIP8 emulator;
	size_t bytes = emulator.ReadRom("Pong_1P.ch8");

	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Failed to initialize SDL: %s\n", SDL_GetError());
        return -1;
    }
	int inst_res = 0;
	char quit = false;

	Uint64 timer = SDL_GetTicks64();
	Sint64 accumulated = 0;
	while (!quit)
    {
        SDL_Event e;
        while (SDL_PollEvent(&e) != 0)
        {
            if (e.type == SDL_QUIT)
            {
                quit = true;
            }
            // handle inputs
			if(SDL_KEYDOWN == e.type)
			{
				for(int i=0;i<KEYBOARD_KEYS;++i)
					if(e.key.keysym.sym == key_map[i])
						emulator.Pressed(i);
			}
			if(SDL_KEYUP == e.type)
			{
				for(int i=0;i<KEYBOARD_KEYS;++i)
					if(e.key.keysym.sym == key_map[i])
						emulator.UnPressed(i);
			}
        }

		Uint64 now = SDL_GetTicks64();
		Uint64 elapsed = (now - timer);
		timer = now;
		accumulated += elapsed * 1000;

#define US_PER_TICK (1000000 / CPU_FREQ_HZ)

		
		char redraw = false;
		while(accumulated > US_PER_TICK)
		{
			inst_res = emulator.ParseInstruction();
			
			if(0 != inst_res)
			{
				quit = true;
				break;
			}

			accumulated -= US_PER_TICK;
			redraw = true;
		}
		// render
		if(redraw)
		{
			DrawFrom(emulator.GET_RAM() + DISPLAY_RESERVE, DISPLAY_RES_X, DISPLAY_RES_Y);

		}
		SDL_Delay(12);
    }
	SDL_Quit();
	return 0;
}

void printRam(const unsigned char* mem, size_t size)
{
	int format_choice = 0;
	while(size--)
	{
		const char *format[] = {"0x%02x", "%02x "};
		int value = (*mem++);
		printf(format[format_choice & 1],value);
		format_choice++;
		if(format_choice == 2*8)
		{
			format_choice = 0;
			printf("\n");
		}
	}
}

int GetCycleCount(Uint64 elapsed, int cpu_freq)
{
	return SDL_ceil(elapsed * cpu_freq / 1000.f);
}

void DrawFrom(const unsigned char* display, int res_x, int res_y)
{
	printf("\033[2J\033[H\033c");
	for(int y = 0; y < res_y; ++y)
	{
		for(int x = 0; x < res_x; ++x)
		{
			char data = display[x / 8 + res_x / 8 * y];

			char data_bit = data & (0x80 >> (x & 7));
			const char* disp[] = {"\033[47m ", "\033[40m "};
			printf("%s", disp[!data_bit]);
		};
		printf("\n");
	}
}