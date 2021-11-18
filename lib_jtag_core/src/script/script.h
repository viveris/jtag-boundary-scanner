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
* @file   script.h
* @brief  JTAG Boundary Scanner scripts support header file.
* @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
*/

#define _script_ctx_

#ifndef _script_printf_func_
typedef int (* SCRIPT_PRINTF_FUNC)(void * ctx, int MSGTYPE, char * string, ... );
#define _script_printf_func_
#endif

#ifdef SCRIPT_64BITS_SUPPORT
#define env_var_value uint64_t
#define STRTOVALUE strtoull
#define LONGHEXSTR "%llX"
#else
#define env_var_value uint32_t
#define STRTOVALUE strtoul
#define LONGHEXSTR "%.8X"
#endif

#define MAX_LABEL_SIZE 64
#define MAX_LABEL 256

typedef struct _script_label
{
	char label_name[MAX_LABEL_SIZE];
	unsigned int offset;
} script_label;

typedef struct _script_ctx
{
	SCRIPT_PRINTF_FUNC script_printf;
	void * app_ctx;

	void * env;

	void * cmdlist;

	FILE * script_file;
	char script_file_path[1024];

	int cur_label_index;
	script_label labels[MAX_LABEL];

	int cur_script_offset;

	int dry_run;

	int last_error_code;
	env_var_value last_data_value;
	int last_flags;

	char pre_command[1024 + 32];

	uint32_t rand_seed;

} script_ctx;

script_ctx * init_script(void * app_ctx, unsigned int flags, void * env);
int  execute_file_script( script_ctx * ctx, char * filename );
int  execute_line_script( script_ctx * ctx, char * line );
int  execute_ram_script( script_ctx * ctx, unsigned char * script_buffer, int buffersize );
void setOutputFunc_script( script_ctx * ctx, SCRIPT_PRINTF_FUNC ext_printf );
script_ctx * deinit_script(script_ctx * ctx);
