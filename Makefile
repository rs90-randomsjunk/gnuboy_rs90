
prefix = 
exec_prefix = ${prefix}
bindir = ${exec_prefix}/bin

CC = /opt/rs90-toolchain/bin/mipsel-linux-gcc
LD = $(CC)
AS = $(CC)

CFLAGS = -Ofast -march=mips32 -mno-abicalls -fno-PIC -mno-abicalls -falign-functions -falign-loops -falign-labels -falign-jumps -fno-builtin -fno-common -flto
CFLAGS += -D_GNU_SOURCE=1 -DIS_LITTLE_ENDIAN

LDFLAGS = -nodefaultlibs -lc -lgcc -lm -lSDL -lasound -lz -no-pie -Wl,--as-needed -Wl,--gc-sections -s -flto

ifeq ($(PROFILE), YES)
CFLAGS 		+= -fprofile-generate="/home/retrofw/profile"
LDFLAGS 	+= -lgcov
else ifeq ($(PROFILE), APPLY)
CFLAGS		+= -fprofile-use
endif

ASFLAGS = $(CFLAGS)

TARGETS =  sdlgnuboy.dge

ASM_OBJS =

SYS_DEFS = -DHAVE_CONFIG_H -DIS_LITTLE_ENDIAN  -DIS_LINUX -DNATIVE_AUDIO
SYS_OBJS = sys/nix/nix.o $(ASM_OBJS)
SYS_INCS = -I./sys/nix -Ifont -Isrc/core

SDL_OBJS = sys/sdl/sdl.o sys/sdl/keymap.o sys/sdl/scaler.o sys/sdl/font_drawing.o sys/alsa/alsa.o

all: $(TARGETS)

include Rules

$(TARGETS): $(OBJS) $(SYS_OBJS) $(SDL_OBJS)
	$(LD) $(CFLAGS) $(OBJS) $(SYS_OBJS) $(SDL_OBJS) -o $@ $(LDFLAGS)


clean:
	rm -f *gnuboy gmon.out src/core/*.o sys/*.o sys/*/*.o
	
clean_gcda:
	rm -f src/core/*.gcda sys/*.gcda sys/*/*.gcda





