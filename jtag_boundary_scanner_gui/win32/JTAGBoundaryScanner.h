/*
 * JTAG Boundary Scanner
 * Copyright (c) 2008 - 2021 Viveris Technologies
 *
 * JTAG Boundary Scanner is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 3.0 of the License, or (at your option) any later version.
 *
 * JTAG Boundary Scanner is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 3 for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with JTAG Boundary Scanners; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

/**
* @file   jtagboundaryscan.h
* @brief  This header file provides definitions related to JTAG Boundary Scan utility.
* @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
* @author David de la Cruz <david.de-la-cruz@b2i.fr>
*/

#ifndef _JTAGBOUNDARYSCAN_H_
#define _JTAGBOUNDARYSCAN_H_

#define MAX_LOADSTRING 100

#define NB_PIX_PER_CHAR_H				8
#define NB_PIX_PER_CHECKBOX_H			20
#define NB_PIX_PER_CHECKBOX_V			20
#define NB_CHECKBOX						4

#define NB_PIX_FIRST_LINE_OFFS			20

// This structure is used to compute graphical positions of pin name and related checkboxes.
typedef struct graph_desc_
{
	int NbCol;				// number of colomn.
	int NbLines;			// number of lines.
	DWORD NbCharPerPinName;	// Number of characters per pin name.
	DWORD NbPixPerPinName;	// Number of pixels per pin name.
	DWORD NbPixPerCol;		// Number of pixels per colomn.

} graph_desc;

char * get_id_str(int numberofdevice);
void openconsole();

#endif // _JTAGBOUNDARYSCAN_H_
