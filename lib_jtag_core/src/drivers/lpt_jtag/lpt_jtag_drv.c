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
 * @file   lpt_jtag_drv.c
 * @brief  parallel port probes driver
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

#ifndef _WIN64

#include <stdio.h>
#include <string.h>

#if defined(WIN32)
#include <conio.h>
#endif

#include "../drv_loader.h"
#include "../../jtag_core_internal.h"
#include "../../jtag_core.h"

#include "../../bsdl_parser/bsdl_loader.h"

#include "../../dbg_logs.h"

#if defined(WIN32)

#include <windows.h>

HANDLE h;

#endif

#define LPT1 0x3bc
#define LPT2 0x378
#define LPT3 0x278

#define IGNORE_PORT 0
#define READ_PORT 1

unsigned short lpt_address;


int sub_probe_id;

typedef struct _drv_desc
{
	char *drv_id;
	char *drv_desc;
	int id;
}drv_desc;

#define PROBE_INSIGHT_IJC 0
#define PROBE_ALTERA_BYTEBLASTER 1
#define PROBE_MACRAIGOR_WIGGLER 2

const static drv_desc subdrv_list[]=
{
	{"LPT_INSIGHT_JTAG","LPT Memec IJC-4",PROBE_INSIGHT_IJC},
	{"LPT_ALTERA_BYTEBLASTER_JTAG","LPT Altera ByteBlaster",PROBE_ALTERA_BYTEBLASTER},
	{"LPT_MACRAIGOR_WIGGLERJTAG","LPT Macgraigor Wiggler",PROBE_MACRAIGOR_WIGGLER}
};

void out_io_port( unsigned short port, unsigned char data )
{
#if defined(WIN32)
	_outp( port, data );
#endif
}

unsigned char in_io_port( unsigned short port )
{
#if defined(WIN32)
	return _inp( port );
#else
	return 0x00;
#endif
}


int test_port(void)
{
	// search for valid parallel port
	out_io_port(LPT1, 0x55);
	if ((int)in_io_port(LPT1) == 0x55)
	{
		return LPT1;
	}

	out_io_port(LPT2, 0x55);
	if ((int)in_io_port(LPT2) == 0x55)
	{
		return LPT2;
	}

	out_io_port(LPT3, 0x55);
	if ((int)in_io_port(LPT3) == 0x55)
	{
		return LPT3;
	}

	return(0);	// return zero if none found
}

int putp(int tdi, int tms, int rp)
{
	int tdo = -1;
	int lpt_data;

	switch (sub_probe_id)
	{
	case PROBE_INSIGHT_IJC:
		// Insight/Xilinx :
		// D4  :   TDO DRV LOW (AL)
		// D3  :    OE (AL)
		// D1  :   TCK (AH)
		// D2  :   TMS (AH)
		// D0  :   TDI (AH) (To device)
		// OnLine :TDO (AH) (From device)

		lpt_data = 0x10; //Output to TDO off / Outputs enabled
		if (tms == 1) lpt_data |= 0x04;
		if (tdi == 1) lpt_data |= 0x01;

		out_io_port(lpt_address, (unsigned char)lpt_data);	// TCK low

		lpt_data |= 0x02;
		out_io_port(lpt_address, (unsigned char)lpt_data);	// TCK high

		if (rp == READ_PORT)
		{
			tdo = ((int)in_io_port((unsigned short)(lpt_address + 1)) & 0x10) >> 4;	// get TDO data
		}

	break;

	case PROBE_ALTERA_BYTEBLASTER:
		// Byteblaster :
		// AutoF : OE (AH)
		// D0  :   TCK (AH)
		// D1  :   TMS (AH)
		// D6  :   TDI (AH) (To device)
		// BUSY :  TDO (AL) (From device)
		// D4  :   Loopback test out (AH)
		// Ack  :  Loopback test in (AH)

		lpt_data = 0x00;

		if (tms == 1) lpt_data |= 0x02;
		if (tdi == 1) lpt_data |= 0x40;

		out_io_port(lpt_address, (unsigned char)lpt_data);	// TCK low

		lpt_data |= 0x01;
		out_io_port(lpt_address, (unsigned char)lpt_data);	// TCK high

		if (rp == READ_PORT)
		{
			tdo = (((int)in_io_port((unsigned short)(lpt_address + 1)) & 0x80) >> 7) ^ 1;	// get TDO data
		}

	break;

	case PROBE_MACRAIGOR_WIGGLER:
		// Macraigor Wiggler :
		// AutoF : OE (AH)
		// D0  :   RST (AH)
		// D1  :   TMS (AH)
		// D2  :   TCK (AH)
		// D3  :   TDI (AH) (To device)
		// D4  :   TRST (AH)
		// D7  :   "PC VCC"
		// BUSY :  TDO (AL) (From device)
		// D6  :   Loopback test out (AH)
		// Error  :  Loopback test in (AH)

		lpt_data = 0x10 | 0x80; //Set PC VCC & TRST to 1

		if (tms == 1) lpt_data |= 0x02;
		if (tdi == 1) lpt_data |= 0x08;

		out_io_port(lpt_address, (unsigned char)lpt_data);	// TCK low

		lpt_data |= 0x04;
		out_io_port(lpt_address, (unsigned char)lpt_data);	// TCK high

		if (rp == READ_PORT)
		{
			tdo = (((int)in_io_port((unsigned short)(lpt_address + 1)) & 0x80) >> 7) ^ 1;	// get TDO data
		}

	break;

	default:
		return 0;
		break;
	}

	return tdo;
}

int drv_LPT_Detect(jtag_core * jc)
{

#if defined(WIN32)
	h = CreateFile("\\\\.\\giveio", GENERIC_READ, 0, NULL,OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (h == INVALID_HANDLE_VALUE)
	{
		jtagcore_logs_printf(jc,MSG_ERROR,"drv_LPT_Init : Can't load giveio !\r\n");
		return 0;
	}

	jtagcore_logs_printf(jc,MSG_ERROR,"drv_LPT_Detect : giveio enabled !\r\n");
#endif

	lpt_address = test_port();	// find a valid parallel port address
	if (!lpt_address)
	{
		jtagcore_logs_printf(jc,MSG_ERROR,"drv_LPT_Init : No LPT port found !\r\n");
		return 0;
	}

	return 3;
}


int drv_LPT_Init(jtag_core * jc, int sub_drv,char * params)
{
	int probe_detected;

#if defined(WIN32)
	h = CreateFile("\\\\.\\giveio", GENERIC_READ, 0, NULL,OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (h == INVALID_HANDLE_VALUE)
	{
		jtagcore_logs_printf(jc,MSG_ERROR,"drv_LPT_Init : Can't load giveio !\r\n");
		return -1;
	}
#endif

	lpt_address = test_port();	// find a valid parallel port address
	if (!lpt_address)
	{
		jtagcore_logs_printf(jc,MSG_ERROR,"drv_LPT_Init : No LPT port found !\r\n");
		return -2;
	}

	sub_probe_id = subdrv_list[sub_drv].id;

	switch(sub_probe_id)
	{
		case PROBE_ALTERA_BYTEBLASTER:
			probe_detected = 1;

			/*
			// Loop back test
			out_io_port(lpt_address, 0x00);
			if( in_io_port(lpt_address + 1) & 0x40)
			{
				probe_detected = 0;
			}

			out_io_port(lpt_address, 0x10);
			if( !(in_io_port(lpt_address + 1) & 0x40))
			{
				probe_detected = 0;
			}
			*/

			// Enable outputs.
			if(probe_detected)
				out_io_port((unsigned short)(lpt_address + 0x02), 0x02);
		break;

		default:
			probe_detected = 1;
		break;
	}

	if(probe_detected)
	{
		jtagcore_logs_printf(jc,MSG_INFO_0,"drv_LPT_Init : Probe Driver loaded successfully...\r\n");
		return 0;
	}
	else
	{
		jtagcore_logs_printf(jc,MSG_ERROR,"drv_LPT_Init : No probe found !\r\n");
		return -1;
	}
}

int drv_LPT_DeInit(jtag_core * jc)
{
#if defined(WIN32)
	CloseHandle(h);
#endif
	return 0;
}

int drv_LPT_TDOTDI_xfer(jtag_core * jc, unsigned char * str_out, unsigned char * str_in, int size)
{
	int i;
	int tms, tdi;


	if (size)
	{
		if (str_out[0] & JTAG_STR_TMS)
		{
			tms = 1;
		}
		else
		{
			tms = 0;
		}
	}

	i = 0;
	if (!str_in)
	{
		while (i < size)
		{

			if (str_out[i] & JTAG_STR_DOUT)
			{
				tdi = 1;
			}
			else
			{
				tdi = 0;
			}

			putp(tdi, tms, IGNORE_PORT);

			i++;
		}
	}
	else
	{
		while (i < size)
		{

			if (str_out[i] & JTAG_STR_DOUT)
			{
				tdi = 1;
			}
			else
			{
				tdi = 0;
			}

			str_in[i] = putp(tdi, tms, READ_PORT);

			i++;
		}
	}
	return 0;
}

int drv_LPT_TMS_xfer(jtag_core * jc, unsigned char * str_out, int size)
{
	int i;
	int tms, tdi;

	tdi = 0;
	i = 0;

	while (i < size)
	{
		if (str_out[i] & JTAG_STR_TMS)
		{
			tms = 1;
		}
		else
		{
			tms = 0;
		}

		putp(tdi, tms, IGNORE_PORT);

		i++;
	}

	return 0;
}

int drv_LPT_libGetDrv(jtag_core * jc, int sub_drv, unsigned int infotype,void * returnvalue)
{

	drv_ptr drv_funcs =
	{
		(DRV_DETECT)         drv_LPT_Detect,
		(DRV_INIT)           drv_LPT_Init,
		(DRV_DEINIT)         drv_LPT_DeInit,
		(DRV_TXTMS)          drv_LPT_TMS_xfer,
		(DRV_TXRXDATA)       drv_LPT_TDOTDI_xfer,
		(DRV_GETMODULEINFOS) drv_LPT_libGetDrv
	};

	return GetDrvInfo(
			jc,
			infotype,
			returnvalue,
			subdrv_list[sub_drv].drv_id,
			subdrv_list[sub_drv].drv_desc,
			&drv_funcs
			);
}

#endif