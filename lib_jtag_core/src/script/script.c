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
* @brief  command line parser.
* @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdint.h>

#define MAX_PATH 256

#include "../jtag_core.h"
#include "../bsdl_parser/bsdl_loader.h"

#include "../os_interface/os_interface.h"

#include "script.h"
#include "env.h"

typedef int (* CMD_FUNC)( jtag_core * jc, char * line);

PRINTF_FUNC script_printf;

typedef struct cmd_list_
{
	char * command;
	CMD_FUNC func;
}cmd_list;

void setOutputFunc( PRINTF_FUNC ext_printf )
{
	script_printf = ext_printf;

	return;
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

static int get_next_word(char * line, int offset)
{
	while( !is_end_line(line[offset]) && ( line[offset] == ' ' ) )
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

	param_cnt = 0;
	do
	{
		offs = copy_param(NULL, line, offs);

		offs = get_next_word( line, offs );

		if(line[offs] == 0 || line[offs] == '#')
			return -1;

		param_cnt++;
	}while( param_cnt < param );

	return offs;
}

static int get_param(char * line, int param_offset,char * param)
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

static int cmd_autoinit( jtag_core * jc, char * line)
{
	int i;
	int number_of_devices, dev_nb;
	int loaded_bsdl;
	char szExecPath[MAX_PATH + 1];
	char filename[MAX_PATH + 1];
	char entityname[DEFAULT_BUFLEN];
	char file[MAX_PATH + 1];

	filefoundinfo fileinfo;
	void* h_file_find;

	unsigned long chip_id;

	loaded_bsdl = 0;

	// BSDL Auto load : check which bsdl file match with the device
	// And load it.

	jtagcore_scan_and_init_chain(jc);

	number_of_devices = jtagcore_get_number_of_devices(jc);

	script_printf(MSG_INFO_0,"%d devices found\n",number_of_devices);

	// Get the bsdl_files folder path

	genos_getcurrentdirectory(szExecPath,MAX_PATH);
	i = strlen(szExecPath);

	while(i && (szExecPath[i]!= DIR_SEPARATOR_CHAR) )
		i--;

	szExecPath[i] = 0;

	strcpy(filename,szExecPath);
	strcat(filename,DIR_SEPARATOR"bsdl_files"DIR_SEPARATOR);

	h_file_find = genos_find_first_file( filename, "*.*", &fileinfo );

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
						if( chip_id == jtagcore_get_dev_id(jc, dev_nb) )
						{
							if(jtagcore_get_number_of_pins(jc, dev_nb) > 0)
							{
								// Device already loaded !
								script_printf(MSG_WARNING,"Device %d BSDL already loaded ! ID conflit ?\n",dev_nb);
							}

							// The BSDL ID match with the device.
							if(jtagcore_loadbsdlfile(jc, filename, dev_nb) == JTAG_CORE_NO_ERROR)
							{
								entityname[0] = 0;
								jtagcore_get_dev_name(jc, dev_nb, entityname, file);

								script_printf(MSG_INFO_0,"Device %d (%.8X - %s) - BSDL Loaded : %s\n",dev_nb,chip_id,entityname,file);
							}
							else
							{
								script_printf(MSG_ERROR,"ERROR while loading %s !\n",filename);
							}
						}
					}
				}
			}
		}while(genos_find_next_file( h_file_find, filename, "*.*", &fileinfo ));


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
				script_printf(MSG_WARNING,"Device %d (%.8X) - NO BSDL Loaded !\n",dev_nb,jtagcore_get_dev_id(jc, dev_nb));
			}
		}
	}
	else
	{
		script_printf(MSG_ERROR,"Can't access ."DIR_SEPARATOR"bsdl_files sub folder !\n");
	}

	return loaded_bsdl;
}

static int cmd_print( jtag_core * jc, char * line)
{
	int i;

	i = get_param_offset(line, 1);
	if(i>=0)
		script_printf(MSG_NONE,"%s\n",&line[i]);

	return JTAG_CORE_NOT_FOUND;
}

static int cmd_pause( jtag_core * jc, char * line)
{
	int i;
	char delay_str[DEFAULT_BUFLEN];

	i = get_param(line, 1,delay_str);

	if(i>=0)
	{
		genos_pause(atoi(delay_str));

		return JTAG_CORE_NOT_FOUND;
	}

	script_printf(MSG_ERROR,"Bad/Missing parameter(s) ! : %s\n",line);

	return JTAG_CORE_BAD_PARAMETER;
}

static int cmd_init_and_scan( jtag_core * jc, char * line)
{
	int ret;

	ret = jtagcore_scan_and_init_chain(jc);

	if( ret == JTAG_CORE_NO_ERROR )
	{
		script_printf(MSG_INFO_0,"JTAG Scan done\n");

		return JTAG_CORE_NOT_FOUND;
	}
	else
	{
		script_printf(MSG_INFO_0,"JTAG Scan return code : %d\n",ret);
	}

	return 0;
}

static int cmd_print_nb_dev( jtag_core * jc, char * line)
{
	int i;

	i = jtagcore_get_number_of_devices(jc);

	script_printf(MSG_INFO_0,"%d device(s) found in chain\n",i);

	return JTAG_CORE_NOT_FOUND;
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

static char * get_id_str( jtag_core * jc, int numberofdevice)
{
	// compare passed device ID to the one returned from the ID command
	int i;
	unsigned int idcode = 0;
	char * stringbuffer;
	char tempstr[DEFAULT_BUFLEN];

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

static int cmd_print_devs_list( jtag_core * jc, char * line)
{
	int i;

	i = jtagcore_get_number_of_devices(jc);
	if(i>0)
	{
		script_printf(MSG_INFO_0,"%s\n",get_id_str(jc,i));
	}

	return JTAG_CORE_NOT_FOUND;
}

static int cmd_print_probes_list( jtag_core * jc, char * line)
{
	int i,j;
	char probe_list[64];
	int nb_of_drivers,nb_of_probes;

	nb_of_drivers = jtagcore_get_number_of_probes_drv(jc);
	j = 0;
	while (j < nb_of_drivers)
	{
		nb_of_probes = jtagcore_get_number_of_probes(jc, j);
		i = 0;
		while( i < nb_of_probes )
		{
			jtagcore_get_probe_name(jc, PROBE_ID(j,i), probe_list);
			script_printf(MSG_INFO_0,"ID 0x%.8X : %s\n",PROBE_ID(j,i),probe_list);
			i++;
		}
		j++;
	}

	return JTAG_CORE_NOT_FOUND;
}

static int cmd_open_probe( jtag_core * jc, char * line)
{
	int ret;
	char probe_id[64];

	if(get_param(line, 1,probe_id)>0)
	{
		ret = jtagcore_select_and_open_probe(jc, strtol(probe_id, NULL, 16));
		if(ret != JTAG_CORE_NO_ERROR)
		{
			script_printf(MSG_ERROR,"Code %d !\n",ret);
		}
		else
		{
			script_printf(MSG_INFO_0,"Probe Ok !\n");
		}
	}
	else
	{
		script_printf(MSG_ERROR,"Bad/Missing parameter(s) ! : %s\n",line);

		return JTAG_CORE_BAD_PARAMETER;
	}

	return JTAG_CORE_NOT_FOUND;
}

static int cmd_load_bsdl( jtag_core * jc, char * line)
{
	int i,j;
	char dev_index[DEFAULT_BUFLEN];
	char filename[DEFAULT_BUFLEN];

	i = get_param(line, 1,filename);
	j = get_param(line, 2,dev_index);

	if(i>=0 && j>=0)
	{
		if (jtagcore_loadbsdlfile(jc, filename, atoi(dev_index)) >= 0)
		{
			script_printf(MSG_INFO_0,"BSDL %s loaded and parsed !\n",filename);
			return JTAG_CORE_NOT_FOUND;
		}
		else
		{
			script_printf(MSG_ERROR,"File open & parsing error (%s)!\n",filename);
			return 0;
		}
	}

	script_printf(MSG_ERROR,"Bad/Missing parameter(s) ! : %s\n",line);

	return JTAG_CORE_BAD_PARAMETER;
}

static int cmd_set_scan_mode( jtag_core * jc, char * line)
{
	int i,j;
	char dev_index[DEFAULT_BUFLEN];
	char scan_mode[DEFAULT_BUFLEN];

	i = get_param(line, 1,dev_index);
	j = get_param(line, 2,scan_mode);

	if(i>=0 && j>=0)
	{
		if( !strcmp(scan_mode,"EXTEST") )
		{
			jtagcore_set_scan_mode(jc, atoi(dev_index),JTAG_CORE_EXTEST_SCANMODE);
			script_printf(MSG_INFO_0,"EXTEST mode\n");
		}
		else
		{
			if( !strcmp(scan_mode,"SAMPLE") )
			{
				jtagcore_set_scan_mode(jc, atoi(dev_index),JTAG_CORE_SAMPLE_SCANMODE);

				script_printf(MSG_INFO_0,"SAMPLE mode\n");
			}
			else
			{
				script_printf(MSG_ERROR,"%s : unknown mode !\n",scan_mode);
				return 0;
			}
		}

		return JTAG_CORE_NOT_FOUND;
	}

	script_printf(MSG_ERROR,"Bad/Missing parameter(s) ! : %s\n",line);

	return JTAG_CORE_BAD_PARAMETER;
}

static int cmd_push_and_pop( jtag_core * jc, char * line)
{
	int ret;

	ret = jtagcore_push_and_pop_chain(jc, JTAG_CORE_WRITE_READ);

	if(ret != JTAG_CORE_NO_ERROR)
	{
		script_printf(MSG_ERROR,"Code %d !\n",ret);
		return 0;
	}
	else
	{
		script_printf(MSG_INFO_0,"JTAG chain updated\n");
	}

	return JTAG_CORE_NOT_FOUND;
}

static int cmd_set_pin_mode( jtag_core * jc, char * line)
{
	int i,j,k,id;
	char dev_index[DEFAULT_BUFLEN];
	char pinname[DEFAULT_BUFLEN];
	char mode[DEFAULT_BUFLEN];

	i = get_param(line, 1,dev_index);
	j = get_param(line, 2,pinname);
	k = get_param(line, 3,mode);

	if(i>=0 && j>=0 && k>=0)
	{
		id = jtagcore_get_pin_id(jc, atoi(dev_index), pinname);

		if(id>=0)
		{
			jtagcore_set_pin_state(jc, atoi(dev_index), id, JTAG_CORE_OE, atoi(mode));

			script_printf(MSG_INFO_0,"Pin %s mode set to %d\n",pinname,atoi(mode));

			return JTAG_CORE_NOT_FOUND;
		}
		else
		{
			script_printf(MSG_ERROR,"Pin %s not found\n",pinname);
			return 0;
		}
	}

	script_printf(MSG_ERROR,"Bad/Missing parameter(s) ! : %s\n",line);

	return JTAG_CORE_BAD_PARAMETER;
}

static int cmd_set_pin_state( jtag_core * jc, char * line)
{
	int i,j,k,id;
	char dev_index[DEFAULT_BUFLEN];
	char pinname[DEFAULT_BUFLEN];
	char state[DEFAULT_BUFLEN];

	i = get_param(line, 1,dev_index);
	j = get_param(line, 2,pinname);
	k = get_param(line, 3,state);

	if(i>=0 && j>=0 && k>=0)
	{
		id = jtagcore_get_pin_id(jc, atoi(dev_index), pinname);

		if(id>=0)
		{
			jtagcore_set_pin_state(jc, atoi(dev_index), id, JTAG_CORE_OUTPUT, atoi(state));

			script_printf(MSG_INFO_0,"Pin %s set to %d\n",pinname,atoi(state));

			return JTAG_CORE_NOT_FOUND;
		}
		else
		{
			script_printf(MSG_ERROR,"Pin %s not found\n",pinname);
			return 0;
		}
	}

	script_printf(MSG_ERROR,"Bad/Missing parameter(s) ! : %s\n",line);

	return JTAG_CORE_BAD_PARAMETER;
}

static int cmd_get_pin_state( jtag_core * jc, char * line)
{
	int i,j,k,ret,id;
	char dev_index[DEFAULT_BUFLEN];
	char pinname[DEFAULT_BUFLEN];
	char mode[DEFAULT_BUFLEN];

	i = get_param(line, 1,dev_index);
	j = get_param(line, 2,pinname);
	k = get_param(line, 3,mode);

	if(i>=0 && j>=0 && k>=0)
	{
		id = jtagcore_get_pin_id(jc, atoi(dev_index), pinname);

		if(id>=0)
		{
			ret = jtagcore_get_pin_state(jc, atoi(dev_index), id, JTAG_CORE_INPUT);

			script_printf(MSG_INFO_0,"Pin %s state : %d\n",pinname,ret);

			return JTAG_CORE_NOT_FOUND;
		}
		else
		{
			script_printf(MSG_ERROR,"Pin %s not found\n",pinname);
			return 0;
		}
	}

	script_printf(MSG_ERROR,"Bad/Missing parameter(s) ! : %s\n",line);

	return JTAG_CORE_BAD_PARAMETER;
}

/////////////////////////////////////////////////////////////////////////////////////////
// I2C Commands
/////////////////////////////////////////////////////////////////////////////////////////

static int cmd_set_i2c_sda_pin( jtag_core * jc, char * line)
{
	int i,j,id;
	char dev_index[DEFAULT_BUFLEN];
	char pinname[DEFAULT_BUFLEN];

	i = get_param(line, 1,dev_index);
	j = get_param(line, 2,pinname);

	if(i>=0 && j>=0)
	{
		id = jtagcore_get_pin_id(jc, atoi(dev_index), pinname);

		if(id>=0)
		{
			jtagcore_i2c_set_sda_pin(jc, atoi(dev_index), id);
			script_printf(MSG_INFO_0,"SDA set to Pin %s\n",pinname);
			return JTAG_CORE_NOT_FOUND;
		}
		else
		{
			script_printf(MSG_ERROR,"Pin %s not found\n",pinname);
			return 0;
		}
	}

	script_printf(MSG_ERROR,"Bad/Missing parameter(s) ! : %s\n",line);

	return JTAG_CORE_BAD_PARAMETER;
}

static int cmd_set_i2c_scl_pin( jtag_core * jc, char * line)
{
	int i,j,id;
	char dev_index[DEFAULT_BUFLEN];
	char pinname[DEFAULT_BUFLEN];

	i = get_param(line, 1,dev_index);
	j = get_param(line, 2,pinname);

	if(i>=0 && j>=0)
	{
		id = jtagcore_get_pin_id(jc, atoi(dev_index), pinname);

		if(id>=0)
		{
			jtagcore_i2c_set_scl_pin(jc, atoi(dev_index), id);
			script_printf(MSG_INFO_0,"SCL set to Pin %s\n",pinname);
			return JTAG_CORE_NOT_FOUND;
		}
		else
		{
			script_printf(MSG_ERROR,"Pin %s not found\n",pinname);
			return 0;
		}
	}

	script_printf(MSG_ERROR,"Bad/Missing parameter(s) ! : %s\n",line);

	return JTAG_CORE_BAD_PARAMETER;
}

static int cmd_do_i2c_wr( jtag_core * jc, char * line)
{
	// jtag_set_do_i2c_wr E8 EAACCDD4455
	int i, j;
	int i2cadr, size, ret;
	char adresse[DEFAULT_BUFLEN];
	char data[DEFAULT_BUFLEN];
	unsigned char tmp_buffer2[DEFAULT_BUFLEN];
	char tmp_buffer3[16];

	i = get_param(line, 1,adresse);
	j = get_param(line, 2,data);

	if(i>=0 && j>=0)
	{
		i2cadr = strtol(adresse,0,16);
		size  = strlen(data);
		size = size / 2;
		for(i = 0; i<size; i++)
		{
			tmp_buffer3[0] = data[i*2];
			tmp_buffer3[1] = data[i*2 + 1];
			tmp_buffer3[2] = 0;

			tmp_buffer2[i] = (char)strtol(tmp_buffer3,0,16);
		}

		ret = jtagcore_i2c_write_read(jc, i2cadr, 0, size, tmp_buffer2, 0, 0);

		if ( ret <= 0)
		{
			if(ret == 0)
			{
				script_printf(MSG_WARNING,"Device Ack not detected ! 0x%.2X\n",i2cadr);
			}
			else
			{
				script_printf(MSG_ERROR,"Code %d !\n",ret);
				return 0;
			}
		}
		else
		{
			for(i=0;i<size;i++)
			{
				sprintf(&data[i*3]," %.2X",tmp_buffer2[i]);
			}
			script_printf(MSG_INFO_0,"WR I2C 0x%.2X :%s\n",i2cadr,data);
		}
		return JTAG_CORE_NOT_FOUND;
	}

	script_printf(MSG_ERROR,"Bad/Missing parameter(s) ! : %s\n",line);

	return JTAG_CORE_BAD_PARAMETER;
}

static int cmd_do_i2c_rd( jtag_core * jc, char * line)
{
	// jtag_set_do_i2c_rd 0xE8 8
	int i,j,i2cadr,size;
	char adresse[DEFAULT_BUFLEN];
	char sizebuf[DEFAULT_BUFLEN];
	char tmp_buffer[DEFAULT_BUFLEN];
	char tmp_buffer2[DEFAULT_BUFLEN];
	char tmp_buffer3[16];
	int ret;

	i = get_param(line, 1,adresse);
	j = get_param(line, 2,sizebuf);

	if(i>=0 && j>=0)
	{
		i2cadr = strtol(adresse,0,16);
		size  = atoi(sizebuf);

		ret = jtagcore_i2c_write_read(jc, i2cadr, 0, 0, (unsigned char*)tmp_buffer2, size, (unsigned char*)tmp_buffer2);

		if ( ret <= 0)
		{
			if(ret == 0)
			{
				script_printf(MSG_WARNING,"Device Ack not detected ! 0x%.2X\n",i2cadr);
			}
			else
			{
				script_printf(MSG_ERROR,"Code %d !\n",ret);
				return 0;
			}
		}
		else
		{
			memset(tmp_buffer, 0, sizeof(tmp_buffer));
			for (i = 0; i<size; i++)
			{
				sprintf(tmp_buffer3, " %.2X", tmp_buffer2[i]);
				strcat(tmp_buffer, tmp_buffer3);
			}
			script_printf(MSG_INFO_0,"RD I2C 0x%.2X :%s\n",i2cadr,tmp_buffer);
		}
		return JTAG_CORE_NOT_FOUND;
	}

	script_printf(MSG_ERROR,"Bad/Missing parameter(s) ! : %s\n",line);

	return JTAG_CORE_BAD_PARAMETER;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MDIO Commands
/////////////////////////////////////////////////////////////////////////////////////////

static int cmd_set_mdio_mdc_pin( jtag_core * jc, char * line)
{
	int i,j,id;
	char dev_index[DEFAULT_BUFLEN];
	char pinname[DEFAULT_BUFLEN];

	i = get_param(line, 1,dev_index);
	j = get_param(line, 2,pinname);

	if(i>=0 && j>=0)
	{
		id = jtagcore_get_pin_id(jc, atoi(dev_index), pinname);

		if(id>=0)
		{
			jtagcore_mdio_set_mdc_pin(jc, atoi(dev_index), id);
			script_printf(MSG_INFO_0,"MDC set to Pin %s\n",pinname);
			return JTAG_CORE_NOT_FOUND;
		}
		else
		{
			script_printf(MSG_ERROR,"Pin %s not found\n",pinname);
			return 0;
		}
	}

	script_printf(MSG_ERROR,"Bad/Missing parameter(s) ! : %s\n",line);

	return JTAG_CORE_BAD_PARAMETER;
}

static int cmd_set_mdio_mdio_pin( jtag_core * jc, char * line)
{
	int i,j,id;
	char dev_index[DEFAULT_BUFLEN];
	char pinname[DEFAULT_BUFLEN];

	i = get_param(line, 1,dev_index);
	j = get_param(line, 2,pinname);

	if(i>=0 && j>=0)
	{
		id = jtagcore_get_pin_id(jc, atoi(dev_index), pinname);

		if(id>=0)
		{
			jtagcore_mdio_set_mdio_pin(jc, atoi(dev_index), id);
			script_printf(MSG_INFO_0,"MDIO set to Pin %s\n",pinname);
			return JTAG_CORE_NOT_FOUND;
		}
		else
		{
			script_printf(MSG_ERROR,"Pin %s not found\n",pinname);
			return 0;
		}
	}

	script_printf(MSG_ERROR,"Bad/Missing parameter(s) ! : %s\n",line);

	return JTAG_CORE_BAD_PARAMETER;
}

static int cmd_do_mdio_wr( jtag_core * jc, char * line)
{
	// jtag_mdio_wr 01 04 EAAC
	int i,j,k,mdioadr,regadr,datatowrite;
	char address[DEFAULT_BUFLEN];
	char reg[DEFAULT_BUFLEN];
	char data[DEFAULT_BUFLEN];
	int ret;

	i = get_param(line, 1,address);
	j = get_param(line, 2,reg);
	k = get_param(line, 3,data);

	if(i>=0 && j>=0 && k>=0)
	{
		mdioadr = strtol(address,0,16);
		regadr = strtol(reg,0,16);
		datatowrite = strtol(data, 0, 16);

		ret = jtagcore_mdio_write(jc, mdioadr, regadr, datatowrite);
		if( ret < 0 )
		{
			script_printf(MSG_ERROR,"Code %d !\n",ret);
			return 0;
		}

		script_printf(MSG_INFO_0,"WR MDIO 0x%.2X : [0x%.2X] = 0x%.4X\n", mdioadr ,regadr, datatowrite);

		return JTAG_CORE_NOT_FOUND;
	}

	script_printf(MSG_ERROR,"Bad/Missing parameter(s) ! : %s\n",line);

	return JTAG_CORE_BAD_PARAMETER;
}

static int cmd_do_mdio_rd( jtag_core * jc, char * line)
{
	// jtag_mdio_rd 01 04
	int i,j,mdioadr,regadr,dataread;
	char address[DEFAULT_BUFLEN];
	char reg[DEFAULT_BUFLEN];

	i = get_param(line, 1,address);
	j = get_param(line, 2,reg);

	if(i>=0 && j>=0)
	{
		mdioadr = strtol(address,0,16);
		regadr = strtol(reg,0,16);

		dataread = jtagcore_mdio_read(jc, mdioadr, regadr);
		if( dataread < 0 )
		{
			script_printf(MSG_ERROR,"Code %d !\n",dataread);
			return 0;
		}

		script_printf(MSG_INFO_0,"RD MDIO 0x%.2X : [0x%.2X] = 0x%.4X\n", mdioadr ,regadr, dataread);

		return JTAG_CORE_NOT_FOUND;
	}

	script_printf(MSG_ERROR,"Bad/Missing parameter(s) ! : %s\n",line);

	return JTAG_CORE_BAD_PARAMETER;
}

/////////////////////////////////////////////////////////////////////////////////////////
// SPI Commands
/////////////////////////////////////////////////////////////////////////////////////////
static int cmd_set_spi_cs_pin( jtag_core * jc, char * line)
{
	int i,j,k,id;
	char dev_index[DEFAULT_BUFLEN];
	char pinname[DEFAULT_BUFLEN];
	char polarity[DEFAULT_BUFLEN];

	i = get_param(line, 1,dev_index);
	j = get_param(line, 2,pinname);
	k = get_param(line, 3,polarity);

	if(i>=0 && j>=0 && k>=0)
	{
		id = jtagcore_get_pin_id(jc, atoi(dev_index), pinname);

		if(id>=0)
		{
			jtagcore_spi_set_cs_pin(jc, atoi(dev_index), id, atoi(polarity));
			script_printf(MSG_INFO_0,"CS set to Pin %s with polarity %d\n",pinname,atoi(polarity));
			return JTAG_CORE_NOT_FOUND;
		}
		else
		{
			script_printf(MSG_ERROR,"Pin %s not found\n",pinname);
			return 0;
		}
	}

	script_printf(MSG_ERROR,"Bad/Missing parameter(s) ! : %s\n",line);

	return JTAG_CORE_BAD_PARAMETER;
}

static int cmd_set_spi_clk_pin( jtag_core * jc, char * line)
{
	int i,j,k,id;
	char dev_index[DEFAULT_BUFLEN];
	char pinname[DEFAULT_BUFLEN];
	char polarity[DEFAULT_BUFLEN];

	i = get_param(line, 1,dev_index);
	j = get_param(line, 2,pinname);
	k = get_param(line, 3,polarity);

	if(i>=0 && j>=0 && k>=0)
	{
		id = jtagcore_get_pin_id(jc, atoi(dev_index), pinname);

		if(id>=0)
		{
			jtagcore_spi_set_clk_pin(jc, atoi(dev_index), id, atoi(polarity));
			script_printf(MSG_INFO_0,"CLK set to Pin %s with polarity %d\n",pinname,atoi(polarity));
			return JTAG_CORE_NOT_FOUND;
		}
		else
		{
			script_printf(MSG_ERROR,"Pin %s not found\n",pinname);
			return 0;
		}
	}

	script_printf(MSG_ERROR,"Bad/Missing parameter(s) ! : %s\n",line);

	return JTAG_CORE_BAD_PARAMETER;
}

static int cmd_set_spi_mosi_pin( jtag_core * jc, char * line)
{
	int i,j,k,id;
	char dev_index[DEFAULT_BUFLEN];
	char pinname[DEFAULT_BUFLEN];
	char phase[DEFAULT_BUFLEN];

	i = get_param(line, 1,dev_index);
	j = get_param(line, 2,pinname);
	k = get_param(line, 3,phase);

	if(i>=0 && j>=0 && k>=0)
	{
		id = jtagcore_get_pin_id(jc, atoi(dev_index), pinname);

		if(id>=0)
		{
			jtagcore_spi_set_mosi_pin(jc, atoi(dev_index), id, atoi(phase));
			script_printf(MSG_INFO_0,"MOSI set to Pin %s with polarity %d\n",pinname,atoi(phase));
			return JTAG_CORE_NOT_FOUND;
		}
		else
		{
			script_printf(MSG_ERROR,"Pin %s not found\n",pinname);
			return 0;
		}
	}

	script_printf(MSG_ERROR,"Bad/Missing parameter(s) ! : %s\n",line);

	return JTAG_CORE_BAD_PARAMETER;
}

static int cmd_set_spi_miso_pin( jtag_core * jc, char * line)
{
	int i,j,k,id;
	char dev_index[DEFAULT_BUFLEN];
	char pinname[DEFAULT_BUFLEN];
	char phase[DEFAULT_BUFLEN];

	i = get_param(line, 1,dev_index);
	j = get_param(line, 2,pinname);
	k = get_param(line, 3,phase);

	if(i>=0 && j>=0 && k>=0)
	{
		id = jtagcore_get_pin_id(jc, atoi(dev_index), pinname);

		if(id>=0)
		{
			jtagcore_spi_set_miso_pin(jc, atoi(dev_index), id, atoi(phase));
			script_printf(MSG_INFO_0,"MISO set to Pin %s with polarity %d\n",pinname,atoi(phase));
			return JTAG_CORE_NOT_FOUND;
		}
		else
		{
			script_printf(MSG_ERROR,"Pin %s not found\n",pinname);
			return 0;
		}
	}

	script_printf(MSG_ERROR,"Parameters error: %s\n",line);

	return JTAG_CORE_BAD_PARAMETER;
}

static int cmd_spi_rd_wr( jtag_core * jc, char * line)
{
	int i,j,k,size;
	char data_out_txt[DEFAULT_BUFLEN];
	unsigned char data_out[DEFAULT_BUFLEN];
	unsigned char data_in[DEFAULT_BUFLEN];
	char lsbfirst[DEFAULT_BUFLEN];
	char tmp_buffer[3];
	int ret;

	// jtag_spi_rd_wr 00123344 1  (DATA LSBFirst)
	i = get_param(line, 1,data_out_txt);
	j = get_param(line, 2,lsbfirst);

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
			data_out[k] = (char)strtol(tmp_buffer,0,16);
		}

		ret = jtagcore_spi_write_read(jc, size,data_out,data_in, 0);
		if( ret < 0 )
		{
			script_printf(MSG_ERROR,"Code %d !\n",ret);
			return 0;
		}

		script_printf(MSG_INFO_0,"SPI TX:");
		for(k = 0; k<size; k++)
		{
			script_printf(MSG_NONE," %.2X",data_out[k]);
		}
		script_printf(MSG_NONE,"\n");

		script_printf(MSG_INFO_0,"SPI RX:");
		for(k = 0; k<size; k++)
		{
			script_printf(MSG_NONE," %.2X",data_in[k]);
		}
		script_printf(MSG_NONE,"\n");

		return JTAG_CORE_NOT_FOUND;
	}

	script_printf(MSG_ERROR,"Parameters error: %s\n",line);

	return JTAG_CORE_BAD_PARAMETER;
}

static int cmd_get_pins_list( jtag_core * jc, char * line)
{
	int i,j,nb_of_pins;
	char dev_index[DEFAULT_BUFLEN];
	char pinname[DEFAULT_BUFLEN];
	int type;

	i = get_param(line, 1,dev_index);
	if(i>=0)
	{
		nb_of_pins = jtagcore_get_number_of_pins(jc,atoi(dev_index));
		if(nb_of_pins>=0)
		{
			script_printf(MSG_INFO_0,"Device %d : %d pin(s)\n",atoi(dev_index),nb_of_pins);
			for(j = 0;j < nb_of_pins;j++)
			{
				if(jtagcore_get_pin_properties(jc, atoi(dev_index), j, pinname, sizeof(pinname), &type) == JTAG_CORE_NO_ERROR)
				{
					script_printf(MSG_NONE,"%s : ",pinname);
					if(type & JTAG_CORE_PIN_IS_INPUT)
					{
						script_printf(MSG_NONE," in  ");
					}
					else
					{
						script_printf(MSG_NONE,"     ");
					}

					if(type & JTAG_CORE_PIN_IS_OUTPUT)
					{
						script_printf(MSG_NONE," out ");
					}
					else
					{
						script_printf(MSG_NONE,"     ");
					}

					if(type & JTAG_CORE_PIN_IS_TRISTATES)
					{
						script_printf(MSG_NONE," tris");
					}
					else
					{
						script_printf(MSG_NONE,"     ");
					}

					script_printf(MSG_NONE,"\n");

				}
			}
		}
	}

	return 0;
}


static int cmd_help( jtag_core * jc, char * line);

static int cmd_version( jtag_core * jc, char * line)
{
	script_printf(MSG_INFO_0,"Lib version : %s, Date : "__DATE__" "__TIME__"\n",LIB_JTAG_CORE_VERSION);
	return 1;
}

static int set_env_var_cmd( jtag_core * jc, char * line )
{
	int i,j,ret;
	char varname[DEFAULT_BUFLEN];
	char varvalue[DEFAULT_BUFLEN];

	ret = -1;

	i = get_param(line, 1,varname);
	j = get_param(line, 2,varvalue);

	if(i>=0 && j>=0)
	{
		ret = jtagcore_setEnvVar( jc, (char*)&varname, (char*)&varvalue );
	}

	return ret;
}

static int print_env_var_cmd( jtag_core * jc, char * line )
{
	int i,ret;
	char varname[DEFAULT_BUFLEN];
	char varvalue[DEFAULT_BUFLEN];
	char * ptr;

	ret = -1;

	i = get_param(line, 1,varname);

	if(i>=0)
	{
		ptr = jtagcore_getEnvVar( jc, (char*)&varname, (char*)&varvalue );
		if(ptr)
		{
			script_printf(MSG_INFO_1,"%s = %s",varname,varvalue);
		}
	}

	return ret;
}

cmd_list cmdlist[] =
{
	{"print",                   cmd_print},
	{"help",                    cmd_help},
	{"?",                       cmd_help},
	{"version",                 cmd_version},
	{"pause",                   cmd_pause},

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

	{"set",                     set_env_var_cmd},
	{"print_env_var_cmd",       print_env_var_cmd},

	{0 , 0}
};

static int extract_cmd(char * line, char * command)
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

int exec_cmd( jtag_core * jc, char * command,char * line)
{
	int i;

	i = 0;
	while(cmdlist[i].func)
	{
		if( !strcmp(cmdlist[i].command,command) )
		{
			cmdlist[i].func(jc,line);
			return 1;
		}

		i++;
	}

	return JTAG_CORE_CMD_NOT_FOUND;
}

static int cmd_help( jtag_core * jc, char * line)
{
	int i;

	script_printf(MSG_INFO_0,"Supported Commands :\n\n");

	i = 0;
	while(cmdlist[i].func)
	{
		script_printf(MSG_NONE,"%s\n",cmdlist[i].command);
		i++;
	}

	return 1;
}

int jtagcore_execScriptLine( jtag_core * jc, char * line )
{
	char command[DEFAULT_BUFLEN];

	command[0] = 0;

	if( extract_cmd(line, command) )
	{
		if(strlen(command))
		{
			if(exec_cmd(jc,command,line) == JTAG_CORE_CMD_NOT_FOUND )
			{
				script_printf(MSG_ERROR,"Command not found ! : %s\n",line);

				return 0;
			}
		}

		return 1;
	}

	return 0;
}

int jtagcore_execScriptFile( jtag_core * jc, char * script_path )
{
	int err;
	FILE * f;
	char line[DEFAULT_BUFLEN];

	err = JTAG_CORE_INTERNAL_ERROR;

	f = fopen(script_path,"r");
	if(f)
	{
		do
		{
			if(!fgets(line,sizeof(line),f))
				break;

			if(feof(f))
				break;

			jtagcore_execScriptLine(jc, line);
		}while(1);

		fclose(f);

		err = JTAG_CORE_NOT_FOUND;
	}
	else
	{
		script_printf(MSG_ERROR,"Can't open %s !",script_path);

		err = JTAG_CORE_ACCESS_ERROR;
	}

	return err;
}

int jtagcore_execScriptRam( jtag_core * jc, unsigned char * script_buffer, int buffersize )
{
	int err = 0;
	int buffer_offset,line_offset;
	char line[DEFAULT_BUFLEN];

	buffer_offset = 0;
	line_offset = 0;

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

		jtagcore_execScriptLine(jc, line);

		if( (buffer_offset >= buffersize) || !script_buffer[buffer_offset])
			break;

	}while(buffer_offset < buffersize);

	return err;
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
