#using the proper command to compile everything
CC = g++ -std=c++17 -pedantic-errors -Wall -Wextra
CCWIN = x86_64-w64-mingw32-g++ --static -std=c++17 -pedantic-errors -Wall -Wextra

#hard Drive
HDD = /run/media/dimanari/HDD/

# all the flags we can ever want
CFLAGS = -I code -I $(HDD)Coding/OpenGL/glm/ `sdl2-config --cflags`
CFLAGSWIN = -I code -I $(HDD)Coding/OpenGL/glm/ -I "$(HDD)Coding/SDL 2/SDL2-2.0.14/include/" \
	-I "$(HDD)Coding/OpenGL/glew-2.2.0/include/"

# valgrind used to be a nice tool for memory leak detection but is no longer updated
vlg = valgrind --leak-check=yes --track-origins=yes

# release and debug flags so we can debug our code and optimize it at will
RELEASE = -DNDEBUG -O3
DEBUG = -D_DEBUG -g

# included libraries
libs = `sdl2-config --libs` -lSDL2_image -lSDL2_ttf -lSDL2_mixer -lGL -lGLU -lGLEW
libs_win = -L $(HDD)Coding/OpenGL/glew-2.2.0/lib/x64/ -L"$(HDD)Coding/SDL 2/SDL2-2.0.14/lib/x64/" \
	-lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer -lopengl32 -lglu32 -lglew32

# header libraries are a shared dependancy

BUILD_DIR := out
SOURCEDIR := code
header_files := $(shell find $(SOURCEDIR) -name '*.hpp') $(shell find $(SOURCEDIR) -name '*.h')

SRCS := $(shell find $(SOURCEDIR) -name '*.cpp')

OBJS := $(addprefix $(BUILD_DIR)/,$(patsubst $(SOURCEDIR)/%.cpp,%.o,$(SRCS)))
# generate paths for all game objects
game_obj = $(OBJS)#$(patsubst code/%.cpp,out/%.o,$(wildcard code/*/*.cpp))

# actual game object paths
objects = $(game_obj)
objects_debug = $(objects:.o=_d.o)
objects_win = $(objects:.o=_win.o)

# all of our final files
all : release debug windows

# names of executables
debug : game_d.out
release : game_r.out
windows : game.exe

# how to compile object files
$(BUILD_DIR)/%.o : $(SOURCEDIR)/%.cpp $(header_files)
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $(RELEASE) -c -o $@ $<

$(BUILD_DIR)/%_d.o : $(SOURCEDIR)/%.cpp $(header_files)
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $(DEBUG) -c -o $@ $<

$(BUILD_DIR)/%_win.o : $(SOURCEDIR)/%.cpp $(header_files)
	mkdir -p $(@D)
	$(CCWIN) $(CFLAGSWIN) $(RELEASE) -c -o $@ $<

# how to compile output file
game_d.out : $(objects_debug)
	$(CC) $(CFLAGS) $(DEBUG) -o $@ $^ $(libs)

%_r.out : $(objects)
	$(CC) $(CFLAGS) $(RELEASE) -o $@ $^ $(libs)

%.exe : $(objects_win)
	$(CCWIN) $(CFLAGSWIN) $(RELEASE) -o $@ $^ $(libs_win)

# run our files through make
debug_r : game_d.out
	./$^

release_r : game_r.out
	./$^

release_win : game.exe
	wine ./$^

$$(@D):
	@mkdir -p $$@
#cleanup
.PHONY : clean
clean : 
	rm -f out/*/*.o *.out *.exe
