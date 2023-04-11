#CC=i386-pc-linux-gcc
CC=gcc
BUILD_CC=gcc
CPP=g++

TARGET := $(shell uname)

BASEDIR=../win32
INCLUDES = -I$(BASEDIR) -I../../lib_jtag_core/src -I -I../../build

DEBUG ?= 0
DEBUG_ASAN ?= 0

EXEC=JTAGBoundaryScanner

ifeq ($(DEBUG), 1)
	CFLAGS=-O0 $(INCLUDES) -Wall -g -DDEBUG
	LDFLAGS=
else
	CFLAGS=-O3 $(INCLUDES) -Wall
	LDFLAGS= -s
endif

ifeq ($(DEBUG_ASAN), 1)
	CFLAGS += -fsanitize=address -fno-omit-frame-pointer
	LDFLAGS += -static-libasan -fsanitize=address
endif

LIBJTAGCORE = libjtagcore.so

ifeq ($(TARGET), Linux)
LDFLAGS += -lc -lm -ldl -lpthread -Wl,-rpath=.  -Wl,-Map,foo.map
endif

ifeq ($(findstring CYGWIN,$(shell uname)),CYGWIN)
LIBJTAGCORE = libjtagcore.dll
endif

ifeq ($(TARGET), mingw32)
CC=i686-w64-mingw32-gcc
CPP=i686-w64-mingw32-g++
RESC=i686-w64-mingw32-windres
LDFLAGS += -static-libgcc -static-libstdc++ -mwindows -lwinmm -lws2_32 -lgdi32 -lcomdlg32
LIBJTAGCORE= libjtagcore.dll
EXEC=JTAGBoundaryScanner.exe
endif

ifeq ($(TARGET), mingw64)
CC=x86_64-w64-mingw32-gcc
CPP=x86_64-w64-mingw32-g++
RESC=x86_64-w64-mingw32-windres
LDFLAGS += -static-libgcc -static-libstdc++ -mwindows -lwinmm -lws2_32 -lgdi32 -lcomdlg32
LIBJTAGCORE= libjtagcore.dll
EXEC=JTAGBoundaryScanner.exe
endif

ifeq ($(TARGET), Darwin)
MACOSX_ARCH ?= -arch arm64 -arch x86_64
MACOSX_MIN_VER ?= 10.9
CFLAGS += -DOSX ${MACOSX_ARCH} -mmacosx-version-min=${MACOSX_MIN_VER}
LDFLAGS += -lc -lm -ldl -lpthread -framework Cocoa ${MACOSX_ARCH} -mmacosx-version-min=${MACOSX_MIN_VER}
LIBJTAGCORE= libjtagcore.dylib
else
#LDFLAGS += $(shell $(BASEDIR)/thirdpartylibs/fltk/fltk-1.x.x/fltk-config --ldflags)
endif

all:
	$(MAKE) $(EXEC)

JTAGBoundaryScanner.exe: win32_JTAGBoundaryScanner_res.o JTAGBoundaryScanner.o fileselector.o socket.o $(LIBJTAGCORE) 
	$(CC) -o $@    $^ $(LDFLAGS)
	cp $@ ../../build

JTAGBoundaryScanner: JTAGBoundaryScanner.o fileselector.o socket.o $(LIBJTAGCORE)
	$(CC) -o $@    $^ $(LDFLAGS)
	cp $@ ../../build

$(LIBJTAGCORE): ../../build/$(LIBJTAGCORE)
	ln ../../build/$(LIBJTAGCORE)  $(LIBJTAGCORE)

win32_JTAGBoundaryScanner_res.o: $(BASEDIR)/JTAGBoundaryScanner.rc
	$(RESC) $< $@ -I$(BASEDIR)/ -I$(BASEDIR)/images

JTAGBoundaryScanner.o: $(BASEDIR)/JTAGBoundaryScanner.c
	$(CC) -o $@ -c $< $(CFLAGS)

fileselector.o : $(BASEDIR)/fileselector.c
	$(CC) -o $@ -c $< $(CFLAGS)

socket.o : $(BASEDIR)/socket.c
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm -rf *.o
	rm -rf *.so
	rm -rf *.dll
	rm -rf *.map

mrproper: clean
	rm -rf $(EXEC)
	rm -rf *.map *.ncb *.opt *.plg *.dll *.exe *.bak *.dylib

.PHONY: clean mrproper
