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
 * @file   bsdl_strings.c
 * @brief  bsdl file string keywords
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

#include <string.h>

#include "../drivers/drv_loader.h"
#include "../jtag_core_internal.h"
#include "../jtag_core.h"

#include "bsdl_loader.h"
#include "bsdl_strings.h"

type_strings celltype_str[]=
{
	{"BC_1",CELLTYPE_BC1},
	{"BC_2",CELLTYPE_BC2},
	{"BC_3",CELLTYPE_BC3},
	{"BC_4",CELLTYPE_BC4},
	{"BC_5",CELLTYPE_BC5},
	{"BC_6",CELLTYPE_BC6},
	{"BC_7",CELLTYPE_BC7},

	{0,CELLTYPE_UNKNOWN}
};

type_strings bittype_str[]=
{
	{"INPUT",BITTYPE_INPUT},
	{"OBSERVE_ONLY",BITTYPE_INPUT},
	{"OUTPUT",BITTYPE_OUTPUT},
	{"OUTPUT2", BITTYPE_OUTPUT },
	{"OUTPUT3",BITTYPE_TRISTATE_OUTPUT},
	{"BIDIR",BITTYPE_INOUT},
	{"CONTROL",BITTYPE_CONTROL},
	{"CONTROLR",BITTYPE_CONTROL},
	{"INTERNAL",BITTYPE_INTERNAL},
	{0,BITTYPE_UNKNOWN}
};

type_strings statetype_str[]=
{
	{"X",STATE_UNDEF},
	{"1",STATE_HIGH},
	{"0",STATE_LOW},
	{"Z",STATE_HIGHZ},
	{0,STATE_UNKNOWN}
};

type_strings pintype_str[]=
{
	{"IN",IO_IN},
	{"OUT",IO_OUT},
	{"INOUT",IO_INOUT},
	{"BUFFER", IO_OUT },
	{0,IO_UNDEF}
};

int get_typecode(type_strings * typelist,char * name)
{
	int i;

	i = 0;
	while( typelist[i].type_name )
	{
		if(!strcmp( typelist[i].type_name, name ) )
		{
			return typelist[i].type_code;
		}
		i++;
	}

	return typelist[i].type_code;
}
