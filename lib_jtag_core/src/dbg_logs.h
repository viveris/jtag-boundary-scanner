/*
 * JTAG Core library
 * Copyright (c) 2008-2019 Viveris Technologies
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

enum MSGTYPE
{
	MSG_INFO_0 = 0,
	MSG_INFO_1,
	MSG_WARNING,
	MSG_ERROR,
	MSG_DEBUG
};

int jtagcore_logs_printf(jtag_core * jc, int MSGTYPE, char * chaine, ...);
