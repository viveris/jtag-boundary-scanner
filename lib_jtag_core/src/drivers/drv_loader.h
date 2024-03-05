/*
 * JTAG Core library
 * Copyright (c) 2008 - 2024 Viveris Technologies
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
 * @file   drv_loader.h
 * @brief  driver functions definitions
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */
 
enum {
	GET_DRV_ID = 1,
	GET_DRV_DESCRIPTION,
	GET_DRV_FUNCPTR,
	GET_DRV_DETECT
};

typedef int (*DRV_DETECT) (void* jtag_core);
typedef int (*DRV_INIT) (void* jtag_core,int sub_drv,char * params);
typedef int (*DRV_TXRXDATA) (void* jtag_core, unsigned char * str_out, unsigned char * str_in, int size);
typedef int (*DRV_TXTMS) (void* jtag_core, unsigned char * str_out, int size);
typedef int (*DRV_GETMODULEINFOS) (void* jtag_core,int sub_drv,unsigned int infotype, void * returnvalue);
typedef int (*DRV_DEINIT) (void* jtag_core);

typedef struct drv_ptr_
{
	DRV_DETECT    drv_Detect;
	DRV_INIT      drv_Init;
	DRV_DEINIT    drv_DeInit;
	DRV_TXTMS     drv_TX_TMS;
	DRV_TXRXDATA  drv_TXRX_DATA;
	DRV_GETMODULEINFOS drv_Get_ModInfos;
} drv_ptr;

int GetDrvInfo(void * jc_ctx, unsigned long infotype, void * returnvalue, const char * drv_id, const char * drv_desc, drv_ptr * drv_func);