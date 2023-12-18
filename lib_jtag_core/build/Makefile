CC ?= gcc
BUILD_CC=gcc
AR ?= ar

UNAME := $(shell uname)

BASEDIR=../src
INCLUDES = -I$(BASEDIR)/

DEBUG ?= 0
DEBUG_ASAN ?= 0

ifeq ($(DEBUG), 1)
	CFLAGS=-O0 $(INCLUDES) -Wall -g -DDEBUG
	LDFLAGS= -shared
else
	CFLAGS=-O3 $(INCLUDES) -Wall
	LDFLAGS= -shared -s
endif

ifeq ($(DEBUG_ASAN), 1)
	CFLAGS += -fsanitize=address -fno-omit-frame-pointer
	LDFLAGS += -static-libasan -fsanitize=address
endif

EXEC=libjtagcore.so

ifeq ($(TARGET), Linux)
CFLAGS += -fPIC -Wl,-Map,foo.map
LDFLAGS += -lc -lm  -ldl
endif

ifeq ($(TARGET), mingw32)
CC=i686-w64-mingw32-gcc
RESC=i686-w64-mingw32-windres
LDFLAGS += -static-libgcc -lws2_32 ../src/os_interface/win32/libjtagcore.def -Wl,-Map=output.map
EXEC=libjtagcore.dll
endif

ifeq ($(TARGET), mingw64)
CC=x86_64-w64-mingw32-gcc
RESC=x86_64-w64-mingw32-windres
LDFLAGS += -static-libgcc -lws2_32 ../src/os_interface/win32/libjtagcore.def
EXEC=libjtagcore.dll
endif

ifeq ($(findstring CYGWIN,$(shell uname)),CYGWIN)
LDFLAGS= -shared -lc -lm  -ldl
EXEC=libjtagcore.dll
endif

ifeq ($(TARGET), Darwin)
MACOSX_ARCH ?= -arch arm64 -arch x86_64
MACOSX_MIN_VER ?= 10.9
CFLAGS += ${MACOSX_ARCH} -mmacosx-version-min=${MACOSX_MIN_VER}
LDFLAGS += -lc -lm -ldl ${MACOSX_ARCH} -dynamiclib -current_version 2.0 -install_name @executable_path/../Frameworks/libjtagcore.dylib -mmacosx-version-min=${MACOSX_MIN_VER}
EXEC=libjtagcore.dylib
endif

LIB_JTAG_CORE = jtag_core.o dbg_logs.o bsdl_loader.o bsdl_strings.o drv_loader.o script.o env.o fs.o os_interface.o network.o
PROBES_DRIVERS = drivers_list.o jlink_jtag_drv.o linux_gpio_jtag_drv.o lpt_jtag_drv.o ftdi_jtag_drv.o
PROTOCOLS_DRIVERS = i2c_over_jtag.o mdio_over_jtag.o spi_over_jtag.o memory_over_jtag.o
NATSORT = strnatcmp.o

all: $(BASEDIR)/config_script.h $(EXEC)

libjtagcore.dylib: $(LIB_JTAG_CORE) $(PROTOCOLS_DRIVERS) $(PROBES_DRIVERS) $(NATSORT)
	$(CC) -o $@    $^ $(LDFLAGS)
	cp $@ ../../build

libjtagcore.a: $(LIB_JTAG_CORE) $(PROTOCOLS_DRIVERS) $(PROBES_DRIVERS) $(NATSORT)
	$(AR) r $@    $^
	cp $@ ../../build

libjtagcore.so: $(LIB_JTAG_CORE) $(PROTOCOLS_DRIVERS) $(PROBES_DRIVERS) $(NATSORT)
	$(CC) -o $@    $^ $(LDFLAGS)
	cp $@ ../../build

libjtagcore.dll: win32_libjtagcore_res.o $(LIB_JTAG_CORE) $(PROTOCOLS_DRIVERS) $(PROBES_DRIVERS) $(NATSORT)
	$(CC) -o $@    $^ $(LDFLAGS)
	cp $@ ../../build

win32_libjtagcore_res.o: $(BASEDIR)/os_interface/win32/libjtagcore.rc
	$(RESC) $< $@

jtag_core.o: $(BASEDIR)/jtag_core.c
	$(CC) -o $@ -c $< $(CFLAGS)

dbg_logs.o: $(BASEDIR)/dbg_logs.c
	$(CC) -o $@ -c $< $(CFLAGS)

bsdl_loader.o: $(BASEDIR)/bsdl_parser/bsdl_loader.c
	$(CC) -o $@ -c $< $(CFLAGS)

bsdl_strings.o: $(BASEDIR)/bsdl_parser/bsdl_strings.c
	$(CC) -o $@ -c $< $(CFLAGS)

i2c_over_jtag.o: $(BASEDIR)/bus_over_jtag/i2c_over_jtag.c
	$(CC) -o $@ -c $< $(CFLAGS)

mdio_over_jtag.o: $(BASEDIR)/bus_over_jtag/mdio_over_jtag.c
	$(CC) -o $@ -c $< $(CFLAGS)

spi_over_jtag.o: $(BASEDIR)/bus_over_jtag/spi_over_jtag.c
	$(CC) -o $@ -c $< $(CFLAGS)

memory_over_jtag.o: $(BASEDIR)/bus_over_jtag/memory_over_jtag.c
	$(CC) -o $@ -c $< $(CFLAGS)

drv_loader.o: $(BASEDIR)/drivers/drv_loader.c
	$(CC) -o $@ -c $< $(CFLAGS)

drivers_list.o: $(BASEDIR)/drivers/drivers_list.c
	$(CC) -o $@ -c $< $(CFLAGS)

lpt_jtag_drv.o: $(BASEDIR)/drivers/lpt_jtag/lpt_jtag_drv.c
	$(CC) -o $@ -c $< $(CFLAGS)

jlink_jtag_drv.o: $(BASEDIR)/drivers/jlink_jtag/jlink_jtag_drv.c
	$(CC) -o $@ -c $< $(CFLAGS)

ftdi_jtag_drv.o: $(BASEDIR)/drivers/ftdi_jtag/ftdi_jtag_drv.c
	$(CC) -o $@ -c $< $(CFLAGS)

linux_gpio_jtag_drv.o: $(BASEDIR)/drivers/linux_gpio_jtag/linux_gpio_jtag_drv.c
	$(CC) -o $@ -c $< $(CFLAGS)

script.o: $(BASEDIR)/script/script.c
	$(CC) -o $@ -c $< $(CFLAGS)

env.o: $(BASEDIR)/script/env.c
	$(CC) -o $@ -c $< $(CFLAGS)

fs.o: $(BASEDIR)/os_interface/fs.c
	$(CC) -o $@ -c $< $(CFLAGS)

os_interface.o: $(BASEDIR)/os_interface/os_interface.c
	$(CC) -o $@ -c $< $(CFLAGS)

network.o: $(BASEDIR)/os_interface/network.c
	$(CC) -o $@ -c $< $(CFLAGS)

strnatcmp.o: $(BASEDIR)/natsort/strnatcmp.c
	$(CC) -o $@ -c $< $(CFLAGS)

$(BASEDIR)/config_script.h: $(BASEDIR)/config.script
	cd $(BASEDIR);xxd -i config.script > config_script.h

clean:
	rm -rf *.o *.so

mrproper: clean
	rm -rf $(EXEC)

.PHONY: clean mrproper
