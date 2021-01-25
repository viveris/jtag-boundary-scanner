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
 * @file   jtag_core_internal.h
 * @brief  jtag core library internal structures and defines types
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

#define _jtag_core_

#define MAX_NB_JTAG_DEVICE 64
#define MAX_NUMBER_BITS_IN_CHAIN ( 256 * 1024 )
#define MAX_NUMBER_PINS_PER_DEV  ( 64 * 1024 )
#define MAX_BSDL_FILE_SIZE ( 1024 * 1024 )
#define MAX_NUMBER_OF_BSDL_LINES ( 64 * 1024 )

typedef struct _jtag_device
{
	void * bsdl;
	unsigned long devices_id;
	unsigned char * out_boundary_scan;
	unsigned char * in_boundary_scan;
	int boundary_scan_size;
	int scan_mode;
}jtag_device;

#define MAX_BUS_WIDTH 32

typedef struct _jtag_core
{
	drv_ptr io_functions;

	void * jtagcore_print_callback;
	int logs_level;

	void * envvar;

	int t;

	int nb_of_devices_in_chain;

	int total_IR_lenght;
	jtag_device devices_list[MAX_NB_JTAG_DEVICE];
	int IR_filled;


	// I2C over JTAG
	int i2c_sda_device;
	int i2c_scl_device;

	int i2c_sda_pin;
	int i2c_scl_pin;

	// SPI over JTAG
	int spi_mosi_device;
	int spi_miso_device;
	int spi_cs_device;
	int spi_clk_device;

	int spi_mosi_pin;
	int spi_miso_pin;
	int spi_cs_pin;
	int spi_clk_pin;

	int spi_cs_pol;
	int spi_clk_pol;
	int spi_mosi_pol;
	int spi_miso_pol;

	int spi_lsb_first;

	// MDIO over JTAG
	int mdio_mdc_pin;
	int mdio_mdc_device;
	int mdio_mdio_pin;
	int mdio_mdio_device;

	// Memory over JTAG
	int ram_address_pin[MAX_BUS_WIDTH];
	int ram_address_device[MAX_BUS_WIDTH];
	int ram_data_pin[MAX_BUS_WIDTH];
	int ram_data_device[MAX_BUS_WIDTH];
	int ram_ctrl_pin[16];
	int ram_ctrl_pin_pol[16];
	int ram_ctrl_device[16];

}jtag_core;

#define JTAG_STR_DOUT    0x01
#define JTAG_STR_TMS     0x02
#define JTAG_STR_CLKDIS  0x04
#define JTAG_STR_JTAGRST 0x08
#define JTAG_STR_DIN     0x10
#define JTAG_STR_DINREQ  0x20

int jtagcore_loaddriver(jtag_core * jc,int id, char * parameters);