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
 * @file   dbg_logs.c
 * @brief  logs/debug output
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "./drivers/drv_loader.h"
#include "jtag_core_internal.h"
#include "jtag_core.h"

#include "dbg_logs.h"

int jtagcore_logs_printf(jtag_core * jc,int MSGTYPE,char * chaine, ...)
{
	char tmp_msg[1024+1];
	char tmp_msg2[1024];
	JTAGCORE_PRINT_FUNC print_callback;

	if( jc->logs_level > MSGTYPE )
	{
		if( jc->jtagcore_print_callback )
		{
			va_list marker;
			va_start( marker, chaine );

			print_callback = jc->jtagcore_print_callback;

			switch(MSGTYPE)
			{
				case MSG_INFO_0:
					strcpy(tmp_msg,"Info : ");
				break;
				case MSG_INFO_1:
					strcpy(tmp_msg,"Info : ");
				break;
				case MSG_WARNING:
					strcpy(tmp_msg,"Warning : ");
				break;
				case MSG_ERROR:
					strcpy(tmp_msg,"Error : ");
				break;
				case MSG_DEBUG:
					strcpy(tmp_msg,"Debug : ");
				break;
				default:
					strcpy(tmp_msg,"Unknown : ");
				break;
			}

			vsprintf(tmp_msg2,chaine,marker);
			strncat(tmp_msg,tmp_msg2,sizeof(tmp_msg) - ( strlen(tmp_msg) + 1 ) );

			print_callback(tmp_msg);

			va_end( marker );
		}
	}
    return 0;
}

int jtagcore_set_logs_callback(jtag_core * jc, JTAGCORE_PRINT_FUNC jtag_core_print)
{
	if(jc)
	{
		jc->jtagcore_print_callback = jtag_core_print;
		
		return JTAG_CORE_NO_ERROR;
	}
	return JTAG_CORE_BAD_PARAMETER;
}

int jtagcore_set_logs_level(jtag_core * jc,int level)
{
	if(jc)
	{
		jc->logs_level = level;

		return JTAG_CORE_NO_ERROR;
	}
	return JTAG_CORE_BAD_PARAMETER;
}
