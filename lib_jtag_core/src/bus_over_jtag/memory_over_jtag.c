/*
 * JTAG Core library
 * Copyright (c) 2008-2019 Viveris Technologies
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
 * @file   memory_over_jtag.c
 * @brief  parallel memory over jtag API
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
// Memory Over JTAG API
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int jtagcore_memory_clear_pins(jtag_core * jc)
{
	int i;

	for(i=0;i<MAX_BUS_WIDTH;i++)
	{
		jc->ram_address_pin[i] = -1;
		jc->ram_address_device[i] = -1;
		jc->ram_data_pin[i] = -1;
		jc->ram_data_device[i] = -1;
	}

	for(i=0;i<16;i++)
	{
		jc->ram_ctrl_pin[i] = -1;
		jc->ram_ctrl_pin_pol[i] = -1;
		jc->ram_ctrl_device[i] = -1;
	}

	return JTAG_CORE_NO_ERROR;
}

int jtagcore_memory_set_address_pin(jtag_core * jc, int address_bit, int device, int pin)
{
	jtag_bsdl * bsdl_file;

	if (device < jc->nb_of_devices_in_chain && device < MAX_NB_JTAG_DEVICE  && address_bit < MAX_BUS_WIDTH)
	{
		if (jc->devices_list[device].bsdl)
		{
			bsdl_file = jc->devices_list[device].bsdl;

			if ( pin < bsdl_file->number_of_pins )
			{
				jc->ram_address_pin[address_bit] = pin;
				jc->ram_address_device[address_bit] = device;
				return JTAG_CORE_NO_ERROR;
			}
		}
	}

	return JTAG_CORE_BAD_PARAMETER;
}

int jtagcore_memory_set_data_pin(jtag_core * jc, int data_bit, int device, int pin)
{
	jtag_bsdl * bsdl_file;

	if (device < jc->nb_of_devices_in_chain && device < MAX_NB_JTAG_DEVICE && data_bit < MAX_BUS_WIDTH)
	{
		if (jc->devices_list[device].bsdl)
		{
			bsdl_file = jc->devices_list[device].bsdl;

			if ( pin < bsdl_file->number_of_pins )
			{
				jc->ram_data_pin[data_bit] = pin;
				jc->ram_data_device[data_bit] = device;
				return JTAG_CORE_NO_ERROR;
			}
		}
	}

	return JTAG_CORE_BAD_PARAMETER;
}

int jtagcore_memory_set_ctrl_pin(jtag_core * jc, int ctrl, int polarity, int device, int pin)
{
	jtag_bsdl * bsdl_file;

	if (device < jc->nb_of_devices_in_chain && device < MAX_NB_JTAG_DEVICE && ctrl < 16)
	{
		if (jc->devices_list[device].bsdl)
		{
			bsdl_file = jc->devices_list[device].bsdl;

			if ( pin < bsdl_file->number_of_pins )
			{
				jc->ram_ctrl_pin[ctrl] = pin;
				jc->ram_ctrl_pin_pol[ctrl] = polarity;
				jc->ram_ctrl_device[ctrl] = device;
				return JTAG_CORE_NO_ERROR;
			}
		}
	}

	return JTAG_CORE_BAD_PARAMETER;
}

unsigned long jtagcore_memory_read(jtag_core * jc, int mem_adr)
{
	int i;
	unsigned long value;

	// Set address bus
	for(i=0;i<MAX_BUS_WIDTH;i++)
	{
		if(jc->ram_address_pin[i]>=0 && jc->ram_address_device[i]>=0)
		{
			jtagcore_set_pin_state(jc, jc->ram_address_device[i], jc->ram_address_pin[i], JTAG_CORE_OE, 1);

			if( mem_adr & (0x00000001<<i) )
			{
				jtagcore_set_pin_state(jc, jc->ram_address_device[i], jc->ram_address_pin[i], JTAG_CORE_OUTPUT, 1);

			}
			else
			{
				jtagcore_set_pin_state(jc, jc->ram_address_device[i], jc->ram_address_pin[i], JTAG_CORE_OUTPUT, 0);
			}
		}
	}

	// Set data bus tristate.
	for(i=0;i<MAX_BUS_WIDTH;i++)
	{
		if(jc->ram_data_pin[i]>=0 && jc->ram_data_device[i]>=0)
		{
			jtagcore_set_pin_state(jc, jc->ram_data_device[i], jc->ram_data_pin[i], JTAG_CORE_OE, 0);
		}
	}

	// deassert control pins.
	for(i=0;i<16;i++)
	{
		if(jc->ram_ctrl_pin[i]>=0 && jc->ram_ctrl_device[i]>=0)
		{
			jtagcore_set_pin_state(jc, jc->ram_ctrl_device[i], jc->ram_ctrl_pin[i], JTAG_CORE_OE, 1);

			jtagcore_set_pin_state(jc, jc->ram_ctrl_device[i], jc->ram_ctrl_pin[i], JTAG_CORE_OUTPUT, (jc->ram_ctrl_pin_pol[i]&1)^1 );
		}
	}

	jtagcore_push_and_pop_chain(jc, JTAG_CORE_WRITE_ONLY);

	// Assert CS
	if(jc->ram_ctrl_pin[JTAG_CORE_RAM_CS_CTRL]>=0 && jc->ram_ctrl_device[JTAG_CORE_RAM_CS_CTRL]>=0)
	{
		jtagcore_set_pin_state(jc, jc->ram_ctrl_device[JTAG_CORE_RAM_CS_CTRL], jc->ram_ctrl_pin[JTAG_CORE_RAM_CS_CTRL], JTAG_CORE_OUTPUT, jc->ram_ctrl_pin_pol[JTAG_CORE_RAM_CS_CTRL] & 1 );
	}

	jtagcore_push_and_pop_chain(jc, JTAG_CORE_WRITE_ONLY);

	// Assert RD
	if(jc->ram_ctrl_pin[JTAG_CORE_RAM_RD_CTRL]>=0 && jc->ram_ctrl_device[JTAG_CORE_RAM_RD_CTRL]>=0)
	{
		jtagcore_set_pin_state(jc, jc->ram_ctrl_device[JTAG_CORE_RAM_RD_CTRL], jc->ram_ctrl_pin[JTAG_CORE_RAM_RD_CTRL], JTAG_CORE_OUTPUT, jc->ram_ctrl_pin_pol[JTAG_CORE_RAM_RD_CTRL] & 1 );
	}

	jtagcore_push_and_pop_chain(jc, JTAG_CORE_WRITE_ONLY);

	if ( jtagcore_push_and_pop_chain(jc, JTAG_CORE_WRITE_READ) != JTAG_CORE_NO_ERROR )
	{
		return 0xFFFFFFFF;
	}

	// read the data bus.
	value = 0x00000000;
	for(i=0;i<MAX_BUS_WIDTH;i++)
	{
		if(jc->ram_data_pin[i]>=0 && jc->ram_data_device[i]>=0)
		{
			if(jtagcore_get_pin_state(jc, jc->ram_data_device[i], jc->ram_data_pin[i], JTAG_CORE_INPUT))
			{
				value |= (0x00000001<<i);
			}
		}
	}

	// dessert RD
	if(jc->ram_ctrl_pin[JTAG_CORE_RAM_RD_CTRL]>=0 && jc->ram_ctrl_device[JTAG_CORE_RAM_RD_CTRL]>=0)
	{
		jtagcore_set_pin_state(jc, jc->ram_ctrl_device[JTAG_CORE_RAM_RD_CTRL], jc->ram_ctrl_pin[JTAG_CORE_RAM_RD_CTRL], JTAG_CORE_OUTPUT, ( jc->ram_ctrl_pin_pol[JTAG_CORE_RAM_RD_CTRL] & 1 ) ^ 1 );
	}

	// deassert CS
	if(jc->ram_ctrl_pin[JTAG_CORE_RAM_CS_CTRL]>=0 && jc->ram_ctrl_device[JTAG_CORE_RAM_CS_CTRL]>=0)
	{
		jtagcore_set_pin_state(jc, jc->ram_ctrl_device[JTAG_CORE_RAM_CS_CTRL], jc->ram_ctrl_pin[JTAG_CORE_RAM_CS_CTRL], JTAG_CORE_OUTPUT, ( jc->ram_ctrl_pin_pol[JTAG_CORE_RAM_CS_CTRL] & 1 ) ^ 1 );
	}

	jtagcore_push_and_pop_chain(jc, JTAG_CORE_WRITE_ONLY);

	return value;
}

int jtagcore_memory_write(jtag_core * jc, int mem_adr, unsigned long data)
{

	int i;

	// Set address bus
	for(i=0;i<MAX_BUS_WIDTH;i++)
	{
		if(jc->ram_address_pin[i]>=0 && jc->ram_address_device[i]>=0)
		{
			jtagcore_set_pin_state(jc, jc->ram_address_device[i], jc->ram_address_pin[i], JTAG_CORE_OE, 1);

			if( mem_adr & (0x00000001<<i) )
			{
				jtagcore_set_pin_state(jc, jc->ram_address_device[i], jc->ram_address_pin[i], JTAG_CORE_OUTPUT, 1);

			}
			else
			{
				jtagcore_set_pin_state(jc, jc->ram_address_device[i], jc->ram_address_pin[i], JTAG_CORE_OUTPUT, 0);
			}
		}
	}

	// Set data bus
	for(i=0;i<MAX_BUS_WIDTH;i++)
	{
		if(jc->ram_data_pin[i]>=0 && jc->ram_data_device[i]>=0)
		{
			jtagcore_set_pin_state(jc, jc->ram_data_device[i], jc->ram_data_pin[i], JTAG_CORE_OE, 1);

			if( data & (0x00000001<<i) )
			{
				jtagcore_set_pin_state(jc, jc->ram_data_device[i], jc->ram_data_pin[i], JTAG_CORE_OUTPUT, 1);

			}
			else
			{
				jtagcore_set_pin_state(jc, jc->ram_data_device[i], jc->ram_data_pin[i], JTAG_CORE_OUTPUT, 0);
			}
		}
	}

	// deassert control pins.
	for(i=0;i<16;i++)
	{
		if(jc->ram_ctrl_pin[i]>=0 && jc->ram_ctrl_device[i]>=0)
		{
			jtagcore_set_pin_state(jc, jc->ram_ctrl_device[i], jc->ram_ctrl_pin[i], JTAG_CORE_OE, 1);

			jtagcore_set_pin_state(jc, jc->ram_ctrl_device[i], jc->ram_ctrl_pin[i], JTAG_CORE_OUTPUT, (jc->ram_ctrl_pin_pol[i]&1)^1 );
		}
	}

	jtagcore_push_and_pop_chain(jc, JTAG_CORE_WRITE_ONLY);

	// Assert CS
	if(jc->ram_ctrl_pin[JTAG_CORE_RAM_CS_CTRL]>=0 && jc->ram_ctrl_device[JTAG_CORE_RAM_CS_CTRL]>=0)
	{
		jtagcore_set_pin_state(jc, jc->ram_ctrl_device[JTAG_CORE_RAM_CS_CTRL], jc->ram_ctrl_pin[JTAG_CORE_RAM_CS_CTRL], JTAG_CORE_OUTPUT, jc->ram_ctrl_pin_pol[JTAG_CORE_RAM_CS_CTRL] & 1 );
	}

	// Assert WR
	if(jc->ram_ctrl_pin[JTAG_CORE_RAM_WR_CTRL]>=0 && jc->ram_ctrl_device[JTAG_CORE_RAM_WR_CTRL]>=0)
	{
		jtagcore_set_pin_state(jc, jc->ram_ctrl_device[JTAG_CORE_RAM_WR_CTRL], jc->ram_ctrl_pin[JTAG_CORE_RAM_WR_CTRL], JTAG_CORE_OUTPUT, jc->ram_ctrl_pin_pol[JTAG_CORE_RAM_WR_CTRL] & 1 );
	}

	jtagcore_push_and_pop_chain(jc, JTAG_CORE_WRITE_ONLY);

	// dessert WR
	if(jc->ram_ctrl_pin[JTAG_CORE_RAM_WR_CTRL]>=0 && jc->ram_ctrl_device[JTAG_CORE_RAM_WR_CTRL]>=0)
	{
		jtagcore_set_pin_state(jc, jc->ram_ctrl_device[JTAG_CORE_RAM_WR_CTRL], jc->ram_ctrl_pin[JTAG_CORE_RAM_WR_CTRL], JTAG_CORE_OUTPUT, (jc->ram_ctrl_pin_pol[JTAG_CORE_RAM_WR_CTRL] & 1) ^ 1 );
	}

	jtagcore_push_and_pop_chain(jc, JTAG_CORE_WRITE_ONLY);

	// deassert CS
	if(jc->ram_ctrl_pin[JTAG_CORE_RAM_CS_CTRL]>=0 && jc->ram_ctrl_device[JTAG_CORE_RAM_CS_CTRL]>=0)
	{
		jtagcore_set_pin_state(jc, jc->ram_ctrl_device[JTAG_CORE_RAM_CS_CTRL], jc->ram_ctrl_pin[JTAG_CORE_RAM_CS_CTRL], JTAG_CORE_OUTPUT, ( jc->ram_ctrl_pin_pol[JTAG_CORE_RAM_CS_CTRL] & 1 ) ^ 1 );
	}

	// Set data bus tristate.
	for(i=0;i<MAX_BUS_WIDTH;i++)
	{
		if(jc->ram_data_pin[i]>=0 && jc->ram_data_device[i]>=0)
		{
			jtagcore_set_pin_state(jc, jc->ram_data_device[i], jc->ram_data_pin[i], JTAG_CORE_OE, 0);
		}
	}

	jtagcore_push_and_pop_chain(jc, JTAG_CORE_WRITE_ONLY);

	return JTAG_CORE_NO_ERROR;
}
