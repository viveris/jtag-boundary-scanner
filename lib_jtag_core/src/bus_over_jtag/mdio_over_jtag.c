/*
 * JTAG Core library
 * Copyright (c) 2008 - 2024 Viveris Technologies
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
 * @file   mdio_over_jtag.c
 * @brief  MDC/MDIO over jtag API
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
// MDIO Over JTAG API
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int jtagcore_mdio_set_mdc_pin(jtag_core * jc, int device, int pin)
{
	jtag_bsdl * bsdl_file;

	if (device < jc->nb_of_devices_in_chain && device < MAX_NB_JTAG_DEVICE && device >= 0)
	{
		if (jc->devices_list[device].bsdl)
		{
			bsdl_file = jc->devices_list[device].bsdl;

			if (pin < bsdl_file->number_of_pins && pin >= 0)
			{
				jc->mdio_mdc_pin = pin;
				jc->mdio_mdc_device = device;
				return JTAG_CORE_NO_ERROR;
			}
		}
	}

	return JTAG_CORE_BAD_PARAMETER;
}

int jtagcore_mdio_set_mdio_pin(jtag_core * jc, int device, int pin)
{
	jtag_bsdl * bsdl_file;

	if (device < jc->nb_of_devices_in_chain && device < MAX_NB_JTAG_DEVICE && device >=0)
	{
		if (jc->devices_list[device].bsdl)
		{
			bsdl_file = jc->devices_list[device].bsdl;

			if (pin < bsdl_file->number_of_pins && pin >= 0)
			{
				jc->mdio_mdio_pin = pin;
				jc->mdio_mdio_device = device;
				return JTAG_CORE_NO_ERROR;
			}
		}
	}

	return JTAG_CORE_BAD_PARAMETER;
}

static void mdio_push_bit(jtag_core * jc, int bit_state)
{
	if (bit_state)
		jtagcore_set_pin_state(jc, jc->mdio_mdio_device, jc->mdio_mdio_pin, JTAG_CORE_OE, 0); // MDIO High
	else
		jtagcore_set_pin_state(jc, jc->mdio_mdio_device, jc->mdio_mdio_pin, JTAG_CORE_OE, 1); // MDIO Low

	jtagcore_set_pin_state(jc, jc->mdio_mdc_device, jc->mdio_mdc_pin, JTAG_CORE_OUTPUT, 0); // MDC Low

	jtagcore_push_and_pop_chain(jc, JTAG_CORE_WRITE_ONLY);

	jtagcore_set_pin_state(jc, jc->mdio_mdc_device, jc->mdio_mdc_pin, JTAG_CORE_OUTPUT, 1); // MDC High
	jtagcore_push_and_pop_chain(jc, JTAG_CORE_WRITE_ONLY);
 }


static int mdio_get_bit(jtag_core * jc)
{
	int state;

	jtagcore_set_pin_state(jc, jc->mdio_mdc_device, jc->mdio_mdc_pin, JTAG_CORE_OUTPUT, 1); // MDC High

	jtagcore_push_and_pop_chain(jc, JTAG_CORE_WRITE_ONLY);

	jtagcore_set_pin_state(jc, jc->mdio_mdc_device, jc->mdio_mdc_pin, JTAG_CORE_OUTPUT, 0); // MDC Low

	if ( jtagcore_push_and_pop_chain(jc, JTAG_CORE_WRITE_READ) != JTAG_CORE_NO_ERROR )
	{
		return JTAG_CORE_IO_ERROR;
	}

	state = jtagcore_get_pin_state(jc, jc->mdio_mdio_device, jc->mdio_mdio_pin, JTAG_CORE_INPUT);

	return state;
}


int jtagcore_mdio_write(jtag_core * jc, int phy_adr, int reg_adr, int data)
{
	int i;

	if( jc )
	{
		jtagcore_set_pin_state(jc, jc->mdio_mdc_device, jc->mdio_mdc_pin, JTAG_CORE_OE, 1);
		jtagcore_set_pin_state(jc, jc->mdio_mdio_device, jc->mdio_mdio_pin, JTAG_CORE_OE, 0);

		jtagcore_set_pin_state(jc, jc->mdio_mdc_device, jc->mdio_mdc_pin, JTAG_CORE_OUTPUT, 0);
		jtagcore_set_pin_state(jc, jc->mdio_mdio_device, jc->mdio_mdio_pin, JTAG_CORE_OUTPUT, 0);

		jtagcore_push_and_pop_chain(jc, JTAG_CORE_WRITE_ONLY);

		for (i = 0; i < 32; i++)
		{
			mdio_push_bit(jc, 1);
		}

		// Start.
		mdio_push_bit(jc, 0);
		mdio_push_bit(jc, 1);

		//Write
		mdio_push_bit(jc, 0);
		mdio_push_bit(jc, 1);

		// dev address
		for (i=0;i<5;i++)
		{
			if(phy_adr & (0x10>>i))
			{
				mdio_push_bit(jc, 1);
			}
			else
			{
				mdio_push_bit(jc, 0);
			}
		}

		// reg address
		for (i=0;i<5;i++)
		{
			if(reg_adr & (0x10>>i))
			{
				mdio_push_bit(jc, 1);
			}
			else
			{
				mdio_push_bit(jc, 0);
			}
		}

		//TA
		mdio_push_bit(jc, 1);
		mdio_push_bit(jc, 0);

		// data
		for (i=0;i<16;i++)
		{
			if(data & (0x8000>>i))
			{
				mdio_push_bit(jc, 1);
			}
			else
			{
				mdio_push_bit(jc, 0);
			}
		}

		mdio_push_bit(jc, 1);
		mdio_push_bit(jc, 1);
		mdio_push_bit(jc, 1);

		return 0;
	}
	return JTAG_CORE_BAD_PARAMETER;
}

int jtagcore_mdio_read(jtag_core * jc, int phy_adr, int reg_adr)
{
	int i,data;
	int bit_state;

	if( jc )
	{
		jtagcore_set_pin_state(jc, jc->mdio_mdc_device, jc->mdio_mdc_pin, JTAG_CORE_OE, 1);
		jtagcore_set_pin_state(jc, jc->mdio_mdio_device, jc->mdio_mdio_pin, JTAG_CORE_OE, 0);

		jtagcore_set_pin_state(jc, jc->mdio_mdc_device, jc->mdio_mdc_pin, JTAG_CORE_OUTPUT, 0);
		jtagcore_set_pin_state(jc, jc->mdio_mdio_device, jc->mdio_mdio_pin, JTAG_CORE_OUTPUT, 0);

		jtagcore_push_and_pop_chain(jc, JTAG_CORE_WRITE_ONLY);

		for (i = 0; i < 32; i++)
		{
			mdio_push_bit(jc, 1);
		}

		// Start.
		mdio_push_bit(jc, 0);
		mdio_push_bit(jc, 1);

		//Read
		mdio_push_bit(jc, 1);
		mdio_push_bit(jc, 0);

		// dev address
		for (i=0;i<5;i++)
		{
			if(phy_adr & (0x10>>i))
			{
				mdio_push_bit(jc, 1);
			}
			else
			{
				mdio_push_bit(jc, 0);
			}
		}

		// reg address
		for (i=0;i<5;i++)
		{
			if(reg_adr & (0x10>>i))
			{
				mdio_push_bit(jc, 1);
			}
			else
			{
				mdio_push_bit(jc, 0);
			}
		}

		// TA
		mdio_push_bit(jc, 1);

		jtagcore_set_pin_state(jc, jc->mdio_mdc_device, jc->mdio_mdc_pin, JTAG_CORE_OUTPUT, 0); // MDC Low
		jtagcore_push_and_pop_chain(jc, JTAG_CORE_WRITE_ONLY);

		data = 0;
		// data
		for (i=0;i<16;i++)
		{
			bit_state = mdio_get_bit(jc);
			if(bit_state < 0 )
			{
				mdio_push_bit(jc, 1);
				mdio_push_bit(jc, 1);
				mdio_push_bit(jc, 1);

				return bit_state;
			}

			if(bit_state)
			{
				data |= (0x8000>>i);
			}
		}

		mdio_push_bit(jc, 1);
		mdio_push_bit(jc, 1);
		mdio_push_bit(jc, 1);


		return data;
	}
	return JTAG_CORE_BAD_PARAMETER;
}

