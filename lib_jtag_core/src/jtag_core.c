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
 * @file   jtag_core.c
 * @brief  Main jtag core library functions
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "drivers/drv_loader.h"
#include "jtag_core_internal.h"
#include "jtag_core.h"

#include "script/env.h"

#include "bsdl_parser/bsdl_loader.h"
#include "bsdl_parser/bsdl_strings.h"

#include "drivers/drivers_list.h"

#include "config_script.h"

jtag_core * jtagcore_init()
{
	jtag_core * jc;
	script_ctx * sctx;

	jc = (jtag_core *)malloc(sizeof(jtag_core));
	if ( jc )
	{
		memset( jc, 0, sizeof(jtag_core) );

		jtagcore_setEnvVar( jc, "LIBVERSION", "v"LIB_JTAG_CORE_VERSION);

		sctx = jtagcore_initScript(jc);

		jtagcore_execScriptRam( sctx, config_script, config_script_len );

		jtagcore_execScriptFile( sctx, "config.script" );

		jtagcore_deinitScript(sctx);
	}

	return jc;
}

int jtagcore_resetchain(jtag_core * jc)
{
	int i;
	unsigned char buf_out[16];

	if (jc)
	{
		// Jtag Sync / Reset
		for (i = 0; i < 6; i++)
		{
			buf_out[i] = JTAG_STR_TMS;
		}

		jc->io_functions.drv_TX_TMS(jc, (unsigned char *)&buf_out, i);

		return JTAG_CORE_NO_ERROR;
	}

	return JTAG_CORE_BAD_PARAMETER;
}

int jtagcore_scan_and_init_chain(jtag_core * jc)
{
	unsigned char buf_in[512], buf_out[512];
	int i,j;
	int device_found_1, device_found_2, device_found;

	device_found = 0;
	if (jc )
	{
		for(i = 0;i<jc->nb_of_devices_in_chain;i++)
		{
			if (jc->devices_list[i].bsdl)
			{
				unload_bsdlfile(jc,jc->devices_list[i].bsdl);
				jc->devices_list[i].bsdl = 0;
			}
		}

		jc->nb_of_devices_in_chain = 0;
		jc->IR_filled = 0;

		if (!jc->io_functions.drv_TX_TMS)
			return JTAG_CORE_NO_PROBE;

		jtagcore_resetchain(jc);

		// Go to shift-IR
		buf_out[0] = 0x00;
		buf_out[1] = JTAG_STR_TMS;
		buf_out[2] = JTAG_STR_TMS;
		buf_out[3] = 0x00;
		buf_out[4] = 0x00;
		jc->io_functions.drv_TX_TMS(jc, (unsigned char *)&buf_out, 5);

		// Measure the total IR lenght

		//Flush the IR regs
		for (i = 0; i < sizeof(buf_out); i++)
		{
			buf_out[i] = 0;
		}

		jc->io_functions.drv_TXRX_DATA(jc, (unsigned char *)&buf_out, (unsigned char *)&buf_in, sizeof(buf_out));
		jc->io_functions.drv_TXRX_DATA(jc, (unsigned char *)&buf_out, (unsigned char *)&buf_in, sizeof(buf_out));

		for (i = 0; i < sizeof(buf_out); i++)
		{
			buf_out[i] = JTAG_STR_DOUT;
		}
		jc->io_functions.drv_TXRX_DATA(jc, (unsigned char *)&buf_out, (unsigned char *)&buf_in, sizeof(buf_out));

		i = 0;
		while( i < sizeof(buf_in) && !buf_in[i])
		{
			i++;
		}

		jc->total_IR_lenght = i;

		// Send the bypass instruction to all devices
		for (i = 0; i < sizeof(buf_out); i++)
		{
			buf_out[i] = JTAG_STR_DOUT;
		}

		jc->io_functions.drv_TXRX_DATA(jc, (unsigned char *)&buf_out, 0, sizeof(buf_out));
		jc->io_functions.drv_TXRX_DATA(jc, (unsigned char *)&buf_out, 0, sizeof(buf_out));
		buf_out[0] = JTAG_STR_DOUT | JTAG_STR_TMS;
		jc->io_functions.drv_TXRX_DATA(jc, (unsigned char *)&buf_out, 0, 1);

		// Go to the Shift-DR
		buf_out[0] = JTAG_STR_TMS;
		buf_out[1] = JTAG_STR_TMS;
		buf_out[2] = 0x00;
		buf_out[3] = 0x00;
		jc->io_functions.drv_TX_TMS(jc, (unsigned char *)&buf_out, 4);


		// Now fill the full chain to count the number of devices
		for (i = 0; i < sizeof(buf_out); i++)
		{
			buf_out[i] = 0x00;
		}
		jc->io_functions.drv_TXRX_DATA(jc, (unsigned char *)&buf_out, 0, sizeof(buf_out));

		for (i = 0; i < sizeof(buf_out); i++)
		{
			buf_out[i] = JTAG_STR_DOUT;
		}
		jc->io_functions.drv_TXRX_DATA(jc, (unsigned char *)&buf_out, (unsigned char *)&buf_in, sizeof(buf_out));

		device_found_1 = 0;
		i = 0;
		while ((device_found_1 < sizeof(buf_out)) && (!buf_in[device_found_1]))
		{
			device_found_1++;
		}

		// Fill the full chain with the opposite state to count again the number of devices
		for (i = 0; i < sizeof(buf_out); i++)
		{
			buf_out[i] = JTAG_STR_DOUT;
		}
		jc->io_functions.drv_TXRX_DATA(jc, (unsigned char *)&buf_out, 0, sizeof(buf_out));

		for (i = 0; i < sizeof(buf_out); i++)
		{
			buf_out[i] = 0x00;
		}
		jc->io_functions.drv_TXRX_DATA(jc, (unsigned char *)&buf_out, (unsigned char *)&buf_in, sizeof(buf_out));

		device_found_2 = 0;
		i = 0;
		while ((device_found_2 < sizeof(buf_out)) && (buf_in[device_found_2]))
		{
			device_found_2++;
		}

		if (device_found_1 == device_found_2)
		{
			if(device_found_1!=sizeof(buf_out))
			{
				// Detection valid !
				device_found = device_found_1;

				jc->nb_of_devices_in_chain = device_found;
			}
		}

		if (device_found)
		{
			// Jtag Sync / Reset
			jtagcore_resetchain(jc);

			// Go to shift-DR
			buf_out[0] = 0x00;
			buf_out[1] = JTAG_STR_TMS;
			buf_out[2] = 0x00;
			buf_out[3] = 0x00;
			jc->io_functions.drv_TX_TMS(jc, (unsigned char *)&buf_out, 4);

			memset(buf_out, 0x00, sizeof(buf_out));

			// Read all the devices ID.
			for (i = 0; i < device_found && i < MAX_NB_JTAG_DEVICE; i++)
			{
				jc->io_functions.drv_TXRX_DATA(jc, (unsigned char *)&buf_out, (unsigned char *)&buf_in, 32);
				jc->devices_list[i].devices_id = 0x00000000;
				for (j = 0; j < 32; j++)
				{
					if (buf_in[j])
						jc->devices_list[i].devices_id |= (0x00000001 << j);
				}
			}
		}

		jtagcore_resetchain(jc);

		// Idle...
		buf_out[0] = 0x00;
		buf_out[1] = 0x00;
		buf_out[2] = 0x00;
		buf_out[3] = 0x00;
		buf_out[4] = 0x00;
		buf_out[5] = 0x00;
		jc->io_functions.drv_TX_TMS(jc, (unsigned char *)&buf_out, 6);

		return JTAG_CORE_NO_ERROR;
	}

	return JTAG_CORE_BAD_PARAMETER;

}

int jtagcore_get_number_of_devices(jtag_core * jc)
{
	if (jc)
	{
		return jc->nb_of_devices_in_chain;
	}

	return JTAG_CORE_BAD_PARAMETER;
}

unsigned long jtagcore_get_dev_id(jtag_core * jc,int device)
{
	if (jc && device >= 0)
	{
		if (device < jc->nb_of_devices_in_chain && device < MAX_NB_JTAG_DEVICE)
		{
			return jc->devices_list[device].devices_id;
		}
	}

	return JTAG_CORE_BAD_PARAMETER;
}

int jtagcore_loadbsdlfile(jtag_core * jc, char * path, int device)
{
	jtag_bsdl * bsdl_file;
	int i;

	if ( (device < jc->nb_of_devices_in_chain && device < MAX_NB_JTAG_DEVICE && device >= 0) || device == -1)
	{
		bsdl_file = load_bsdlfile(jc,path);
		if (bsdl_file)
		{
			if (device == -1)
			{
				device = 0;
				i = 0;
				while (i < jc->nb_of_devices_in_chain)
				{
					if (jc->devices_list[i].bsdl)
					{
						unload_bsdlfile(jc,jc->devices_list[i].bsdl);
						jc->devices_list[i].bsdl = 0;
					}

					i++;
				}

				jc->nb_of_devices_in_chain = 1;

				jc->devices_list[0].bsdl = bsdl_file;

			}
			else
			{
				if (jc->devices_list[device].bsdl)
				{
					unload_bsdlfile(jc,jc->devices_list[device].bsdl);
				}

				jc->devices_list[device].bsdl = bsdl_file;
			}

			if (jc->devices_list[device].in_boundary_scan)
				free(jc->devices_list[device].in_boundary_scan);

			if (jc->devices_list[device].out_boundary_scan)
				free(jc->devices_list[device].out_boundary_scan);

			jc->devices_list[device].in_boundary_scan = malloc(bsdl_file->number_of_chainbits);
			if (jc->devices_list[device].in_boundary_scan)
				memset(jc->devices_list[device].in_boundary_scan, 0, bsdl_file->number_of_chainbits);

			jc->devices_list[device].out_boundary_scan = malloc(bsdl_file->number_of_chainbits);
			if (jc->devices_list[device].out_boundary_scan)
			{
				memset(jc->devices_list[device].out_boundary_scan, 0, bsdl_file->number_of_chainbits);
				for (i = 0; i < bsdl_file->number_of_chainbits; i++)
				{
					if(bsdl_file->chain_list[i].safe_state == STATE_HIGH)
						jc->devices_list[device].out_boundary_scan[i] = 0x01;
					else
						jc->devices_list[device].out_boundary_scan[i] = 0x00;

				}

				return JTAG_CORE_NO_ERROR;
			}

			return JTAG_CORE_MEM_ERROR;

		}
	}

	return JTAG_CORE_BAD_PARAMETER;
}

unsigned long jtagcore_get_bsdl_id(jtag_core * jc, char * path)
{
	jtag_bsdl * bsdl_file;
	unsigned long chip_id;

	bsdl_file = load_bsdlfile(jc,path);
	if (bsdl_file)
	{
		chip_id = bsdl_file->chip_id;
		unload_bsdlfile(jc,bsdl_file);
		return chip_id;
	}

	return 0;
}

int jtagcore_get_dev_name(jtag_core * jc, int device, char * devname, char * bsdlpath)
{
	jtag_bsdl * bsdl_file;

	if ( device < jc->nb_of_devices_in_chain && device < MAX_NB_JTAG_DEVICE && device >= 0 )
	{
		if (jc->devices_list[device].bsdl)
		{
			bsdl_file = jc->devices_list[device].bsdl;

			if(devname)
			{
				strcpy(devname,bsdl_file->entity_name);
			}

			if(bsdlpath)
			{
				strcpy(bsdlpath,bsdl_file->src_filename);
			}

			return JTAG_CORE_NO_ERROR;
		}

		if(devname)
		{
			devname[0] = '\0';
		}

		if(bsdlpath)
		{
			bsdlpath[0] = '\0';
		}

		return JTAG_CORE_NO_ERROR;
	}

	return JTAG_CORE_BAD_PARAMETER;
}

int jtagcore_get_number_of_pins(jtag_core * jc, int device)
{
	jtag_bsdl * bsdl_file;

	if ( device < jc->nb_of_devices_in_chain && device < MAX_NB_JTAG_DEVICE && device >= 0 )
	{
		if (jc->devices_list[device].bsdl)
		{
			bsdl_file = jc->devices_list[device].bsdl;

			return bsdl_file->number_of_pins;
		}
	}

	return JTAG_CORE_BAD_PARAMETER;
}

int jtagcore_get_pin_properties(jtag_core * jc, int device,int pin,char * pinname,int maxsize,int * type)
{
	jtag_bsdl * bsdl_file;
	int type_code;

	if (device >= jc->nb_of_devices_in_chain ||
		device >= MAX_NB_JTAG_DEVICE ||
		device < 0 ||
		pin < 0 )
	{
		return JTAG_CORE_BAD_PARAMETER;
	}

	if (jc->devices_list[device].bsdl)
	{
		bsdl_file = jc->devices_list[device].bsdl;

		if (pin < bsdl_file->number_of_pins  )
		{
			type_code = 0x00;

			if (pinname)
			{
				strncpy(pinname, bsdl_file->pins_list[pin].pinname, maxsize - 1);
				pinname[maxsize - 1] = '\0';
			}

			if (type)
			{
				if (bsdl_file->pins_list[pin].ctrl_bit_number != -1)
					type_code |= JTAG_CORE_PIN_IS_TRISTATES;

				if (bsdl_file->pins_list[pin].out_bit_number != -1)
					type_code |= JTAG_CORE_PIN_IS_OUTPUT;

				if (bsdl_file->pins_list[pin].in_bit_number != -1)
					type_code |= JTAG_CORE_PIN_IS_INPUT;

				*type = type_code;
			}

			return JTAG_CORE_NO_ERROR;
		}
	}

	return JTAG_CORE_BAD_PARAMETER;
}

int jtagcore_get_pin_state(jtag_core * jc, int device, int pin, int type)
{
	jtag_bsdl * bsdl_file;
	int bit_number, ret;
	int disable_state;

	ret = JTAG_CORE_BAD_PARAMETER;
	if (device < jc->nb_of_devices_in_chain && device < MAX_NB_JTAG_DEVICE && device >= 0 && pin >= 0 )
	{
		if (jc->devices_list[device].bsdl)
		{
			bsdl_file = jc->devices_list[device].bsdl;

			if (pin < bsdl_file->number_of_pins)
			{
				switch (type)
				{
				case JTAG_CORE_OUTPUT:
					bit_number = bsdl_file->pins_list[pin].out_bit_number;

					if (bit_number != -1)
					{
						if (jc->devices_list[device].scan_mode == JTAG_CORE_EXTEST_SCANMODE)
						{
							if (jc->devices_list[device].out_boundary_scan[bit_number])
								ret = 0x01;
							else
								ret = 0x00;
						}
						else
						{
							if (jc->devices_list[device].in_boundary_scan[bit_number])
								ret = 0x01;
							else
								ret = 0x00;
						}
					}

					break;

				case JTAG_CORE_OE:
					bit_number = bsdl_file->pins_list[pin].ctrl_bit_number;

					if (bit_number != -1)
					{
						disable_state = bsdl_file->chain_list[bsdl_file->pins_list[pin].out_bit_number].control_disable_state;
						if (jc->devices_list[device].scan_mode == JTAG_CORE_EXTEST_SCANMODE)
						{
							if (disable_state == STATE_HIGH)
							{
								if (jc->devices_list[device].out_boundary_scan[bit_number])
									ret = 0x00;
								else
									ret = 0x01;
							}
							else
							{
								if (jc->devices_list[device].out_boundary_scan[bit_number])
									ret = 0x01;
								else
									ret = 0x00;
							}
						}
						else
						{
							if (disable_state)
							{
								if (jc->devices_list[device].in_boundary_scan[bit_number])
									ret = 0x00;
								else
									ret = 0x01;
							}
							else
							{
								if (jc->devices_list[device].in_boundary_scan[bit_number])
									ret = 0x01;
								else
									ret = 0x00;
							}
						}
					}
					break;

				case JTAG_CORE_INPUT:
					bit_number = bsdl_file->pins_list[pin].in_bit_number;

					if (bit_number != -1)
					{
						if (jc->devices_list[device].in_boundary_scan[bit_number])
							ret = 0x01;
						else
							ret = 0x00;
					}
					break;

				}
			}
		}
	}
	return ret;
}

int jtagcore_set_pin_state(jtag_core * jc, int device, int pin, int type,int state)
{
	jtag_bsdl * bsdl_file;
	int bit_number,ret;
	int disable_state;

	ret = JTAG_CORE_BAD_PARAMETER;

	if ( device < jc->nb_of_devices_in_chain && device < MAX_NB_JTAG_DEVICE && device >= 0 && pin >= 0 )
	{
		if (jc->devices_list[device].bsdl)
		{
			bsdl_file = jc->devices_list[device].bsdl;

			if (pin < bsdl_file->number_of_pins)
			{
				switch (type)
				{
				case JTAG_CORE_OUTPUT:
					bit_number = bsdl_file->pins_list[pin].out_bit_number;

					if (bit_number != -1)
					{
						if (state)
							jc->devices_list[device].out_boundary_scan[bit_number] = 0x01;
						else
							jc->devices_list[device].out_boundary_scan[bit_number] = 0x00;

						ret = 0;
					}

					break;

				case JTAG_CORE_OE:
					bit_number = bsdl_file->pins_list[pin].ctrl_bit_number;

					if (bit_number != -1)
					{
						disable_state = bsdl_file->chain_list[bsdl_file->pins_list[pin].out_bit_number].control_disable_state;

						if (disable_state == STATE_HIGH)
						{
							if (state)
								jc->devices_list[device].out_boundary_scan[bit_number] = 0x00;
							else
								jc->devices_list[device].out_boundary_scan[bit_number] = 0x01;
						}
						else
						{
							if (state)
								jc->devices_list[device].out_boundary_scan[bit_number] = 0x01;
							else
								jc->devices_list[device].out_boundary_scan[bit_number] = 0x00;
						}
						ret = 0;
					}
					break;

				}
			}
		}
	}
	return ret;

}

int jtagcore_get_pin_id(jtag_core * jc, int device, char * pinname)
{
	jtag_bsdl * bsdl_file;
	int pin;

	if (device < jc->nb_of_devices_in_chain && device < MAX_NB_JTAG_DEVICE && device >= 0 && pinname)
	{
		if (jc->devices_list[device].bsdl)
		{
			bsdl_file = jc->devices_list[device].bsdl;

			for ( pin = 0; pin < bsdl_file->number_of_pins; pin++)
			{
				if(bsdl_file->pins_list[pin].pinname)
				{
					if(!strcmp(bsdl_file->pins_list[pin].pinname, pinname))
					{
						return pin;
					}
				}
			}

			return JTAG_CORE_NOT_FOUND;
		}
	}

	return JTAG_CORE_BAD_PARAMETER;
}


int jtagcore_set_scan_mode(jtag_core * jc, int device, int scan_mode)
{
	int ret;

	ret = JTAG_CORE_BAD_PARAMETER;

	if (device < jc->nb_of_devices_in_chain && device < MAX_NB_JTAG_DEVICE && device >= 0)
	{
		switch (scan_mode)
		{
			case JTAG_CORE_SAMPLE_SCANMODE:
				jc->devices_list[device].scan_mode = JTAG_CORE_SAMPLE_SCANMODE;
				jc->IR_filled = 0;

				ret = JTAG_CORE_NO_ERROR;
			break;
			case JTAG_CORE_EXTEST_SCANMODE:
				jc->devices_list[device].scan_mode = JTAG_CORE_EXTEST_SCANMODE;
				jc->IR_filled = 0;

				ret = JTAG_CORE_NO_ERROR;
				break;
			default:
				break;
		}
	}

	return ret;
}

int jtagcore_push_and_pop_chain(jtag_core * jc, int mode)
{
	unsigned char buf_out[512];
	unsigned char buf_in[512];
	int d,i,irlen;
	int bit,first_bit,jtag_chain_check_needed;
	jtag_bsdl * bsdl;

	jtag_chain_check_needed = 0;

	if (jc)
	{
		if (jc->nb_of_devices_in_chain && jc->io_functions.drv_TX_TMS)
		{
			//Idle State -> Go to shift-DR
			buf_out[0] = 0x00;
			buf_out[1] = JTAG_STR_TMS;
			buf_out[2] = 0x00;
			buf_out[3] = 0x00;
			jc->io_functions.drv_TX_TMS(jc, (unsigned char *)&buf_out, 4);
			for (d = 0; d < jc->nb_of_devices_in_chain; d++)
			{
				bsdl = jc->devices_list[d].bsdl;
				if (bsdl && jc->devices_list[d].out_boundary_scan && jc->devices_list[d].in_boundary_scan)
				{
					if (mode == JTAG_CORE_WRITE_READ)
					{
						if (d == (jc->nb_of_devices_in_chain - 1)) // Last device in chain ?
						{
							jc->io_functions.drv_TXRX_DATA(jc, jc->devices_list[d].out_boundary_scan, jc->devices_list[d].in_boundary_scan, bsdl->number_of_chainbits - 1);
							buf_out[0] = jc->devices_list[d].out_boundary_scan[bsdl->number_of_chainbits - 1] | JTAG_STR_TMS;
							jc->io_functions.drv_TXRX_DATA(jc, (unsigned char *)&buf_out, &jc->devices_list[d].in_boundary_scan[bsdl->number_of_chainbits - 1], 1);
						}
						else
						{
							jc->io_functions.drv_TXRX_DATA(jc, jc->devices_list[d].out_boundary_scan, jc->devices_list[d].in_boundary_scan, bsdl->number_of_chainbits);
						}

						// Check the incomming data
						if(bsdl->number_of_chainbits)
						{
							first_bit = jc->devices_list[d].in_boundary_scan[0];
							bit = 0;
							while( (bit < bsdl->number_of_chainbits) && (first_bit == jc->devices_list[d].in_boundary_scan[bit]) )
							{
								bit++;
							}

							if( bit == bsdl->number_of_chainbits)
							{
								// All bits have the same value.
								// Program a jtag chain check
								jtag_chain_check_needed |= 1;
								jc->IR_filled = 0;
							}
						}
					}
					else
					{
						// Write only mode - Ignore incoming chain.
						if (d == (jc->nb_of_devices_in_chain - 1)) // Last device in chain ?
						{
							jc->io_functions.drv_TXRX_DATA(jc, jc->devices_list[d].out_boundary_scan, 0, bsdl->number_of_chainbits - 1);
							buf_out[0] = jc->devices_list[d].out_boundary_scan[bsdl->number_of_chainbits - 1] | JTAG_STR_TMS;
							jc->io_functions.drv_TXRX_DATA(jc, (unsigned char *)&buf_out, 0, 1);
						}
						else
						{
							jc->io_functions.drv_TXRX_DATA(jc, jc->devices_list[d].out_boundary_scan, 0, bsdl->number_of_chainbits);
						}
					}

				}
				else
				{
					// Bypassed
					if (d == (jc->nb_of_devices_in_chain-1)) // Last device in chain ?
						buf_out[0] =  JTAG_STR_TMS;
					else
						buf_out[0] = 0x00;
					jc->io_functions.drv_TXRX_DATA(jc, (unsigned char *)&buf_out, 0, 1);
				}
			}

			if (!jc->IR_filled)
			{
				jc->IR_filled = 1;
				//  Exit1-DR -> Go to shift-IR
				buf_out[0] = JTAG_STR_TMS;
				buf_out[1] = JTAG_STR_TMS;
				buf_out[2] = JTAG_STR_TMS;
				buf_out[3] = 0x00;
				buf_out[4] = 0x00;
				jc->io_functions.drv_TX_TMS(jc, (unsigned char *)&buf_out, 5);


				if(jtag_chain_check_needed)
				{
					// Zero test
					for(i=0;i<512;i++)
					{
						buf_out[i] = 0;
					}

					jc->io_functions.drv_TXRX_DATA(jc, (unsigned char *)&buf_out, 0, sizeof(buf_in));
					jc->io_functions.drv_TXRX_DATA(jc, (unsigned char *)&buf_out, 0, sizeof(buf_in));
					jc->io_functions.drv_TXRX_DATA(jc, (unsigned char *)&buf_out, 0, sizeof(buf_in));
					memset(buf_in,1,sizeof(buf_in));
					jc->io_functions.drv_TXRX_DATA(jc, (unsigned char *)&buf_out, buf_in, sizeof(buf_in));

					i = 0;
					while(i<sizeof(buf_in) && buf_in[i] == 0)
					{
						i++;
					}

					if( i != sizeof(buf_in) )
					{
						goto jtag_error;
					}

					// One test

					for(i=0;i<512;i++)
					{
						buf_out[i] = JTAG_STR_DOUT;
					}

					jc->io_functions.drv_TXRX_DATA(jc, (unsigned char *)&buf_out, 0, sizeof(buf_in));
					jc->io_functions.drv_TXRX_DATA(jc, (unsigned char *)&buf_out, 0, sizeof(buf_in));
					jc->io_functions.drv_TXRX_DATA(jc, (unsigned char *)&buf_out, 0, sizeof(buf_in));
					memset(buf_in,0,sizeof(buf_in));
					jc->io_functions.drv_TXRX_DATA(jc, (unsigned char *)&buf_out, buf_in, sizeof(buf_in));

					i = 0;
					while(i<sizeof(buf_in) && buf_in[i] == JTAG_STR_DOUT)
					{
						i++;
					}

					if( i != sizeof(buf_in) )
					{
						goto jtag_error;
					}

					// Data toggle test.

					for(i=0;i<512;i++)
					{
						if(i & 1)
							buf_out[i] = JTAG_STR_DOUT;
						else
							buf_out[i] = 0x00;
					}

					jc->io_functions.drv_TXRX_DATA(jc, (unsigned char *)&buf_out, 0, sizeof(buf_in));
					jc->io_functions.drv_TXRX_DATA(jc, (unsigned char *)&buf_out, 0, sizeof(buf_in));
					jc->io_functions.drv_TXRX_DATA(jc, (unsigned char *)&buf_out, 0, sizeof(buf_in));
					memset(buf_in,0,sizeof(buf_in));
					jc->io_functions.drv_TXRX_DATA(jc, (unsigned char *)&buf_out, buf_in, sizeof(buf_in));

					i = 1;
					bit = buf_in[0];
					while( i < sizeof(buf_in) && buf_in[i] != bit)
					{
						bit = buf_in[i];
						i++;
					}

					if( i != sizeof(buf_in) )
					{
						goto jtag_error;
					}

					// All tests passed - reprogram the IR register.
				}

				for (d = 0; d < jc->nb_of_devices_in_chain; d++)
				{
					bsdl = jc->devices_list[d].bsdl;
					if ( bsdl )
					{
						switch ( jc->devices_list[d].scan_mode )
						{
							case JTAG_CORE_SAMPLE_SCANMODE:
								for (i = 0; i < bsdl->number_of_bits_per_instruction; i++)
								{
									buf_out[(bsdl->number_of_bits_per_instruction - 1) - i] = bsdl->SAMPLE_Instruction[i] - '0';
								}
							break;
							case JTAG_CORE_EXTEST_SCANMODE:
								for (i = 0; i < bsdl->number_of_bits_per_instruction; i++)
								{
									buf_out[(bsdl->number_of_bits_per_instruction - 1) - i] = bsdl->EXTEST_Instruction[i] - '0';
								}
							break;
						}


						if (d == (jc->nb_of_devices_in_chain - 1)) // Last device in chain ?
						{
							jc->io_functions.drv_TXRX_DATA(jc, (unsigned char *)&buf_out, 0, bsdl->number_of_bits_per_instruction - 1);
							buf_out[0] = buf_out[bsdl->number_of_bits_per_instruction - 1] | JTAG_STR_TMS;
							jc->io_functions.drv_TXRX_DATA(jc, (unsigned char *)&buf_out, 0, 1);
						}
						else
						{
							jc->io_functions.drv_TXRX_DATA(jc, (unsigned char *)&buf_out, 0, bsdl->number_of_bits_per_instruction);
						}
					}
					else
					{
						// Bypass
						irlen = 3;
						switch ( jc->nb_of_devices_in_chain )
						{
						case 1:
							irlen = jc->total_IR_lenght;
							break;
						case 2:
							if (d)
							{
								if ( jc->devices_list[0].bsdl )
								{
									bsdl = jc->devices_list[0].bsdl;
									irlen = jc->total_IR_lenght - bsdl->number_of_bits_per_instruction;
								}
							}
							else
							{
								if ( jc->devices_list[1].bsdl )
								{
									bsdl = jc->devices_list[1].bsdl;
									irlen = jc->total_IR_lenght - bsdl->number_of_bits_per_instruction;
								}
							}
							break;
						default:
							irlen = 3;
							break;
						}

						memset(buf_out, JTAG_STR_DOUT, sizeof(buf_out));

						if (d == (jc->nb_of_devices_in_chain - 1)) // Last device in chain ?
						{
							jc->io_functions.drv_TXRX_DATA(jc, (unsigned char *)&buf_out, 0, irlen - 1);
							buf_out[0] = JTAG_STR_DOUT | JTAG_STR_TMS;
							jc->io_functions.drv_TXRX_DATA(jc, (unsigned char *)&buf_out, 0, 1);
						}
						else
						{
							jc->io_functions.drv_TXRX_DATA(jc, (unsigned char *)&buf_out, 0, irlen);
						}
					}
				}
			}

			// Return To idle
			buf_out[0] = JTAG_STR_TMS;
			buf_out[1] = 0x00;
			buf_out[2] = 0x00;
			buf_out[3] = 0x00;
			buf_out[4] = 0x00;
			buf_out[5] = 0x00;
			buf_out[6] = 0x00;
			buf_out[7] = 0x00;
			jc->io_functions.drv_TX_TMS(jc, (unsigned char *)&buf_out, 6);

			return JTAG_CORE_NO_ERROR;
		}
	}

	return JTAG_CORE_BAD_PARAMETER;

jtag_error:
	// General error, bypass all devices.

	jc->IR_filled = 0;

	// Send the bypass instruction to all devices
	for (i = 0; i < sizeof(buf_out); i++)
	{
		buf_out[i] = JTAG_STR_DOUT;
	}

	jc->io_functions.drv_TXRX_DATA(jc, (unsigned char *)&buf_out, 0, sizeof(buf_out));
	jc->io_functions.drv_TXRX_DATA(jc, (unsigned char *)&buf_out, 0, sizeof(buf_out));
	buf_out[0] = JTAG_STR_DOUT | JTAG_STR_TMS;
	jc->io_functions.drv_TXRX_DATA(jc, (unsigned char *)&buf_out, 0, 1);

	return JTAG_CORE_IO_ERROR;
}

int jtagcore_get_number_of_probes_drv(jtag_core * jc)
{
	int i;

	if (jc)
	{
		i = 0;
		while (staticdrvs[i].getinfosfunc != (const DRV_GETMODULEINFOS)-1)
		{
			i++;
		}

		return i;
	}

	return JTAG_CORE_BAD_PARAMETER;
}

int jtagcore_get_number_of_probes(jtag_core * jc, int probe_driver_id)
{
	int totalprobe;

	if (jc)
	{
		totalprobe = 0;
		if( probe_driver_id < jtagcore_get_number_of_probes_drv(jc) && probe_driver_id >= 0 )
		{
			if( staticdrvs[probe_driver_id].getinfosfunc(jc,staticdrvs[ probe_driver_id ].sub_drv_id, GET_DRV_DETECT, &totalprobe) == JTAG_CORE_NO_ERROR )
			{
				return totalprobe;
			}
		}
	}

	return JTAG_CORE_BAD_PARAMETER;
}

int jtagcore_get_probe_name(jtag_core * jc, int probe_id, char * name)
{
	if ( ( probe_id >> 8 ) < jtagcore_get_number_of_probes_drv(jc) && probe_id >= 0 )
	{
		staticdrvs[ probe_id>>8 ].getinfosfunc(jc, probe_id & 0xFF, GET_DRV_DESCRIPTION, name);

		return JTAG_CORE_NO_ERROR;
	}

	return JTAG_CORE_BAD_PARAMETER;
}

int jtagcore_select_and_open_probe(jtag_core * jc, int probe_id)
{
	if ( ( probe_id >> 8 ) < jtagcore_get_number_of_probes_drv(jc) && probe_id >= 0 )
	{
		return jtagcore_loaddriver(jc, probe_id, NULL);
	}

	return JTAG_CORE_BAD_PARAMETER;
}

int jtagcore_setEnvVar( jtag_core * jc, char * varname, char * varvalue )
{
	envvar_entry * tmp_env;

	tmp_env = setEnvVar( jc->envvar, varname, varvalue );

	if( tmp_env )
	{
		jc->envvar = tmp_env;
		return JTAG_CORE_NO_ERROR;
	}
	else
	{
		return JTAG_CORE_MEM_ERROR;
	}
}

char * jtagcore_getEnvVar( jtag_core * jc, char * varname, char * varvalue)
{
	return getEnvVar( jc->envvar, varname, varvalue);
}

char * jtagcore_getEnvVarIndex( jtag_core * jc, int index, char * varvalue)
{
	return getEnvVarIndex( jc->envvar, index, varvalue);
}

int jtagcore_getEnvVarValue( jtag_core * jc, char * varname)
{
	return getEnvVarValue( jc->envvar, varname);
}

void jtagcore_deinit(jtag_core * jc)
{
	if( jc )
	{
		free( jc );
	}
}
