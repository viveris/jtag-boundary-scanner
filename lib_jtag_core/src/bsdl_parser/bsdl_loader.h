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
 * @file   bsdl_loader.h
 * @brief  bsdl file parser header
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

#define MAX_ELEMENT_SIZE (64+1)

typedef struct _pin_ctrl
{
	char pinname[MAX_ELEMENT_SIZE];
	int  pintype;

	char physical_pin[MAX_ELEMENT_SIZE];

	int ctrl_bit_number;
	int out_bit_number;
	int in_bit_number;
}pin_ctrl;

typedef struct _jtag_chain
{
	int bit_index;

	int bit_cell_type;                // BC_1,BC_2,...

	char pinname[MAX_ELEMENT_SIZE];   // Pin name.

	int bit_type;                     // None , ctrl , in, out.

	int safe_state;                   // Default - Safe state. (0,1,-1)

	int control_bit_index; // Indicate the associated control bit. -1 if no control bit.
	int control_disable_state;
	int control_disable_result;

}jtag_chain;

typedef struct _jtag_bsdl
{
	unsigned long chip_id;

	char src_filename[512];
	char entity_name[512];

	int number_of_chainbits;
	jtag_chain * chain_list;

	int number_of_pins;
	pin_ctrl * pins_list;

	int number_of_bits_per_instruction;
	char IDCODE_Instruction[MAX_ELEMENT_SIZE];
	char EXTEST_Instruction[MAX_ELEMENT_SIZE];
	char BYPASS_Instruction[MAX_ELEMENT_SIZE];
	char SAMPLE_Instruction[MAX_ELEMENT_SIZE];

}jtag_bsdl;

jtag_bsdl * load_bsdlfile(jtag_core * jc,char *filename);
void unload_bsdlfile(jtag_core * jc, jtag_bsdl * bsdl);
