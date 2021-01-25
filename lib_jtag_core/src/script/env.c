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
* @file   env.c
* @brief  JTAG Boundary Scanner internal variables support.
* @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "../drivers/drv_loader.h"
#include "../jtag_core_internal.h"
#include "../jtag_core.h"
#include "../bsdl_parser/bsdl_loader.h"

#include "../os_interface/os_interface.h"

#define MAX_CFG_STRING_SIZE 1024

#include "env.h"

int jtagcore_setEnvVar( jtag_core * jc, char * varname, char * varvalue )
{
	int i;
	envvar_entry * tmp_envvars;

	i = 0;

	tmp_envvars = (envvar_entry *)jc->envvar;

	if(!tmp_envvars)
	{
		tmp_envvars = malloc(sizeof(envvar_entry) );
		if(!tmp_envvars)
			return -1;

		memset( tmp_envvars,0,sizeof(envvar_entry));

		jc->envvar = (void*)tmp_envvars;
	}

	// is the variable already there
	while(tmp_envvars[i].name)
	{
		if(!strcmp(tmp_envvars[i].name,varname) )
		{
			break;
		}
		i++;
	}

	if(tmp_envvars[i].name)
	{
		// the variable already exist - update it.
		if(tmp_envvars[i].varvalue)
		{
			free(tmp_envvars[i].varvalue);
			tmp_envvars[i].varvalue = NULL;
		}

		if(varvalue)
		{
			tmp_envvars[i].varvalue = malloc(strlen(varvalue)+1);

			if(!tmp_envvars[i].varvalue)
				return -1;

			memset(tmp_envvars[i].varvalue,0,strlen(varvalue)+1);
			if(varvalue)
				strcpy(tmp_envvars[i].varvalue,varvalue);
		}
	}
	else
	{
		// No variable found, alloc an new entry
		if(strlen(varname))
		{
			tmp_envvars[i].name = malloc(strlen(varname)+1);
			if(!tmp_envvars[i].name)
				return -1;

			memset(tmp_envvars[i].name,0,strlen(varname)+1);
			strcpy(tmp_envvars[i].name,varname);

			if(varvalue)
			{
				tmp_envvars[i].varvalue = malloc(strlen(varvalue)+1);

				if(!tmp_envvars[i].varvalue)
					return -1;

				memset(tmp_envvars[i].varvalue,0,strlen(varvalue)+1);
				if(varvalue)
					strcpy(tmp_envvars[i].varvalue,varvalue);
			}

			tmp_envvars = realloc(tmp_envvars,sizeof(envvar_entry) * (i + 1 + 1));
			memset(&tmp_envvars[i + 1],0,sizeof(envvar_entry));
		}
	}

	jc->envvar = (void*)tmp_envvars;

	return 1;
}

char * jtagcore_getEnvVar( jtag_core * jc, char * varname, char * varvalue)
{
	int i;
	envvar_entry * tmp_envvars;

	i = 0;

	tmp_envvars = (envvar_entry *)jc->envvar;
	if(!tmp_envvars)
		return NULL;

	// search the variable...
	while(tmp_envvars[i].name)
	{
		if(!strcmp(tmp_envvars[i].name,varname) )
		{
			break;
		}
		i++;
	}

	if(tmp_envvars[i].name)
	{
		if(varvalue)
			strcpy(varvalue,tmp_envvars[i].varvalue);

		return tmp_envvars[i].varvalue;
	}
	else
	{
		return NULL;
	}
}

int jtagcore_getEnvVarValue( jtag_core * jc, char * varname)
{
	int value;
	char * str_return;

	value = 0;

	if(!varname)
		return 0;

	str_return = jtagcore_getEnvVar( jc, varname, NULL);

	if(str_return)
	{
		if( strlen(str_return) > 2 )
		{
			if( str_return[0]=='0' && ( str_return[0]=='x' || str_return[0]=='X'))
			{
				value = (int)strtol(str_return, NULL, 0);
			}
			else
			{
				value = atoi(str_return);
			}
		}
		else
		{
			value = atoi(str_return);
		}
	}

	return value;
}

char * jtagcore_getEnvVarIndex( jtag_core * jc, int index, char * varvalue)
{
	int i;
	envvar_entry * tmp_envvars;

	i = 0;

	tmp_envvars = (envvar_entry *)jc->envvar;
	if(!tmp_envvars)
		return NULL;

	// search the variable...
	while(tmp_envvars[i].name && i < index)
	{
		i++;
	}

	if(tmp_envvars[i].name)
	{
		if(varvalue)
			strcpy(varvalue,tmp_envvars[i].varvalue);

		return tmp_envvars[i].name;
	}
	else
	{
		return NULL;
	}
}

envvar_entry * duplicate_env_vars(envvar_entry * src)
{
	int i,j;
	envvar_entry * tmp_envvars;

	if(!src)
		return NULL;

	i = 0;
	// count entry
	while(src[i].name)
	{
		i++;
	}

	tmp_envvars = malloc(sizeof(envvar_entry) * (i + 1));
	if(tmp_envvars)
	{
		memset(tmp_envvars,0,sizeof(envvar_entry) * (i + 1));
		for(j=0;j<i;j++)
		{
			if(src[j].name)
			{
				tmp_envvars[j].name = malloc(strlen(src[j].name) + 1);
				strcpy(tmp_envvars[j].name,src[j].name);
			}

			if(src[j].varvalue)
			{
				tmp_envvars[j].varvalue = malloc(strlen(src[j].varvalue) + 1);
				strcpy(tmp_envvars[j].varvalue,src[j].varvalue);
			}
		}
	}

	return tmp_envvars;
}

void free_env_vars(envvar_entry * src)
{
	int i,j;

	if(!src)
		return;

	i = 0;
	// count entry
	while(src[i].name)
	{
		i++;
	}

	for(j=0;j<i;j++)
	{
		if(src[j].name)
		{
			free(src[j].name);
		}

		if(src[j].varvalue)
		{
			free(src[j].varvalue);
		}
	}

	free(src);

	return;
}
