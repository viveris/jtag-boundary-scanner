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
 * @file   bsdl_loader.h
 * @brief  bsdl file parser header
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */
#ifdef __cplusplus
extern "C" {
#endif

#ifdef _jtag_core_
#include "jtag_bsdl.h"
#else
#include "../libjtag_bsdl.h"
#endif

jtag_bsdl * load_bsdlfile(jtag_core * jc,char *filename);
void unload_bsdlfile(jtag_core * jc, jtag_bsdl * bsdl);

#ifdef __cplusplus
}
#endif
