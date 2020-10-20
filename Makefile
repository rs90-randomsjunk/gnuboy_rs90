
prefix = 
exec_prefix = ${prefix}
bindir = ${exec_prefix}/bin

PROFILE=0
#PROFILE=APPLY

CC = /opt/rs90-toolchain/bin/mipsel-linux-gcc
LD = $(CC)
AS = $(CC)
DEBUG = 0

ifeq ($(DEBUG), YES)
CFLAGS		+= -O0 -g3
CFLAGS += -D_GNU_SOURCE=1 -DIS_LITTLE_ENDIAN -DNOSCALE
LDFLAGS = -lc -lgcc -lm -lSDL -lasound -lz
else
CFLAGS		+= -Ofast -mips32 -fdata-sections -ffunction-sections -mno-fp-exceptions -mno-check-zero-division -mframe-header-opt -fsingle-precision-constant -fno-common -march=mips32 -mtune=mips32 
CFLAGS		+= -mplt -mno-shared -fno-builtin
CFLAGS		+= -flto -fsection-anchors
CFLAGS		+= -fno-stack-protector -fomit-frame-pointer

CFLAGS += -D_GNU_SOURCE=1 -DIS_LITTLE_ENDIAN -DNOSCALE
LDFLAGS = -nodefaultlibs -lc -lgcc -lm -lSDL -lasound -lz -Wl,--as-needed -Wl,--gc-sections -flto -s
endif

ifeq ($(PROFILE), YES)
CFLAGS 		+= -fprofile-generate="/media/data/local/home"
LDFLAGS 	+= -lgcov
else ifeq ($(PROFILE), APPLY)
CFLAGS		+= -fprofile-use
endif

ASFLAGS = $(CFLAGS)

TARGETS =  sdlgnuboy.dge

ASM_OBJS =

SYS_DEFS = -DIS_LITTLE_ENDIAN  -DIS_LINUX -DNATIVE_AUDIO
SYS_OBJS = sys/nix/nix.o $(ASM_OBJS)
SYS_INCS = -I./sys/nix -Ifont -Isrc/core -Isys

SDL_OBJS = sys/sdl/sdl.o sys/sdl/keymap.o sys/sdl/scaler.o sys/sdl/font_drawing.o sys/alsa/alsa.o

all: $(TARGETS)

include Rules

$(TARGETS): $(OBJS) $(SYS_OBJS) $(SDL_OBJS)
	$(LD) $(CFLAGS) $(OBJS) $(SYS_OBJS) $(SDL_OBJS) -o $@ $(LDFLAGS)


clean:
	rm -f *gnuboy gmon.out src/core/*.o sys/*.o sys/*/*.o
	
clean_gcda:
	rm -f src/core/*.gcda sys/*.gcda sys/*/*.gcda





