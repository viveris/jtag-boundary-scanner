/*
 * JTAG Boundary Scanner
 * Copyright (c) 2008 - 2024 Viveris Technologies
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
* @file   env.h
* @brief  Internal variables support header file.
* @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
*/
#ifdef SCRIPT_64BITS_SUPPORT
#define env_var_value uint64_t
#define signed_env_var_value int64_t
#define STRTOVALUE strtoull
#else
#define env_var_value uint32_t
#define signed_env_var_value int32_t
#define STRTOVALUE strtoul
#endif

//#define STATIC_ENV_BUFFER 1
#define ENV_PAGE_SIZE (16*1024)
#define ENV_MAX_TOTAL_BUFFER_SIZE (1024 * 1024) // 1MB
#define ENV_MAX_STRING_SIZE 512

typedef struct envvar_entry_
{
#ifdef STATIC_ENV_BUFFER
	unsigned char buf[ENV_PAGE_SIZE];
#else
	unsigned char * buf;
#endif
	unsigned int  bufsize;
}envvar_entry;

envvar_entry * initEnv( envvar_entry * src, envvar_entry * dst );

int setEnvVarDat( envvar_entry * env, char * varname, char * vardata );
int setEnvVarValue( envvar_entry * env, char * varname, env_var_value value );

char * getEnvVarDat( envvar_entry * env, char * varname, char * vardata, int maxsize );
env_var_value getEnvVarValue( envvar_entry * env, char * varname );

char * getEnvVarDatIndex( envvar_entry * env, int index, char * vardata, int maxsize );

void deinitEnv( envvar_entry * env );
