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
 * @file   spi_over_jtag.c
 * @brief  spi over jtag API
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../drivers/drv_loader.h"

#include "../jtag_core_internal.h"
#include "../jtag_core.h"

#include "../bsdl_parser/bsdl_loader.h"

#include "../drivers/drivers_list.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SPI Over JTAG API
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int jtagcore_spi_set_mosi_pin(jtag_core * jc, int device, int pin, int sample_clk_phase)
{
	jtag_bsdl * bsdl_file;

	if (device < jc->nb_of_devices_in_chain && device < MAX_NB_JTAG_DEVICE && device >= 0)
	{
		if (jc->devices_list[device].bsdl)
		{
			bsdl_file = jc->devices_list[device].bsdl;

			if (pin < bsdl_file->number_of_pins && pin >= 0)
			{
				jc->spi_mosi_pin = pin;
				if(sample_clk_phase)
					jc->spi_mosi_pol = 1;
				else
					jc->spi_mosi_pol = 0;
				jc->spi_mosi_device = device;
				return JTAG_CORE_NO_ERROR;
			}
		}
	}

	return JTAG_CORE_BAD_PARAMETER;
}

int jtagcore_spi_set_miso_pin(jtag_core * jc, int device, int pin, int sample_clk_phase)
{
	jtag_bsdl * bsdl_file;

	if (device < jc->nb_of_devices_in_chain && device < MAX_NB_JTAG_DEVICE && device >= 0)
	{
		if (jc->devices_list[device].bsdl)
		{
			bsdl_file = jc->devices_list[device].bsdl;

			if (pin < bsdl_file->number_of_pins && pin >= 0)
			{
				jc->spi_miso_pin = pin;
				if(sample_clk_phase)
					jc->spi_miso_pol = 1;
				else
					jc->spi_miso_pol = 0;
				jc->spi_miso_device = device;
				return JTAG_CORE_NO_ERROR;
			}
		}
	}

	return JTAG_CORE_BAD_PARAMETER;
}

int jtagcore_spi_set_clk_pin(jtag_core * jc, int device, int pin, int polarity)
{
	jtag_bsdl * bsdl_file;

	if (device < jc->nb_of_devices_in_chain && device < MAX_NB_JTAG_DEVICE && device >= 0)
	{
		if (jc->devices_list[device].bsdl)
		{
			bsdl_file = jc->devices_list[device].bsdl;

			if (pin < bsdl_file->number_of_pins && pin >= 0)
			{
				jc->spi_clk_pin = pin;
				if(polarity)
					jc->spi_clk_pol = 1;
				else
					jc->spi_clk_pol = 0;
				jc->spi_clk_device = device;
				return JTAG_CORE_NO_ERROR;
			}
		}
	}

	return JTAG_CORE_BAD_PARAMETER;
}

int jtagcore_spi_set_cs_pin(jtag_core * jc, int device, int pin, int polarity)
{
	jtag_bsdl * bsdl_file;

	if (device < jc->nb_of_devices_in_chain && device < MAX_NB_JTAG_DEVICE && device >= 0)
	{
		if (jc->devices_list[device].bsdl)
		{
			bsdl_file = jc->devices_list[device].bsdl;

			if (pin < bsdl_file->number_of_pins && pin >= 0)
			{
				jc->spi_cs_pin = pin;
				if(polarity)
					jc->spi_cs_pol = 1;
				else
					jc->spi_cs_pol = 0;
				jc->spi_cs_device = device;
				return JTAG_CORE_NO_ERROR;
			}
		}
	}

	return JTAG_CORE_BAD_PARAMETER;
}

int jtagcore_spi_set_bitorder(jtag_core * jc, int lsb_first)
{
	if(lsb_first)
		jc->spi_lsb_first = 1;
	else
		jc->spi_lsb_first = 0;

	return JTAG_CORE_NO_ERROR;
}


int jtagcore_spi_write_read(jtag_core * jc, int wr_size,unsigned char * wr_buffer,unsigned char * rd_buffer, int flags)
{
	int i,j;
	unsigned char byte_mask;

	if( jc )
	{
		if(jc->spi_lsb_first)
			byte_mask = (0x01);
		else
			byte_mask = (0x80);

		jtagcore_set_pin_state(jc, jc->spi_mosi_device, jc->spi_mosi_pin, JTAG_CORE_OUTPUT, 0);
		if(wr_size)
		{
			if( wr_buffer[0] & byte_mask )
			{
				jtagcore_set_pin_state(jc, jc->spi_mosi_device, jc->spi_mosi_pin, JTAG_CORE_OUTPUT, 1);
			}
		}

		jtagcore_set_pin_state(jc, jc->spi_miso_device, jc->spi_miso_pin, JTAG_CORE_OUTPUT, 1);
		jtagcore_set_pin_state(jc, jc->spi_cs_device, jc->spi_cs_pin, JTAG_CORE_OUTPUT, 1 ^ jc->spi_cs_pol);
		jtagcore_set_pin_state(jc, jc->spi_clk_device, jc->spi_clk_pin, JTAG_CORE_OUTPUT, 0 ^ jc->spi_clk_pol);

		jtagcore_set_pin_state(jc, jc->spi_miso_device, jc->spi_miso_pin, JTAG_CORE_OE, 0);
		jtagcore_set_pin_state(jc, jc->spi_mosi_device, jc->spi_mosi_pin, JTAG_CORE_OE, 1);
		jtagcore_set_pin_state(jc, jc->spi_cs_device, jc->spi_cs_pin, JTAG_CORE_OE, 1);
		jtagcore_set_pin_state(jc, jc->spi_clk_device, jc->spi_clk_pin, JTAG_CORE_OE, 1);

		jtagcore_push_and_pop_chain(jc, JTAG_CORE_WRITE_ONLY);

		jtagcore_set_pin_state(jc, jc->spi_cs_device, jc->spi_cs_pin, JTAG_CORE_OUTPUT, 0 ^ jc->spi_cs_pol);

		jtagcore_push_and_pop_chain(jc, JTAG_CORE_WRITE_ONLY);

		for(i=0;i<wr_size;i++)
		{
			for(j=0;j<8;j++)
			{
				if(jc->spi_lsb_first)
					byte_mask = (0x01<<j);
				else
					byte_mask = (0x80>>j);

				if(!jc->spi_mosi_pol) // CPHA = 0
				{	// MOSI OUT on clock phase 0
					if( wr_buffer[i] & byte_mask )
					{
						jtagcore_set_pin_state(jc, jc->spi_mosi_device, jc->spi_mosi_pin, JTAG_CORE_OUTPUT, 1);
					}
					else
					{
						jtagcore_set_pin_state(jc, jc->spi_mosi_device, jc->spi_mosi_pin, JTAG_CORE_OUTPUT, 0);
					}
				}

				if(!jc->spi_miso_pol) // CPHA = 0
				{	// MISO IN on clock phase 0

					// clock phase 0
					jtagcore_set_pin_state(jc, jc->spi_clk_device, jc->spi_clk_pin, JTAG_CORE_OUTPUT, 1 ^ jc->spi_clk_pol);
					if ( jtagcore_push_and_pop_chain(jc, JTAG_CORE_WRITE_READ) != JTAG_CORE_NO_ERROR )
					{
						return JTAG_CORE_IO_ERROR;
					}

					if(jtagcore_get_pin_state(jc,jc->spi_miso_device,jc->spi_miso_pin,JTAG_CORE_INPUT))
					{
						rd_buffer[i] |= byte_mask;
					}
					else
					{
						rd_buffer[i] &= ~(byte_mask);
					}
				}
				else
				{
					// clock phase 0
					jtagcore_set_pin_state(jc, jc->spi_clk_device, jc->spi_clk_pin, JTAG_CORE_OUTPUT, 1 ^ jc->spi_clk_pol);
					jtagcore_push_and_pop_chain(jc, JTAG_CORE_WRITE_ONLY);
				}

				if(jc->spi_mosi_pol) // CPHA = 1
				{	// MOSI OUT on clock phase 1
					if( wr_buffer[i] & byte_mask )
					{
						jtagcore_set_pin_state(jc, jc->spi_mosi_device, jc->spi_mosi_pin, JTAG_CORE_OUTPUT, 1);
					}
					else
					{
						jtagcore_set_pin_state(jc, jc->spi_mosi_device, jc->spi_mosi_pin, JTAG_CORE_OUTPUT, 0);
					}
				}

				if(jc->spi_miso_pol) // CPHA = 1
				{
					// clock phase 1
					jtagcore_set_pin_state(jc, jc->spi_clk_device, jc->spi_clk_pin, JTAG_CORE_OUTPUT, 0 ^ jc->spi_clk_pol);

					if ( jtagcore_push_and_pop_chain(jc, JTAG_CORE_WRITE_READ) != JTAG_CORE_NO_ERROR )
					{
						return JTAG_CORE_IO_ERROR;
					}

					// MISO IN on clock phase 1
					if(jtagcore_get_pin_state(jc,jc->spi_miso_device,jc->spi_miso_pin,JTAG_CORE_INPUT))
					{
						rd_buffer[i] |= byte_mask;
					}
					else
					{
						rd_buffer[i] &= ~(byte_mask);
					}
				}
				else
				{
					// clock phase 1
					jtagcore_set_pin_state(jc, jc->spi_clk_device, jc->spi_clk_pin, JTAG_CORE_OUTPUT, 0 ^ jc->spi_clk_pol);
					jtagcore_push_and_pop_chain(jc, JTAG_CORE_WRITE_ONLY);
				}
			}
		}

		jtagcore_set_pin_state(jc, jc->spi_cs_device, jc->spi_cs_pin, JTAG_CORE_OUTPUT, 1 ^ jc->spi_cs_pol);

		jtagcore_push_and_pop_chain(jc, JTAG_CORE_WRITE_ONLY);

		return JTAG_CORE_NO_ERROR;
	}

	return JTAG_CORE_BAD_PARAMETER;
}
