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
 * @file   ftdi_jtag_drv.c
 * @brief  FTDI based probes driver
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

#include <stdio.h>
#include <string.h>
#if !defined(WIN32)
#include <sys/time.h>
#endif
#include "../drv_loader.h"
#include "../../jtag_core_internal.h"
#include "../../jtag_core.h"

#include "../../bsdl_parser/bsdl_loader.h"

#include "../../dbg_logs.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "ftdi/ftd2xx.h"

#ifdef __cplusplus
}
#endif

typedef struct _drv_desc
{
	char drv_id[128];
	char drv_desc[128];
	int id;
	int ftdi_index;
}drv_desc;

#define PROBE_OLIMEX_OCD 0

static drv_desc subdrv_list[]=
{
	{"USB_OLIMEX_ARM_OCD","USB Olimex ARM USB OCD",PROBE_OLIMEX_OCD,0},
	{"USB_OLIMEX_ARM_OCD","USB Olimex ARM USB OCD",PROBE_OLIMEX_OCD,0},
	{"USB_OLIMEX_ARM_OCD","USB Olimex ARM USB OCD",PROBE_OLIMEX_OCD,0},
	{"USB_OLIMEX_ARM_OCD","USB Olimex ARM USB OCD",PROBE_OLIMEX_OCD,0},
	{"USB_OLIMEX_ARM_OCD","USB Olimex ARM USB OCD",PROBE_OLIMEX_OCD,0},
	{"USB_OLIMEX_ARM_OCD","USB Olimex ARM USB OCD",PROBE_OLIMEX_OCD,0},
	{"USB_OLIMEX_ARM_OCD","USB Olimex ARM USB OCD",PROBE_OLIMEX_OCD,0},
	{"USB_OLIMEX_ARM_OCD","USB Olimex ARM USB OCD",PROBE_OLIMEX_OCD,0}
};


static HMODULE lib_handle = 0;

static FT_HANDLE ftdih = NULL;
static FT_DEVICE ftdi_device;
static unsigned char nTRST;
static unsigned char nTRSTnOE;
static unsigned char nSRST;
static unsigned char nSRSTnOE;


static unsigned char low_direction;
static unsigned char low_output;

static unsigned char high_output;
static unsigned char high_direction;

unsigned char ftdi_out_buf[64 * 1024];
unsigned char ftdi_in_buf[64 * 1024];

#if !defined(WIN32)

int Sleep( unsigned int timeout_ms )
{
        struct timeval tv;
        tv.tv_sec = timeout_ms/1000;
        tv.tv_usec = (timeout_ms%1000) * 1000;
        select(0, NULL, NULL, NULL, &tv);
        return 0;
}
#endif



#if !defined(FTDILIB)

typedef FT_STATUS(WINAPI * FT_OPEN)(DWORD deviceNumber, FT_HANDLE *pHandle);
typedef FT_STATUS(WINAPI * FT_OPENEX)(PVOID pArg1, DWORD Flags, FT_HANDLE *pHandle);
typedef FT_STATUS(WINAPI * FT_READ)(FT_HANDLE ftHandle, LPVOID lpBuffer, DWORD nBufferSize, LPDWORD lpBytesReturned);
typedef FT_STATUS(WINAPI * FT_WRITE)(FT_HANDLE ftHandle, LPVOID lpBuffer, DWORD nBufferSize, LPDWORD lpBytesWritten);
typedef FT_STATUS(WINAPI * FT_GETSTATUS)(FT_HANDLE ftHandle, DWORD *dwRxBytes, DWORD *dwTxBytes, DWORD *dwEventDWord);
typedef FT_STATUS(WINAPI * FT_PURGE)(FT_HANDLE ftHandle, ULONG Mask);
typedef FT_STATUS(WINAPI * FT_SETUSBPARAMETERS)(FT_HANDLE ftHandle, ULONG ulInTransferSize, ULONG ulOutTransferSize);
typedef FT_STATUS(WINAPI * FT_SETLATENCYTIMER)(FT_HANDLE ftHandle, UCHAR ucLatency);
typedef FT_STATUS(WINAPI * FT_SETEVENTNOTIFICATION)(FT_HANDLE ftHandle, DWORD dwEventMask, PVOID pvArg);
typedef FT_STATUS(WINAPI * FT_CLOSE)(FT_HANDLE ftHandle);
typedef FT_STATUS(WINAPI * FT_LISTDEVICES)(LPVOID pArg1, LPVOID pArg2, DWORD Flags);
typedef FT_STATUS(WINAPI * FT_SETBITMODE)(FT_HANDLE ftHandle, UCHAR ucMask, UCHAR ucEnable);
typedef FT_STATUS(WINAPI * FT_SETTIMEOUTS)(FT_HANDLE ftHandle, ULONG ReadTimeout, ULONG WriteTimeout);
typedef FT_STATUS(WINAPI * FT_GETQUEUESTATUS)(FT_HANDLE ftHandle, DWORD *dwRxBytes);
typedef FT_STATUS(WINAPI * FT_GETDEVICEINFO)(FT_HANDLE ftHandle, FT_DEVICE *lpftDevice, LPDWORD lpdwID, PCHAR SerialNumber, PCHAR Description, LPVOID Dummy);
typedef FT_STATUS(WINAPI * FT_RESETDEVICE)(FT_HANDLE ftHandle);
typedef FT_STATUS(WINAPI * FT_SETCHARS)(FT_HANDLE ftHandle, UCHAR EventChar, UCHAR EventCharEnabled, UCHAR ErrorChar, UCHAR ErrorCharEnabled);

FT_OPEN pFT_Open;
FT_OPENEX pFT_OpenEx;
FT_READ pFT_Read;
FT_WRITE pFT_Write;
FT_GETSTATUS pFT_GetStatus;
FT_PURGE pFT_Purge;
FT_SETUSBPARAMETERS pFT_SetUSBParameters;
FT_SETLATENCYTIMER pFT_SetLatencyTimer;
FT_SETEVENTNOTIFICATION pFT_SetEventNotification;
FT_CLOSE pFT_Close;
FT_LISTDEVICES pFT_ListDevices;
FT_SETBITMODE pFT_SetBitMode;
FT_SETTIMEOUTS pFT_SetTimeouts;
FT_GETQUEUESTATUS pFT_GetQueueStatus;
FT_GETDEVICEINFO pFT_GetDeviceInfo;
FT_RESETDEVICE pFT_ResetDevice;
FT_SETCHARS pFT_SetChars;


#else



#endif


static int ft2232_set_data_bits_low_byte(unsigned char value, unsigned char direction)
{
	FT_STATUS status;
	DWORD dw_bytes_written = 0;
	unsigned char buf[3];
	unsigned long bytes_written;

	buf[0] = 0x80;		// command "set data bits low byte"
	buf[1] = value;		// value
	buf[2] = direction;	// direction

	status = pFT_Write(ftdih, buf, sizeof(buf), &dw_bytes_written);
	if (status != FT_OK) {
		bytes_written = dw_bytes_written;
		return -1;
	}

	return 0;
}

static int ft2232_set_data_bits_high_byte(unsigned char value, unsigned char direction)
{
	FT_STATUS status;
	DWORD dw_bytes_written = 0;
	unsigned char buf[3];
	unsigned long bytes_written;

	buf[0] = 0x82;		// command "set data bits high byte"
	buf[1] = value;		// value
	buf[2] = direction;	// direction

	status = pFT_Write(ftdih, buf, sizeof(buf), &dw_bytes_written);
	if (status != FT_OK) {
		bytes_written = dw_bytes_written;
		return -1;
	}

	return 0;
}

int drv_FTDI_Detect(jtag_core * jc)
{
	int numDevs,i;
	FT_STATUS status;
	char SerialNumber[512];


	if(lib_handle == NULL)
		lib_handle = LoadLibrary("ftd2xx.dll");

	if (lib_handle)
	{
		pFT_ListDevices = (FT_LISTDEVICES)GetProcAddress(lib_handle, "FT_ListDevices");
		if (!pFT_ListDevices)
			return 0;

		status = pFT_ListDevices(&numDevs, NULL, FT_LIST_NUMBER_ONLY);
		if (status != FT_OK && !numDevs)
		{
			jtagcore_logs_printf(jc,MSG_ERROR,"pFT_ListDevices : Error %x !\r\n",status);
			return 0;
		}

		i = 0;
		while(i<numDevs)
		{
			status = pFT_ListDevices((PVOID)i, SerialNumber, FT_LIST_BY_INDEX | FT_OPEN_BY_DESCRIPTION);
			if (status != FT_OK)
			{
				jtagcore_logs_printf(jc,MSG_ERROR,"pFT_ListDevices : Error %x !\r\n",status);
				return 0;
			}

			strcpy(subdrv_list[i].drv_id,SerialNumber);
			strcpy(subdrv_list[i].drv_desc,SerialNumber);
			strcat(subdrv_list[i].drv_desc," ");

			status = pFT_ListDevices((PVOID)i, SerialNumber, FT_LIST_BY_INDEX | FT_OPEN_BY_SERIAL_NUMBER);
			if (status != FT_OK)
			{
				jtagcore_logs_printf(jc,MSG_ERROR,"pFT_ListDevices : Error %x !\r\n",status);
				return 0;
			}

			strcat(subdrv_list[i].drv_desc,SerialNumber);

			i++;
		}


		return numDevs;
	}

	return 0;
}

int drv_FTDI_Init(jtag_core * jc, int sub_drv, char * params)
{
	FT_STATUS status;
	DWORD deviceID;
	char SerialNumber[16];
	char Description[64];
	char tmp_str[64];
	DWORD openex_flags = 0;
	char *openex_string = NULL;
	int numDevs;
	int baseclock, divisor, tckfreq;
	DWORD devIndex;
	int nbRead,nbtosend;
	int i;

	#ifdef WIN32

	if(lib_handle == NULL)
		lib_handle = LoadLibrary("ftd2xx.dll");

	if (lib_handle)
	{
		pFT_Write = (FT_WRITE)GetProcAddress(lib_handle, "FT_Write");
		if (!pFT_Write)
			goto loadliberror;

		pFT_Read = (FT_READ)GetProcAddress(lib_handle, "FT_Read");
		if (!pFT_Read)
			goto loadliberror;

		pFT_GetStatus = (FT_GETSTATUS)GetProcAddress(lib_handle, "FT_GetStatus");
		if (!pFT_GetStatus)
			goto loadliberror;

		pFT_Open = (FT_OPEN)GetProcAddress(lib_handle, "FT_Open");
		if (!pFT_Open)
			goto loadliberror;

		pFT_Close = (FT_CLOSE)GetProcAddress(lib_handle, "FT_Close");
		if (!pFT_Close)
			goto loadliberror;

		pFT_Purge = (FT_PURGE)GetProcAddress(lib_handle, "FT_Purge");
		if (!pFT_Purge)
			goto loadliberror;

		pFT_SetUSBParameters = (FT_SETUSBPARAMETERS)GetProcAddress(lib_handle, "FT_SetUSBParameters");
		if (!pFT_SetUSBParameters)
			goto loadliberror;

		pFT_SetLatencyTimer = (FT_SETLATENCYTIMER)GetProcAddress(lib_handle, "FT_SetLatencyTimer");
		if (!pFT_SetLatencyTimer)
			goto loadliberror;

		pFT_SetEventNotification = (FT_SETEVENTNOTIFICATION)GetProcAddress(lib_handle, "FT_SetEventNotification");
		if (!pFT_SetEventNotification)
			goto loadliberror;

		pFT_OpenEx = (FT_OPENEX)GetProcAddress(lib_handle, "FT_OpenEx");
		if (!pFT_OpenEx)
			goto loadliberror;

		pFT_ListDevices = (FT_LISTDEVICES)GetProcAddress(lib_handle, "FT_ListDevices");
		if (!pFT_ListDevices)
			goto loadliberror;

		pFT_SetBitMode = (FT_SETBITMODE)GetProcAddress(lib_handle, "FT_SetBitMode");
		if (!pFT_SetBitMode)
			goto loadliberror;

		pFT_SetTimeouts = (FT_SETTIMEOUTS)GetProcAddress(lib_handle, "FT_SetTimeouts");
		if (!pFT_SetTimeouts)
			goto loadliberror;

		pFT_GetQueueStatus = (FT_GETQUEUESTATUS)GetProcAddress(lib_handle, "FT_GetQueueStatus");
		if (!pFT_GetQueueStatus)
			goto loadliberror;

		pFT_GetDeviceInfo = (FT_GETDEVICEINFO)GetProcAddress(lib_handle, "FT_GetDeviceInfo");
		if (!pFT_GetDeviceInfo)
			goto loadliberror;

		pFT_ResetDevice = (FT_RESETDEVICE)GetProcAddress(lib_handle, "FT_ResetDevice");
		if (!pFT_ResetDevice)
			goto loadliberror;

		pFT_SetChars = (FT_SETCHARS)GetProcAddress(lib_handle, "FT_SetChars");
		if (!pFT_SetChars)
			goto loadliberror;


	}
	else
	{
		jtagcore_logs_printf(jc,MSG_ERROR,"drv_FTDI_Init : Can't open ftd2xx.dll !\r\n");
		return -1;
	}
	#else
		// TODO : Linux lib loader.
		return -1;
	#endif


	status = pFT_ListDevices(&numDevs, NULL, FT_LIST_NUMBER_ONLY);
	if (status != FT_OK && !numDevs)
	{
		jtagcore_logs_printf(jc,MSG_ERROR,"pFT_ListDevices : Error %x !\r\n",status);
		goto loadliberror;
	}

	devIndex = sub_drv;
	status = pFT_ListDevices((PVOID)devIndex, SerialNumber, FT_LIST_BY_INDEX | FT_OPEN_BY_SERIAL_NUMBER);
	if (status != FT_OK)
	{
		jtagcore_logs_printf(jc,MSG_ERROR,"pFT_ListDevices : Error %x !\r\n",status);
		goto loadliberror;
	}

	status = pFT_OpenEx(SerialNumber, FT_OPEN_BY_SERIAL_NUMBER, &ftdih);
	if (status != FT_OK)
	{
		jtagcore_logs_printf(jc,MSG_ERROR,"pFT_OpenEx : Error %x !\r\n",status);
		goto loadliberror;
	}

	status = pFT_ResetDevice(ftdih);
	if (status != FT_OK) {
		jtagcore_logs_printf(jc,MSG_ERROR,"pFT_ResetDevice : Error %x !\r\n",status);
		goto loadliberror;
	}

	status = pFT_GetQueueStatus(ftdih, &nbRead);
	if (status != FT_OK) {
		jtagcore_logs_printf(jc,MSG_ERROR,"pFT_GetQueueStatus : Error %x !\r\n",status);
		goto loadliberror;
	}

	if ( nbRead > 0)
		pFT_Read(ftdih, &ftdi_in_buf, nbRead, &nbRead);

	//Set USB request transfer sizes to 64K
	status = pFT_SetUSBParameters(ftdih, 65536, 65535);
	if (status != FT_OK) {
		jtagcore_logs_printf(jc,MSG_ERROR,"pFT_SetUSBParameters : Error %x !\r\n",status);
		goto loadliberror;
	}

	//Disable event and error characters
	status = pFT_SetChars(ftdih, 0, 0, 0, 0);
	if (status != FT_OK) {
		jtagcore_logs_printf(jc,MSG_ERROR,"pFT_SetChars : Error %x !\r\n",status);
		goto loadliberror;
	}

	//Sets the read and write timeouts in milliseconds
	status = pFT_SetTimeouts(ftdih, 5000, 5000);
	if (status != FT_OK) {
		jtagcore_logs_printf(jc,MSG_ERROR,"pFT_SetTimeouts : Error %x !\r\n",status);
		goto loadliberror;
	}

	//Set the latency timer (default is 16mS)
	status = pFT_SetLatencyTimer(ftdih, 2);
	if (status != FT_OK) {
		jtagcore_logs_printf(jc,MSG_ERROR,"pFT_SetLatencyTimer : Error %x !\r\n",status);
		goto loadliberror;
	}

	//Reset controller
	status = pFT_SetBitMode(ftdih, 0x0, 0x00);
	if (status != FT_OK) {
		jtagcore_logs_printf(jc,MSG_ERROR,"pFT_SetBitMode : Error %x !\r\n",status);
		goto loadliberror;
	}

	//Enable MPSSE mode
	status = pFT_SetBitMode(ftdih, 0x0, 0x02);
	if (status != FT_OK) {
		jtagcore_logs_printf(jc,MSG_ERROR,"pFT_SetBitMode : Error %x !\r\n",status);
		goto loadliberror;
	}

	status = pFT_SetBitMode(ftdih, 0x0b, 2);
	if (status != FT_OK) {
		jtagcore_logs_printf(jc,MSG_ERROR,"pFT_SetBitMode : Error %x !\r\n",status);
		goto loadliberror;
	}

	status = pFT_GetDeviceInfo(ftdih, &ftdi_device, &deviceID,
		SerialNumber, Description, NULL);
	if (status != FT_OK) {
		jtagcore_logs_printf(jc,MSG_ERROR,"pFT_GetDeviceInfo : Error %x !\r\n",status);
		goto loadliberror;
	}

	/*
	Olimex ARM-USB-OCD-H JTAG signals

	VREF – voltage follower input for the output buffers adjust JTAG signals as per your target board voltage levels
	ADBUS0 -> TCK; (out)
	ADBUS1 -> TDI; (out)
	ADBUS2 -> TDO; (in)
	ADBUS3 -> TMS; (out)
	ADBUS4 -> 0 to enable JTAG buffers;  (GPIOL0) (out)
	ADBUS5 -> 0 if target present;       (GPIOL1) (in)
	ADBUS6 -> TSRST in;                  (GPIOL2) (in)
	ADBUS7 -> RTCK; (in)                 (GPIOL3) (in)

	ACBUS0 -> TRST;                      (GPIOH0)
	ACBUS1 -> SRST;                      (GPIOH1)
	ACBUS2 -> TRST buffer enable         (GPIOH2)
	ACBUS3 -> RED LED;                   (GPIOH3)
	*/

	low_direction = 0x00;
	for(i=0;i<8;i++)
	{
		sprintf(tmp_str,"PROBE_FTDI_SET_PIN_DIR_ADBUS%d",i);
		if( jtagcore_getEnvVarValue( jc, tmp_str) > 0 )
		{
			low_direction |= (0x01<<i);
		}
	}

	low_output = 0x00;
	for(i=0;i<8;i++)
	{
		sprintf(tmp_str,"PROBE_FTDI_SET_PIN_DEFAULT_STATE_ADBUS%d",i);
		if( jtagcore_getEnvVarValue( jc, tmp_str) > 0 )
		{
			low_output |= (0x01<<i);
		}
	}

	high_direction = 0x00;
	for(i=0;i<4;i++)
	{
		sprintf(tmp_str,"PROBE_FTDI_SET_PIN_DIR_ACBUS%d",i);
		if( jtagcore_getEnvVarValue( jc, tmp_str) > 0 )
		{
			high_direction |= (0x01<<i);
		}
	}

	ft2232_set_data_bits_low_byte(low_output, low_direction);

	nTRST = 0x01;
	nTRSTnOE = 0x4;
	nSRST = 0x02;
	nSRSTnOE = 0x00;// no output enable for nSRST

	high_output = 0x0;

	if ( 0 ) //jtag_reset_config & RESET_TRST_OPEN_DRAIN) {
	{
		high_output |= nTRSTnOE;
		high_output &= ~nTRST;
	}
	else {
		high_output &= ~nTRSTnOE;
		high_output |= nTRST;
	}

	// Clock divisor
	// 0x86 ValueL ValueH
	// FT2232D/H
	// TCK clock = (12Mhz or 60Mhz)/ ((1 + ([ValueH << 8 | ValueL]))*2)

	nbtosend = 0;
	baseclock = jtagcore_getEnvVarValue( jc, "PROBE_FTDI_INTERNAL_FREQ_KHZ");
	tckfreq =   jtagcore_getEnvVarValue( jc, "PROBE_FTDI_TCK_FREQ_KHZ");
	if( baseclock <= 0 || tckfreq <= 0){
		jtagcore_logs_printf(jc,MSG_ERROR,"drv_FTDI_Init : Invalid probe clock settings !\r\n");
		goto loadliberror;
	}

	divisor = ( ( baseclock / tckfreq ) - 2 ) / 2;

	ftdi_out_buf[nbtosend++] = 0x86;
	ftdi_out_buf[nbtosend++] = divisor & 0xFF;
	ftdi_out_buf[nbtosend++] = (divisor>>8) & 0xFF;

	status = pFT_Write(ftdih, ftdi_out_buf, nbtosend, &nbtosend);
	if (status != FT_OK) {
		jtagcore_logs_printf(jc,MSG_ERROR,"pFT_Write : Error %x !\r\n",status);
		goto loadliberror;
	}

	/* turn red LED on */
	high_output |= 0x08;

	/* Release system reset */
	high_output |= nSRST;

 	ft2232_set_data_bits_high_byte(high_output , high_direction);

	jtagcore_logs_printf(jc,MSG_INFO_0,"drv_FTDI_Init : Probe Driver loaded successfully...\r\n");

	return 0;

loadliberror:
	FreeLibrary(lib_handle);
	lib_handle = NULL;
	return -1;
}

int drv_FTDI_DeInit(jtag_core * jc)
{
	pFT_Close(ftdih);
	FreeLibrary(lib_handle);
	lib_handle = NULL;
	return 0;
}

int drv_FTDI_TDOTDI_xfer(jtag_core * jc, unsigned char * str_out, unsigned char * str_in, int size)
{
	int i,j,l,payloadsize;
	unsigned char cur_tms_state;
	int nbRead, nbtosend,rounded_size;
	FT_STATUS status;
	int read_ptr;

	read_ptr = 0;
	memset(ftdi_out_buf, 0, sizeof(ftdi_out_buf));
	memset(ftdi_in_buf, 0, sizeof(ftdi_in_buf));

	if (size)
	{
		i = 0;
		cur_tms_state = 0;

		nbtosend = 0;

		// Set the first TMS/DOUT
		if (str_out[i] & JTAG_STR_TMS)
		{

			ftdi_out_buf[nbtosend] = 0x4B; // cmd

			if (str_in)
				ftdi_out_buf[nbtosend] |= JTAG_STR_DINREQ;

			nbtosend++;

			ftdi_out_buf[nbtosend++] = 0x00; // 1 Bit

			ftdi_out_buf[nbtosend] = 0x00;

			if (str_out[i] & JTAG_STR_DOUT)
				ftdi_out_buf[nbtosend] |= 0x80;

			if (str_out[i] & JTAG_STR_TMS)
			{
				ftdi_out_buf[nbtosend] |= 0x3F;
				cur_tms_state = 1;
			}

			nbtosend++;

			status = pFT_Write(ftdih, ftdi_out_buf, nbtosend, &nbtosend);

			if (str_in)
			{
				status = pFT_GetQueueStatus(ftdih, &nbRead);
				while (nbRead < 1 && (status == FT_OK))
				{
					Sleep(3);
					status = pFT_GetQueueStatus(ftdih, &nbRead);
				}

				status = pFT_Read(ftdih, &ftdi_in_buf, nbRead, &nbRead);

				for (l = 0; l < 1; l++)
				{
					if (ftdi_in_buf[l >> 3] & (0x01 << (l & 7)))
					{
						str_in[read_ptr] = JTAG_STR_DOUT;
					}
					else
					{
						str_in[read_ptr] = 0x00;
					}
					read_ptr++;
				}
			}

			i++;
		}

		nbtosend = 0;
		ftdi_out_buf[nbtosend] = 0x19;

		if (str_in)
			ftdi_out_buf[nbtosend] |= JTAG_STR_DINREQ;

		nbtosend++;

		ftdi_out_buf[nbtosend++] = 0x00;
		ftdi_out_buf[nbtosend++] = 0x00;

		ftdi_out_buf[nbtosend] = 0x00;

		rounded_size = (size - i) & ~(0x7);

		j = 0;
		payloadsize = 0;
		while (payloadsize < rounded_size)
		{
			if (str_out[i] & JTAG_STR_DOUT)
			{
				ftdi_out_buf[nbtosend] |= (0x01 << (j & 0x7));
			}

			j++;

			if (!(j & 0x7))
			{
				nbtosend++;
				ftdi_out_buf[nbtosend] = 0x00;
			}

			payloadsize++;
			i++;

		}

		if (payloadsize)
		{
			ftdi_out_buf[1] = ( ( (payloadsize>>3)-1 ) & 0xff);
			ftdi_out_buf[2] = ( ( (payloadsize>>3)-1 ) >> 8);
			status = pFT_Write(ftdih, ftdi_out_buf, nbtosend, &nbtosend);

			if (str_in)
			{
				do
				{
					Sleep(3);
					status = pFT_GetQueueStatus(ftdih, &nbRead);
				} while (nbRead < (payloadsize >> 3) && (status == FT_OK ));

				status = pFT_Read(ftdih, &ftdi_in_buf, nbRead, &nbRead);

				for (l = 0; l < payloadsize; l++)
				{
					if (ftdi_in_buf[l >> 3] & (0x01 << (l & 7)))
					{
						str_in[read_ptr] = JTAG_STR_DOUT;
					}
					else
					{
						str_in[read_ptr] = 0x00;
					}
					read_ptr++;
				}

			}
		}


		// Send the remaining bits...
		nbtosend = 0;
		ftdi_out_buf[nbtosend] = 0x1B; //bit mode

		if (str_in)
			ftdi_out_buf[nbtosend] |= JTAG_STR_DINREQ;

		nbtosend++;

		ftdi_out_buf[nbtosend++] = 0x00;

		ftdi_out_buf[nbtosend] = 0x00;

		j = 0;
		payloadsize = 0;
		while (i < size) // Should left less than 8 bits.
		{
			if (str_out[i] & JTAG_STR_DOUT)
			{
				ftdi_out_buf[nbtosend] |= (0x01 << (j & 0x7));
			}

			j++;

			payloadsize++;
			i++;
		}

		nbtosend++;

		if (payloadsize)
		{
			ftdi_out_buf[1] = ((payloadsize - 1) & 0xf);
			status = pFT_Write(ftdih, ftdi_out_buf, nbtosend, &nbtosend);

			if (str_in)
			{
				do
				{
					Sleep(3);
					status = pFT_GetQueueStatus(ftdih, &nbRead);
				} while ((nbRead < ((payloadsize / 8) - 1)) && (status == FT_OK));

				status = pFT_Read(ftdih, &ftdi_in_buf, nbRead, &nbRead);

				for (l = 0; l < payloadsize; l++)
				{
					if (ftdi_in_buf[l >> 3] & (0x01 << (l & 7)))
					{
						str_in[read_ptr] = JTAG_STR_DOUT;
					}
					else
					{
						str_in[read_ptr] = 0x00;
					}
					read_ptr++;
				}
			}
		}
	}

	return 0;
}

int drv_FTDI_TMS_xfer(jtag_core * jc, unsigned char * str_out, int size)
{
	int i;
	int nbtosend;
	FT_STATUS status;
	unsigned char databyte;

	memset(ftdi_out_buf, 0, sizeof(ftdi_out_buf));
	memset(ftdi_in_buf, 0, sizeof(ftdi_in_buf));

	if (size)
	{
		i = 0;
		databyte = 0x00;
		while (size)
		{
			if (str_out[i] & JTAG_STR_TMS)
				databyte |= (0x01 << (i % 6));

			i++;
			size--;
			if (!(i % 6))
			{
				nbtosend = 0;
				ftdi_out_buf[nbtosend] = 0x4B; // cmd
				nbtosend++;

				ftdi_out_buf[nbtosend++] = 0x06 - 1; // 6 Bit

				if ((databyte&0x20) && size)
					ftdi_out_buf[nbtosend++] = databyte|0x40;
				else
					ftdi_out_buf[nbtosend++] = databyte;

				status = pFT_Write(ftdih, ftdi_out_buf, nbtosend, &nbtosend);

				databyte = 0x00;
			}
		}

		if ((i % 6))
		{
			nbtosend = 0;
			ftdi_out_buf[nbtosend] = 0x4B; // cmd

			nbtosend++;
			ftdi_out_buf[nbtosend++] = (i % 6) - 1; // 1 Bit

			ftdi_out_buf[nbtosend++] = databyte;

			status = pFT_Write(ftdih, ftdi_out_buf, nbtosend, &nbtosend);

			databyte = 0x00;
		}
		else
		{

		}

	}

	return 0;
}

int drv_FTDI_libGetDrv(jtag_core * jc,int sub_drv,unsigned int infotype,void * returnvalue)
{

	drv_ptr drv_funcs =
	{
		(DRV_DETECT)         drv_FTDI_Detect,
		(DRV_INIT)           drv_FTDI_Init,
		(DRV_DEINIT)         drv_FTDI_DeInit,
		(DRV_TXTMS)          drv_FTDI_TMS_xfer,
		(DRV_TXRXDATA)       drv_FTDI_TDOTDI_xfer,
		(DRV_GETMODULEINFOS) drv_FTDI_libGetDrv
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
