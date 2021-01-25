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
 * @file   jlink_jtag_drv.c
 * @brief  JLINK based probes driver
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

#include <stdio.h>
#include <string.h>
#include "../drv_loader.h"
#include "../../jtag_core_internal.h"
#include "../../jtag_core.h"

#include "../../bsdl_parser/bsdl_loader.h"

#include "../../dbg_logs.h"

#if defined(WIN32)
// Compiling on Windows
#include <windows.h>
#elif defined(__linux__)
#include <dlfcn.h>
#include <stdbool.h>
#else
#error "Unsupported OS (only available on Windows or Linux)"
#endif

typedef struct _drv_desc
{
	char *drv_id;
	char *drv_desc;
	int id;
}drv_desc;

#define PROBE_JLINK_ARM 0

const static drv_desc subdrv_list[]=
{
	{"JLINK_ARM","USB JLINK ARM",PROBE_JLINK_ARM}
};

unsigned char jlink_out_tms_buf[64 * 1024];
unsigned char jlink_out_tdi_buf[64 * 1024];
unsigned char jlink_in_buf[64 * 1024];

#if defined(WIN32)

#define MODULE_NAME		"JLinkARM.dll"

typedef const char* (WINAPIV * JL_OPENEX)(const char* pfLog, void*);
typedef int  (WINAPIV * JL_JTAG_STORERAW)(const unsigned char* pTDI, const unsigned char* pTMS, unsigned int NumBits);
typedef int  (WINAPIV * JL_JTAG_STOREGETRAW)(const unsigned char* pTDI, unsigned char* pTDO, const unsigned char* pTMS, unsigned int NumBits);
typedef void (WINAPIV * JL_JTAG_SYNCBITS)(void);
typedef void (WINAPIV * JL_SETSPEED)(unsigned int Speed);
typedef void (WINAPIV * JL_SETRESETDELAY)(int ms);
typedef void (WINAPIV * JL_RESETPULLSRESET)(unsigned char OnOff);
typedef void (WINAPIV * JL_RESET)(void);
typedef int  (WINAPIV * JL_HASERROR)(void);
typedef void (WINAPIV * JL_CLOSE)(void);

#else

#define MODULE_NAME		"./libjlinkarm.so"

#if defined(__x86_64__)
    #define __cdecl
#endif
#if defined(__i386__)
    #define __cdecl __attribut__((cdecl))
#endif



typedef void* HMODULE;

typedef const char* (__cdecl * JL_OPENEX)(const char* pfLog, void*);
typedef int  (__cdecl * JL_JTAG_STORERAW)(const unsigned char* pTDI, const unsigned char* pTMS, unsigned int NumBits);
typedef int  (__cdecl * JL_JTAG_STOREGETRAW)(const unsigned char* pTDI, unsigned char* pTDO, const unsigned char* pTMS, unsigned int NumBits);
typedef void (__cdecl * JL_JTAG_SYNCBITS)(void);
typedef void (__cdecl * JL_SETSPEED)(unsigned int Speed);
typedef void (__cdecl * JL_SETRESETDELAY)(int ms);
typedef void (__cdecl * JL_RESETPULLSRESET)(unsigned char OnOff);
typedef void (__cdecl * JL_RESET)(void);
typedef int  (__cdecl * JL_HASERROR)(void);
typedef void (__cdecl * JL_CLOSE)(void);

void* GetProcAddress(HMODULE handle, const char* name)
{
	if(!handle || !name)
		return NULL;

	dlerror();

	return dlsym(handle, name);
}

bool FreeLibrary(HMODULE handle)
{
	dlerror();
	if(0 != dlclose(handle)) {
		return false;
	}

	return true;
}

HMODULE LoadLibrary(const char* path)
{
	dlerror();
	return dlopen(path, RTLD_NOW);
}

#endif

static HMODULE lib_handle = 0;

JL_OPENEX           pJLINKARM_OpenEx;
JL_JTAG_STORERAW    pJLINKARM_JTAG_StoreRaw;
JL_JTAG_STOREGETRAW pJLINKARM_JTAG_StoreGetRaw;
JL_JTAG_SYNCBITS    pJLINKARM_JTAG_SyncBits;
JL_SETSPEED         pJLINKARM_SetSpeed;
JL_SETRESETDELAY    pJLINKARM_SetResetDelay;
JL_RESETPULLSRESET  pJLINKARM_ResetPullsRESET;
JL_RESET            pJLINKARM_Reset;
JL_HASERROR         pJLINKARM_HasError;
JL_CLOSE            pJLINKARM_Close;

int drv_JLINK_Detect(jtag_core * jc)
{
	if(lib_handle == NULL) {
		lib_handle = LoadLibrary(MODULE_NAME);
	}

	if (lib_handle)
	{
		return 1;
	}

	return 0;
}

int drv_JLINK_Init(jtag_core * jc, int sub_drv, char * params)
{
	const char* sError;


	if(lib_handle == NULL) {
		lib_handle = LoadLibrary(MODULE_NAME);
	}

	if (lib_handle)
	{
		pJLINKARM_OpenEx = (JL_OPENEX)GetProcAddress(lib_handle, "JLINKARM_OpenEx");
		if (!pJLINKARM_OpenEx)
			goto loadliberror;

		pJLINKARM_JTAG_StoreRaw = (JL_JTAG_STORERAW)GetProcAddress(lib_handle, "JLINKARM_JTAG_StoreRaw");
		if (!pJLINKARM_JTAG_StoreRaw)
			goto loadliberror;

		pJLINKARM_JTAG_StoreGetRaw = (JL_JTAG_STOREGETRAW)GetProcAddress(lib_handle, "JLINKARM_JTAG_StoreGetRaw");
		if (!pJLINKARM_JTAG_StoreGetRaw)
			goto loadliberror;

		pJLINKARM_JTAG_SyncBits = (JL_JTAG_SYNCBITS)GetProcAddress(lib_handle, "JLINKARM_JTAG_SyncBits");
		if (!pJLINKARM_JTAG_SyncBits)
			goto loadliberror;

		pJLINKARM_SetSpeed = (JL_SETSPEED)GetProcAddress(lib_handle, "JLINKARM_SetSpeed");
		if (!pJLINKARM_SetSpeed)
			goto loadliberror;

		pJLINKARM_SetResetDelay = (JL_SETRESETDELAY)GetProcAddress(lib_handle, "JLINKARM_SetResetDelay");
		if (!pJLINKARM_SetResetDelay)
			goto loadliberror;

		pJLINKARM_ResetPullsRESET = (JL_RESETPULLSRESET)GetProcAddress(lib_handle, "JLINKARM_ResetPullsRESET");
		if (!pJLINKARM_ResetPullsRESET)
			goto loadliberror;

		pJLINKARM_Reset = (JL_RESET)GetProcAddress(lib_handle, "JLINKARM_Reset");
		if (!pJLINKARM_Reset)
			goto loadliberror;

		pJLINKARM_HasError = (JL_HASERROR)GetProcAddress(lib_handle, "JLINKARM_HasError");
		if (!pJLINKARM_HasError)
			goto loadliberror;

		pJLINKARM_Close = (JL_CLOSE)GetProcAddress(lib_handle, "JLINKARM_Close");
		if (!pJLINKARM_Close)
			goto loadliberror;
	}
	else
	{
		jtagcore_logs_printf(jc,MSG_ERROR,"drv_JLINK_Init : Can't load JLinkARM.dll !\r\n");
		return -1;
	}

	sError = pJLINKARM_OpenEx(NULL, 0);
	if (sError) {
		jtagcore_logs_printf(jc,MSG_ERROR,"pJLINKARM_OpenEx : Error 0x%x !\r\n",sError);
		return -1;
	}

	pJLINKARM_SetSpeed(1000); // 1 Mhz

	jtagcore_logs_printf(jc,MSG_INFO_0,"drv_JLINK_Init : Probe Driver loaded successfully...\r\n");

	return 0;

loadliberror:
	FreeLibrary(lib_handle);
	lib_handle = NULL;
	return -1;
}

int drv_JLINK_DeInit(jtag_core * jc)
{
	pJLINKARM_Close();
	FreeLibrary(lib_handle);
	lib_handle = NULL;
	return 0;
}

int drv_JLINK_TDOTDI_xfer(jtag_core * jc, unsigned char * str_out, unsigned char * str_in, int size)
{
	int i;
	int bitoffset;

	memset(jlink_out_tms_buf, 0, sizeof(jlink_out_tms_buf));
	memset(jlink_out_tdi_buf, 0, sizeof(jlink_out_tdi_buf));

	if (!str_in)
	{
		if (size)
		{
			i = 0;
			bitoffset = 0;
			while (i < size)
			{
				if (str_out[i] & JTAG_STR_TMS)
					jlink_out_tms_buf[bitoffset >> 3] |= (0x01 << (bitoffset & 7));

				if (str_out[i] & JTAG_STR_DOUT)
					jlink_out_tdi_buf[bitoffset >> 3] |= (0x01 << (bitoffset & 7));

				bitoffset++;
				i++;
			}

			pJLINKARM_JTAG_StoreRaw(jlink_out_tdi_buf, jlink_out_tms_buf, bitoffset);
			pJLINKARM_JTAG_SyncBits();
		}
	}
	else
	{
		if (size)
		{
			memset(jlink_in_buf, 0, sizeof(jlink_in_buf));

			i = 0;
			bitoffset = 0;
			while (i < size)
			{
				if (str_out[i] & JTAG_STR_TMS)
					jlink_out_tms_buf[bitoffset >> 3] |= (0x01 << (bitoffset & 7));

				if (str_out[i] & JTAG_STR_DOUT)
					jlink_out_tdi_buf[bitoffset >> 3] |= (0x01 << (bitoffset & 7));

				bitoffset++;
				i++;
			}

			pJLINKARM_JTAG_StoreGetRaw(jlink_out_tdi_buf, jlink_in_buf, jlink_out_tms_buf, bitoffset);
			pJLINKARM_JTAG_SyncBits();

			i = 0;
			while (i < size)
			{
				if (jlink_in_buf[i >> 3] & (0x01 << (i & 7)))
				{
					str_in[i] = JTAG_STR_DOUT;
				}
				else
				{
					str_in[i] = 0x00;
				}
				i++;
			}

		}
	}

	return 0;
}

int drv_JLINK_TMS_xfer(jtag_core * jc, unsigned char * str_out, int size)
{
	int bitoffset,i;

	memset(jlink_out_tms_buf, 0, sizeof(jlink_out_tms_buf));
	memset(jlink_out_tdi_buf, 0, sizeof(jlink_out_tdi_buf));

	if (size)
	{
		i = 0;
		bitoffset = 0;
		while (i < size)
		{
			if (str_out[i] & JTAG_STR_TMS)
				jlink_out_tms_buf[bitoffset >> 3] |= (0x01 << (bitoffset & 7));

			bitoffset++;
			i++;
		}

		pJLINKARM_JTAG_StoreRaw(jlink_out_tdi_buf, jlink_out_tms_buf, bitoffset);
		pJLINKARM_JTAG_SyncBits();

	}

	return 0;
}

int drv_JLINK_libGetDrv(jtag_core * jc,int sub_drv,unsigned int infotype,void * returnvalue)
{
	drv_ptr drv_funcs =
	{
		(DRV_DETECT)         drv_JLINK_Detect,
		(DRV_INIT)           drv_JLINK_Init,
		(DRV_DEINIT)         drv_JLINK_DeInit,
		(DRV_TXTMS)          drv_JLINK_TMS_xfer,
		(DRV_TXRXDATA)       drv_JLINK_TDOTDI_xfer,
		(DRV_GETMODULEINFOS) drv_JLINK_libGetDrv
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

