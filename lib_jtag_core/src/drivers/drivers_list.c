/*
 * JTAG Core library
 * Copyright (c) 2008 - 2023 Viveris Technologies
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
 * @file   drivers_list.h
 * @brief  drivers list
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

#include "drv_loader.h"

#include "../jtag_core_internal.h"
#include "../jtag_core.h"

#include "../bsdl_parser/bsdl_loader.h"

#ifdef WIN32
#include "./ftdi_jtag/ftdi_jtag_drv.h"
#include "./lpt_jtag/lpt_jtag_drv.h"
#endif 

#if defined(__linux__)
#include "./linux_gpio_jtag/linux_gpio_jtag_drv.h"
#endif

#if defined(__linux__) || defined(WIN32)
#include "./jlink_jtag/jlink_jtag_drv.h"
#endif

#include "drivers_list.h"

const drv_entry staticdrvs[] =
{
#ifdef WIN32
	{(DRV_GETMODULEINFOS)drv_FTDI_libGetDrv,0},
#if !defined(_WIN64)
	{(DRV_GETMODULEINFOS)drv_LPT_libGetDrv,0},
	{(DRV_GETMODULEINFOS)drv_LPT_libGetDrv,1},
	{(DRV_GETMODULEINFOS)drv_LPT_libGetDrv,2},
#endif
#endif
#if defined(__linux__) || defined(WIN32)
	{(DRV_GETMODULEINFOS)drv_JLINK_libGetDrv,0},
#endif
#if defined(__linux__)
	{(DRV_GETMODULEINFOS)drv_LinuxGPIO_libGetDrv,0},
#endif
	{(DRV_GETMODULEINFOS)-1,0}
};
