/*
 * JTAG Core library
 * Copyright (c) 2008 - 2026 Viveris Technologies
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
 * @file   dummy_jtag_drv.c
 * @brief  dummy probe driver
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

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

#endif

#define IGNORE_PORT 0
#define READ_PORT 1

//#define DUMMY_JTAG_DEBUG 1

// Fake Altera EP2S15F484
#define FAKE_CHIP_ID 0x020910DD
#define FAKE_CHIP_IR_LEN 10
#define FAKE_CHIP_SCAN_LEN 1140

typedef struct _jtag_tap_controller_core
{
	int tap_state;

	uint8_t ir_reg_shift[512];
	uint8_t ir_reg[512];
	int     ir_reg_len;

	uint8_t id_reg[512];
	int id_reg_len;

	uint8_t bypass_reg[512];
	int bypass_reg_len;

	uint8_t scan_reg[512];
	int scan_reg_len;

}jtag_tap_controller_core;

jtag_tap_controller_core tap;

enum
{
	state_test_logic_reset=0,

	state_run_test_idle,

	// --------------------

	state_select_dr_scan,
	state_capture_dr,
	state_shift_dr,
	state_exit_1_dr,
	state_pause_dr,
	state_exit_2_dr,
	state_update_dr,

	// --------------------

	state_select_ir_scan,
	state_capture_ir,
	state_shift_ir,
	state_exit_1_ir,
	state_pause_ir,
	state_exit_2_ir,
	state_update_ir

	// --------------------
};

#ifdef DUMMY_JTAG_DEBUG

typedef struct _tap_state
{
	int id;
	char * name;
}tap_state;

tap_state tap_state_def[]=
{
	{state_test_logic_reset,"state_test_logic_reset"},
	{state_run_test_idle,"state_run_test_idle"},
	{state_select_dr_scan,"state_select_dr_scan"},
	{state_capture_dr,"state_capture_dr"},
	{state_shift_dr,"state_shift_dr"},
	{state_exit_1_dr,"state_exit_1_dr"},
	{state_pause_dr,"state_pause_dr"},
	{state_exit_2_dr,"state_exit_2_dr"},
	{state_update_dr,"state_update_dr"},
	{state_select_ir_scan,"state_select_ir_scan"},
	{state_capture_ir,"state_capture_ir"},
	{state_shift_ir,"state_shift_ir"},
	{state_exit_1_ir,"state_exit_1_ir"},
	{state_pause_ir,"state_pause_ir"},
	{state_exit_2_ir,"state_exit_2_ir"},
	{state_update_ir,"state_update_ir"},
	{-1,NULL}
};

char * get_state_name(int state)
{
	int i;

	i = 0;
	while( (tap_state_def[i].id != state ) && tap_state_def[i].name )
	{
		i++;
	}

	return tap_state_def[i].name;
}

#endif

static int jtag_tap_ctrl_state( jtag_tap_controller_core * tap_ctrl, int tms )
{
	switch(tap_ctrl->tap_state)
	{
		case state_test_logic_reset:
			if(tms)
				tap_ctrl->tap_state = state_test_logic_reset;
			else
				tap_ctrl->tap_state = state_run_test_idle;
		break;

		case state_run_test_idle:
			if(tms)
				tap_ctrl->tap_state = state_select_dr_scan;
			else
				tap_ctrl->tap_state = state_run_test_idle;
		break;

	// --------------------

		case state_select_dr_scan:
			if(tms)
				tap_ctrl->tap_state = state_select_ir_scan;
			else
				tap_ctrl->tap_state = state_capture_dr;
		break;

		case state_capture_dr:
			if(tms)
				tap_ctrl->tap_state = state_exit_1_dr;
			else
				tap_ctrl->tap_state = state_shift_dr;
		break;

		case state_shift_dr:
			if(tms)
				tap_ctrl->tap_state = state_exit_1_dr;
			else
				tap_ctrl->tap_state = state_shift_dr;
		break;

		case state_exit_1_dr:
			if(tms)
				tap_ctrl->tap_state = state_update_dr;
			else
				tap_ctrl->tap_state = state_pause_dr;
		break;

		case state_pause_dr:
			if(tms)
				tap_ctrl->tap_state = state_exit_2_dr;
			else
				tap_ctrl->tap_state = state_pause_dr;
		break;

		case state_exit_2_dr:
			if(tms)
				tap_ctrl->tap_state = state_update_dr;
			else
				tap_ctrl->tap_state = state_shift_dr;
		break;

		case state_update_dr:
			if(tms)
				tap_ctrl->tap_state = state_select_dr_scan;
			else
				tap_ctrl->tap_state = state_run_test_idle;
		break;

	// --------------------

		case state_select_ir_scan:
			if(tms)
				tap_ctrl->tap_state = state_test_logic_reset;
			else
				tap_ctrl->tap_state = state_capture_ir;
		break;

		case state_capture_ir:
			if(tms)
				tap_ctrl->tap_state = state_exit_1_ir;
			else
				tap_ctrl->tap_state = state_shift_ir;
		break;

		case state_shift_ir:
			if(tms)
				tap_ctrl->tap_state = state_exit_1_ir;
			else
				tap_ctrl->tap_state = state_shift_ir;
		break;

		case state_exit_1_ir:
			if(tms)
				tap_ctrl->tap_state = state_update_ir;
			else
				tap_ctrl->tap_state = state_pause_ir;
		break;

		case state_pause_ir:
			if(tms)
				tap_ctrl->tap_state = state_exit_2_ir;
			else
				tap_ctrl->tap_state = state_pause_ir;
		break;

		case state_exit_2_ir:
			if(tms)
				tap_ctrl->tap_state = state_update_ir;
			else
				tap_ctrl->tap_state = state_shift_ir;
		break;

		case state_update_ir:
			if(tms)
				tap_ctrl->tap_state = state_select_dr_scan;
			else
				tap_ctrl->tap_state = state_run_test_idle;
		break;
	}

	return tap_ctrl->tap_state;
}

static int shift_reg(uint8_t * reg, int len, int in_state)
{
	int i;

	for( i = 0; i < len - 1; i++)
	{
		if ( reg[((i+1) >> 3)] & (0x01<<((i+1)&7)) )
		{
			reg[((i) >> 3)] |= (0x01<<((i)&7));
		}
		else
		{
			reg[((i) >> 3)] &= ~(0x01<<((i)&7));
		}
	}

	i = (len-1);
	if( in_state )
		reg[(i >> 3)] |= (0x01<<(i&7));
	else
		reg[(i >> 3)] &= (~(0x01<<(i&7)));

	return reg[0] & 1;
}

static int get_bit(uint8_t * reg, int len, int idx)
{
	if( idx < len )
	{
		if ( reg[((idx) >> 3)] & (0x01<<((idx)&7)) )
			return 1;
		else
			return 0;
	}

	return 0;
}

static void set_bit(uint8_t * reg, int len, int idx, int state)
{
	if( idx < len )
	{
		if ( state )
			reg[((idx) >> 3)] |=  (0x01<<((idx)&7));
		else
			reg[((idx) >> 3)] &= ~(0x01<<((idx)&7));
	}
}

int jtag_core_update( jtag_core * jc, jtag_tap_controller_core * tap_ctrl, int tdi, int tms )
{
	int tdo_ir,tdo_id,tdo_bypass,tdo_scan;
	int i,tdo,t;

	tdo = 0;

	tdo_ir = get_bit((uint8_t*)&tap.ir_reg_shift, tap.ir_reg_len, 0);
	tdo_id = get_bit((uint8_t*)&tap.id_reg, tap.id_reg_len, 0);
	tdo_bypass = get_bit((uint8_t*)&tap.bypass_reg, tap.bypass_reg_len, 0);
	tdo_scan = get_bit((uint8_t*)&tap.scan_reg, tap.scan_reg_len, 0);

	switch( tap_ctrl->tap_state )
	{
		case state_test_logic_reset:
			tap.id_reg[0] = ((FAKE_CHIP_ID >> 0) & 0xFF);
			tap.id_reg[1] = ((FAKE_CHIP_ID >> 8) & 0xFF);
			tap.id_reg[2] = ((FAKE_CHIP_ID >> 16) & 0xFF);
			tap.id_reg[3] = ((FAKE_CHIP_ID >> 24) & 0xFF);

			memset(&tap.ir_reg, 0, 512);
		break;

		case state_capture_dr:
			tap.id_reg[0] = ((FAKE_CHIP_ID >> 0) & 0xFF);
			tap.id_reg[1] = ((FAKE_CHIP_ID >> 8) & 0xFF);
			tap.id_reg[2] = ((FAKE_CHIP_ID >> 16) & 0xFF);
			tap.id_reg[3] = ((FAKE_CHIP_ID >> 24) & 0xFF);
		break;

		case state_capture_ir:
			memset(&tap.ir_reg_shift, 0, 512);
			set_bit((uint8_t*)&tap.ir_reg_shift, tap.ir_reg_len, 1 , 1);
		break;

		case state_shift_ir:
			tdo = tdo_ir;
		break;

		case state_shift_dr:
			t = 0;
			i = 0;
			while( i < tap.ir_reg_len )
			{
				if( get_bit((uint8_t*)&tap.ir_reg, tap.ir_reg_len, i ) )
				{
					t++;
				}
				i++;
			}

			if( t == tap.ir_reg_len )
			{
				// All IR bits to 1 ? -> Bypass
				tdo = tdo_bypass;
			}
			else
			{
				// Sample or EXTEST ?
				if( (tap.ir_reg[0] == 0x0F && tap.ir_reg[1] == 0x00) || (tap.ir_reg[0] == 0x05 && tap.ir_reg[1] == 0x00) )
					tdo = tdo_scan;
				else
					tdo = tdo_id;
			}
		break;

		case state_update_ir:
			jtagcore_logs_printf(jc,MSG_DEBUG,"ir : ");
			i = tap.ir_reg_len;
			while(i)
			{
				jtagcore_logs_printf(jc,MSG_DEBUG,"%d",get_bit((uint8_t*)&tap.ir_reg_shift, tap.ir_reg_len, i - 1));
				i--;
			}
			jtagcore_logs_printf(jc,MSG_DEBUG,"\r\n");
			memcpy(&tap.ir_reg,&tap.ir_reg_shift, 512);
		break;
	}

	if( tap_ctrl->tap_state == state_shift_ir )
	{
		tdo_ir = shift_reg((uint8_t*)&tap.ir_reg_shift, tap.ir_reg_len, tdi);
	}

	if( tap_ctrl->tap_state == state_shift_dr  )
	{
		tdo_id = shift_reg((uint8_t*)&tap.id_reg, tap.id_reg_len, tdi);
		tdo_bypass = shift_reg((uint8_t*)&tap.bypass_reg, tap.bypass_reg_len, tdi);
		tdo_scan = shift_reg((uint8_t*)&tap.scan_reg, tap.scan_reg_len, tdi);
	}

	jtag_tap_ctrl_state( tap_ctrl, tms );

	return tdo;
}

static int putp(jtag_core * jc, int tdi, int tms, int rp)
{
	int tdo;

	tdo = jtag_core_update( jc, &tap, tdi, tms );

#ifdef DUMMY_JTAG_DEBUG
	jtagcore_logs_printf(jc,MSG_DEBUG,"state:%s\r\n", get_state_name(tap.tap_state));
#endif

	return tdo;
}

int drv_dummy_Detect(jtag_core * jc)
{
	return 1;
}

int drv_dummy_Init(jtag_core * jc, int sub_drv,char * params)
{
	int probe_detected;

	probe_detected = 1;

	memset(&tap,0,sizeof(jtag_tap_controller_core));

	tap.ir_reg_len = FAKE_CHIP_IR_LEN;
	tap.id_reg_len = 32;
	tap.scan_reg_len = FAKE_CHIP_SCAN_LEN;
	tap.bypass_reg_len = 1;

	if(probe_detected)
	{
		jtagcore_logs_printf(jc,MSG_INFO_0,"drv_dummy_Init : Probe Driver loaded successfully...\r\n");
		return 0;
	}
	else
	{
		jtagcore_logs_printf(jc,MSG_ERROR,"drv_dummy_Init : No probe found !\r\n");
		return -1;
	}
}

int drv_dummy_DeInit(jtag_core * jc)
{
	return 0;
}

int drv_dummy_TDOTDI_xfer(jtag_core * jc, unsigned char * str_out, unsigned char * str_in, int size)
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

			putp(jc, tdi, tms, IGNORE_PORT);

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

			str_in[i] = putp(jc, tdi, tms, READ_PORT);

			i++;
		}
	}
	return 0;
}

int drv_dummy_TMS_xfer(jtag_core * jc, unsigned char * str_out, int size)
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

		putp(jc, tdi, tms, IGNORE_PORT);

		i++;
	}

	return 0;
}

int drv_dummy_libGetDrv(jtag_core * jc, int sub_drv, unsigned int infotype,void * returnvalue)
{

	drv_ptr drv_funcs =
	{
		(DRV_DETECT)         drv_dummy_Detect,
		(DRV_INIT)           drv_dummy_Init,
		(DRV_DEINIT)         drv_dummy_DeInit,
		(DRV_TXTMS)          drv_dummy_TMS_xfer,
		(DRV_TXRXDATA)       drv_dummy_TDOTDI_xfer,
		(DRV_GETMODULEINFOS) drv_dummy_libGetDrv
	};

	return GetDrvInfo(
			jc,
			infotype,
			returnvalue,
			"DUMMY_JTAG",
			"DUMMY JTAG",
			&drv_funcs
			);
}
