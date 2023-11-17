/*
 * JTAG Boundary Scanner
 * Copyright (c) 2008 - 2023 Viveris Technologies
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
* @brief  Internal variables support.
* @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "env.h"

/*
|L|H|str varname\0|L|H|str vardata\0|L|H|str varname\0|L|H|str vardata\0|0|0|

(H*256)+L = str size (\0 included)
if L & H == 0 -> end of buffer
*/

#ifdef STATIC_ENV_BUFFER
static envvar_entry static_envvar;
#endif

static int stringcopy(char * dst, char * src, unsigned int maxsize)
{
	int s;

	if( !dst || (maxsize <= 0))
		return 0;

	if( !src )
	{
		if(maxsize)
		{
			*dst = '\0';
			return 1;
		}
		else
		{
			return 0;
		}
	}

	s = 0;
	while( *src && s < (maxsize - 1))
	{
		*dst++ = *src++;
		s++;
	}

	*dst = '\0';
	s++;

	return s;
}

static uint16_t getEnvStrSize(unsigned char * buf)
{
	uint16_t size;

	size = *buf++;
	size += (((uint16_t)*buf)<<8);

	return size;
}

static void setEnvStrSize(unsigned char * buf, uint16_t size)
{
	*buf++ = size & 0xFF;
	*buf = (size >> 8) & 0xFF;

	return;
}

static int getEnvEmptyOffset(envvar_entry * env)
{
	int off;
	unsigned short varname_size, vardata_size;

	off = 0;

	varname_size = getEnvStrSize(&env->buf[off]); // var name string size
	vardata_size = getEnvStrSize(&env->buf[off + 2 + varname_size]); // var data string size

	while( varname_size )
	{
		off += (2 + varname_size + 2 + vardata_size);

		varname_size = getEnvStrSize(&env->buf[off]); // var name string size
		vardata_size = getEnvStrSize(&env->buf[off + 2 + varname_size]); // var data string size
	}

	return off;
}

static int getEnvBufOff(envvar_entry * env, char * varname)
{
	unsigned short curstr_size;
	int str_index,i;

	i = 0;

	str_index = 0;
	curstr_size = getEnvStrSize(&env->buf[i]);
	i += 2;

	while( curstr_size )
	{
		if( !(str_index & 1) ) // variable name ?
		{
			if( !strcmp((char*)&env->buf[i],varname ) )
			{
				// this is the variable we are looking for.
				return (i - 2);
			}
			else
			{
				// not the right variable - skip this string.
				i += curstr_size;
			}
		}
		else
		{
			//variable data - skip this string.
			i += curstr_size;
		}

		if( i < env->bufsize - 2 )
		{
			curstr_size = getEnvStrSize(&env->buf[i]);
			i += 2;
		}
		else
		{
			curstr_size = 0;
		}

		str_index++;
	}

	return -1; // Not found.
}

static int pushStr(envvar_entry * env, int offset, char * str)
{
	int size;

	if(!str || offset < 0)
		return -1;

	size = strlen(str) + 1;
	if(size > 0xFFFF)
		return -1;

	if( ( offset + 2 + size ) <  env->bufsize )
	{
		setEnvStrSize(&env->buf[offset], size);
		offset += 2;
		stringcopy((char*)&env->buf[offset], str, size);
		offset += size;

		return offset;
	}

	return -1;
}

#ifndef STATIC_ENV_BUFFER
static envvar_entry * realloc_env_buffer(envvar_entry * env, unsigned int size)
{
	unsigned int tmp_bufsize;
	unsigned char * tmpbuf;

	if( !env )
	{
		env = malloc( sizeof(envvar_entry) );
		if(!env)
			return NULL;

		memset( env,0,sizeof(envvar_entry));
	}

	tmp_bufsize = size;

	if( tmp_bufsize & (ENV_PAGE_SIZE - 1) )
	{
		tmp_bufsize = (tmp_bufsize & (~(ENV_PAGE_SIZE - 1))) + ENV_PAGE_SIZE;
	}

	if( tmp_bufsize > ENV_MAX_TOTAL_BUFFER_SIZE )
	{
		return env;
	}

	if( env->bufsize != tmp_bufsize )
	{
		tmpbuf = realloc(env->buf, tmp_bufsize);
		if(tmpbuf)
		{
			if( env->bufsize < tmp_bufsize )
				memset(&tmpbuf[env->bufsize], 0, tmp_bufsize - env->bufsize );

			env->bufsize = tmp_bufsize;
			env->buf = tmpbuf;
		}
	}

	return env;
}
#endif

static int pushEnvEntry(envvar_entry * env, char * varname, char * vardata)
{
	unsigned int total_size;
	int varname_len,vardata_len;
	int offset;

	if(!varname || !vardata)
		return -1;

	varname_len = strlen(varname) + 1;
	vardata_len = strlen(vardata) + 1;

	if( varname_len > ENV_MAX_STRING_SIZE || vardata_len > ENV_MAX_STRING_SIZE)
	{
		return -2;
	}

	total_size = 2 + varname_len + 2 + vardata_len;

	offset = getEnvEmptyOffset(env);
	if( offset < 0 )
	{
		return -1;
	}

#ifndef STATIC_ENV_BUFFER
	realloc_env_buffer(env, offset + total_size + 4);
#endif

	if( (total_size + offset) <  env->bufsize )
	{
		offset = pushStr(env, offset, varname);
		offset = pushStr(env, offset, vardata);

		return offset;
	}

	return -1;
}

envvar_entry * setEnvVar( envvar_entry * env, char * varname, char * vardata )
{
	int i,off,ret;
	unsigned short varname_size, vardata_size;
	int varname_len, vardata_len;
	int oldentrysize;

	i = 0;

	varname_len = 0;
	vardata_len = 0;

	if( !varname )
		return env;

	varname_len = strlen(varname);

	if( vardata )
		vardata_len = strlen(vardata);

	if( varname_len > ENV_MAX_STRING_SIZE || vardata_len > ENV_MAX_STRING_SIZE)
	{
		return env;
	}

	if(!env)
	{
#ifdef STATIC_ENV_BUFFER
		memset(&static_envvar,0,ENV_PAGE_SIZE);
		static_envvar.bufsize = ENV_PAGE_SIZE;

		env = &static_envvar;
#else
		env = malloc( sizeof(envvar_entry) );
		if(!env)
			return NULL;

		memset( env,0,sizeof(envvar_entry));

		env->bufsize = ENV_PAGE_SIZE;
		env->buf = malloc(env->bufsize);
		if(!env->buf)
		{
			free(env);
			return NULL;
		}
		memset(env->buf,0,env->bufsize);
#endif
	}

	off = getEnvBufOff( env, varname );
	if( off >= 0 )
	{
		varname_size = getEnvStrSize(&env->buf[off]); // var name string size
		vardata_size = getEnvStrSize(&env->buf[off + 2 + varname_size]); // var data string size

		oldentrysize = 2 + varname_size + 2 + vardata_size;

		if(vardata)
		{
			vardata_len = strlen(vardata);
			if( vardata_len + 1 > 0xFFFF )
				return env;

			if( vardata_len + 1 > vardata_size )
			{
				// add new entry, and pack the strings
				ret = pushEnvEntry(env, varname, vardata);
				if( ret > 0 )
				{
					unsigned char byte;
					for(i=0;i<env->bufsize - (off + oldentrysize);i++)
					{
						byte = env->buf[off + i + oldentrysize];
						env->buf[off + i + oldentrysize] = '\0';
						env->buf[off + i] = byte;
					}
				}
			}
			else
			{
				vardata_size = getEnvStrSize(&env->buf[off + 2 + varname_size]); // var data string size
				if(vardata_size)
				{
					memset((char*)&env->buf[off + 2 + varname_size + 2], 0, vardata_size );
					stringcopy( (char*)&env->buf[off + 2 + varname_size + 2], vardata, vardata_size);
				}
			}
		}
		else
		{
			// unset variable
			unsigned char byte;
			for(i=0;i<env->bufsize - (off + oldentrysize);i++)
			{
				byte = env->buf[off + i + oldentrysize];
				env->buf[off + i + oldentrysize] = '\0';
				env->buf[off + i] = byte;
			}
		}
	}
	else
	{
		if(vardata)
		{
			// New variable
			ret = pushEnvEntry(env, varname, vardata);
		}
	}

	return env;
}

char * getEnvVar( envvar_entry * env, char * varname, char * vardata)
{
	int off;
	unsigned short varname_size, vardata_size;

	envvar_entry * tmp_envvars;

	tmp_envvars = (envvar_entry *)env;
	if(!tmp_envvars)
		return NULL;

	off = getEnvBufOff( env, varname );
	if( off >= 0 )
	{
		varname_size = getEnvStrSize(&env->buf[off]); // var name string size
		vardata_size = getEnvStrSize(&env->buf[off + 2 + varname_size]); // var data string size

		if( varname_size>0 && vardata_size>0)
		{
			if(vardata)
			{
				stringcopy(vardata, (char*)&env->buf[off + 2 + varname_size + 2], ENV_MAX_STRING_SIZE);
			}

			return (char*)&env->buf[off + 2 + varname_size + 2];
		}
	}

	return NULL;
}

env_var_value getEnvVarValue( envvar_entry * env, char * varname)
{
	env_var_value value;
	char * str_return;

	value = 0;

	if(!varname)
		return 0;

	str_return = getEnvVar( env, varname, NULL);

	if(str_return)
	{
		if( strlen(str_return) > 2 )
		{
			if( str_return[0]=='0' && ( str_return[1]=='x' || str_return[1]=='X'))
			{
				value = (env_var_value)STRTOVALUE(str_return, NULL, 0);
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

envvar_entry * setEnvVarValue( envvar_entry * env, char * varname, env_var_value value)
{
	char tmp_str[128];

	tmp_str[128 - 1] = 0;

	snprintf(tmp_str,sizeof(tmp_str) - 1, "%d",value);

	return setEnvVar( env, varname, tmp_str );
}

char * getEnvVarIndex( envvar_entry * env, int index, char * vardata)
{
	int str_index, off;
	unsigned short varname_size, vardata_size;

	off = 0;

	str_index = 0;

	varname_size = getEnvStrSize(&env->buf[off]); // var name string size
	vardata_size = getEnvStrSize(&env->buf[off + 2 + varname_size]); // var data string size

	while( varname_size )
	{
		if( str_index == index )
		{
			if( varname_size>0 && vardata_size>0)
			{
				if(vardata)
				{
					stringcopy(vardata,(char*)&env->buf[off + 2 + varname_size + 2], ENV_MAX_STRING_SIZE);
				}

				return (char*)&env->buf[off + 2 + varname_size + 2];
			}
		}

		off += (2 + varname_size + 2 + vardata_size);

		varname_size = getEnvStrSize(&env->buf[off]); // var name string size
		vardata_size = getEnvStrSize(&env->buf[off + 2 + varname_size]); // var data string size

		str_index++;
	}

	return NULL; // Not found.
}

envvar_entry * duplicate_env_vars(envvar_entry * src)
{
#ifndef STATIC_ENV_BUFFER
	envvar_entry * tmp_envvars;
#endif

	if(!src)
		return NULL;

	if(!src->buf)
		return NULL;

#ifndef STATIC_ENV_BUFFER
	tmp_envvars = malloc(sizeof(envvar_entry));
	if(tmp_envvars)
	{
		memset(tmp_envvars,0,sizeof(envvar_entry));
		tmp_envvars->buf = malloc(src->bufsize);
		if(!tmp_envvars->buf)
		{
			free(tmp_envvars);
			return NULL;
		}

		memcpy(tmp_envvars->buf,src->buf,src->bufsize);

		return tmp_envvars;
	}
#endif
	return NULL;
}

void free_env_vars(envvar_entry * src)
{
#ifndef STATIC_ENV_BUFFER
	if(!src)
		return;

	if(src->buf)
		free(src->buf);

	free(src);
#endif
	return;
}
