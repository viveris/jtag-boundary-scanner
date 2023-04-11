TARGET ?= mingw32

all: JTAGBoundaryScanner

lib_jtag_core: 
	$(MAKE) TARGET=$(TARGET) -C ../lib_jtag_core/build/

JTAGBoundaryScanner: lib_jtag_core
	$(MAKE) TARGET=$(TARGET) -C ../jtag_boundary_scanner_gui/build/

clean:
	$(MAKE) TARGET=$(TARGET) -C ../lib_jtag_core/build/ clean
	$(MAKE) TARGET=$(TARGET) -C ../jtag_boundary_scanner_gui/build/ clean

mrproper:
	$(MAKE) TARGET=$(TARGET) -C ../lib_jtag_core/build/ mrproper
	$(MAKE) TARGET=$(TARGET) -C ../jtag_boundary_scanner_gui/build/ mrproper
	-rm -rf *.so
	-rm -rf *.a
	-rm -rf *.obj
	-rm -rf *.opt
	-rm -rf *.dll
	-rm -rf *.exe
	-rm -rf *.lib
	-rm -rf *.ncb
	-rm -rf *.exp
	-rm -rf *.dylib
	-rm -rf *.dmg
	-rm -rf *.app
	-rm -rf JTAGBoundaryScanner
	-rm -rf lib_jtag_core
	-rm -rf Debug_*
	-rm -rf Release_*
	-rm -rf *.zip

.PHONY: lib_jtag_core JTAGBoundaryScanner
