/*
 * JTAG Core library
 * Copyright (c) 2008-2021 Viveris Technologies
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
* @file   script.c
* @brief  Script engine.
* @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdint.h>

#define MAX_PATH 256

#include "../drivers/drv_loader.h"
#include "../jtag_core_internal.h"
#include "script.h"
#include "../jtag_core.h"

#include "../bsdl_parser/bsdl_loader.h"
#include "../os_interface/os_interface.h"

#include "env.h"

#ifndef _script_cmd_func_
typedef int (* CMD_FUNC)( script_ctx * ctx, char * line);
#define _script_cmd_func_
#endif

typedef struct cmd_list_
{
	char * command;
	CMD_FUNC func;
}cmd_list;

typedef struct label_list_
{
	char * label;
	unsigned int file_offset;
}label_list;

extern cmd_list script_commands_list[];

static int dummy_script_printf(void * ctx, int MSGTYPE, char * string, ... )
{
	return 0;
}

static int is_end_line(char c)
{
	if( c == 0 || c == '#' || c == '\r' || c == '\n' )
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

static int is_space(char c)
{
	if( c == ' ' || c == '\t' )
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

static int is_label(char * command)
{
	int i;

	i = 0;
	while(command[i])
	{
		i++;
	}

	if(i>1)
	{
		if(command[i - 1] == ':')
			return 1;
	}

	return 0;
}

static int is_variable(char * command)
{
	if(strlen(command)>1)
	{
		if(command[0] == '$' && command[1] && ( command[1] != ' ' || command[1] != '\t' ))
			return 1;
		else
			return 0;
	}

	return 0;
}

static int get_next_word(char * line, int offset)
{
	while( !is_end_line(line[offset]) && ( line[offset] == ' ' || line[offset] == '\t' ) )
	{
		offset++;
	}

	return offset;
}

static int copy_param(char * dest, char * line, int offs)
{
	int i,insidequote;

	i = 0;
	insidequote = 0;
	while( !is_end_line(line[offs]) && ( insidequote || !is_space(line[offs]) ) && (i < (DEFAULT_BUFLEN - 1)) )
	{
		if(line[offs] != '"')
		{
			if(dest)
				dest[i] = line[offs];

			i++;
		}
		else
		{
			if(insidequote)
				insidequote = 0;
			else
				insidequote = 1;
		}

		offs++;
	}

	if(dest)
		dest[i] = 0;

	return offs;
}

static int get_param_offset(char * line, int param)
{
	int param_cnt, offs;

	offs = 0;
	offs = get_next_word(line, offs);

	if( param )
	{
		param_cnt = 0;
		do
		{
			offs = copy_param(NULL, line, offs);

			offs = get_next_word( line, offs );

			if(line[offs] == 0 || line[offs] == '#' || line[offs] == '\r' || line[offs] == '\n')
				return -1;

			param_cnt++;
		}while( param_cnt < param );
	}

	return offs;
}

static int get_param( script_ctx * ctx, char * line, int param_offset,char * param)
{
	int offs;
	char var_str[DEFAULT_BUFLEN];

	offs = get_param_offset(line, param_offset);

	if(offs>=0)
	{
		if(line[offs] != '$')
		{
			offs = copy_param(param, line, offs);
		}
		else
		{
			copy_param(var_str, line, offs);

			if(!strcmp(var_str,"$LASTDATA"))
			{
				sprintf(param,"0x"LONGHEXSTR,ctx->last_data_value);
				return 1;
			}

			if(!strcmp(var_str,"$LASTFLAGS"))
			{
				sprintf(param,"0x%X",ctx->last_flags);
				return 1;
			}

			if(!strcmp(var_str,"$LASTERROR"))
			{
				sprintf(param,"%d",ctx->last_error_code);
				return 1;
			}

			if( !getEnvVar( *((envvar_entry **)ctx->env), (char*)&var_str[1], param) )
			{
				copy_param(param, line, offs);
			}
		}

		return 1;
	}

	return -1;
}

static int get_param_str( script_ctx * ctx, char * line, int param_offset,char * param)
{
	int offs;

	offs = get_param_offset(line, param_offset);

	if(offs>=0)
	{
		offs = copy_param(param, line, offs);

		return 1;
	}

	return -1;
}

static env_var_value str_to_int(char * str)
{
	env_var_value value;

	value = 0;

	if(str)
	{
		if( strlen(str) > 2 )
		{
			if( str[0]=='0' && ( str[1]=='x' || str[1]=='X'))
			{
				value = (env_var_value)STRTOVALUE(str, NULL, 0);
			}
			else
			{
				value = atoi(str);
			}
		}
		else
		{
			value = atoi(str);
		}
	}

	return value;
}

static env_var_value get_script_variable( script_ctx * ctx, char * varname)
{
	env_var_value value;

	if(!strcmp(varname,"$LASTDATA"))
	{
		return ctx->last_data_value;
	}

	if(!strcmp(varname,"$LASTFLAGS"))
	{
		return ctx->last_flags;
	}

	if(!strcmp(varname,"$LASTERROR"))
	{
		return (env_var_value)(ctx->last_error_code);
	}

	if(varname[0] == '$')
		value = getEnvVarValue( *((envvar_entry **)ctx->env), (char*)&varname[1]);
	else
		value = str_to_int((char*)varname);

	return value;
}

static void set_script_variable( script_ctx * ctx, char * varname, env_var_value value)
{
	char tmp_str[64];

	if(!strcmp(varname,"$LASTDATA"))
	{
		ctx->last_data_value = value;

		return;
	}

	if(!strcmp(varname,"$LASTFLAGS"))
	{
		ctx->last_flags = value;

		return;
	}

	if(!strcmp(varname,"$LASTERROR"))
	{
		ctx->last_error_code = value;

		return;
	}

	if(varname[0] == '$' && varname[1])
	{
		sprintf(tmp_str,"0x"LONGHEXSTR,value);
		*((envvar_entry **)ctx->env) = (void *)setEnvVar( *((envvar_entry **)ctx->env), (char*)&varname[1], tmp_str );

		return;
	}
}

script_ctx * init_script(void * app_ctx, unsigned int flags, void * env)
{
	script_ctx * ctx;

	ctx = malloc(sizeof(script_ctx));

	if(ctx)
	{
		memset(ctx,0,sizeof(script_ctx));

		ctx->env = ((envvar_entry**)env);

		setOutputFunc_script( ctx, dummy_script_printf );

		ctx->app_ctx = (void*)app_ctx;
		ctx->cur_label_index = 0;

		ctx->cmdlist = (void*)script_commands_list;

		ctx->script_file = NULL;
	}

	return ctx;
}

static int extract_cmd( script_ctx * ctx, char * line, char * command)
{
	int offs,i;

	i = 0;
	offs = 0;

	offs = get_next_word(line, offs);

	if( !is_end_line(line[offs]) )
	{
		while( !is_end_line(line[offs]) && !is_space(line[offs]) && i < (DEFAULT_BUFLEN - 1) )
		{
			command[i] = line[offs];
			offs++;
			i++;
		}

		command[i] = 0;

		return i;
	}

	return 0;
}

static int exec_cmd( script_ctx * ctx, char * command,char * line)
{
	int i;
	cmd_list * cmdlist;

	cmdlist = (cmd_list*)ctx->cmdlist;

	i = 0;
	while(cmdlist[i].func)
	{
		if( !strcmp(cmdlist[i].command,command) )
		{
			return cmdlist[i].func(ctx,line);
		}

		i++;
	}

	return JTAG_CORE_CMD_NOT_FOUND;
}

static int add_label( script_ctx * ctx, char * label )
{
	int i,j;
	char tmp_label[MAX_LABEL_SIZE];

	if(ctx->cur_label_index < MAX_LABEL)
	{
		i = 0;
		while(i<(MAX_LABEL_SIZE - 1) && label[i] && label[i] != ':')
		{
			tmp_label[i] = label[i];
			i++;
		}
		tmp_label[i] = 0;

		i = 0;
		while(i<ctx->cur_label_index)
		{
			if( !strcmp( tmp_label, ctx->labels[i].label_name ) )
			{
				break;
			}
			i++;
		}

		j = i;

		i = 0;
		while(i<(MAX_LABEL_SIZE - 1) && label[i])
		{
			ctx->labels[j].label_name[i] = tmp_label[i];
			i++;
		}

		ctx->labels[j].label_name[i] = 0;
		ctx->labels[j].offset = ctx->cur_script_offset;

		if(ctx->cur_label_index == j)
		{
			ctx->cur_label_index++;
		}
	}
	return 0;
}

static int goto_label( script_ctx * ctx, char * label )
{
	int i;
	char tmp_label[MAX_LABEL_SIZE];

	i = 0;
	while(i<(MAX_LABEL_SIZE - 1) && label[i] && label[i] != ':')
	{
		tmp_label[i] = label[i];
		i++;
	}
	tmp_label[i] = 0;

	i = 0;
	while(i<ctx->cur_label_index)
	{
		if( !strcmp( tmp_label, ctx->labels[i].label_name ) )
		{
			break;
		}
		i++;
	}

	if( i != ctx->cur_label_index)
	{
		ctx->cur_script_offset = ctx->labels[i].offset;

		if(ctx->script_file)
			fseek(ctx->script_file,ctx->cur_script_offset,SEEK_SET);

		return JTAG_CORE_NO_ERROR;
	}
	else
	{
		ctx->script_printf( ctx, MSG_ERROR, "Label %s not found\n", tmp_label );

		return JTAG_CORE_NOT_FOUND;
	}
}

///////////////////////////////////////////////////////////////////////////////

static int alu_operations( script_ctx * ctx, char * line)
{
	int i;
	int valid;
	env_var_value data_value;
	env_var_value value_1,value_2;

	char params_str[5][DEFAULT_BUFLEN];

	for(i=0;i<5;i++)
	{
		params_str[i][0] = 0;
	}

	valid = 0;
	for(i=0;i<5;i++)
	{
		get_param_str( ctx, line, i, (char*)&params_str[i] );
		if(strlen((char*)&params_str[i]))
			valid++;
	}

	data_value = 0;
	if( ( (valid == 3) || (valid == 5) ) && params_str[1][0] == '=' && params_str[0][0] == '$')
	{
		value_1 = get_script_variable( ctx, params_str[2]);

		if(valid == 5)
		{
			value_2 = get_script_variable( ctx, params_str[4]);

			if(!strcmp(params_str[3],"+"))
				data_value = value_1 + value_2;

			if(!strcmp(params_str[3],"-"))
				data_value = value_1 - value_2;

			if(!strcmp(params_str[3],"*"))
				data_value = value_1 * value_2;

			if(!strcmp(params_str[3],"/") && value_2)
				data_value = value_1 / value_2;

			if(!strcmp(params_str[3],"&"))
				data_value = value_1 & value_2;

			if(!strcmp(params_str[3],"^"))
				data_value = value_1 ^ value_2;

			if(!strcmp(params_str[3],"|"))
				data_value = value_1 | value_2;

			if(!strcmp(params_str[3],">>"))
				data_value = value_1 >> value_2;

			if(!strcmp(params_str[3],"<<"))
				data_value = value_1 << value_2;
		}
		else
		{
			data_value = value_1;
		}

		if(data_value)
			ctx->last_flags = 1;
		else
			ctx->last_flags = 0;

		set_script_variable( ctx, (char*)&params_str[0], data_value);

		return JTAG_CORE_NO_ERROR;
	}

	return JTAG_CORE_BAD_PARAMETER;
}

void setOutputFunc_script( script_ctx * ctx, SCRIPT_PRINTF_FUNC ext_printf )
{
	ctx->script_printf = ext_printf;

	return;
}

int execute_line_script( script_ctx * ctx, char * line )
{
	char command[DEFAULT_BUFLEN];

	command[0] = 0;

	if( extract_cmd(ctx, line, command) )
	{
		if(strlen(command))
		{
			if(!is_label(command))
			{
				if(!ctx->dry_run)
				{
					if(!is_variable(command))
					{
						ctx->last_error_code = exec_cmd(ctx,command,line);

						if( ctx->last_error_code == JTAG_CORE_CMD_NOT_FOUND )
						{
							ctx->script_printf( ctx, MSG_ERROR, "Command not found ! : %s\n", line );

							return ctx->last_error_code;
						}
					}
					else
					{
						ctx->last_error_code = alu_operations(ctx,line);
					}
				}
				else
					ctx->last_error_code = JTAG_CORE_NO_ERROR;
			}
			else
			{
				add_label(ctx,command);

				ctx->last_error_code = JTAG_CORE_NO_ERROR;
			}

			return ctx->last_error_code;
		}
	}

	ctx->last_error_code = JTAG_CORE_BAD_CMD;

	return ctx->last_error_code;
}

int execute_file_script( script_ctx * ctx, char * filename )
{
	int err;
	char line[DEFAULT_BUFLEN];

	err = JTAG_CORE_INTERNAL_ERROR;

	ctx->script_file = fopen(filename,"r");
	if(ctx->script_file)
	{
		strncpy(ctx->script_file_path,filename,DEFAULT_BUFLEN);
		ctx->script_file_path[DEFAULT_BUFLEN-1] = 0;

		// Dry run -> populate the labels...
		ctx->dry_run++;
		do
		{
			if(!fgets(line,sizeof(line),ctx->script_file))
				break;

			ctx->cur_script_offset = ftell(ctx->script_file);

			if(feof(ctx->script_file))
				break;

			execute_line_script(ctx, line);
		}while(1);

		fseek(ctx->script_file,0,SEEK_SET);
		ctx->cur_script_offset = ftell(ctx->script_file);

		ctx->dry_run--;
		if(!ctx->dry_run)
		{
			if(strlen(ctx->pre_command))
			{
				err = execute_line_script(ctx, ctx->pre_command);
				if(err != JTAG_CORE_NO_ERROR)
				{
					fclose(ctx->script_file);
					return err;
				}
			}

			do
			{
				if(!fgets(line,sizeof(line),ctx->script_file))
					break;

				ctx->cur_script_offset = ftell(ctx->script_file);

				if(feof(ctx->script_file))
					break;

				err = execute_line_script(ctx, line);
			}while(1);
		}

		fclose(ctx->script_file);

		err = JTAG_CORE_NO_ERROR;
	}
	else
	{
		ctx->script_printf( ctx, MSG_ERROR, "Can't open %s !", filename );
		ctx->script_file_path[0] = 0;

		err = JTAG_CORE_ACCESS_ERROR;
	}

	return err;
}

int execute_ram_script( script_ctx * ctx, unsigned char * script_buffer, int buffersize )
{
	int err = 0;
	int buffer_offset,line_offset;
	char line[DEFAULT_BUFLEN];
	int cont;

	ctx->dry_run++;
	cont = 1;

	while( cont )
	{
		buffer_offset = 0;
		line_offset = 0;
		ctx->cur_script_offset = 0;

		do
		{
			memset(line,0,DEFAULT_BUFLEN);
			line_offset = 0;
			while( (buffer_offset < buffersize) && script_buffer[buffer_offset] && script_buffer[buffer_offset]!='\n' && script_buffer[buffer_offset]!='\r' && (line_offset < DEFAULT_BUFLEN - 1))
			{
				line[line_offset++] = script_buffer[buffer_offset++];
			}

			while( (buffer_offset < buffersize) && script_buffer[buffer_offset] && (script_buffer[buffer_offset]=='\n' || script_buffer[buffer_offset]=='\r') )
			{
				buffer_offset++;
			}

			ctx->cur_script_offset = buffer_offset;

			execute_line_script(ctx, line);

			buffer_offset = ctx->cur_script_offset;

			if( (buffer_offset >= buffersize) || !script_buffer[buffer_offset])
				break;

		}while(buffer_offset < buffersize);

		if( !ctx->dry_run || (ctx->dry_run > 1) )
		{
			cont = 0;
		}
		else
		{
			ctx->dry_run = 0;

			if(strlen(ctx->pre_command))
				execute_line_script(ctx, ctx->pre_command);
		}
	}

	return err;
}

script_ctx * deinit_script(script_ctx * ctx)
{
	if(ctx)
	{
		free(ctx);
	}

	ctx = NULL;

	return ctx;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////// Generic commands/operations /////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static int cmd_goto( script_ctx * ctx, char * line)
{
	int i;
	char label_str[DEFAULT_BUFLEN];

	i = get_param( ctx, line, 1, label_str );

	if(i>=0)
	{
		return goto_label( ctx, label_str );
	}

	return JTAG_CORE_BAD_PARAMETER;
}

static int cmd_if( script_ctx * ctx, char * line)
{
	//"if" command example :
	// if $VARIABLE > 0x2222 then goto label

	int i;
	int eval;
	int ret;
	int valid;
	char params_str[5][DEFAULT_BUFLEN];
	env_var_value value_1,value_2,tmp_val;
	int op_offset;

	ret = JTAG_CORE_BAD_PARAMETER;

	eval = 0;

	for(i=0;i<5;i++)
	{
		params_str[i][0] = 0;
	}

	valid = 0;
	for(i=0;i<5;i++)
	{
		get_param( ctx, line, i, (char*)&params_str[i] );
		if(strlen((char*)&params_str[i]))
			valid++;
	}

	i = 0;
	while( i < 5 && strcmp((char*)&params_str[i],"then") )
	{
		i++;
	}

	if( i < 5 )
	{
		if( i == 2)
		{
			value_1 = get_script_variable( ctx, params_str[1]);

			if(value_1)
				eval = 1;

			ret = JTAG_CORE_NO_ERROR;
		}

		if ( i == 4 )
		{
			value_1 = get_script_variable( ctx, params_str[1]);
			value_2 = get_script_variable( ctx, params_str[3]);

			if(!strcmp((char*)&params_str[2],">=") && ( (signed_env_var_value)value_1 >= (signed_env_var_value)value_2 ) )
				eval = 1;

			if(!strcmp((char*)&params_str[2],"<=") && ( (signed_env_var_value)value_1 <= (signed_env_var_value)value_2 ) )
				eval = 1;

			if(!strcmp((char*)&params_str[2],">") && ( (signed_env_var_value)value_1 > (signed_env_var_value)value_2 ) )
				eval = 1;

			if(!strcmp((char*)&params_str[2],"<") && ( (signed_env_var_value)value_1 < (signed_env_var_value)value_2 ) )
				eval = 1;

			if(!strcmp((char*)&params_str[2],"==") && ( value_1 == value_2 ) )
				eval = 1;

			if(!strcmp((char*)&params_str[2],"!=") && ( value_1 != value_2 ) )
				eval = 1;

			if(!strcmp((char*)&params_str[2],"&") && ( value_1 & value_2 ) )
				eval = 1;

			if(!strcmp((char*)&params_str[2],"^") && ( value_1 ^ value_2 ) )
				eval = 1;

			if(!strcmp((char*)&params_str[2],"|") && ( value_1 | value_2 ) )
				eval = 1;

			tmp_val = value_1 >> value_2;
			if(!strcmp((char*)&params_str[2],">>") && tmp_val )
				eval = 1;

			tmp_val = value_1 << value_2;
			if(!strcmp((char*)&params_str[2],"<<") && tmp_val )
				eval = 1;

			ret = JTAG_CORE_NO_ERROR;
		}

		if( eval )
		{
			op_offset = get_param_offset(line, i + 1);

			if(op_offset >= 0)
			{
				ret = execute_line_script( ctx, (char*)&line[op_offset] );
			}
		}

		return ret;
	}

	return JTAG_CORE_BAD_PARAMETER;
}

static int cmd_return( script_ctx * ctx, char * line )
{
	if(ctx->script_file)
	{
		fseek(ctx->script_file,0,SEEK_END);
	}

	return JTAG_CORE_NO_ERROR;
}

static int cmd_system( script_ctx * ctx, char * line )
{
	int offs;
	int ret;

	offs = get_param_offset(line, 1);

	if(offs>=0)
	{
		ret = system(&line[offs]);

		if( ret != 1 )
			return JTAG_CORE_NO_ERROR;
		else
			return JTAG_CORE_NOT_FOUND;
	}

	return JTAG_CORE_BAD_PARAMETER;
}

static int cmd_print_env_var( script_ctx * ctx, char * line )
{
	int i;
	char varname[DEFAULT_BUFLEN];
	char varvalue[DEFAULT_BUFLEN];
	char * ptr;

	i = get_param( ctx, line, 1, varname );

	if(i>=0)
	{
		ptr = getEnvVar( *((envvar_entry **)ctx->env), (char*)&varname, (char*)&varvalue );
		if(ptr)
		{
			ctx->script_printf( ctx, MSG_INFO_1, "%s = %s", varname, varvalue );

			return JTAG_CORE_NO_ERROR;
		}

		return JTAG_CORE_NOT_FOUND;
	}
	else
	{
		return JTAG_CORE_BAD_PARAMETER;
	}
}

static int cmd_version( script_ctx * ctx, char * line)
{
	ctx->script_printf( ctx, MSG_INFO_0, "Lib version : %s, Date : "__DATE__" "__TIME__"\n", LIB_JTAG_CORE_VERSION );

	return JTAG_CORE_NO_ERROR;
}

static int cmd_print( script_ctx * ctx, char * line)
{
	int i,j,s;
	char tmp_str[DEFAULT_BUFLEN];
	char str[DEFAULT_BUFLEN*2];
	char * ptr;

	str[0] = '\0';

	j = 1;
	do
	{
		ptr = NULL;

		i = get_param_offset(line, j);
		s = 0;

		if(i>=0)
		{
			tmp_str[0] = '\0';
			get_param( ctx, line, j, (char *)&tmp_str );
			s = strlen(tmp_str);
			if(s)
			{
				if(tmp_str[0] != '$')
				{
					genos_strndstcat((char*)str,tmp_str,sizeof(str));
					genos_strndstcat((char*)str," ",sizeof(str));
					str[sizeof(str) - 1] = '\0'; 
				}
				else
				{
					ptr = getEnvVar( *((envvar_entry **)ctx->env), &tmp_str[1], NULL);
					if( ptr )
					{
						genos_strndstcat((char*)str,ptr,sizeof(str));
						genos_strndstcat((char*)str," ",sizeof(str)); 
					}
					else
					{
						genos_strndstcat((char*)str,tmp_str,sizeof(str));
						genos_strndstcat((char*)str," ",sizeof(str));
					}
					str[sizeof(str) - 1] = '\0';
				}
			}
		}

		j++;
	}while(s);

	ctx->script_printf( ctx, MSG_NONE, "%s\n", str );

	return JTAG_CORE_NO_ERROR;
}

static int cmd_pause( script_ctx * ctx, char * line)
{
	int i;
	char delay_str[DEFAULT_BUFLEN];

	i = get_param( ctx, line, 1, delay_str );

	if(i>=0)
	{
		genos_pause(str_to_int(delay_str));

		return JTAG_CORE_NO_ERROR;
	}

	ctx->script_printf( ctx, MSG_ERROR, "Bad/Missing parameter(s) ! : %s\n", line );

	return JTAG_CORE_BAD_PARAMETER;
}

static int cmd_help( script_ctx * ctx, char * line)
{
	int i;

	cmd_list * cmdlist;

	cmdlist = (cmd_list*)ctx->cmdlist;

	ctx->script_printf( ctx, MSG_INFO_0, "Supported Commands :\n\n" );

	i = 0;
	while(cmdlist[i].func)
	{
		ctx->script_printf( ctx, MSG_NONE, "%s\n", cmdlist[i].command );
		i++;
	}

	return JTAG_CORE_NO_ERROR;
}

static int cmd_call( script_ctx * ctx, char * line )
{
	int offs;
	char path[DEFAULT_BUFLEN];
	char function[DEFAULT_BUFLEN];
	script_ctx * new_ctx;
	int ret;
	jtag_core * jc;

	jc = (jtag_core *)ctx->app_ctx;

	path[0] = '\0';
	get_param( ctx, line, 1, (char*)&path );

	offs = get_param_offset(line, 1);

	if(offs>=0)
	{

		ret = JTAG_CORE_INTERNAL_ERROR;

		new_ctx = init_script((void*)jc,0x00000000,(void*)&jc->envvar);
		if(new_ctx)
		{
			new_ctx->script_printf = ctx->script_printf;

			function[0] = '\0';
			get_param( ctx, line, 2, (char*)&function );

			if(!strcmp(path,"."))
			{
				if(strlen(function))
				{
					snprintf(new_ctx->pre_command,sizeof(new_ctx->pre_command),"goto %s",function);

					ret = execute_file_script( new_ctx, (char*)&ctx->script_file_path );

					new_ctx->pre_command[0] = 0;

					if( ret == JTAG_CORE_ACCESS_ERROR )
					{
						ctx->script_printf( ctx, MSG_ERROR, "call : script not found ! : %s\n", path );
					}
				}
			}
			else
			{
				if(strlen(function))
				{
					snprintf(new_ctx->pre_command,sizeof(new_ctx->pre_command),"goto %s",function);

					ret = execute_file_script( new_ctx, (char*)&path );

					new_ctx->pre_command[0] = 0;

					if( ret == JTAG_CORE_ACCESS_ERROR )
					{
						ctx->script_printf( ctx, MSG_ERROR, "call : script/function not found ! : %s %s\n", path,function );
					}
				}
				else
				{
					ret = execute_file_script( new_ctx, (char*)&path );

					if( ret == JTAG_CORE_ACCESS_ERROR )
					{
						ctx->script_printf( ctx, MSG_ERROR, "call : script not found ! : %s\n", path );
					}
				}
			}

			deinit_script(new_ctx);
		}

		ctx->last_error_code = ret;

		return ret;
	}

	return JTAG_CORE_BAD_PARAMETER;
}

static int cmd_set_env_var( script_ctx * ctx, char * line )
{
	int i,j,ret;
	char varname[DEFAULT_BUFLEN];
	char varvalue[DEFAULT_BUFLEN];
	envvar_entry * tmp_env;

	ret = JTAG_CORE_BAD_PARAMETER;

	i = get_param( ctx, line, 1, varname );
	j = get_param( ctx, line, 2, varvalue );

	if(i>=0 && j>=0)
	{
		tmp_env = setEnvVar( *((envvar_entry **)ctx->env), (char*)&varname, (char*)&varvalue );
		if(tmp_env)
		{
			*((envvar_entry **)ctx->env) = tmp_env;
			ret = JTAG_CORE_NO_ERROR;
		}
		else
			ret = JTAG_CORE_MEM_ERROR;
	}

	return ret;
}

static int cmd_rand( script_ctx * ctx, char * line)
{
	int i;
	uint32_t seed;

	char rand_seed[DEFAULT_BUFLEN];

	seed = ctx->rand_seed;

	i = get_param( ctx, line, 1, rand_seed );
	if(i>=0)
	{
		seed = str_to_int((char*)rand_seed);
	}

	/* Algorithm "xor" from p. 4 of Marsaglia, "Xorshift RNGs" */
	seed ^= seed << 13;
	seed ^= seed >> 17;
	seed ^= seed << 5;

	ctx->rand_seed = seed;

	ctx->last_data_value = seed;

	return JTAG_CORE_NO_ERROR;
}

int is_valid_hex_quartet(char c)
{
	if( (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f') )
		return 1;
	else
		return 0;
}

int is_valid_hex_byte(char *str)
{
	if( is_valid_hex_quartet(str[0]) && is_valid_hex_quartet(str[1]) )
		return 1;
	else
		return 0;
}

char * arrayresize(char * array, int size, unsigned char c)
{
	int cursize;
	char * ptr;

	if(array)
	{
		if( array[0] == '0' && (array[1] == 'x' || array[1] == 'X') )
		{
			ptr = (array + 2);
		}
		else
		{
			ptr = (array);
		}
	}
	else
	{
		array = malloc(DEFAULT_BUFLEN);
		if(array)
		{
			memset(array,0,DEFAULT_BUFLEN);
		}

		ptr = array;
	}

	if(ptr)
	{
		cursize = 0;
		while(is_valid_hex_byte(&ptr[cursize*2]))
		{
			cursize++;
		}

		if( cursize < size  )
		{
			while( cursize < size )
			{
				sprintf(&ptr[(cursize*2) + 0],"%.2X",c);
				//ptr[(cursize*2) + 0] = '0';
				//ptr[(cursize*2) + 1] = '0';
				cursize++;
			}
		}
		else
		{
			ptr[ (size * 2) ] = 0;
		}
	}

	return array;
}


static int cmd_initarray( script_ctx * ctx, char * line)
{
	//initarray  $VARIABLE_1_TEST $BYTES $VALUE

	int i,j,ret;
	char varname[DEFAULT_BUFLEN];
	char varsize[DEFAULT_BUFLEN];
	char varvalue[DEFAULT_BUFLEN];
	char * ptr;
	int size;

	ret = JTAG_CORE_BAD_PARAMETER;

	strcpy(varvalue,"0");

	i = get_param_str( ctx, line, 1, varname );
	j = get_param( ctx, line, 2, varsize );
	get_param( ctx, line, 3, varvalue );

	if(i>=0 && j>=0)
	{
		size = atoi(varsize);

		if(size)
		{
			ptr = getEnvVar( *((envvar_entry **)ctx->env),(char*)&varname, NULL);
			if(ptr)
			{
				arrayresize(ptr, size, (unsigned char)(str_to_int((char*)&varvalue)&0xFF));
			}
			else
			{
				ptr = malloc( DEFAULT_BUFLEN );
				if(ptr)
				{
					memset(ptr,0x00, DEFAULT_BUFLEN);
					arrayresize(ptr, size, (unsigned char)(str_to_int((char*)&varvalue)&0xFF));
				}
			}

			if(ptr)
				*((envvar_entry **)ctx->env) = (void *)setEnvVar( *((envvar_entry **)ctx->env), (char*)&varname[1], ptr );
		}
		ret = JTAG_CORE_NO_ERROR;
	}

	return ret;

}


///////////////////////////////////////////////////////////////////////////////
/////////////////////// JTAG commands/operations //////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static int cmd_autoinit( script_ctx * ctx, char * line)
{
	jtag_core * jc;
	int number_of_devices, dev_nb;
	int loaded_bsdl;
	char szExecPath[MAX_PATH + 1];
	char scanfolder[MAX_PATH + 1];
	char filename[MAX_PATH + 1];
	char entityname[DEFAULT_BUFLEN];
	char file[MAX_PATH + 1];

	filefoundinfo fileinfo;
	void* h_file_find;

	unsigned long chip_id;

	jc = (jtag_core *)ctx->app_ctx;
	loaded_bsdl = 0;

	// BSDL Auto load : check which bsdl file match with the device
	// And load it.

	jtagcore_scan_and_init_chain(jc);

	number_of_devices = jtagcore_get_number_of_devices(jc);

	ctx->script_printf( ctx, MSG_INFO_0, "%d device(s) found\n", number_of_devices );

	// Get the bsdl_files folder path

	genos_getcurrentdirectory(szExecPath,MAX_PATH);

	strcpy(scanfolder,szExecPath);
	strcat(scanfolder,DIR_SEPARATOR"bsdl_files"DIR_SEPARATOR);

	h_file_find = genos_find_first_file( scanfolder, "*.*", &fileinfo );

	// Scan and check files in the folder.
	if (h_file_find)
	{
		do
		{
			strcpy(filename,szExecPath);
			strcat(filename,DIR_SEPARATOR"bsdl_files"DIR_SEPARATOR);
			strcat(filename,fileinfo.filename);

			if ( ! fileinfo.isdirectory )
			{
				chip_id = jtagcore_get_bsdl_id(jc, filename);
				if( chip_id )
				{
					for(dev_nb=0;dev_nb < number_of_devices;dev_nb++)
					{
						if( chip_id == (jtagcore_get_dev_id(jc, dev_nb) & (~0xF0000000)) )
						{
							if(jtagcore_get_number_of_pins(jc, dev_nb) > 0)
							{
								// Device already loaded !
								ctx->script_printf( ctx, MSG_WARNING, "Device %d BSDL already loaded ! ID conflit ?\n", dev_nb );
							}

							// The BSDL ID match with the device.
							if(jtagcore_loadbsdlfile(jc, filename, dev_nb) == JTAG_CORE_NO_ERROR)
							{
								entityname[0] = 0;
								jtagcore_get_dev_name(jc, dev_nb, entityname, file);

								ctx->script_printf( ctx, MSG_INFO_0, "Device %d (%.8X - %s) - BSDL Loaded : %s\n", dev_nb, chip_id, entityname, file );
							}
							else
							{
								ctx->script_printf( ctx, MSG_ERROR, "ERROR while loading %s !\n", filename );
							}
						}
					}
				}
			}
		}while(genos_find_next_file( h_file_find, scanfolder, "*.*", &fileinfo ));

		genos_find_close( h_file_find );

		loaded_bsdl = 0;
		// Count the loaded bsdl
		for(dev_nb=0;dev_nb < number_of_devices;dev_nb++)
		{
			if(jtagcore_get_number_of_pins(jc, dev_nb) > 0)
			{
				loaded_bsdl++;
			}
			else
			{
				ctx->script_printf( ctx, MSG_WARNING, "Device %d (%.8X) - NO BSDL Loaded !\n", dev_nb, jtagcore_get_dev_id(jc, dev_nb ) );
			}
		}
	}
	else
	{
		ctx->script_printf( ctx, MSG_ERROR, "Can't access the bsdl sub folder ! : %s\n", filename );
		return JTAG_CORE_ACCESS_ERROR;
	}

	return loaded_bsdl;
}

static int cmd_init_and_scan( script_ctx * ctx, char * line)
{
	jtag_core * jc;
	int ret;

	jc = (jtag_core *)ctx->app_ctx;

	ret = jtagcore_scan_and_init_chain(jc);

	if( ret == JTAG_CORE_NO_ERROR )
	{
		ctx->script_printf( ctx, MSG_INFO_0, "JTAG Scan done\n" );

		return JTAG_CORE_NO_ERROR;
	}
	else
	{
		ctx->script_printf( ctx, MSG_INFO_0, "JTAG Scan return code : %d\n", ret );
	}

	return ret;
}

static int cmd_print_nb_dev( script_ctx * ctx, char * line)
{
	jtag_core * jc;
	int i;

	jc = (jtag_core *)ctx->app_ctx;

	i = jtagcore_get_number_of_devices(jc);

	ctx->script_printf( ctx, MSG_INFO_0, "%d device(s) found in chain\n", i );

	ctx->last_data_value = i;

	return JTAG_CORE_NO_ERROR;
}

static void bsdl_id_str(unsigned long id, char * str)
{
	int i;

	str[0] = 0;

	for (i = 0; i < 32; i++)
	{
		if ((0x80000000 >> i)&id)
		{
			strcat(str, "1");
		}
		else
		{
			strcat(str, "0");
		}
		if (i == 3) strcat(str, " ");
		if (i == 19) strcat(str, " ");
		if (i == 30) strcat(str, " ");
	}

	str[i] = 0;
}

static char * get_id_str( script_ctx * ctx, int numberofdevice)
{
	// compare passed device ID to the one returned from the ID command
	jtag_core * jc;
	int i;
	unsigned int idcode = 0;
	char * stringbuffer;
	char tempstr[DEFAULT_BUFLEN];

	jc = (jtag_core *)ctx->app_ctx;

	stringbuffer = 0;

	stringbuffer = malloc(256 * numberofdevice);
	if (stringbuffer)
	{
		memset(stringbuffer, 0, 256 * numberofdevice);

		// and read the IDCODES
		for (i = 0; i < numberofdevice; i++)
		{
			idcode = jtagcore_get_dev_id(jc, i);
			sprintf(tempstr, "Device %d : 0x%.8X - ", i, idcode);

			bsdl_id_str(idcode, &tempstr[strlen(tempstr)]);

			strcat(stringbuffer, tempstr);
			strcat(stringbuffer, "\n");
		}
	}

	return stringbuffer;
}

static int cmd_print_devs_list( script_ctx * ctx, char * line)
{
	jtag_core * jc;
	int i;
	char *ptr;

	jc = (jtag_core *)ctx->app_ctx;

	i = jtagcore_get_number_of_devices(jc);
	if(i>0)
	{
		ptr = get_id_str(ctx,i);
		if(ptr)
		{
			ctx->script_printf( ctx, MSG_INFO_0, "%s\n", ptr );
			free(ptr);
		}
	}

	return JTAG_CORE_NOT_FOUND;
}

static int cmd_print_probes_list( script_ctx * ctx, char * line)
{
	jtag_core * jc;
	int i,j;
	char probe_list[64];
	int nb_of_drivers,nb_of_probes;

	jc = (jtag_core *)ctx->app_ctx;

	nb_of_drivers = jtagcore_get_number_of_probes_drv(jc);
	j = 0;
	while (j < nb_of_drivers)
	{
		nb_of_probes = jtagcore_get_number_of_probes(jc, j);
		i = 0;
		while( i < nb_of_probes )
		{
			jtagcore_get_probe_name(jc, PROBE_ID(j,i), probe_list);
			ctx->script_printf( ctx, MSG_INFO_0, "ID 0x%.8X : %s\n", PROBE_ID(j,i), probe_list );
			i++;
		}
		j++;
	}

	return JTAG_CORE_NO_ERROR;
}

static int cmd_open_probe( script_ctx * ctx, char * line)
{
	int ret;
	char probe_id[64];
	jtag_core * jc;

	jc = (jtag_core *)ctx->app_ctx;

	if(get_param( ctx, line, 1, probe_id )>0)
	{
		ret = jtagcore_select_and_open_probe(jc, strtoul(probe_id, NULL, 16));
		if(ret != JTAG_CORE_NO_ERROR)
		{
			ctx->script_printf( ctx, MSG_ERROR, "Code %d !\n", ret );
			return ret;
		}
		else
		{
			ctx->script_printf( ctx, MSG_INFO_0, "Probe Ok !\n" );
			return JTAG_CORE_NO_ERROR;
		}
	}
	else
	{
		ctx->script_printf( ctx, MSG_ERROR, "Bad/Missing parameter(s) ! : %s\n", line );

		return JTAG_CORE_BAD_PARAMETER;
	}
}

static int cmd_load_bsdl( script_ctx * ctx, char * line)
{
	int i,j;
	char dev_index[DEFAULT_BUFLEN];
	char filename[DEFAULT_BUFLEN];
	jtag_core * jc;

	jc = (jtag_core *)ctx->app_ctx;

	i = get_param( ctx, line, 1, filename );
	j = get_param( ctx, line, 2, dev_index );

	if(i>=0 && j>=0)
	{
		if (jtagcore_loadbsdlfile(jc, filename, atoi(dev_index)) >= 0)
		{
			ctx->script_printf( ctx, MSG_INFO_0, "BSDL %s loaded and parsed !\n", filename );
			return JTAG_CORE_NO_ERROR;
		}
		else
		{
			ctx->script_printf( ctx, MSG_ERROR, "File open & parsing error (%s)!\n", filename );
			return JTAG_CORE_ACCESS_ERROR;
		}
	}

	ctx->script_printf( ctx, MSG_ERROR, "Bad/Missing parameter(s) ! : %s\n", line );

	return JTAG_CORE_BAD_PARAMETER;
}

static int cmd_set_scan_mode( script_ctx * ctx, char * line)
{
	int i,j;
	char dev_index[DEFAULT_BUFLEN];
	char scan_mode[DEFAULT_BUFLEN];
	jtag_core * jc;

	jc = (jtag_core *)ctx->app_ctx;

	i = get_param( ctx, line, 1, dev_index );
	j = get_param( ctx, line, 2, scan_mode );

	if(i>=0 && j>=0)
	{
		if( !strcmp(scan_mode,"EXTEST") )
		{
			jtagcore_set_scan_mode(jc, atoi(dev_index),JTAG_CORE_EXTEST_SCANMODE);
			ctx->script_printf( ctx, MSG_INFO_0, "EXTEST mode\n" );
		}
		else
		{
			if( !strcmp(scan_mode,"SAMPLE") )
			{
				jtagcore_set_scan_mode(jc, atoi(dev_index),JTAG_CORE_SAMPLE_SCANMODE);

				ctx->script_printf( ctx, MSG_INFO_0, "SAMPLE mode\n" );
			}
			else
			{
				ctx->script_printf( ctx, MSG_ERROR, "%s : unknown mode !\n", scan_mode );
				return JTAG_CORE_BAD_PARAMETER;
			}
		}

		return JTAG_CORE_NO_ERROR;
	}

	ctx->script_printf( ctx, MSG_ERROR, "Bad/Missing parameter(s) ! : %s\n", line );

	return JTAG_CORE_BAD_PARAMETER;
}

static int cmd_push_and_pop( script_ctx * ctx, char * line)
{
	int ret;
	jtag_core * jc;

	jc = (jtag_core *)ctx->app_ctx;

	ret = jtagcore_push_and_pop_chain(jc, JTAG_CORE_WRITE_READ);

	if(ret != JTAG_CORE_NO_ERROR)
	{
		ctx->script_printf( ctx, MSG_ERROR, "Code %d !\n", ret );
		return ret;
	}
	else
	{
		ctx->script_printf( ctx, MSG_INFO_0, "JTAG chain updated\n" );
	}

	return JTAG_CORE_NO_ERROR;
}

static int cmd_set_pin_mode( script_ctx * ctx, char * line)
{
	int i,j,k,id;
	char dev_index[DEFAULT_BUFLEN];
	char pinname[DEFAULT_BUFLEN];
	char mode[DEFAULT_BUFLEN];
	jtag_core * jc;

	jc = (jtag_core *)ctx->app_ctx;

	i = get_param( ctx, line, 1, dev_index );
	j = get_param( ctx, line, 2, pinname );
	k = get_param( ctx, line, 3, mode );

	if(i>=0 && j>=0 && k>=0)
	{
		id = jtagcore_get_pin_id(jc, atoi(dev_index), pinname);

		if(id>=0)
		{
			jtagcore_set_pin_state(jc, atoi(dev_index), id, JTAG_CORE_OE, atoi(mode));

			ctx->script_printf( ctx, MSG_INFO_0, "Pin %s mode set to %d\n", pinname, atoi(mode) );

			return JTAG_CORE_NO_ERROR;
		}
		else
		{
			ctx->script_printf( ctx, MSG_ERROR, "Pin %s not found\n", pinname );
			return JTAG_CORE_NOT_FOUND;
		}
	}

	ctx->script_printf( ctx, MSG_ERROR, "Bad/Missing parameter(s) ! : %s\n", line );

	return JTAG_CORE_BAD_PARAMETER;
}

static int cmd_set_pin_state( script_ctx * ctx, char * line)
{
	int i,j,k,id;
	char dev_index[DEFAULT_BUFLEN];
	char pinname[DEFAULT_BUFLEN];
	char state[DEFAULT_BUFLEN];
	jtag_core * jc;

	jc = (jtag_core *)ctx->app_ctx;

	i = get_param( ctx, line, 1, dev_index );
	j = get_param( ctx, line, 2, pinname );
	k = get_param( ctx, line, 3, state );

	if(i>=0 && j>=0 && k>=0)
	{
		id = jtagcore_get_pin_id(jc, atoi(dev_index), pinname);

		if(id>=0)
		{
			jtagcore_set_pin_state(jc, atoi(dev_index), id, JTAG_CORE_OUTPUT, atoi(state));

			ctx->script_printf( ctx, MSG_INFO_0, "Pin %s set to %d\n", pinname, atoi(state) );

			return JTAG_CORE_NO_ERROR;
		}
		else
		{
			ctx->script_printf( ctx, MSG_ERROR, "Pin %s not found\n", pinname );
			return JTAG_CORE_NOT_FOUND;
		}
	}

	ctx->script_printf( ctx, MSG_ERROR, "Bad/Missing parameter(s) ! : %s\n", line );

	return JTAG_CORE_BAD_PARAMETER;
}

static int cmd_get_pin_state( script_ctx * ctx, char * line)
{
	int i,j,k,ret,id;
	char dev_index[DEFAULT_BUFLEN];
	char pinname[DEFAULT_BUFLEN];
	char mode[DEFAULT_BUFLEN];
	jtag_core * jc;

	jc = (jtag_core *)ctx->app_ctx;

	i = get_param( ctx, line, 1, dev_index );
	j = get_param( ctx, line, 2, pinname );
	k = get_param( ctx, line, 3, mode );

	if(i>=0 && j>=0 && k>=0)
	{
		id = jtagcore_get_pin_id(jc, atoi(dev_index), pinname);

		if(id>=0)
		{
			ret = jtagcore_get_pin_state(jc, atoi(dev_index), id, JTAG_CORE_INPUT);

			ctx->script_printf( ctx, MSG_INFO_0, "Pin %s state : %d\n", pinname, ret );

			ctx->last_data_value = ret;

			return JTAG_CORE_NO_ERROR;
		}
		else
		{
			ctx->script_printf( ctx, MSG_ERROR, "Pin %s not found\n", pinname );
			return JTAG_CORE_NOT_FOUND;
		}
	}

	ctx->script_printf( ctx, MSG_ERROR, "Bad/Missing parameter(s) ! : %s\n", line );

	return JTAG_CORE_BAD_PARAMETER;
}

/////////////////////////////////////////////////////////////////////////////////////////
// I2C Commands
/////////////////////////////////////////////////////////////////////////////////////////

static int cmd_set_i2c_sda_pin( script_ctx * ctx, char * line)
{
	int i,j,id;
	char dev_index[DEFAULT_BUFLEN];
	char pinname[DEFAULT_BUFLEN];
	jtag_core * jc;

	jc = (jtag_core *)ctx->app_ctx;

	i = get_param( ctx, line, 1, dev_index );
	j = get_param( ctx, line, 2, pinname );

	if(i>=0 && j>=0)
	{
		id = jtagcore_get_pin_id(jc, atoi(dev_index), pinname);

		if(id>=0)
		{
			jtagcore_i2c_set_sda_pin(jc, atoi(dev_index), id);
			ctx->script_printf( ctx, MSG_INFO_0, "SDA set to Pin %s\n", pinname );
			return JTAG_CORE_NO_ERROR;
		}
		else
		{
			ctx->script_printf( ctx, MSG_ERROR, "Pin %s not found\n", pinname );
			return JTAG_CORE_NOT_FOUND;
		}
	}

	ctx->script_printf( ctx, MSG_ERROR, "Bad/Missing parameter(s) ! : %s\n", line );

	return JTAG_CORE_BAD_PARAMETER;
}

static int cmd_set_i2c_scl_pin( script_ctx * ctx, char * line)
{
	int i,j,id;
	char dev_index[DEFAULT_BUFLEN];
	char pinname[DEFAULT_BUFLEN];
	jtag_core * jc;

	jc = (jtag_core *)ctx->app_ctx;

	i = get_param( ctx, line, 1, dev_index );
	j = get_param( ctx, line, 2, pinname );

	if(i>=0 && j>=0)
	{
		id = jtagcore_get_pin_id(jc, atoi(dev_index), pinname);

		if(id>=0)
		{
			jtagcore_i2c_set_scl_pin(jc, atoi(dev_index), id);
			ctx->script_printf( ctx, MSG_INFO_0, "SCL set to Pin %s\n", pinname );
			return JTAG_CORE_NO_ERROR;
		}
		else
		{
			ctx->script_printf( ctx, MSG_ERROR, "Pin %s not found\n", pinname );
			return JTAG_CORE_NOT_FOUND;
		}
	}

	ctx->script_printf( ctx, MSG_ERROR, "Bad/Missing parameter(s) ! : %s\n", line );

	return JTAG_CORE_BAD_PARAMETER;
}

static int cmd_do_i2c_wr( script_ctx * ctx, char * line)
{
	// jtag_set_do_i2c_wr E8 EAACCDD4455
	int i, j;
	int i2cadr, size, ret;
	char adresse[DEFAULT_BUFLEN];
	char data[DEFAULT_BUFLEN];
	unsigned char tmp_buffer2[DEFAULT_BUFLEN];
	char tmp_buffer3[16];
	jtag_core * jc;

	jc = (jtag_core *)ctx->app_ctx;

	i = get_param( ctx, line, 1, adresse );
	j = get_param( ctx, line, 2, data );

	if(i>=0 && j>=0)
	{
		i2cadr = strtoul(adresse,0,16);
		size  = strlen(data);
		size = size / 2;
		for(i = 0; i<size; i++)
		{
			tmp_buffer3[0] = data[i*2];
			tmp_buffer3[1] = data[i*2 + 1];
			tmp_buffer3[2] = 0;

			tmp_buffer2[i] = (char)strtoul(tmp_buffer3,0,16);
		}

		ret = jtagcore_i2c_write_read(jc, i2cadr, 0, size, tmp_buffer2, 0, 0);

		if ( ret <= 0)
		{
			if(ret == 0)
			{
				ctx->script_printf( ctx, MSG_WARNING, "Device Ack not detected ! 0x%.2X\n", i2cadr );
			}
			else
			{
				ctx->script_printf( ctx, MSG_ERROR, "Code %d !\n", ret );
			}

			return ret;
		}
		else
		{
			for(i=0;i<size;i++)
			{
				sprintf(&data[i*3]," %.2X",tmp_buffer2[i]);
			}
			ctx->script_printf( ctx, MSG_INFO_0, "WR I2C 0x%.2X :%s\n", i2cadr, data );

			return JTAG_CORE_NO_ERROR;
		}
	}

	ctx->script_printf( ctx, MSG_ERROR, "Bad/Missing parameter(s) ! : %s\n", line );

	return JTAG_CORE_BAD_PARAMETER;
}

static int cmd_do_i2c_rd( script_ctx * ctx, char * line)
{
	// jtag_set_do_i2c_rd 0xE8 8
	int i,j,i2cadr,size;
	char adresse[DEFAULT_BUFLEN];
	char sizebuf[DEFAULT_BUFLEN];
	char tmp_buffer[DEFAULT_BUFLEN];
	char tmp_buffer2[DEFAULT_BUFLEN];
	char tmp_buffer3[16];
	int ret;
	jtag_core * jc;

	jc = (jtag_core *)ctx->app_ctx;

	i = get_param( ctx, line, 1, adresse );
	j = get_param( ctx, line, 2, sizebuf );

	if(i>=0 && j>=0)
	{
		i2cadr = strtoul(adresse,0,16);
		size  = atoi(sizebuf);

		ret = jtagcore_i2c_write_read(jc, i2cadr, 0, 0, (unsigned char*)tmp_buffer2, size, (unsigned char*)tmp_buffer2);

		if ( ret <= 0)
		{
			if(ret == 0)
			{
				ctx->script_printf( ctx, MSG_WARNING, "Device Ack not detected ! 0x%.2X\n", i2cadr );
			}
			else
			{
				ctx->script_printf( ctx, MSG_ERROR, "Code %d !\n", ret );
			}

			return ret;
		}
		else
		{
			memset(tmp_buffer, 0, sizeof(tmp_buffer));
			for (i = 0; i<size; i++)
			{
				sprintf(tmp_buffer3, " %.2X", tmp_buffer2[i]);
				strcat(tmp_buffer, tmp_buffer3);
			}
			ctx->script_printf( ctx, MSG_INFO_0, "RD I2C 0x%.2X :%s\n", i2cadr, tmp_buffer );

			return JTAG_CORE_NO_ERROR;
		}
	}

	ctx->script_printf( ctx, MSG_ERROR, "Bad/Missing parameter(s) ! : %s\n", line );

	return JTAG_CORE_BAD_PARAMETER;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MDIO Commands
/////////////////////////////////////////////////////////////////////////////////////////

static int cmd_set_mdio_mdc_pin( script_ctx * ctx, char * line)
{
	int i,j,id;
	char dev_index[DEFAULT_BUFLEN];
	char pinname[DEFAULT_BUFLEN];
	jtag_core * jc;

	jc = (jtag_core *)ctx->app_ctx;

	i = get_param( ctx, line, 1, dev_index );
	j = get_param( ctx, line, 2, pinname );

	if(i>=0 && j>=0)
	{
		id = jtagcore_get_pin_id(jc, atoi(dev_index), pinname);

		if(id>=0)
		{
			jtagcore_mdio_set_mdc_pin(jc, atoi(dev_index), id);
			ctx->script_printf( ctx, MSG_INFO_0, "MDC set to Pin %s\n", pinname );
			return JTAG_CORE_NO_ERROR;
		}
		else
		{
			ctx->script_printf( ctx, MSG_ERROR, "Pin %s not found\n",  pinname );
			return JTAG_CORE_NOT_FOUND;
		}
	}

	ctx->script_printf( ctx, MSG_ERROR, "Bad/Missing parameter(s) ! : %s\n", line );

	return JTAG_CORE_BAD_PARAMETER;
}

static int cmd_set_mdio_mdio_pin( script_ctx * ctx, char * line)
{
	int i,j,id;
	char dev_index[DEFAULT_BUFLEN];
	char pinname[DEFAULT_BUFLEN];
	jtag_core * jc;

	jc = (jtag_core *)ctx->app_ctx;

	i = get_param( ctx, line, 1, dev_index );
	j = get_param( ctx, line, 2, pinname );

	if(i>=0 && j>=0)
	{
		id = jtagcore_get_pin_id(jc, atoi(dev_index), pinname);

		if(id>=0)
		{
			jtagcore_mdio_set_mdio_pin(jc, atoi(dev_index), id);
			ctx->script_printf( ctx, MSG_INFO_0, "MDIO set to Pin %s\n", pinname );
			return JTAG_CORE_NO_ERROR;
		}
		else
		{
			ctx->script_printf( ctx, MSG_ERROR, "Pin %s not found\n", pinname );
			return JTAG_CORE_NOT_FOUND;
		}
	}

	ctx->script_printf( ctx, MSG_ERROR, "Bad/Missing parameter(s) ! : %s\n", line );

	return JTAG_CORE_BAD_PARAMETER;
}

static int cmd_do_mdio_wr( script_ctx * ctx, char * line)
{
	// jtag_mdio_wr 01 04 EAAC
	int i,j,k,mdioadr,regadr,datatowrite;
	char address[DEFAULT_BUFLEN];
	char reg[DEFAULT_BUFLEN];
	char data[DEFAULT_BUFLEN];
	int ret;
	jtag_core * jc;

	jc = (jtag_core *)ctx->app_ctx;

	i = get_param( ctx, line, 1, address );
	j = get_param( ctx, line, 2, reg );
	k = get_param( ctx, line, 3, data );

	if(i>=0 && j>=0 && k>=0)
	{
		mdioadr = strtoul(address,0,16);
		regadr = strtoul(reg,0,16);
		datatowrite = strtoul(data, 0, 16);

		ret = jtagcore_mdio_write(jc, mdioadr, regadr, datatowrite);
		if( ret < 0 )
		{
			ctx->script_printf( ctx, MSG_ERROR, "Code %d !\n", ret );
			return ret;
		}

		ctx->script_printf( ctx, MSG_INFO_0, "WR MDIO 0x%.2X : [0x%.2X] = 0x%.4X\n", mdioadr ,regadr, datatowrite );

		return JTAG_CORE_NO_ERROR;
	}

	ctx->script_printf( ctx, MSG_ERROR, "Bad/Missing parameter(s) ! : %s\n", line );

	return JTAG_CORE_BAD_PARAMETER;
}

static int cmd_do_mdio_rd( script_ctx * ctx, char * line)
{
	// jtag_mdio_rd 01 04
	int i,j,mdioadr,regadr,dataread;
	char address[DEFAULT_BUFLEN];
	char reg[DEFAULT_BUFLEN];
	jtag_core * jc;

	jc = (jtag_core *)ctx->app_ctx;

	i = get_param( ctx, line, 1, address );
	j = get_param( ctx, line, 2, reg );

	if(i>=0 && j>=0)
	{
		mdioadr = strtoul(address,0,16);
		regadr = strtoul(reg,0,16);

		dataread = jtagcore_mdio_read(jc, mdioadr, regadr);
		if( dataread < 0 )
		{
			ctx->script_printf( ctx, MSG_ERROR, "Code %d !\n", dataread );
			return dataread;
		}

		ctx->last_data_value = dataread;

		ctx->script_printf( ctx, MSG_INFO_0, "RD MDIO 0x%.2X : [0x%.2X] = 0x%.4X\n", mdioadr ,regadr, dataread );

		return JTAG_CORE_NO_ERROR;
	}

	ctx->script_printf( ctx, MSG_ERROR, "Bad/Missing parameter(s) ! : %s\n", line );

	return JTAG_CORE_BAD_PARAMETER;
}

/////////////////////////////////////////////////////////////////////////////////////////
// SPI Commands
/////////////////////////////////////////////////////////////////////////////////////////
static int cmd_set_spi_cs_pin( script_ctx * ctx, char * line)
{
	int i,j,k,id;
	char dev_index[DEFAULT_BUFLEN];
	char pinname[DEFAULT_BUFLEN];
	char polarity[DEFAULT_BUFLEN];
	jtag_core * jc;

	jc = (jtag_core *)ctx->app_ctx;

	i = get_param( ctx, line, 1, dev_index );
	j = get_param( ctx, line, 2, pinname );
	k = get_param( ctx, line, 3, polarity );

	if(i>=0 && j>=0 && k>=0)
	{
		id = jtagcore_get_pin_id(jc, atoi(dev_index), pinname);

		if(id>=0)
		{
			jtagcore_spi_set_cs_pin(jc, atoi(dev_index), id, atoi(polarity));
			ctx->script_printf( ctx, MSG_INFO_0, "CS set to Pin %s with polarity %d\n", pinname, atoi(polarity) );
			return JTAG_CORE_NO_ERROR;
		}
		else
		{
			ctx->script_printf( ctx, MSG_ERROR, "Pin %s not found\n", pinname );
			return JTAG_CORE_NOT_FOUND;
		}
	}

	ctx->script_printf( ctx, MSG_ERROR, "Bad/Missing parameter(s) ! : %s\n", line );

	return JTAG_CORE_BAD_PARAMETER;
}

static int cmd_set_spi_clk_pin( script_ctx * ctx, char * line)
{
	int i,j,k,id;
	char dev_index[DEFAULT_BUFLEN];
	char pinname[DEFAULT_BUFLEN];
	char polarity[DEFAULT_BUFLEN];
	jtag_core * jc;

	jc = (jtag_core *)ctx->app_ctx;

	i = get_param( ctx, line, 1, dev_index );
	j = get_param( ctx, line, 2, pinname );
	k = get_param( ctx, line, 3, polarity );

	if(i>=0 && j>=0 && k>=0)
	{
		id = jtagcore_get_pin_id(jc, atoi(dev_index), pinname);

		if(id>=0)
		{
			jtagcore_spi_set_clk_pin(jc, atoi(dev_index), id, atoi(polarity));
			ctx->script_printf( ctx, MSG_INFO_0, "CLK set to Pin %s with polarity %d\n", pinname, atoi(polarity) );
			return JTAG_CORE_NO_ERROR;
		}
		else
		{
			ctx->script_printf( ctx, MSG_ERROR, "Pin %s not found\n", pinname );
			return JTAG_CORE_NOT_FOUND;
		}
	}

	ctx->script_printf( ctx, MSG_ERROR, "Bad/Missing parameter(s) ! : %s\n", line );

	return JTAG_CORE_BAD_PARAMETER;
}

static int cmd_set_spi_mosi_pin( script_ctx * ctx, char * line)
{
	int i,j,k,id;
	char dev_index[DEFAULT_BUFLEN];
	char pinname[DEFAULT_BUFLEN];
	char phase[DEFAULT_BUFLEN];
	jtag_core * jc;

	jc = (jtag_core *)ctx->app_ctx;

	i = get_param( ctx, line, 1, dev_index );
	j = get_param( ctx, line, 2, pinname );
	k = get_param( ctx, line, 3, phase );

	if(i>=0 && j>=0 && k>=0)
	{
		id = jtagcore_get_pin_id(jc, atoi(dev_index), pinname);

		if(id>=0)
		{
			jtagcore_spi_set_mosi_pin(jc, atoi(dev_index), id, atoi(phase));
			ctx->script_printf( ctx, MSG_INFO_0, "MOSI set to Pin %s with polarity %d\n", pinname, atoi(phase) );
			return JTAG_CORE_NO_ERROR;
		}
		else
		{
			ctx->script_printf( ctx, MSG_ERROR, "Pin %s not found\n", pinname );
			return JTAG_CORE_NOT_FOUND;
		}
	}

	ctx->script_printf( ctx, MSG_ERROR, "Bad/Missing parameter(s) ! : %s\n", line );

	return JTAG_CORE_BAD_PARAMETER;
}

static int cmd_set_spi_miso_pin( script_ctx * ctx, char * line)
{
	int i,j,k,id;
	char dev_index[DEFAULT_BUFLEN];
	char pinname[DEFAULT_BUFLEN];
	char phase[DEFAULT_BUFLEN];
	jtag_core * jc;

	jc = (jtag_core *)ctx->app_ctx;

	i = get_param( ctx, line, 1, dev_index );
	j = get_param( ctx, line, 2, pinname );
	k = get_param( ctx, line, 3, phase );

	if(i>=0 && j>=0 && k>=0)
	{
		id = jtagcore_get_pin_id(jc, atoi(dev_index), pinname);

		if(id>=0)
		{
			jtagcore_spi_set_miso_pin(jc, atoi(dev_index), id, atoi(phase));
			ctx->script_printf( ctx, MSG_INFO_0, "MISO set to Pin %s with polarity %d\n", pinname, atoi(phase) );
			return JTAG_CORE_NO_ERROR;
		}
		else
		{
			ctx->script_printf( ctx, MSG_ERROR, "Pin %s not found\n", pinname );
			return JTAG_CORE_NOT_FOUND;
		}
	}

	ctx->script_printf( ctx, MSG_ERROR, "Parameters error: %s\n", line );

	return JTAG_CORE_BAD_PARAMETER;
}

static int cmd_spi_rd_wr( script_ctx * ctx, char * line)
{
	int i,j,k,size;
	char data_out_txt[DEFAULT_BUFLEN];
	unsigned char data_out[DEFAULT_BUFLEN];
	unsigned char data_in[DEFAULT_BUFLEN];
	char lsbfirst[DEFAULT_BUFLEN];
	char tmp_buffer[3];
	int ret;
	jtag_core * jc;

	jc = (jtag_core *)ctx->app_ctx;

	// jtag_spi_rd_wr 00123344 1  (DATA LSBFirst)
	i = get_param( ctx, line, 1, data_out_txt );
	j = get_param( ctx, line, 2, lsbfirst );

	if(i>=0)
	{
		if(j>=0)
		{
			jtagcore_spi_set_bitorder(jc, atoi(lsbfirst));
		}

		size  = strlen(data_out_txt);
		size = size / 2;
		for(k = 0; k<size; k++)
		{
			tmp_buffer[0] = data_out_txt[k*2];
			tmp_buffer[1] = data_out_txt[k*2 + 1];
			tmp_buffer[2] = 0;
			data_out[k] = (unsigned char)strtoul(tmp_buffer,0,16);
		}

		ret = jtagcore_spi_write_read(jc, size,data_out,data_in, 0);
		if( ret < 0 )
		{
			ctx->script_printf( ctx, MSG_ERROR, "Code %d !\n", ret );
			return ret;
		}

		ctx->script_printf( ctx, MSG_INFO_0, "SPI TX:" );
		for(k = 0; k<size; k++)
		{
			ctx->script_printf( ctx, MSG_NONE, " %.2X", data_out[k] );
		}
		ctx->script_printf( ctx, MSG_NONE, "\n" );

		ctx->script_printf( ctx, MSG_INFO_0, "SPI RX:" );
		for(k = 0; k<size; k++)
		{
			ctx->script_printf( ctx, MSG_NONE, " %.2X", data_in[k] );
		}
		ctx->script_printf( ctx, MSG_NONE, "\n" );

		return JTAG_CORE_NOT_FOUND;
	}

	ctx->script_printf( ctx, MSG_ERROR, "Parameters error: %s\n", line );

	return JTAG_CORE_BAD_PARAMETER;
}

static int cmd_get_pins_list( script_ctx * ctx, char * line)
{
	int i,j,nb_of_pins;
	char dev_index[DEFAULT_BUFLEN];
	char pinname[DEFAULT_BUFLEN];
	int type;
	jtag_core * jc;

	jc = (jtag_core *)ctx->app_ctx;

	i = get_param( ctx, line, 1, dev_index );
	if(i>=0)
	{
		nb_of_pins = jtagcore_get_number_of_pins(jc,atoi(dev_index));
		if(nb_of_pins>=0)
		{
			ctx->script_printf( ctx, MSG_INFO_0, "Device %d : %d pin(s)\n", atoi(dev_index), nb_of_pins );
			for(j = 0;j < nb_of_pins;j++)
			{
				if(jtagcore_get_pin_properties(jc, atoi(dev_index), j, pinname, sizeof(pinname), &type) == JTAG_CORE_NO_ERROR)
				{
					ctx->script_printf( ctx, MSG_NONE, "%s : ", pinname );
					if(type & JTAG_CORE_PIN_IS_INPUT)
					{
						ctx->script_printf( ctx, MSG_NONE, " in  " );
					}
					else
					{
						ctx->script_printf( ctx, MSG_NONE, "     " );
					}

					if(type & JTAG_CORE_PIN_IS_OUTPUT)
					{
						ctx->script_printf( ctx, MSG_NONE, " out " );
					}
					else
					{
						ctx->script_printf( ctx, MSG_NONE, "     " );
					}

					if(type & JTAG_CORE_PIN_IS_TRISTATES)
					{
						ctx->script_printf( ctx, MSG_NONE, " tris" );
					}
					else
					{
						ctx->script_printf( ctx, MSG_NONE, "     ");
					}

					ctx->script_printf( ctx, MSG_NONE, "\n");

				}
			}
		}

		return JTAG_CORE_NO_ERROR;
	}

	return JTAG_CORE_BAD_PARAMETER;
}

cmd_list script_commands_list[] =
{
	{"print",                   cmd_print},
	{"help",                    cmd_help},
	{"?",                       cmd_help},
	{"version",                 cmd_version},
	{"pause",                   cmd_pause},
	{"set",                     cmd_set_env_var},
	{"print_env_var",           cmd_print_env_var},
	{"call",                    cmd_call},
	{"system",                  cmd_system},

	{"if",                      cmd_if},
	{"goto",                    cmd_goto},
	{"return",                  cmd_return},

	{"rand",                    cmd_rand},

	{"init_array",              cmd_initarray},

	{"jtag_get_probes_list",    cmd_print_probes_list},
	{"jtag_open_probe",         cmd_open_probe},
	{"jtag_autoinit",           cmd_autoinit},

	{"jtag_init_scan",          cmd_init_and_scan},
	{"jtag_get_nb_of_devices",  cmd_print_nb_dev},
	{"jtag_get_devices_list",   cmd_print_devs_list},
	{"jtag_load_bsdl",          cmd_load_bsdl},
	{"jtag_set_mode",           cmd_set_scan_mode},

	{"jtag_push_pop",           cmd_push_and_pop},

	{"jtag_get_pins_list",      cmd_get_pins_list},

	{"jtag_set_pin_dir",        cmd_set_pin_mode},
	{"jtag_set_pin_state",      cmd_set_pin_state},
	{"jtag_get_pin_state",      cmd_get_pin_state},

	{"jtag_set_i2c_scl_pin",    cmd_set_i2c_scl_pin},
	{"jtag_set_i2c_sda_pin",    cmd_set_i2c_sda_pin},
	{"jtag_i2c_rd",             cmd_do_i2c_rd},
	{"jtag_i2c_wr",             cmd_do_i2c_wr},

	{"jtag_set_mdio_mdc_pin",   cmd_set_mdio_mdc_pin},
	{"jtag_set_mdio_mdio_pin",  cmd_set_mdio_mdio_pin},
	{"jtag_mdio_rd",            cmd_do_mdio_rd},
	{"jtag_mdio_wr",            cmd_do_mdio_wr},

	{"jtag_set_spi_cs_pin",     cmd_set_spi_cs_pin},
	{"jtag_set_spi_mosi_pin",   cmd_set_spi_mosi_pin},
	{"jtag_set_spi_miso_pin",   cmd_set_spi_miso_pin},
	{"jtag_set_spi_clk_pin",    cmd_set_spi_clk_pin},
	{"jtag_spi_rd_wr",          cmd_spi_rd_wr},

	{0 , 0}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

script_ctx * jtagcore_initScript(jtag_core * jc)
{
	return init_script((void*)jc,0x00000000,(void*)&jc->envvar);
}

void jtagcore_setScriptOutputFunc( script_ctx * ctx, SCRIPT_PRINTF_FUNC ext_printf )
{
	setOutputFunc_script(ctx, ext_printf);
}

int jtagcore_execScriptLine( script_ctx * ctx, char * line )
{
	if(!ctx)
		return JTAG_CORE_INTERNAL_ERROR;

	return execute_line_script( ctx, line );
}

int jtagcore_execScriptFile( script_ctx * ctx, char * script_path )
{
	if(!ctx)
		return JTAG_CORE_INTERNAL_ERROR;

	return execute_file_script( ctx, script_path );
}

int jtagcore_execScriptRam( script_ctx * ctx, unsigned char * script_buffer, int buffersize )
{
	if(!ctx)
		return JTAG_CORE_INTERNAL_ERROR;

	return execute_ram_script( ctx, script_buffer, buffersize );
}

script_ctx * jtagcore_deinitScript(script_ctx * ctx)
{
	if(!ctx)
		return NULL;

	return deinit_script(ctx);
}

int jtagcore_savePinsStateScript( jtag_core * jc, int device, char * script_path )
{
	FILE * f;
	int number_of_pins;
	int pin_type;
	int i,state;
	char pin_name[DEFAULT_BUFLEN];

	f = fopen(script_path,"w");
	if(f)
	{
		if(jc)
		{
			number_of_pins = jtagcore_get_number_of_pins( jc, device );

			if(number_of_pins>0)
			{
				for(i=0;i<number_of_pins;i++)
				{
					pin_type = 0;

					jtagcore_get_pin_properties( jc, device, i, (char*)&pin_name, sizeof(pin_name), &pin_type);

					if (pin_type)
					{
						fprintf(f,"print ----------------------------\n");

						// output enable
						if (pin_type & JTAG_CORE_PIN_IS_TRISTATES)
						{
							state = jtagcore_get_pin_state(jc, device, i, JTAG_CORE_OE);
							fprintf(f,"print Pin %s direction : %d\n",pin_name,state);
							fprintf(f,"jtag_set_pin_dir   %d %s %d\n",device,pin_name,state);
						}

						// output data
						if (pin_type & JTAG_CORE_PIN_IS_OUTPUT)
						{
							state = jtagcore_get_pin_state(jc, device, i, JTAG_CORE_OUTPUT);
							fprintf(f,"print Pin %s state : %d\n",pin_name,state);
							fprintf(f,"jtag_set_pin_state %d %s %d\n",device,pin_name,state);
						}

						// input data
						if (pin_type & JTAG_CORE_PIN_IS_INPUT)
						{
							state = jtagcore_get_pin_state(jc, device, i, JTAG_CORE_INPUT);
							fprintf(f,"print Input pin %s state : %d\n",pin_name,state);
						}
						fprintf(f,"\n");
					}
				}

				fprintf(f,"jtag_push_pop\n");
				fprintf(f,"\n");
			}
		}

		fclose(f);
	}
	return 0;
}
