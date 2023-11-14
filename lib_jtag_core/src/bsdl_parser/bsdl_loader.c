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
 * @file   bsdl_loader.c
 * @brief  bsdl file parser
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

#include <stdint.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../drivers/drv_loader.h"
#include "../jtag_core_internal.h"
#include "../jtag_core.h"

#include "bsdl_loader.h"
#include "bsdl_strings.h"

#include "../natsort/strnatcmp.h"

#include "../os_interface/os_interface.h"

#include "../dbg_logs.h"
#define DEBUG 1

char *string_upper(char *str)
{
	while (*str != '\0')
	{
		*str = toupper((unsigned char)*str);
		str++;
	}
	return str;
}

int strcmp_nocase(char *str1,char *str2)
{
	int d;

	for(;; str1++, str2++)
	{
		d = tolower(*str1) - tolower(*str2);

		if(d != 0 || !*str1)
			return d;
	}

	return 0;
}

int getnextvalidline(char *buffer,int buffersize,int * offset)
{
	int l_offset;
	int current_line_offset;

	l_offset = *offset;
	current_line_offset = *offset;

	do
	{
		// Skip all the blank characters
		while( (l_offset < buffersize) &&  (buffer[l_offset] == ' ' || buffer[l_offset] == '\t') )
		{
			l_offset++;
		}

		// End of buffer reached ?
		if(l_offset == buffersize)
		{
			*offset = l_offset;

			return 1;
		}

		// Is it a return or comment ?
		if( buffer[l_offset] != '\r' && buffer[l_offset] != '\n' && (buffer[l_offset] != '-' ||  buffer[l_offset+1] != '-'))
		{
			// No. There is something interesting into this line.
			*offset = current_line_offset;
			return 0;
		}
		else
		{
			// Is it a comment ?
			if(buffer[l_offset] == '-' &&  buffer[l_offset+1] == '-')
			{
				// Yes. Go to the end of the line...
				while(buffer[l_offset] != 0 && buffer[l_offset] != '\r' && buffer[l_offset] != '\n')
				{
					l_offset += 1;
				}
			}
		}

		if( buffer[l_offset] == '\r' || buffer[l_offset] == '\n')
		{
			l_offset++;

			if(l_offset < buffersize)
			{
				if( buffer[l_offset] == '\n' )
				{
					l_offset++;
				}
			}

			current_line_offset = l_offset;
		}
	}while ( (l_offset < buffersize) && buffer[l_offset] );

	*offset = l_offset;

	return 1;
}

char getnextchar(char *buffer,int buffersize,int * offset)
{
	char c;

	if (*offset < buffersize)
	{
		c = buffer[*offset];

		switch (c)
		{
			case 0:
				return 0;
			break;

			case '\r':
				*offset += 1;

				if (*offset < buffersize)
				{
					if (buffer[*offset] == '\n')
					{
						*offset += 1;
					}

					getnextvalidline(buffer, buffersize, offset);
				}

				return ' ';
			break;
			case '\t':
				*offset += 1;
			return ' ';
			break;
			case '\n':
				*offset += 1;
				getnextvalidline(buffer, buffersize, offset);
				return ' ';
			break;
			case '-':
				if (buffer[*offset + 1] == '-')
				{
					while ((*offset < buffersize) && buffer[*offset] != 0 && buffer[*offset] != '\r' && buffer[*offset] != '\n')
					{
						*offset += 1;
					}

					if (*offset < buffersize)
					{
						if (buffer[*offset] == '\r')
						{
							*offset += 1;
							if (buffer[*offset] == '\n')
							{
								*offset += 1;
							}

							getnextvalidline(buffer, buffersize, offset);

							return ' ';
						}

						if (buffer[*offset] == '\n')
						{
							*offset += 1;
							getnextvalidline(buffer, buffersize, offset);

							return ' ';
						}
					}
					else
						return 0;

					return ' ';
				}
				else
				{
					*offset += 1;
					return '-';
				}

			break;

			default:
				*offset += 1;
				return c;
			break;
		}
	}

	return 0;
}

int extract_bsdl_lines(char * bsdl_txt, char ** lines)
{
	int offset;
	int line_start_offset;
	int line_end_offset;
	int endofline;
	int parenthesis_number;
	int inside_quote;
	int number_of_lines;

	offset = 0;
	number_of_lines = 0;

	do
	{
		// Find the first word offset into the line...
		while( bsdl_txt[offset] ==' ' && bsdl_txt[offset] )
		{
			offset++;
		}

		line_start_offset = offset;

		parenthesis_number = 0;
		endofline = 0;
		inside_quote = 0;

		// Scan the line
		while( !endofline )
		{
			switch( bsdl_txt[offset] )
			{
				case 0:
					endofline = 1;
				break;

				case '(':
					if( !inside_quote )
						parenthesis_number++;
					offset++;
				break;
				case ')':
					if(parenthesis_number && !inside_quote)
						parenthesis_number--;
					offset++;
				break;
				case ';':
					if( !parenthesis_number && !inside_quote )
					{
						line_end_offset = offset;

						if( line_end_offset - line_start_offset > 0 )
						{
							if(lines)
							{
								lines[number_of_lines] = malloc( (line_end_offset - line_start_offset) + 2 );
								if( lines[number_of_lines] )
								{
									memset(lines[number_of_lines],0, (line_end_offset - line_start_offset) + 2 );
									memcpy(lines[number_of_lines],&bsdl_txt[line_start_offset], (line_end_offset - line_start_offset) );
								}
							}

							number_of_lines++;
						}

						endofline = 1;

					}
					offset++;
				break;

				case '"':
					if(!inside_quote)
						inside_quote = 1;
					else
						inside_quote = 0;

					offset++;
				break;

				default:
					offset++;
				break;
			}
		}
	}while(bsdl_txt[offset]);

	return number_of_lines;
}

void preprocess_line(char * line)
{
	int inside_quote;
	int read_offset,write_offset;
	int is_string;
	int number_of_spaces;

	inside_quote = 0;
	read_offset = 0;
	write_offset = 0;
	number_of_spaces = 0;

	// first pass : remove extra blank
	while( line[read_offset] )
	{
		switch( line[read_offset] )
		{
			case ' ':

				if(!number_of_spaces || inside_quote)
				{
					line[write_offset] = line[read_offset];
					write_offset++;
				}

				read_offset++;
				number_of_spaces++;

			break;

			case '"':
				if(!inside_quote)
					inside_quote = 1;
				else
					inside_quote = 0;

				line[write_offset] = line[read_offset];
				write_offset++;
				read_offset++;
				number_of_spaces = 0;

			break;

			default:
				line[write_offset] = line[read_offset];
				number_of_spaces = 0;

				write_offset++;
				read_offset++;
			break;

		}
	}

	line[write_offset] = 0;

	// second pass : concatenate strings.

	inside_quote = 0;
	read_offset = 0;
	write_offset = 0;
	number_of_spaces = 0;
	is_string = 0;

	while( line[read_offset] )
	{
		switch( line[read_offset] )
		{

			case '&':
				if( inside_quote || !is_string)
				{
					line[write_offset] = line[read_offset];
					write_offset++;
				}
				read_offset++;
			break;

			case ' ':
				if( inside_quote || !is_string)
				{
					line[write_offset] = line[read_offset];
					write_offset++;
				}
				read_offset++;
			break;

			case '"':
				if(!inside_quote)
				{
					inside_quote = 1;
					if( !is_string )
					{
						line[write_offset] = line[read_offset];
						write_offset++;
					}

					is_string = 1;
				}
				else
				{
					inside_quote = 0;

				}

				read_offset++;
			break;

			default:
				if( !inside_quote )
				{
					if( is_string )
					{
						line[write_offset] = '"';
						write_offset++;
					}
					is_string = 0;
				}

				line[write_offset] = line[read_offset];

				write_offset++;
				read_offset++;
			break;

		}
	}

	if( is_string )
	{
		line[write_offset] = '"';
		write_offset++;
		line[write_offset] = 0;
	}
};

char * check_next_keyword(char * buffer, char * keyword)
{
	int i;

	if( !buffer )
		return 0;

	// skip the current word
	i = 0;
	while (buffer[i] && !strchr("\"() ", buffer[i]) )
	{
		i++;
	}

	// skip the following blank
	while (buffer[i] && strchr("\"() ", buffer[i]) )
	{
		i++;
	}

	if( !strncmp(&buffer[i],keyword,strlen(keyword)) )
	{
		return &buffer[i + strlen(keyword)];
	}

	return 0;
}

int get_next_keyword(jtag_core * jc,char * buffer, char * keyword)
{
	int i,j;

	if( !buffer )
		return 0;

	// skip the current word
	i = 0;
	while (buffer[i] && !strchr("\"():, ", buffer[i]) )
	{
		i++;
	}

	// skip the following blank
	while (buffer[i] && strchr("\"():, ", buffer[i]))
	{
		i++;
	}

	// copy the word
	j = 0;
	while (buffer[i] && !strchr("\"():;, ", buffer[i]))
	{
		if(j < MAX_ELEMENT_SIZE-1)
		{
			keyword[j] = buffer[i];
			j++;
		}
		i++;
	}

	keyword[j] = 0;

	if( j >= (MAX_ELEMENT_SIZE-1) )
	{
		jtagcore_logs_printf(jc,MSG_WARNING,"BSDL loader / get_next_keyword : element too long / truncated : %s\r\n",keyword);
	}

	return i;
}

int get_next_parameter(jtag_core * jc,char * buffer, char * parameter)
{
	int i,j;

	if( !buffer )
		return 0;

	// skip the current word
	i = 0;
	while (buffer[i] && !strchr("\":, ", buffer[i]))
	{
		i++;
	}

	// skip the following blank
	while (buffer[i] && strchr("\":, ", buffer[i]))
	{
		i++;
	}

	// copy the word
	j = 0;
	while (buffer[i] && !strchr("\":, ;", buffer[i]))
	{
		if(j < MAX_ELEMENT_SIZE-1)
		{
			parameter[j] = buffer[i];
			j++;
		}
		i++;
	}

	parameter[j] = 0;

	if( j >= (MAX_ELEMENT_SIZE-1) )
	{
		jtagcore_logs_printf(jc,MSG_WARNING,"BSDL loader / get_next_parameter : element too long / truncated : %s\r\n",parameter);
	}

	return i;
}

int check_next_symbol(char * buffer, char c )
{
	int i;

	if( !buffer )
		return 0;

	// skip the current word
	i = 0;
	while (buffer[i] && !strchr("\"(): ", buffer[i]) && buffer[i] != c)
	{
		i++;
	}

	// skip the following blank
	while(buffer[i] && ( buffer[i] == ' ') )
	{
		i++;
	}

	if(  buffer[i] == c )
	{
		return i;
	}
	else
	{
		return -1;
	}
}

char * get_attribut(char ** lines,char * name, char * entity)
{
	int i,j;
	char * ptr;
	i = 0;

	do
	{
		if(!strncmp(lines[i],"attribute ",10))
		{
			if(!strncmp(&lines[i][10],name,strlen(name)))
			{
				ptr = check_next_keyword(&lines[i][10], "of");
				ptr = check_next_keyword(ptr, entity);

				if( ptr )
				{
					j = 0;
					while( ptr[j] && ptr[j] != ':' && ptr[j] == ' ')
					{
						j++;
					}

					if( ptr[j] == ':' )
					{
						ptr = check_next_keyword(&ptr[j], "entity");
						ptr = check_next_keyword(ptr, "is");

						return ptr;

					}
				}

				return 0;
			}
		}
		i++;
	}while( lines[i] );

	return 0;
}

char * get_attribut_txt(char ** lines,char * name, char * entity)
{
	int i;
	char * attribut_data;

	attribut_data = get_attribut(lines,name, entity);

	if( attribut_data )
	{
		i = 0;
		while( attribut_data[i] && attribut_data[i]!='"')
		{
			i++;
		}

		if( attribut_data[i] == '"' )
		{
			return &attribut_data[i];
		}

		return 0;
	}

	return 0;
}

int get_attribut_int(char ** lines,char * name, char * entity)
{
	int i;
	char * attribut_data;

	attribut_data = get_attribut(lines,name, entity);

	if( attribut_data )
	{
		i = 0;
		while( attribut_data[i] && !( attribut_data[i]>='0' && attribut_data[i]<='9' ) )
		{
			i++;
		}

		if( ( attribut_data[i]>='0' && attribut_data[i]<='9' ) )
		{
			return atoi(&attribut_data[i]);
		}

		return 0;
	}

	return 0;
}

int get_next_pin(jtag_core * jc,char * name,int * type, char *line, int * start_index, int * end_index )
{
	int i;
	int io_list_offset;
	char tmp_str[256];
	int line_parsed,inside_block;

	i = get_next_keyword(jc,line,name);

	io_list_offset = 0;

	while(line[i] == ' ')
		i++;

	if( line[i] == ':' || line[i] == ',' )
	{
		*start_index = 0;
		*end_index = 0;

		if( line[i] == ',' )
		{
			io_list_offset = i + 1;

			while(line[i] != ':' && line[i] != ';' && line[i])
				i++;
			if( line[i] != ':' )
				return 0;
		}

		i += get_next_keyword(jc,&line[i],(char*)&tmp_str);

		string_upper(tmp_str);

		*type = get_typecode(pintype_str,tmp_str);

		i += get_next_keyword(jc,&line[i],(char*)&tmp_str);

		if (!strcmp_nocase("bit_vector",tmp_str))
		{
			while(line[i] != '(' && line[i] != ')' && line[i] != ';')
			{
				i++;
			}

			line_parsed = 0;
			inside_block = 0;

			do
			{

				switch( line[i] )
				{
					case '(':
						inside_block++;
						if(!line_parsed)
						{
							i += get_next_keyword(jc,&line[i],(char*)&tmp_str);
							*start_index = atoi(tmp_str);

							i += get_next_keyword(jc,&line[i],(char*)&tmp_str);

							i += get_next_keyword(jc,&line[i],(char*)&tmp_str);
							*end_index = atoi(tmp_str);

							line_parsed = 1;

						}
					break;
					case ')':
						if ( !inside_block )
						{
							return 0;
						}

						inside_block--;
					break;
					case ';':
						if( io_list_offset )
							return io_list_offset;
						else
							return i;
					break;
					case 0:
					break;

				}
				i++;
			}while (line[i]);

			return 0;
		}

		if (!strcmp_nocase("bit",tmp_str))
		{
			do
			{
				switch( line[i] )
				{
					case ';':
						if( io_list_offset )
							return io_list_offset;
						else
							return i;
					break;
					case 0:
					break;
				}
				i++;
			}while (line[i]);
		}

	}

	return 0;
}

int get_pins_list(jtag_core * jc,jtag_bsdl * bsdl_desc,char ** lines)
{
	int i,j,k,inc,offset, number_of_pins,vector_size;
	char tmp_str[MAX_ELEMENT_SIZE];
	int tmp_type,tmp_start,tmp_end;
	i = 0;

	while( lines[i] )
	{
		if( !strncmp(lines[i],"port ",5) || !strncmp(lines[i],"port(",5))
		{
			j = 0;
			number_of_pins = 0;

			while( lines[i][j] != '(' && lines[i][j])
			{
				j++;
			}

			do
			{
				offset = get_next_pin(jc,(char*)&tmp_str,&tmp_type, &lines[i][j], &tmp_start, &tmp_end );

				if( tmp_start <=tmp_end)
				{
					number_of_pins += ( (tmp_end - tmp_start) + 1 );
				}
				else
				{
					number_of_pins += ( (tmp_start - tmp_end) + 1 );
				}

				j += offset;

			} while( offset);

			if (number_of_pins == 0 || ( number_of_pins > MAX_NUMBER_PINS_PER_DEV ) )
			{
				jtagcore_logs_printf(jc, MSG_ERROR, "get_pins_list : bad number of pin found ! : %d\r\n", number_of_pins);
				return -1;
			}

			bsdl_desc->pins_list =  malloc (sizeof(pin_ctrl) * (number_of_pins+1));
			if( bsdl_desc->pins_list )
			{
				memset( bsdl_desc->pins_list, 0, sizeof(pin_ctrl) * (number_of_pins+1) );

				j = 0;
				number_of_pins = 0;

				while( lines[i][j] != '(' && lines[i][j])
				{
					j++;
				}

				do
				{
					offset = get_next_pin(jc,(char*)&tmp_str,&tmp_type, &lines[i][j], &tmp_start, &tmp_end );

					if( tmp_start <= tmp_end )
					{
						vector_size = ( (tmp_end - tmp_start) + 1 );
						inc = 1;
					}
					else
					{
						vector_size = ( (tmp_start - tmp_end) + 1 );
						inc = -1;
					}

					j += offset;

					for(k = 0;k < vector_size; k++)
					{
						snprintf((char*)&bsdl_desc->pins_list[number_of_pins].pinname,sizeof(((pin_ctrl *)0)->pinname),"%s",(char*)tmp_str);

						if( vector_size > 1 )
						{
							char digistr[32];
							snprintf((char*)digistr,sizeof(digistr),"(%d)",tmp_start);
							genos_strndstcat((char*)&bsdl_desc->pins_list[number_of_pins].pinname,digistr,sizeof(((pin_ctrl *)0)->pinname));
						}
						bsdl_desc->pins_list[number_of_pins].pinname[sizeof(((pin_ctrl *)0)->pinname)-1] = '\0';

						bsdl_desc->pins_list[number_of_pins].pintype = tmp_type;
						number_of_pins++;
						tmp_start += inc;
					}

				} while( offset );

				bsdl_desc->number_of_pins = number_of_pins;

				return number_of_pins;
			}
		}
		i++;
	};

	return 0;
}

int get_jtag_chain(jtag_core * jc,jtag_bsdl * bsdl_desc,char ** lines, char * entityname)
{
	char * jtagchain_str;
	int i,j,bit_count,end_parse;
	char tmp_str[MAX_ELEMENT_SIZE];
	int bit_index;

	bit_count = 0;
	bsdl_desc->number_of_chainbits = get_attribut_int(lines,"BOUNDARY_LENGTH", entityname);
	if ( ( bsdl_desc->number_of_chainbits > 0 ) && ( bsdl_desc->number_of_chainbits < MAX_NUMBER_BITS_IN_CHAIN ) )
	{
		jtagcore_logs_printf(jc,MSG_DEBUG,"Number of bit in the chain = %d\r\n",bsdl_desc->number_of_chainbits);

		bsdl_desc->chain_list = malloc( sizeof(jtag_chain) * bsdl_desc->number_of_chainbits );
		if( !bsdl_desc->chain_list )
		{
			jtagcore_logs_printf(jc,MSG_ERROR,"get_jtag_chain : memory alloc error !\r\n");
			return -1;
		}

		memset(bsdl_desc->chain_list,0, sizeof(jtag_chain) * bsdl_desc->number_of_chainbits );

		jtagchain_str = get_attribut_txt(lines,"BOUNDARY_REGISTER", entityname);

		end_parse = 0;

		if(jtagchain_str)
		{
			if( jtagchain_str[0] == '"' )
			{
				i = 0;
				while( jtagchain_str[i] && !end_parse )
				{
					jtagcore_logs_printf(jc,MSG_DEBUG,"--------------\r\n");

					// Get index
					i += get_next_keyword(jc,&jtagchain_str[i], tmp_str);
					bit_index = atoi(tmp_str);
					if (bit_index >= 0  && bit_index < bsdl_desc->number_of_chainbits)
					{
						bsdl_desc->chain_list[bit_index].bit_index = bit_index;

						jtagcore_logs_printf(jc, MSG_DEBUG, "%d\r\n", bsdl_desc->chain_list[bit_index].bit_index);

						// Look for  (
						while (jtagchain_str[i] != '(' && jtagchain_str[i])
							i++;

						// Get the pin type
						i += get_next_keyword(jc,&jtagchain_str[i], tmp_str);
						string_upper(tmp_str);
						bsdl_desc->chain_list[bit_index].bit_cell_type = get_typecode(celltype_str, tmp_str);

						jtagcore_logs_printf(jc, MSG_DEBUG, "%d\r\n", bsdl_desc->chain_list[bit_index].bit_cell_type);

						// ,
						j = check_next_symbol(&jtagchain_str[i], ',');
						if (j < 0)
							return -1;
						i += j;

						// Get the name
						i += get_next_parameter(jc,&jtagchain_str[i], bsdl_desc->chain_list[bit_index].pinname);

						jtagcore_logs_printf(jc, MSG_DEBUG, "%s\r\n", bsdl_desc->chain_list[bit_index].pinname);

						// ,
						j = check_next_symbol(&jtagchain_str[i], ',');
						if (j < 0)
							return -1;
						i += j;

						// Get the type
						i += get_next_keyword(jc,&jtagchain_str[i], tmp_str);
						string_upper(tmp_str);
						bsdl_desc->chain_list[bit_index].bit_type = get_typecode(bittype_str, tmp_str);

						jtagcore_logs_printf(jc, MSG_DEBUG, "%s , %d\r\n", tmp_str, bsdl_desc->chain_list[bit_index].bit_type);

						// ,
						j = check_next_symbol(&jtagchain_str[i], ',');
						if (j < 0)
							return -1;
						i += j;

						// Get the default state
						i += get_next_keyword(jc,&jtagchain_str[i], tmp_str);
						string_upper(tmp_str);
						bsdl_desc->chain_list[bit_index].safe_state = get_typecode(statetype_str, tmp_str);

						bsdl_desc->chain_list[bit_index].control_bit_index = -1;

						// If no ) Get the ctrl index
						j = check_next_symbol(&jtagchain_str[i], ')');

						if (jtagchain_str[i]==',' || j < 0)
						{
							// ,
							j = check_next_symbol(&jtagchain_str[i], ',');
							if (j < 0)
								return -1;
							i += j;

							// Get the control bit
							i += get_next_keyword(jc,&jtagchain_str[i], tmp_str);

							bsdl_desc->chain_list[bit_index].control_bit_index = atoi(tmp_str);

							j = check_next_symbol(&jtagchain_str[i], ',');
							if (j < 0)
								return -1;
							i += j;

							// Get the polarity
							i += get_next_keyword(jc,&jtagchain_str[i], tmp_str);
							string_upper(tmp_str);
							bsdl_desc->chain_list[bit_index].control_disable_state = get_typecode(statetype_str, tmp_str);

							j = check_next_symbol(&jtagchain_str[i], ',');
							if (j < 0)
								return -1;
							i += j;

							// Get the off state
							i += get_next_keyword(jc,&jtagchain_str[i], tmp_str);
							string_upper(tmp_str);
							bsdl_desc->chain_list[bit_index].control_disable_result = get_typecode(statetype_str, tmp_str);

							// Find and skip )
							j = check_next_symbol(&jtagchain_str[i], ')');
							if (j < 0)
								return -1;
						}

						i += j;

						// Find , (Next bit) or " (End of the chain)
						i++;
						bit_count++;

						if (check_next_symbol(&jtagchain_str[i], '"') >= 0)
						{
							end_parse = 1;
						}
					}
					else
					{
						jtagcore_logs_printf(jc, MSG_ERROR, "get_jtag_chain : Error bit index overrun !\r\n");
					}
				}
			}
		}
	}

	if( ( bit_count == bsdl_desc->number_of_chainbits ) && ( bsdl_desc->number_of_chainbits != 0 ) )
	{
		jtagcore_logs_printf(jc,MSG_DEBUG,"Jtag chain parsing Ok !\r\n");

		for( i = 0 ; i < (long)bsdl_desc->number_of_pins ; i++ )
		{

			bsdl_desc->pins_list[i].in_bit_number   = -1;
			bsdl_desc->pins_list[i].out_bit_number  = -1;
			bsdl_desc->pins_list[i].ctrl_bit_number = -1;

			switch( bsdl_desc->pins_list[i].pintype)
			{
				case IO_IN:
				case IO_OUT:
				case IO_INOUT:

					for( j = 0 ; j < (long)bsdl_desc->number_of_chainbits ; j++ )
					{
						if(!strcmp(bsdl_desc->chain_list[j].pinname, bsdl_desc->pins_list[i].pinname))
						{
							switch( bsdl_desc->chain_list[j].bit_type)
							{
								case BITTYPE_INPUT:
									bsdl_desc->pins_list[i].in_bit_number = bsdl_desc->chain_list[j].bit_index;
								break;
								case BITTYPE_OUTPUT:
									bsdl_desc->pins_list[i].out_bit_number = bsdl_desc->chain_list[j].bit_index;
								break;
								case BITTYPE_TRISTATE_OUTPUT:
									bsdl_desc->pins_list[i].out_bit_number  = bsdl_desc->chain_list[j].bit_index;
									bsdl_desc->pins_list[i].ctrl_bit_number = bsdl_desc->chain_list[j].control_bit_index;
								break;
								case BITTYPE_INOUT:
									bsdl_desc->pins_list[i].in_bit_number   = bsdl_desc->chain_list[j].bit_index;
									bsdl_desc->pins_list[i].out_bit_number  = bsdl_desc->chain_list[j].bit_index;
									bsdl_desc->pins_list[i].ctrl_bit_number = bsdl_desc->chain_list[j].control_bit_index;
								break;
							}
						}
					}
				break;
			}
		}

		return 1;
	}
	else
	{
		jtagcore_logs_printf(jc,MSG_DEBUG,"JTAG chain parsing error : %d - %d  !\r\n",bit_count,bsdl_desc->number_of_chainbits);
		return -1;
	}
}

static int compare_pin_name(const void *a, const void *b)
{
	int ret;
	pin_ctrl const *pa = (pin_ctrl const *)a;
	pin_ctrl const *pb = (pin_ctrl const *)b;

	ret = strnatcmp(pa->pinname, pb->pinname);

	return ret;
}

jtag_bsdl * load_bsdlfile(jtag_core * jc,char *filename)
{
	FILE * bsdl_file;
	jtag_bsdl * bsdl;
	int file_size,offset;
	char * bsdl_txt,*tmp_bsdl_txt;
	char * tmp_ptr;
	int i,number_of_bsdl_lines;
	char entityname[256];
	char * chipid_str,* instruct_str;
	char * instruct_strchr;
	char ** lines;

	jtagcore_logs_printf(jc,MSG_INFO_0,"Open BSDL file %s\r\n",filename);

	bsdl = 0;

	bsdl_file = fopen(filename,"rb");
	if (bsdl_file)
	{
		fseek(bsdl_file, 0, SEEK_END);
		file_size = ftell(bsdl_file);
		fseek(bsdl_file, 0, SEEK_SET);

		if ( file_size <= 0 || file_size > MAX_BSDL_FILE_SIZE )
		{
			jtagcore_logs_printf(jc, MSG_ERROR, "Bad BSDL File size ! : %d\r\n", file_size);
			fclose(bsdl_file);
			return 0;
		}

		jtagcore_logs_printf(jc, MSG_DEBUG, "BSDL file size : %d\r\n", file_size);

		bsdl_txt = malloc(file_size + 1);
		if (bsdl_txt)
		{
			memset(bsdl_txt, 0, file_size + 1);
			if( fread(bsdl_txt, file_size, 1, bsdl_file) != 1 )
			{
				free(bsdl_txt);
				jtagcore_logs_printf(jc, MSG_ERROR, "BSDL File read error !\r\n");
				fclose(bsdl_file);
				return 0;
			}

			tmp_bsdl_txt = malloc(file_size + 1);
			if (tmp_bsdl_txt)
			{
				memset(tmp_bsdl_txt, 0, file_size + 1);
				offset = 0;

				i = 0;
				while (offset < file_size &&  i < file_size)
				{
					tmp_bsdl_txt[i] = getnextchar(bsdl_txt, file_size, &offset);
					i++;
				}

				free(bsdl_txt);

				if (offset < i)
				{
					jtagcore_logs_printf(jc, MSG_ERROR, "Preprocessing error !\r\n");
					free(tmp_bsdl_txt);
					return 0;
				}

				bsdl_txt = tmp_bsdl_txt;
			}
			else
			{
				jtagcore_logs_printf(jc, MSG_ERROR, "Can't allocate file memory !\r\n");
				free(bsdl_txt);
				fclose(bsdl_file);
				return 0;
			}
		}
		else
		{
			jtagcore_logs_printf(jc, MSG_ERROR, "Can't allocate file memory !\r\n");
			fclose(bsdl_file);
			return 0;
		}
	}
	else
	{
		jtagcore_logs_printf(jc, MSG_ERROR, "Can't open %s !\r\n", filename);
		return 0;
	}

	fclose(bsdl_file);

	// Get the first entity offset
	tmp_ptr = strstr(bsdl_txt,"entity");
	if (!tmp_ptr)
	{
		jtagcore_logs_printf(jc, MSG_ERROR, "entry \"entity\" not found !\r\n");
		free(bsdl_txt);
		return 0;
	}

	offset = tmp_ptr - bsdl_txt;

	// skip the entity keyword
	while( bsdl_txt[offset] !=' ' && bsdl_txt[offset] )
	offset++;

	// skip the blank spaces
	while( bsdl_txt[offset] ==' ' && bsdl_txt[offset] )
	offset++;

	// copy the entity name
	i = 0;
	memset(entityname,0,sizeof(entityname));
	while( bsdl_txt[offset] !=' ' && bsdl_txt[offset] && i < ( sizeof(entityname) - 1 ))
	{
		entityname[i] = bsdl_txt[offset];

		offset++;
		i++;
	}

	// skip the blank spaces
	while( bsdl_txt[offset] ==' ' && bsdl_txt[offset] )
	offset++;

	// Check the "is" keyword presence
	if( strncmp(&bsdl_txt[offset],"is",2) )
	{
		jtagcore_logs_printf(jc, MSG_ERROR, "Bad entity entry (\"is\" not found)\r\n");
		free(bsdl_txt);
		return 0;
	}

	jtagcore_logs_printf(jc,MSG_DEBUG,"Entity : %s\r\n",entityname);

	offset += 2;

	// extract and separate each bsdl line
	number_of_bsdl_lines = extract_bsdl_lines(&bsdl_txt[offset],0);
	jtagcore_logs_printf(jc,MSG_DEBUG,"lines :%d\r\n",number_of_bsdl_lines);

	if (number_of_bsdl_lines <= 0 || number_of_bsdl_lines > MAX_NUMBER_OF_BSDL_LINES)
	{
		jtagcore_logs_printf(jc, MSG_ERROR, "No line found !\r\n");
		free(bsdl_txt);
		return 0;
	}

 	lines = malloc( sizeof(char*) * ( number_of_bsdl_lines + 1 ) );
	if( !lines )
	{
		jtagcore_logs_printf(jc,MSG_ERROR,"load_bsdlfile : memory alloc error !\r\n");
		free(tmp_bsdl_txt);
		return 0;
	}

	memset( lines, 0,  sizeof(char*) * ( number_of_bsdl_lines + 1 ) );

	extract_bsdl_lines(&bsdl_txt[offset],lines);

	for(i= 0 ;i< number_of_bsdl_lines;i++)
	{
		preprocess_line(lines[i]);
	}

	bsdl =  malloc ( sizeof(jtag_bsdl) );
	if (!bsdl)
	{
		jtagcore_logs_printf(jc, MSG_ERROR, "load_bsdlfile : memory alloc error !\r\n");
		for( i = 0 ;i < number_of_bsdl_lines ; i++ )
		{
			if(lines[i])
			{
				free(lines[i]);
				lines[i] = 0;
			}
		}
		free(lines);

		free(bsdl_txt);

		return 0;
	}

	memset( bsdl , 0 , sizeof(jtag_bsdl) );

	///////////////////////
	// copy the entity name & the file name
	strncpy(bsdl->entity_name,entityname,sizeof(((jtag_bsdl *)0)->entity_name) - 1);
	bsdl->entity_name[ sizeof(((jtag_bsdl *)0)->entity_name) - 1 ] = '\0';

	i = strlen(filename);
	while(i && filename[i] != '\\')
	{
		i--;
	}

	if(filename[i] == '\\')
		i++;

	strncpy(bsdl->src_filename,&filename[i],sizeof(bsdl->src_filename) - 1);
	bsdl->src_filename[ sizeof(bsdl->src_filename) - 1 ] = '0';

	///////////////////////
	// Extract the chip ID
	bsdl->chip_id = 0x00000000;
	chipid_str = get_attribut_txt(lines,"IDCODE_REGISTER", entityname);
	if(chipid_str)
	{
		if( chipid_str[0] == '"' )
		{
			chipid_str++;
			i = 0;
			while(chipid_str[i]!='"' && chipid_str[i]!=';' && chipid_str[i] && i < 32)
			{
				if(chipid_str[i] == '1')
				{
					bsdl->chip_id |= 0x80000000 >> i;
				}
				i++;
			}
		}
	}

	jtagcore_logs_printf(jc,MSG_INFO_0,"ID Code: 0x%.8X\r\n",bsdl->chip_id);

	///////////////////////
	// Extract the pins list
	get_pins_list(jc,bsdl,lines);

	///////////////////////
	// Extract the JTAG Chain
	if(get_jtag_chain(jc,bsdl,lines,entityname)<0)
	{
		jtagcore_logs_printf(jc,MSG_ERROR,"load_bsdlfile : Error during jtag chain parsing !\r\n");
	}

	if(jtagcore_getEnvVarValue( jc, "BSDL_LOADER_SORT_PINS_NAME" ) > 0)
	{
		// Count the pins
		i = 0;
		while(bsdl->pins_list[i].pinname[0])
		{
			i++;
		}

		// Sort them
		qsort(bsdl->pins_list, i, sizeof bsdl->pins_list[0], compare_pin_name);
	}

	i = 0;
	while(bsdl->pins_list[i].pinname[0])
	{
		char dbg_str[512];
		int namelen,j,k;
		char disval;

		namelen = strlen(bsdl->pins_list[i].pinname);

		sprintf(dbg_str,"Pin %s",bsdl->pins_list[i].pinname);
		for(j=0;j<(16 - namelen);j++)
		{
			strcat(dbg_str," ");
		}

		k = strlen(dbg_str);

		disval = 'X';

		if(bsdl->pins_list[i].out_bit_number >=0 )
		{
			if( bsdl->chain_list[bsdl->pins_list[i].out_bit_number].control_disable_state >= 0 )
			{
				disval = '0' + bsdl->chain_list[bsdl->pins_list[i].out_bit_number].control_disable_state;
			}
		}

		sprintf(&dbg_str[k]," type %.2d, ctrl %.3d (disval:%c), out %.3d, in %.3d\r\n",
								bsdl->pins_list[i].pintype,
								bsdl->pins_list[i].ctrl_bit_number,
								disval,
								bsdl->pins_list[i].out_bit_number,
								bsdl->pins_list[i].in_bit_number);

		jtagcore_logs_printf(jc,MSG_DEBUG,dbg_str);

		i++;
	}

	///////////////////////
	// Get the instruction code

	bsdl->number_of_bits_per_instruction = get_attribut_int(lines,"INSTRUCTION_LENGTH", entityname);

	jtagcore_logs_printf(jc,MSG_INFO_0,"Instructions lenght : %d\r\n",bsdl->number_of_bits_per_instruction);

	instruct_str = get_attribut_txt(lines,"INSTRUCTION_OPCODE", entityname);
	if(instruct_str)
	{
		instruct_strchr = strstr(instruct_str,"IDCODE");
		if(!instruct_strchr)
			instruct_strchr = strstr(instruct_str,"idcode");
		if(instruct_strchr)
		{
			get_next_keyword(jc,instruct_strchr, bsdl->IDCODE_Instruction);
			jtagcore_logs_printf(jc,MSG_DEBUG,"IDCODE : %s\r\n",bsdl->IDCODE_Instruction);
		}

		instruct_strchr = strstr(instruct_str,"EXTEST");
		if(!instruct_strchr)
			instruct_strchr = strstr(instruct_str,"extest");

		if(instruct_strchr)
		{
			get_next_keyword(jc,instruct_strchr, bsdl->EXTEST_Instruction);
			jtagcore_logs_printf(jc,MSG_DEBUG,"EXTEST : %s\r\n",bsdl->EXTEST_Instruction);
		}

		instruct_strchr = strstr(instruct_str,"BYPASS");
		if(!instruct_strchr)
			instruct_strchr = strstr(instruct_str,"bypass");

		if(instruct_strchr)
		{
			get_next_keyword(jc,instruct_strchr, bsdl->BYPASS_Instruction);
			jtagcore_logs_printf(jc,MSG_DEBUG,"BYPASS : %s\r\n",bsdl->BYPASS_Instruction);
		}

		instruct_strchr = strstr(instruct_str,"SAMPLE");
		if(!instruct_strchr)
			instruct_strchr = strstr(instruct_str,"sample");
		if(instruct_strchr)
		{
			get_next_keyword(jc,instruct_strchr, bsdl->SAMPLE_Instruction);
			jtagcore_logs_printf(jc,MSG_DEBUG,"SAMPLE : %s\r\n",bsdl->SAMPLE_Instruction);
		}
	}

	free( bsdl_txt );

	for( i = 0 ;i < number_of_bsdl_lines ; i++ )
	{
		if(lines[i])
		{
			free(lines[i]);
			lines[i] = 0;
		}
	}
	free(lines);

	jtagcore_logs_printf(jc,MSG_INFO_0,"BSDL file %s loaded and parsed\r\n",filename);

	return bsdl;
}

void unload_bsdlfile(jtag_core * jc, jtag_bsdl * bsdl)
{
	if( bsdl )
	{
		if(bsdl->chain_list)
			free(bsdl->chain_list);

		if(bsdl->pins_list)
			free(bsdl->pins_list);

		free( bsdl );
	}
}
