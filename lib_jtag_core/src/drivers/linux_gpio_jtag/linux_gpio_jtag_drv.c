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
 * @file   linux_gpio_jtag_drv.c
 * @brief  jtag linux gpio probes driver
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#if defined(__linux__)
#include <unistd.h>
#endif

#include "../drv_loader.h"
#include "../../jtag_core_internal.h"
#include "../../jtag_core.h"

#include "../../bsdl_parser/bsdl_loader.h"

#include "../../dbg_logs.h"

char linux_gpio_base[512];

#define IGNORE_PORT 0
#define READ_PORT 1

typedef struct
{
	const char* name;
	int gpio_num;
	int dir;
	int state;
	int old_state;
	int negate_polarity;
	int handle;
}io_defs;

#define TMS_INDEX 0
#define TDO_INDEX 1
#define TDI_INDEX 2
#define TCK_INDEX 3

io_defs gpio_var_names[]=
{
	{(const char*)"PROBE_LINUXGPIO_TMS_PIN",-1,1,0,-1,0,-1},
	{(const char*)"PROBE_LINUXGPIO_TDO_PIN",-1,0,0,-1,0,-1},
	{(const char*)"PROBE_LINUXGPIO_TDI_PIN",-1,1,0,-1,0,-1},
	{(const char*)"PROBE_LINUXGPIO_TCK_PIN",-1,1,0,-1,0,-1},
	{(const char*)NULL,-1,-1,0,-1,-1}
};

#if defined(__linux__)

static int putp(int tdi, int tms, int rp)
{
	int tdo = -1;
	int ret;
	char tmp_str[512];
	char rd_value;

	tmp_str[0] = '0';
	tmp_str[1] = '\n';
	tmp_str[2] = 0;

	rd_value = 0;

	if( gpio_var_names[TMS_INDEX].old_state != (tms&1))
	{
		tmp_str[0] = '0' + ((tms&1) ^ (gpio_var_names[TMS_INDEX].negate_polarity&1));

		ret = write(gpio_var_names[TMS_INDEX].handle,tmp_str,2);
		if(ret < 2)
			goto wr_error;

		gpio_var_names[TMS_INDEX].old_state = (tms&1);
	}

	if( gpio_var_names[TDI_INDEX].old_state != (tdi&1))
	{
		tmp_str[0] = '0' + ((tdi&1) ^ (gpio_var_names[TDI_INDEX].negate_polarity&1));
		ret = write(gpio_var_names[TDI_INDEX].handle,tmp_str,2);
		if(ret < 2)
			goto wr_error;

		gpio_var_names[TDI_INDEX].old_state = (tdi&1);
	}

	tmp_str[0] = '0' + (gpio_var_names[TCK_INDEX].negate_polarity&1);
	ret = write(gpio_var_names[TCK_INDEX].handle,tmp_str,2);
	if(ret < 2)
		goto wr_error;

	gpio_var_names[TCK_INDEX].old_state = 0;

	tmp_str[0] = '0' + (1 ^ (gpio_var_names[TCK_INDEX].negate_polarity&1));
	ret = write(gpio_var_names[TCK_INDEX].handle,tmp_str,2);
	if(ret < 2)
		goto wr_error;

	gpio_var_names[TCK_INDEX].old_state = 1;

	tmp_str[0] = '0' + (gpio_var_names[TCK_INDEX].negate_polarity&1);
	ret = write(gpio_var_names[TCK_INDEX].handle,tmp_str,2);
	if(ret < 2)
		goto wr_error;

	gpio_var_names[TCK_INDEX].old_state = 0;

	if (rp == READ_PORT)
	{
		rd_value = 0;
		lseek(gpio_var_names[TDO_INDEX].handle, 0, SEEK_SET);
		ret = read(gpio_var_names[TDO_INDEX].handle,&rd_value,1);
		if(ret < 1)
			goto wr_error;

		if(rd_value >= '0' && rd_value <='1')
		{
			gpio_var_names[TDO_INDEX].old_state = gpio_var_names[TDO_INDEX].state;
			tdo = (rd_value - '0') ^ (gpio_var_names[TDO_INDEX].negate_polarity&1);
			gpio_var_names[TDO_INDEX].state = tdo;
		}
	}

	return tdo;

wr_error:
	return 0;

}
#endif

int drv_LinuxGPIO_Detect(jtag_core * jc)
{
#if defined(__linux__)
	char tmp_str[1024];
	FILE * f;

	if(	jtagcore_getEnvVarValue( jc, "PROBE_GPIO_LINUX_ENABLE") > 0)
	{
		jtagcore_getEnvVar( jc, "PROBE_GPIO_LINUX_BASE_FOLDER", (char*)&linux_gpio_base);

		strncpy(tmp_str,linux_gpio_base,sizeof(tmp_str)-1);
		strncat(tmp_str,"/export",sizeof(tmp_str)-1);

		f = fopen(tmp_str,"rb");
		if(f)
		{
			fclose(f);
			return 3;
		}
	}
#endif
	return 0;
}

#if defined(__linux__)

static int exportGPIO(char * path_base,int pin)
{
	char tmp_str[512];
 	FILE * f;

	strcpy(tmp_str,path_base);
	strcat(tmp_str,"/export");

	f = fopen(tmp_str,"w");
	if(!f)
	{
		return JTAG_CORE_ACCESS_ERROR;
	}

	fprintf(f,"%d\n", pin);
	fclose(f);

    return JTAG_CORE_NO_ERROR;
}

static int setdirGPIO(char * path_base,int pin,int dir)
{
	char tmp_str[1024];
	char tmp_str2[64];
	FILE * f;

	strncpy(tmp_str,path_base,sizeof(tmp_str)-1);
	snprintf(tmp_str2,sizeof(tmp_str2)-1,"/gpio%d/direction",pin);
	strncat(tmp_str,tmp_str2,sizeof(tmp_str)-1);

	f = fopen(tmp_str,"w");
	if(!f)
	{
		return JTAG_CORE_ACCESS_ERROR;
	}

	if(dir)
		fprintf(f,"out\n");
	else
		fprintf(f,"in\n");

	fclose(f);

    return JTAG_CORE_NO_ERROR;
}

#endif

int drv_LinuxGPIO_Init(jtag_core * jc, int sub_drv,char * params)
{
	int probe_detected;

	probe_detected = 0;

	if(	jtagcore_getEnvVarValue( jc, "PROBE_LINUXGPIO_ENABLE") <= 0)
	{
		return JTAG_CORE_NO_PROBE;
	}

#if defined(__linux__)
	int i;
	char tmp_str[1024];

	jtagcore_getEnvVar( jc, "PROBE_LINUXGPIO_BASE_FOLDER", (char*)&linux_gpio_base);

	i = 0;
	while(gpio_var_names[i].name)
	{
		gpio_var_names[i].gpio_num = jtagcore_getEnvVarValue( jc, (char*)gpio_var_names[i].name);

		strncpy(tmp_str,(char*)gpio_var_names[i].name,sizeof(tmp_str)-1);
		strncat(tmp_str,"_INVERT_POLARITY",sizeof(tmp_str)-1);

		gpio_var_names[i].negate_polarity = jtagcore_getEnvVarValue( jc, (char*)tmp_str);

		if( exportGPIO(linux_gpio_base, gpio_var_names[i].gpio_num) != JTAG_CORE_NO_ERROR )
			break;

		if( setdirGPIO(linux_gpio_base, gpio_var_names[i].gpio_num, gpio_var_names[i].dir) != JTAG_CORE_NO_ERROR )
			break;

		i++;
	}

	if(!gpio_var_names[i].name)
	{
		probe_detected = 1;

		for(i=0;i<4;i++)
		{
			snprintf(tmp_str,sizeof(tmp_str),"%s/gpio%d/value",linux_gpio_base,gpio_var_names[i].gpio_num);

			if(gpio_var_names[i].handle != -1)
				close(gpio_var_names[i].handle);

			if(gpio_var_names[i].dir)
				gpio_var_names[i].handle = open(tmp_str, O_WRONLY);
			else
				gpio_var_names[i].handle = open(tmp_str, O_RDONLY);

			if(gpio_var_names[i].handle < 0)
				break;
		}

		if( i == 4 )
		{
			// init gpio
			putp(0, 0, IGNORE_PORT);
			putp(0, 0, IGNORE_PORT);
		}
		else
		{
			probe_detected = 0;
		}
	}
#endif

	if(probe_detected)
	{
		jtagcore_logs_printf(jc,MSG_INFO_0,"drv_LinuxGPIO_Init : Probe Driver loaded successfully...\r\n");
		return 0;
	}
	else
	{
		jtagcore_logs_printf(jc,MSG_ERROR,"drv_LinuxGPIO_Init : No probe found !\r\n");
		return -1;
	}
}

int drv_LinuxGPIO_DeInit(jtag_core * jc)
{
	return 0;
}

int drv_LinuxGPIO_TDOTDI_xfer(jtag_core * jc, unsigned char * str_out, unsigned char * str_in, int size)
{
#if defined(__linux__)
	int i;

	i = 0;
	if (!str_in)
	{
		while (i < size)
		{

			if (str_out[i] & JTAG_STR_TMS)
			{
				gpio_var_names[TMS_INDEX].state = 1;
			}
			else
			{
				gpio_var_names[TMS_INDEX].state = 0;
			}

			if (str_out[i] & JTAG_STR_DOUT)
			{
				gpio_var_names[TDI_INDEX].state = 1;
			}
			else
			{
				gpio_var_names[TDI_INDEX].state = 0;
			}

			putp(gpio_var_names[TDI_INDEX].state, gpio_var_names[TMS_INDEX].state, IGNORE_PORT);

			i++;
		}
	}
	else
	{
		while (i < size)
		{
			if (str_out[i] & JTAG_STR_DOUT)
			{
				gpio_var_names[TDI_INDEX].state = 1;
			}
			else
			{
				gpio_var_names[TDI_INDEX].state = 0;
			}

			if (str_out[i] & JTAG_STR_TMS)
			{
				gpio_var_names[TMS_INDEX].state = 1;
			}
			else
			{
				gpio_var_names[TMS_INDEX].state = 0;
			}

			str_in[i] = putp(gpio_var_names[TDI_INDEX].state, gpio_var_names[TMS_INDEX].state, READ_PORT);

			i++;
		}

	}

	return 0;

#endif
	return -1;
}

int drv_LinuxGPIO_TMS_xfer(jtag_core * jc, unsigned char * str_out, int size)
{
#if defined(__linux__)
	int i;

	gpio_var_names[TDI_INDEX].state = 0;

	i = 0;
	while (i < size)
	{
		if (str_out[i] & JTAG_STR_TMS)
		{
			gpio_var_names[TMS_INDEX].state = 1;
		}
		else
		{
			gpio_var_names[TMS_INDEX].state = 0;
		}

		putp(gpio_var_names[TDI_INDEX].state, gpio_var_names[TMS_INDEX].state, IGNORE_PORT);

		i++;
	}

	return 0;

#endif

	return -1;

}

int drv_LinuxGPIO_libGetDrv(jtag_core * jc, int sub_drv, unsigned int infotype,void * returnvalue)
{

	drv_ptr drv_funcs =
	{
		(DRV_DETECT)         drv_LinuxGPIO_Detect,
		(DRV_INIT)           drv_LinuxGPIO_Init,
		(DRV_DEINIT)         drv_LinuxGPIO_DeInit,
		(DRV_TXTMS)          drv_LinuxGPIO_TMS_xfer,
		(DRV_TXRXDATA)       drv_LinuxGPIO_TDOTDI_xfer,
		(DRV_GETMODULEINFOS) drv_LinuxGPIO_libGetDrv
	};

	return GetDrvInfo(
			jc,
			infotype,
			returnvalue,
			"LINUX_GPIO",
			"GENERIC LINUX GPIO JTAG",
			&drv_funcs
			);
}
