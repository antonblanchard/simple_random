ARCH = $(shell uname -m)
ifneq ("$(ARCH)", "ppc64")
ifneq ("$(ARCH)", "ppc64le")
	CROSS_COMPILE ?= powerpc64le-linux-
endif
endif

MICROWATT = ../..

CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy

GIT_VERSION := "$(shell git describe --dirty --always --tags)"

CFLAGS = -DVERSION=\"$(GIT_VERSION)\" -Os -g -Wall -msoft-float -mno-string -mno-multiple -mno-vsx -mno-altivec -mlittle-endian -mtraceback=no -fno-stack-protector -mstrict-align -ffreestanding -fdata-sections -ffunction-sections  -Ilibc/include -I../ -I../microrl -I$(MICROWATT)/include -D__USE_LIBC
ASFLAGS = $(CFLAGS)
LDFLAGS = -N -T powerpc.lds --gc-sections

all: simple_random.hex

LIBC_SRC := $(wildcard libc/src/*.c)
LIBC_OBJ := $(patsubst libc/src/%.c, libc/obj/%.o,$(LIBC_SRC))
libc_objdir:
	@mkdir -p libc/obj
libc/obj/%.o: libc/src/%.c
	$(CC) $(CFLAGS) -c $< -o $@
libc.o: libc_objdir $(LIBC_OBJ)
	$(LD)  -r -o $@ $(LIBC_OBJ)

simple_random.o: ../simple_random.c ../generate.h ../backend.h ../jenkins.h ../microrl/microrl.h ../mystdio.h
	$(CC) $(CFLAGS) -c $<

generate.o: ../generate.c ../generate.h ../lfsr.h ../helpers.h
	$(CC) $(CFLAGS) -c $<

helpers.o: ../helpers.S ../helpers.h
	$(CC) $(CFLAGS) -c $<

mystdio.o: ../mystdio.c ../mystdio.h
	$(CC) $(CFLAGS) -c $<

microrl.o: ../microrl/microrl.c ../microrl/config.h ../microrl/microrl.h
	$(CC) $(CFLAGS) -c $<

console.o: $(MICROWATT)/lib/console.c
	$(CC) $(CFLAGS) -c $<

simple_random.elf: simple_random.o generate.o head.o libc.o console.o backend_microwatt.o helpers.o microrl.o mystdio.o
	$(LD) $(LDFLAGS) -o $@ $^

simple_random.bin: simple_random.elf
	$(OBJCOPY) -O binary $< $@

simple_random.hex: simple_random.bin
	./bin2hex.py $< > $@

clean:
	@rm -f *.o simple_random.elf simple_random.bin simple_random.hex libc/obj/*
