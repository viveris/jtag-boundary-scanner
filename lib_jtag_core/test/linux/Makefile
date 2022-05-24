CC ?= gcc
AR ?= ar

UNAME := $(shell uname)

BASEDIR=../../src
INCLUDES = -I$(BASEDIR)/

CFLAGS=-O3 $(INCLUDES) -Wall -pthread

LIBDIR=../../build/linux
LIB=libjtag_core.so
EXEC=BSDLtoFloorplan

ifeq ($(UNAME), Linux)
CFLAGS += -fPIC
endif

ifeq ($(findstring CYGWIN,$(shell uname)),CYGWIN)
LIB=libjtag_core.dll
EXEC=BSDLtoFloorplan.exe
endif

ifeq ($(UNAME), Darwin)
CFLAGS += -arch i386 -mmacosx-version-min=10.5
LIB=libjtag_core.dylib
EXEC=BSDLtoFloorplan.exe
endif

all: $(EXEC)

BSDLtoFloorplan: BSDLtoFloorplan.c
	$(CC) -o $@    $^ $(CFLAGS) "$(LIBDIR)/$(LIB)"

clean:
	rm -rf *.o

mrproper: clean
	rm -rf $(EXEC)

.PHONY: clean mrproper
