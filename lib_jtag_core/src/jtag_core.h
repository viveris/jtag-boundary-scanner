/*
 * JTAG Core library
 * Copyright (c) 2008 - 2021 Viveris Technologies
 *
 * JTAG Core library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * JTAG Core library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with JTAG Core library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

/**
 * @file   jtag_core.h
 * @brief  Main jtag core library header
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

#ifndef _jtag_core_
typedef void jtag_core;
#define _jtag_core_
#endif

#ifndef _script_ctx_
typedef void script_ctx;
#define _script_ctx_
#endif

#define LIB_JTAG_CORE_VERSION "0.9.5.2"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions Error / return codes

#define JTAG_CORE_NO_ERROR        0
#define JTAG_CORE_BAD_PARAMETER  -1
#define JTAG_CORE_ACCESS_ERROR   -2
#define JTAG_CORE_IO_ERROR       -3
#define JTAG_CORE_MEM_ERROR      -4
#define JTAG_CORE_NO_PROBE       -5
#define JTAG_CORE_NOT_FOUND      -6
#define JTAG_CORE_CMD_NOT_FOUND  -7
#define JTAG_CORE_INTERNAL_ERROR -8
#define JTAG_CORE_BAD_CMD        -9

#define JTAG_CORE_I2C_BUS_NOTFREE -10

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Lib init functions

// jtagcore_init : Library alloc and init
jtag_core * jtagcore_init();

// jtagcore_init : Library dealloc and deinit
void jtagcore_deinit(jtag_core * jc);

#ifndef _JTAGCORE_PRINT_FUNC_
typedef void (*JTAGCORE_PRINT_FUNC)(char * string);
#define _JTAGCORE_PRINT_FUNC_
#endif

int jtagcore_set_logs_callback(jtag_core * jc, JTAGCORE_PRINT_FUNC jtag_core_print);
int jtagcore_set_logs_level(jtag_core * jc,int level);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Probe driver selection/initialisation functions

// jtagcore_get_number_of_probes_drv : Return the "number of supported probes"

int jtagcore_get_number_of_probes_drv(jtag_core * jc);
int jtagcore_get_number_of_probes(jtag_core * jc, int probe_driver_id);

#define PROBE_ID(drv_id,probe_index) ( (drv_id<<8) | probe_index )

// jtagcore_get_probe_name : Return the probe name (probe_id should be between 0 and "number of supported probes" - 1)

int jtagcore_get_probe_name(jtag_core * jc, int probe_id,char * name);

// jtagcore_select_and_open_probe : Try to init the probe (probe_id should be between 0 and "number of supported probes" - 1)

int jtagcore_select_and_open_probe(jtag_core * jc, int probe_id);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// JTAG Chain init functions

// jtagcore_get_number_of_devices : Scan and init the chain (devices detection)
int jtagcore_scan_and_init_chain(jtag_core * jc);

// jtagcore_get_number_of_devices : return "the number of devices into the chain" found by jtagcore_scan_and_init_chain

int jtagcore_get_number_of_devices(jtag_core * jc);

// jtagcore_get_dev_id : Return the device chip ID
// "device" should be between 0 and "the number of devices into the chain" - 1

unsigned long jtagcore_get_dev_id(jtag_core * jc, int device);

// jtagcore_loadbsdlfile : Load/attach a bsdl file to a device into the chain
// "device" should be between 0 and "the number of devices into the chain" - 1

int jtagcore_loadbsdlfile(jtag_core * jc, char * path, int device);

// jtagcore_get_dev_name : Get the currently loaded bsdl name & path
// "device" should be between 0 and "the number of devices into the chain" - 1
int jtagcore_get_dev_name(jtag_core * jc, int device, char * devname, char * bsdlpath);

// jtagcore_get_bsdl_id : Return the chip id present into a bsdl file

unsigned long jtagcore_get_bsdl_id(jtag_core * jc, char * path);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pins/IO access functions

// jtagcore_get_number_of_pins : Return the number of pins of a device.
// "device" should be between 0 and "the number of devices into the chain" - 1
// and a bsdl must be attached to this device

int jtagcore_get_number_of_pins(jtag_core * jc,int device);

// jtagcore_get_pin_properties : Return the pin properties
// "device" should be between 0 and "the number of devices into the chain" - 1
// and a bsdl must be attached to this device
// "pin" should be between 0 and "number of pins of a device" - 1
// "type" return is a mask between JTAG_CORE_PIN_IS_INPUT / JTAG_CORE_PIN_IS_OUTPUT / JTAG_CORE_PIN_IS_TRISTATES states

int jtagcore_get_pin_properties(jtag_core * jc, int device, int pin, char * pinname, int maxsize, int * type);

#define JTAG_CORE_PIN_IS_INPUT     0x01
#define JTAG_CORE_PIN_IS_OUTPUT    0x02
#define JTAG_CORE_PIN_IS_TRISTATES 0x04

// jtagcore_get_pin_id : Find and return a pin id (if exist) from the pin name
// "device" should be between 0 and "the number of devices into the chain" - 1

int jtagcore_get_pin_id(jtag_core * jc, int device, char * pinname);

// jtagcore_get_pin_state : Return the current pin state
// "device" should be between 0 and "the number of devices into the chain" - 1
// "pin" should be between 0 and "number of pins of a device" - 1
// "type" should be set to JTAG_CORE_INPUT or JTAG_CORE_OUTPUT or JTAG_CORE_OE

int jtagcore_get_pin_state(jtag_core * jc, int device, int pin, int type);

// jtagcore_set_pin_state : Return the current pin state
// "device" should be between 0 and "the number of devices into the chain" - 1
// "pin" should be between 0 and "number of pins of a device" - 1
// "type" should be set to JTAG_CORE_INPUT or JTAG_CORE_OUTPUT or JTAG_CORE_OE

int jtagcore_set_pin_state(jtag_core * jc, int device, int pin, int type, int state);

#define JTAG_CORE_INPUT     0x01
#define JTAG_CORE_OUTPUT    0x02
#define JTAG_CORE_OE        0x04

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Scan chain functions

// jtagcore_set_scan_mode : Set the scan mode for a particular device
// "device" should be between 0 and "the number of devices into the chain" - 1
// "scan_mode" should be set to JTAG_CORE_SAMPLE_SCANMODE (observation only) or JTAG_CORE_EXTEST_SCANMODE (full pin control)
int jtagcore_set_scan_mode(jtag_core * jc, int device, int scan_mode);

// JTAG Scan mode
#define JTAG_CORE_SAMPLE_SCANMODE  0x00
#define JTAG_CORE_EXTEST_SCANMODE  0x01

// jtagcore_push_and_pop_chain : Do a JTAG chain transaction.
// "mode" should be set to JTAG_CORE_WRITE_READ (Push/write and pop/read the chain) or JTAG_CORE_WRITE_ONLY (only Push/write the chain)
int jtagcore_push_and_pop_chain(jtag_core * jc, int mode);

#define JTAG_CORE_WRITE_READ  0x00
#define JTAG_CORE_WRITE_ONLY  0x01

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// I2C over JTAG API functions

// jtagcore_i2c_set_scl_pin : Select the SCL pin
// "device" should be between 0 and "the number of devices into the chain" - 1
// "pin" should be between 0 and "number of pins of a device" - 1

int jtagcore_i2c_set_scl_pin(jtag_core * jc, int device, int pin);

// jtagcore_i2c_set_sda_pin : Select the SDA pin
// "device" should be between 0 and "the number of devices into the chain" - 1
// "pin" should be between 0 and "number of pins of a device" - 1

int jtagcore_i2c_set_sda_pin(jtag_core * jc, int device, int pin);

// jtagcore_i2c_write_read : Do an I2C transfert
// "address" : I2C address (8 bits aligned)
// "address10bits" != 0 if this is an 10bits address
// "wr_size" write size
// "rd_size" read size
int jtagcore_i2c_write_read(jtag_core * jc, int address, int address10bits,int wr_size,unsigned char * wr_buffer,int rd_size,unsigned char * rd_buffer);


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SPI over JTAG API functions

// jtagcore_spi_set_cs_pin : Select the CS pin
// "device" should be between 0 and "the number of devices into the chain" - 1
// "pin" should be between 0 and "number of pins of a device" - 1

int jtagcore_spi_set_cs_pin(jtag_core * jc, int device, int pin, int polarity);

// jtagcore_spi_set_clk_pin : Select the CLK pin
// "device" should be between 0 and "the number of devices into the chain" - 1
// "pin" should be between 0 and "number of pins of a device" - 1

int jtagcore_spi_set_clk_pin(jtag_core * jc, int device, int pin, int polarity);

// jtagcore_spi_set_miso_pin : Select the MISO pin
// "device" should be between 0 and "the number of devices into the chain" - 1
// "pin" should be between 0 and "number of pins of a device" - 1

int jtagcore_spi_set_miso_pin(jtag_core * jc, int device, int pin, int sample_clk_phase);

// jtagcore_spi_set_mosi_pin : Select the MOSI pin
// "device" should be between 0 and "the number of devices into the chain" - 1
// "pin" should be between 0 and "number of pins of a device" - 1

int jtagcore_spi_set_mosi_pin(jtag_core * jc, int device, int pin, int sample_clk_phase);

// jtagcore_spi_write_read : Do a SPI transfert
// "wr_size" is the transaction size

int jtagcore_spi_write_read(jtag_core * jc, int wr_size,unsigned char * wr_buffer,unsigned char * rd_buffer, int flags);

// jtagcore_spi_set_bitorder : Select MSB/LSB first mode

int jtagcore_spi_set_bitorder(jtag_core * jc, int lsb_first);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MDIO over JTAG API functions

// jtagcore_mdio_set_mdio_pin : Select the MDIO pin
// "device" should be between 0 and "the number of devices into the chain" - 1
// "pin" should be between 0 and "number of pins of a device" - 1

int jtagcore_mdio_set_mdio_pin(jtag_core * jc, int device, int pin);

// jtagcore_mdio_set_mdc_pin : Select the MDC pin
// "device" should be between 0 and "the number of devices into the chain" - 1
// "pin" should be between 0 and "number of pins of a device" - 1

int jtagcore_mdio_set_mdc_pin(jtag_core * jc, int device, int pin);

// jtagcore_mdio_read : Read to an MDIO register
// phy_adr : Phylayer address
// reg_adr : Register address

int jtagcore_mdio_read(jtag_core * jc, int phy_adr, int reg_adr);

// jtagcore_mdio_write : Write to an MDIO register
// phy_adr : Phylayer address
// reg_adr : Register address
// data : data value to write

int jtagcore_mdio_write(jtag_core * jc, int phy_adr, int reg_adr, int data);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Parallel Memory over JTAG API functions

#define JTAG_CORE_RAM_CS_CTRL  0x00
#define JTAG_CORE_RAM_WR_CTRL  0x01
#define JTAG_CORE_RAM_RD_CTRL  0x02
#define JTAG_CORE_RAM_RW_CTRL  0x03

int jtagcore_memory_clear_pins(jtag_core * jc);
int jtagcore_memory_set_address_pin(jtag_core * jc, int address_bit, int device, int pin);
int jtagcore_memory_set_data_pin(jtag_core * jc, int data_bit, int device, int pin);
int jtagcore_memory_set_ctrl_pin(jtag_core * jc, int ctrl, int polarity, int device, int pin);
unsigned long jtagcore_memory_read(jtag_core * jc, int mem_adr);
int jtagcore_memory_write(jtag_core * jc, int mem_adr, unsigned long data);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Internal variables functions

int jtagcore_setEnvVar( jtag_core * jc, char * varname, char * varvalue );
char * jtagcore_getEnvVar( jtag_core * jc, char * varname, char * varvalue);
int jtagcore_getEnvVarValue( jtag_core * jc, char * varname);
char * jtagcore_getEnvVarIndex( jtag_core * jc, int index, char * varvalue);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Script execution functions

#define DEFAULT_BUFLEN 1024

// Output message types/levels
enum MSGTYPE
{
    MSG_DEBUG = 0,
	MSG_INFO_0,
	MSG_INFO_1,
	MSG_WARNING,
	MSG_ERROR,
    MSG_NONE,
};

#ifndef _jtag_script_printf_func_
typedef int (* SCRIPT_PRINTF_FUNC)(void * ctx, int MSGTYPE, char * string, ... );
#define _jtag_script_printf_func_
#endif

script_ctx * jtagcore_initScript(jtag_core * jc);

void jtagcore_setScriptOutputFunc( script_ctx * ctx, SCRIPT_PRINTF_FUNC ext_printf );
int  jtagcore_execScriptLine( script_ctx * ctx, char * line );
int  jtagcore_execScriptFile( script_ctx * ctx, char * script_path );
int  jtagcore_execScriptRam( script_ctx * ctx, unsigned char * script_buffer, int buffersize );

script_ctx * jtagcore_deinitScript( script_ctx * ctx );

int jtagcore_savePinsStateScript( jtag_core * jc, int device, char * script_path );
