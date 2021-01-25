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
 * @file   i2c_over_jtag.c
 * @brief  I2C over jtag API
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

#include "../dbg_logs.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// I2C Over JTAG API
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int jtagcore_i2c_set_scl_pin(jtag_core * jc, int device, int pin)
{
	jtag_bsdl * bsdl_file;

	if (device < jc->nb_of_devices_in_chain && device < MAX_NB_JTAG_DEVICE)
	{
		if (jc->devices_list[device].bsdl)
		{
			bsdl_file = jc->devices_list[device].bsdl;

			if (pin < bsdl_file->number_of_pins)
			{
				jc->i2c_scl_pin = pin;
				jc->i2c_scl_device = device;

				jtagcore_logs_printf(jc,MSG_DEBUG,"jtagcore_i2c_set_scl_pin : device %d, pin %d\r\n",device,pin);

				return JTAG_CORE_NO_ERROR;
			}
			else
				jtagcore_logs_printf(jc,MSG_DEBUG,"jtagcore_i2c_set_scl_pin : ERROR invalid pin %d\r\n",pin);
		}
		else
			jtagcore_logs_printf(jc,MSG_DEBUG,"jtagcore_i2c_set_scl_pin : ERROR bsdl not loaded\r\n");
	}

	return JTAG_CORE_BAD_PARAMETER;
}

int jtagcore_i2c_set_sda_pin(jtag_core * jc, int device, int pin)
{
	jtag_bsdl * bsdl_file;

	if (device < jc->nb_of_devices_in_chain && device < MAX_NB_JTAG_DEVICE)
	{
		if (jc->devices_list[device].bsdl)
		{
			bsdl_file = jc->devices_list[device].bsdl;

			if (pin < bsdl_file->number_of_pins)
			{
				jc->i2c_sda_pin = pin;
				jc->i2c_sda_device = device;

				jtagcore_logs_printf(jc,MSG_DEBUG,"jtagcore_i2c_set_sda_pin : device %d, pin %d\r\n",device,pin);

				return JTAG_CORE_NO_ERROR;
			}
			else
				jtagcore_logs_printf(jc,MSG_DEBUG,"jtagcore_i2c_set_sda_pin : ERROR invalid pin %d\r\n",pin);
		}
		else
			jtagcore_logs_printf(jc,MSG_DEBUG,"jtagcore_i2c_set_sda_pin : ERROR bsdl not loaded\r\n");
	}

	return JTAG_CORE_BAD_PARAMETER;
}

static void i2c_start_bit(jtag_core * jc)
{
	// Start bit.
	jtagcore_set_pin_state(jc, jc->i2c_sda_device, jc->i2c_sda_pin, JTAG_CORE_OE, 1); // SDA low
	jtagcore_push_and_pop_chain(jc, JTAG_CORE_WRITE_ONLY);
	jtagcore_set_pin_state(jc, jc->i2c_scl_device, jc->i2c_scl_pin, JTAG_CORE_OE, 1); // SCL low
	jtagcore_push_and_pop_chain(jc, JTAG_CORE_WRITE_ONLY);
}


static void i2c_push_bit(jtag_core * jc, int sda_state)
{
	if (sda_state)
		jtagcore_set_pin_state(jc, jc->i2c_sda_device, jc->i2c_sda_pin, JTAG_CORE_OE, 0); // SDA High
	else
		jtagcore_set_pin_state(jc, jc->i2c_sda_device, jc->i2c_sda_pin, JTAG_CORE_OE, 1); // SDA Low

	jtagcore_push_and_pop_chain(jc, JTAG_CORE_WRITE_ONLY);

	jtagcore_set_pin_state(jc, jc->i2c_scl_device, jc->i2c_scl_pin, JTAG_CORE_OE, 0); // SCL High
	jtagcore_push_and_pop_chain(jc, JTAG_CORE_WRITE_ONLY);
	jtagcore_set_pin_state(jc, jc->i2c_scl_device, jc->i2c_scl_pin, JTAG_CORE_OE, 1); // SCL Low
	jtagcore_set_pin_state(jc, jc->i2c_sda_device, jc->i2c_sda_pin, JTAG_CORE_OE, 0); // SDA High
	jtagcore_push_and_pop_chain(jc, JTAG_CORE_WRITE_ONLY);

	if (!sda_state)
	{
		//jtagcore_set_pin_state(jc, jc->i2c_sda_device, jc->i2c_sda_pin, JTAG_CORE_OE, 0); // SDA High=
		//jtagcore_push_and_pop_chain(jc);
	}

}


static void i2c_stop_bit(jtag_core * jc)
{
	// Stop bit.
	jtagcore_set_pin_state(jc, jc->i2c_sda_device, jc->i2c_sda_pin, JTAG_CORE_OE, 1); // SDA low
	jtagcore_push_and_pop_chain(jc, JTAG_CORE_WRITE_ONLY);
	jtagcore_set_pin_state(jc, jc->i2c_scl_device, jc->i2c_scl_pin, JTAG_CORE_OE, 0); // SCL high
	jtagcore_push_and_pop_chain(jc, JTAG_CORE_WRITE_ONLY);
	jtagcore_set_pin_state(jc, jc->i2c_sda_device, jc->i2c_sda_pin, JTAG_CORE_OE, 0); // SDA high
	jtagcore_push_and_pop_chain(jc, JTAG_CORE_WRITE_ONLY);
}


static int i2c_wait_ack(jtag_core * jc)
{
	int i, acknowledged;

	acknowledged = 0;

	// ACK
	jtagcore_set_pin_state(jc, jc->i2c_sda_device, jc->i2c_sda_pin, JTAG_CORE_OE, 0); // SDA High
	jtagcore_push_and_pop_chain(jc, JTAG_CORE_WRITE_ONLY);
	jtagcore_set_pin_state(jc, jc->i2c_scl_device, jc->i2c_scl_pin, JTAG_CORE_OE, 0); // SCL High
	jtagcore_push_and_pop_chain(jc, JTAG_CORE_WRITE_ONLY);

	i = 0;
	do
	{
		if ( jtagcore_push_and_pop_chain(jc, JTAG_CORE_WRITE_READ) != JTAG_CORE_NO_ERROR )
		{
			return JTAG_CORE_IO_ERROR;
		}

		i++;
	} while (jtagcore_get_pin_state(jc, jc->i2c_sda_device, jc->i2c_sda_pin, JTAG_CORE_INPUT) && i < 10);

	if (i < 10)
		acknowledged = 1;

	jtagcore_set_pin_state(jc, jc->i2c_scl_device, jc->i2c_scl_pin, JTAG_CORE_OE, 1); // SCL Low
	jtagcore_push_and_pop_chain(jc, JTAG_CORE_WRITE_ONLY);

	return acknowledged;
}

int jtagcore_i2c_write_read(jtag_core * jc, int address, int address10bits,int wr_size,unsigned char * wr_buffer,int rd_size,unsigned char * rd_buffer)
{
	int i,j,acknowledged;

	acknowledged = 0;

	if( jc )
	{
		jtagcore_set_pin_state(jc, jc->i2c_scl_device, jc->i2c_scl_pin, JTAG_CORE_OE, 0);
		jtagcore_set_pin_state(jc, jc->i2c_sda_device, jc->i2c_sda_pin, JTAG_CORE_OE, 0);

		jtagcore_set_pin_state(jc, jc->i2c_scl_device, jc->i2c_scl_pin, JTAG_CORE_OUTPUT, 0);
		jtagcore_set_pin_state(jc, jc->i2c_sda_device, jc->i2c_sda_pin, JTAG_CORE_OUTPUT, 0);

		jtagcore_push_and_pop_chain(jc, JTAG_CORE_WRITE_ONLY);

		// Is the bus Free / High state?
		if ( jtagcore_push_and_pop_chain(jc, JTAG_CORE_WRITE_READ) != JTAG_CORE_NO_ERROR )
		{
			return JTAG_CORE_IO_ERROR;
		}

		if (!jtagcore_get_pin_state(jc, jc->i2c_sda_device, jc->i2c_sda_pin, JTAG_CORE_INPUT) || !jtagcore_get_pin_state(jc, jc->i2c_scl_device, jc->i2c_scl_pin, JTAG_CORE_INPUT))
		{
			return JTAG_CORE_I2C_BUS_NOTFREE;
		}

		// Start bit.
		i2c_start_bit(jc);

		for(i=0;i<7;i++)
		{
			i2c_push_bit(jc, address & (0x80 >> i));
		}

		// WR/RD
		if(wr_size)
		{
			i2c_push_bit(jc, 0);
		}
		else
		{
			i2c_push_bit(jc, 1);
		}

		// ACK
		acknowledged = i2c_wait_ack(jc);

		if( acknowledged < 0 ) // Error ?
		{
			jtagcore_set_pin_state(jc, jc->i2c_scl_device, jc->i2c_scl_pin, JTAG_CORE_OE, 0);
			jtagcore_set_pin_state(jc, jc->i2c_sda_device, jc->i2c_sda_pin, JTAG_CORE_OE, 0);

			jtagcore_push_and_pop_chain(jc, JTAG_CORE_WRITE_ONLY);

			return acknowledged;
		}

		if(wr_size)
		{

			for(i=0;i<wr_size;i++)
			{
				for(j=0;j<8;j++)
				{
					i2c_push_bit(jc, wr_buffer[i] & (0x80 >> j));
				}

				// ACK
				i2c_push_bit(jc, 1);
			}

			// Stop bit.
			i2c_stop_bit(jc);
		}
		else
		{
			for(i=0;i<rd_size;i++)
			{
				rd_buffer[i] = 0x00;

				for(j=0;j<8;j++)
				{
					do
					{
						jtagcore_set_pin_state(jc, jc->i2c_scl_device, jc->i2c_scl_pin, JTAG_CORE_OE, 0); // SCL High
						if ( jtagcore_push_and_pop_chain(jc, JTAG_CORE_WRITE_READ) != JTAG_CORE_NO_ERROR )
						{
							jtagcore_set_pin_state(jc, jc->i2c_scl_device, jc->i2c_scl_pin, JTAG_CORE_OE, 0);
							jtagcore_set_pin_state(jc, jc->i2c_sda_device, jc->i2c_sda_pin, JTAG_CORE_OE, 0);

							jtagcore_push_and_pop_chain(jc, JTAG_CORE_WRITE_ONLY);

							return JTAG_CORE_IO_ERROR;
						}

						if ( jtagcore_push_and_pop_chain(jc, JTAG_CORE_WRITE_READ) != JTAG_CORE_NO_ERROR )
						{
							jtagcore_set_pin_state(jc, jc->i2c_scl_device, jc->i2c_scl_pin, JTAG_CORE_OE, 0);
							jtagcore_set_pin_state(jc, jc->i2c_sda_device, jc->i2c_sda_pin, JTAG_CORE_OE, 0);

							jtagcore_push_and_pop_chain(jc, JTAG_CORE_WRITE_ONLY);

							return JTAG_CORE_IO_ERROR;
						}
					}while(!jtagcore_get_pin_state(jc,jc->i2c_scl_device,jc->i2c_scl_pin,JTAG_CORE_INPUT));


					if(jtagcore_get_pin_state(jc,jc->i2c_sda_device,jc->i2c_sda_pin,JTAG_CORE_INPUT))
					{
						rd_buffer[i] |= 0x80>>j;
					}

					jtagcore_set_pin_state(jc, jc->i2c_scl_device, jc->i2c_scl_pin, JTAG_CORE_OE, 1); // SCL Low
					jtagcore_push_and_pop_chain(jc, JTAG_CORE_WRITE_ONLY);
				}

				if(i!=rd_size-1)
				{
					// ACK
					i2c_push_bit(jc, 0);
				}
				else
				{
					// NACK
					i2c_push_bit(jc, 1);
				}
			}

			// Stop bit.
			i2c_stop_bit(jc);
		}

		return acknowledged;
	}

	return JTAG_CORE_BAD_PARAMETER;
}
