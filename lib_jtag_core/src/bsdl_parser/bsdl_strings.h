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
 * @file   bsdl_strings.h
 * @brief  bsdl file string keywords header
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

typedef struct type_strings_
{
	char * type_name;
	int type_code;
}type_strings;

enum CELLTYPE
{
	CELLTYPE_UNKNOWN = 0x00,
	CELLTYPE_BC1,
	CELLTYPE_BC2,
	CELLTYPE_BC3,
	CELLTYPE_BC4,
	CELLTYPE_BC5,
	CELLTYPE_BC6,
	CELLTYPE_BC7
};

enum BITTYPE
{
	BITTYPE_UNKNOWN = 0x00,
	BITTYPE_INPUT,
	BITTYPE_OUTPUT,
	BITTYPE_TRISTATE_OUTPUT,
	BITTYPE_INOUT,
	BITTYPE_CONTROL,
	BITTYPE_INTERNAL
};

enum STATETYPE
{
	STATE_UNKNOWN = 0x00,
	STATE_UNDEF,
	STATE_HIGH,
	STATE_LOW,
	STATE_HIGHZ
};

enum PINIOTYPE
{
	IO_UNDEF = 0x00,
	IO_IN,
	IO_OUT,
	IO_INOUT
};

extern type_strings celltype_str[];
extern type_strings bittype_str[];
extern type_strings statetype_str[];
extern type_strings pintype_str[];

int get_typecode(type_strings * typelist,char * name);
