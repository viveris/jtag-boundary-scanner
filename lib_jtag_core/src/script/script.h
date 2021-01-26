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

#define DEFAULT_BUFLEN 1024

// Output Message level
#define MSG_NONE                         0
#define MSG_INFO_0                       1
#define MSG_INFO_1                       2
#define MSG_WARNING                      3
#define MSG_ERROR                        4
#define MSG_DEBUG                        5

typedef int (* PRINTF_FUNC)(int MSGTYPE, char * string, ... );

void setOutputFunc( PRINTF_FUNC ext_printf );
