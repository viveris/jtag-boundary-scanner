CC = gcc
AR = ar

UNAME := $(shell uname)

BASEDIR=../../src
INCLUDES = -I$(BASEDIR)/

CFLAGS=-O3 $(INCLUDES) -Wall

LDFLAGS= -g -shared -lc -lm  -ldl

EXEC=lib_jtag_core.so

ifeq ($(UNAME), Linux)
CFLAGS += -fPIC
endif

ifeq ($(findstring CYGWIN,$(shell uname)),CYGWIN)
LDFLAGS= -g -shared -lc -lm  -ldl
EXEC=lib_jtag_core.dll
endif

ifeq ($(UNAME), Darwin)
CFLAGS += -arch i386 -mmacosx-version-min=10.5
LDFLAGS +=-arch i386 -dynamiclib -current_version 2.0 -install_name @executable_path/../Frameworks/lib_jtag_core.dylib -mmacosx-version-min=10.5
EXEC=lib_jtag_core.dylib
endif

PROBES_DRIVERS = #lpt_jtag_drv.o jlink_jtag_drv.o ftdi_jtag_drv.o
PROTOCOLS_DRIVERS = i2c_over_jtag.o mdio_over_jtag.o spi_over_jtag.o memory_over_jtag.o

all: $(EXEC)

lib_jtag_core.dylib: jtag_core.o dbg_logs.o bsdl_loader.o bsdl_strings.o drv_loader.o drivers_list.o $(PROTOCOLS_DRIVERS) $(PROBES_DRIVERS)
	$(CC) -o $@    $^ $(LDFLAGS)

lib_jtag_core.a: jtag_core.o dbg_logs.o bsdl_loader.o bsdl_strings.o drv_loader.o drivers_list.o $(PROTOCOLS_DRIVERS) $(PROBES_DRIVERS)
	$(AR) r $@    $^

lib_jtag_core.so: jtag_core.o dbg_logs.o bsdl_loader.o bsdl_strings.o drv_loader.o drivers_list.o $(PROTOCOLS_DRIVERS) $(PROBES_DRIVERS)
	$(CC) -o $@    $^ $(LDFLAGS)

lib_jtag_core.dll: jtag_core.o dbg_logs.o bsdl_loader.o bsdl_strings.o drv_loader.o drivers_list.o $(PROTOCOLS_DRIVERS) $(PROBES_DRIVERS)
	$(CC) -o $@    $^ $(LDFLAGS)

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

clean:
	rm -rf *.o

mrproper: clean
	rm -rf $(EXEC)

.PHONY: clean mrproper
