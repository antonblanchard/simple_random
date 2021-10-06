ARCH = $(shell uname -m)
ifneq ("$(ARCH)", "ppc")
ifneq ("$(ARCH)", "ppc64")
ifneq ("$(ARCH)", "ppc64le")
	CROSS_COMPILE ?= powerpc64le-linux-
	COPTS ?= -mno-vsx -mno-altivec
endif
endif
endif

CDEFS=

# Uncomment the next line to include vector/VSX instructions
#CDEFS += -DVECTOR_INSNS=true

CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld

GIT_VERSION := "$(shell git describe --dirty --always --tags)"

CFLAGS = $(CDEFS) -DVERSION=\"$(GIT_VERSION)\" -O2 -g -Wall -I../ -I../microrl $(COPTS)
ASFLAGS = $(CFLAGS)

all: simple_random

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

backend_posix.o: backend_posix.c ../backend.h

simple_random: simple_random.o generate.o backend_posix.o helpers.o microrl.o mystdio.o
	$(CC) $(LDFLAGS) -o $@ $^

clean:
	@rm -f *.o simple_random
