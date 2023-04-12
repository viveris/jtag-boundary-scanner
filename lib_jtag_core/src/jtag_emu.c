/*
 * JTAG Core library
 * Copyright (c) 2008 - 2023 Viveris Technologies
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
 * @file   jtag_emu.c
 * @brief  jtag device emulator (test purpose)
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

jtag_emu * jtagemu_init(jtag_core * jc)
{
	jtag_emu * je;

	je = (jtag_emu *)malloc(sizeof(jtag_emu));
	if ( je )
	{
		memset( je, 0, sizeof(jtag_emu) );

		je->jc = jc;
	}

	return je;
}

int jtagemu_reset(jtag_emu * je)
{

	if (je)
	{

	}

	return JTAG_CORE_BAD_PARAMETER;
}

int jtagemu_tick(jtag_emu * je, unsigned char io_state)
{

	if (je)
	{

	}

	return JTAG_CORE_BAD_PARAMETER;
}

int jtagemu_get_regbit_state(jtag_emu * je, int regid, int bit)
{
	if (je)
	{

	}

	return JTAG_CORE_BAD_PARAMETER;
}

int jtagemu_loadbsdlfile(jtag_emu * je, char * path, int device)
{
	int i;
	jtag_bsdl * bsdl_file;

	if(je->bsdl_file)
	{
		unload_bsdlfile(je->jc,je->bsdl_file);
		je->bsdl_file = NULL;
	}

	if (je->data_register)
	{
		free(je->data_register);
		je->data_register = NULL;
	}

	if (je->inst_register)
	{
		free(je->inst_register);
		je->inst_register = NULL;
	}

	if (je->in_boundary_scan)
	{
		free(je->in_boundary_scan);
		je->in_boundary_scan = NULL;
	}

	if (je->out_boundary_scan)
	{
		free(je->out_boundary_scan);
		je->out_boundary_scan = NULL;
	}

	je->bsdl_file = load_bsdlfile(je->jc,path);
	if (je->bsdl_file)
	{
		bsdl_file = je->bsdl_file;
		je->data_register = malloc(bsdl_file->number_of_chainbits);
		if(je->data_register)
			memset(je->data_register, 0, bsdl_file->number_of_chainbits);

		je->inst_register = malloc(bsdl_file->number_of_bits_per_instruction);
		if(je->inst_register)
			memset(je->inst_register, 0, bsdl_file->number_of_bits_per_instruction);

		je->in_boundary_scan = malloc(bsdl_file->number_of_chainbits);
		if (je->in_boundary_scan)
			memset(je->in_boundary_scan, 0, bsdl_file->number_of_chainbits);

		je->out_boundary_scan = malloc(bsdl_file->number_of_chainbits);
		if (je->out_boundary_scan)
		{
			memset(je->out_boundary_scan, 0, bsdl_file->number_of_chainbits);
			for (i = 0; i < bsdl_file->number_of_chainbits; i++)
			{
				if(bsdl_file->chain_list[i].safe_state == STATE_HIGH)
					je->out_boundary_scan[i] = 0x01;
				else
					je->out_boundary_scan[i] = 0x00;
			}

			return JTAG_CORE_NO_ERROR;
		}
	}

	return JTAG_CORE_BAD_PARAMETER;
}

void jtagemu_deinit(jtag_emu * je)
{
	if( je )
	{
		if(je->bsdl_file)
		{
			unload_bsdlfile(je->jc,je->bsdl_file);
			je->bsdl_file = NULL;
		}

		if (je->data_register)
		{
			free(je->data_register);
			je->data_register = NULL;
		}

		if (je->inst_register)
		{
			free(je->inst_register);
			je->inst_register = NULL;
		}

		if (je->in_boundary_scan)
		{
			free(je->in_boundary_scan);
			je->in_boundary_scan = NULL;
		}

		if (je->out_boundary_scan)
		{
			free(je->out_boundary_scan);
			je->out_boundary_scan = NULL;
		}

		free( je );
	}
}
