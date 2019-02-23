/*
 * JTAG Boundary Scanner
 * Copyright (c) 2008 - 2019 Viveris Technologies
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
* @file   fileselector.c
* @brief  Win32 file selector
* @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
*/

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include "fileselector.h"

int fileselector(HWND hWnd,char rw,char multi,char * files,char * title,char* selector,char * defext)
{
	OPENFILENAME sfile;

	memset(&sfile,sizeof(sfile),0);
	sfile.lStructSize = sizeof(OPENFILENAME);
	sfile.hwndOwner = hWnd;
	sfile.hInstance = GetModuleHandle(NULL);
	sfile.lpstrCustomFilter = NULL;
	sfile.nFilterIndex = 1;
	sfile.lpstrFileTitle = NULL;
	sfile.lpstrInitialDir = NULL;

	sfile.Flags = OFN_PATHMUSTEXIST|OFN_LONGNAMES|OFN_EXPLORER;
	if(rw!=0)
		sfile.Flags = sfile.Flags|OFN_OVERWRITEPROMPT;
	if(multi)
		sfile.Flags = sfile.Flags|OFN_ALLOWMULTISELECT;

	sfile.lpstrDefExt = defext;
	sfile.nMaxFile = 1024;
	sfile.lpstrFilter = selector;
	sfile.lpstrTitle = title;
	sfile.lpstrFile = (char*)files;
	if(rw==0)
	{
		if(GetOpenFileName(&sfile))
		{
			return 1;
		}
		else
		{
			CommDlgExtendedError();
			return 0;
		}
	}
	else
	{
		if(GetSaveFileName(&sfile))
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}
}
