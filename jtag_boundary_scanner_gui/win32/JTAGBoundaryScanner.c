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
* @file   JTAGBoundaryScanner.c
* @brief  Win32 GUI
* @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
*/

#include "resource.h"
#include <fcntl.h>
#include <windows.h>
#include <stdio.h>
#include <io.h>
#include <Commctrl.h>
#include <zmouse.h>

#include "fileselector.h"
#include "jtagboundaryscanner.h"

#include "socket.h"

#include "jtag_core.h"
#include "bsdl_parser/bsdl_loader.h"
#include "script/script.h"

#define BASE_CHECKBOX_ID 0x4000
#define BASE_PINNAME_ID  0x3000
#define BASE_DEV_ID      0x1000
#define BASE_PROBE_ID    0x1800
#define BASE_COL_NAME    0x2000

// Global Variables:
HINSTANCE hInst;                                       // current instance
TCHAR szTitle[MAX_LOADSTRING];                         // The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];                   // The title bar text


// Foward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int, PRECT);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	DialogProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	DialogProc_I2CTOOL(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	DialogProc_SPITOOL(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    DialogProc_MDIOTOOL(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK	DialogProc_MEMTOOL(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	DialogProc_Logs(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

HWND hWnd;
unsigned long timertick = 0;
unsigned long timertickcfg = 0;
unsigned char Debug_Mode;
int scanposition;

graph_desc graph_pin_list;

jtag_bsdl * bsdl_file;
jtag_core * jc;

int nb_objs;
HANDLE objlist[64*1024];

int last_selected_dev_index;

HWND hDlg_logs;

int Printf_script(int MSGTYPE,char * chaine, ...)
{
	if(MSGTYPE!=MSG_DEBUG)
	{
		va_list marker;
		va_start( marker, chaine );

		vprintf(chaine,marker);

		va_end( marker );
	}
    return 0;
}

void AppendText(HWND hEditWnd, LPCTSTR Text)
{
    int idx = GetWindowTextLength(hEditWnd);
    SendMessage(hEditWnd, EM_SETSEL, (WPARAM)idx, (LPARAM)idx);
    SendMessage(hEditWnd, EM_REPLACESEL, 0, (LPARAM)Text);
}

void logs_callback(char * string)
{
	if(hDlg_logs)
		AppendText(GetDlgItem(hDlg_logs, IDC_EDIT1), string);
}

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{

	MSG msg;
	HACCEL hAccelTable;
	RECT WorkArea;
	int port;

	if(strstr(lpCmdLine,"-server:"))
	{
		jc = jtagcore_init();
		if (jc)
		{
			hDlg_logs = 0;

			if( !strstr(lpCmdLine,"hidewindow") )
			{
				openconsole();
			}

			port = atoi(strstr(lpCmdLine,"-server:") + 8);
			printf("Starting jtag server on port %d.\n",port);
			launch_server(port);

			jtagcore_deinit(jc);
		}

		return 0;
	}


	Debug_Mode=1;

	setOutputFunc( Printf_script );

	bsdl_file = 0;

	nb_objs = 0;
	memset(&graph_pin_list, 0, sizeof(graph_pin_list));

	jc = jtagcore_init();
	if (jc)
	{
		hDlg_logs = 0;

		jtagcore_set_logs_callback(jc,logs_callback);
		jtagcore_set_logs_level(jc,4);

		// Initialize global strings
		LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
		LoadString(hInstance, IDC_JTAGTEST, szWindowClass, MAX_LOADSTRING);
		MyRegisterClass(hInstance);

		// Get System Parameters to retreive working area size.
		SystemParametersInfo(
			SPI_GETWORKAREA,
			0,
			&WorkArea,
			0
			);

		// Perform application initialization:
		if (!InitInstance(hInstance, nCmdShow, &WorkArea))
		{
			return FALSE;
		}

		SetTimer(hWnd, 1, 20, NULL);
		timertick = 0xFFFFFFFF;
		timertickcfg = 0xFFFFFFFF;
		hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_JTAGTEST);

		// Main message loop:
		while (GetMessage(&msg, NULL, 0, 0))
		{
			if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}

		jtagcore_deinit(jc);

		return msg.wParam;
	}

	return -1;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage is only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
 	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_JTAGTEST);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)COLOR_APPWORKSPACE+1;// CreateSolidBrush(RGB(236,233,216)); //(HBRUSH)(COLOR_WINDOWFRAME);
	wcex.lpszMenuName	= (LPCSTR)IDC_JTAGTEST;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);
	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HANDLE, int, PRECT)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow, PRECT lpWorkArea)
{
	hInst = hInstance; // Store instance handle in our global variable


	hWnd=CreateWindowEx(
		0,
		szWindowClass,
		szTitle,
		WS_OVERLAPPEDWINDOW | WS_VSCROLL,
		lpWorkArea->left,
		lpWorkArea->top,
		lpWorkArea->right,
		lpWorkArea->bottom,
		NULL,
		NULL,
		hInstance,
		NULL
	);

	if (!hWnd)
	{
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}

void openconsole()
{
	int hCrt;
	HANDLE handle_out;
	HANDLE handle_in;
	FILE* hf_out;
	FILE* hf_in;

	AllocConsole();

	handle_out = GetStdHandle(STD_OUTPUT_HANDLE);
	hCrt = _open_osfhandle((long)handle_out, _O_TEXT);
	hf_out = _fdopen(hCrt, "w");
	setvbuf(hf_out, NULL, _IONBF, 1);
	*stdout = *hf_out;

	handle_in = GetStdHandle(STD_INPUT_HANDLE);
	hCrt = _open_osfhandle((long)handle_in, _O_TEXT);
	hf_in = _fdopen(hCrt, "r");
	setvbuf(hf_in, NULL, _IONBF, 128);
	*stdin = *hf_in;
}

void closeconsole()
{
	FreeConsole();
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

static char * get_id_str(int numberofdevice)
{
	// compare passed device ID to the one returned from the ID command
	int i;
	unsigned long idcode = 0;
	unsigned char * stringbuffer;
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

int build_checkbox_map(HWND hWnd,int device)
{
	RECT rt;
	int i,j, ckeckboxnb, pin_id;
	SCROLLINFO si;
	int number_of_pins;
	char tmp_name[128];
	int tmp_type,state;

	number_of_pins = jtagcore_get_number_of_pins(jc, device);
	if (number_of_pins)
	{
		// destroy previous checkbox
		for (i = 0; i < nb_objs; i++)
		{
			DestroyWindow(objlist[i]);
		}

		// Initialise graph_desc structure.
		// Find max bits_name len.

		nb_objs = 0;

		graph_pin_list.NbCharPerPinName = 0;
		for (pin_id = 0; pin_id < number_of_pins; pin_id++)
		{
			tmp_name[0] = 0;
			jtagcore_get_pin_properties(jc, device, pin_id, tmp_name, sizeof(tmp_name), 0);

			if (strlen(tmp_name) > graph_pin_list.NbCharPerPinName)
			{
				graph_pin_list.NbCharPerPinName = strlen(tmp_name);
			}
		}
		graph_pin_list.NbCharPerPinName++;

		GetClientRect(hWnd, &rt);

		graph_pin_list.NbPixPerPinName = NB_PIX_PER_CHAR_H * graph_pin_list.NbCharPerPinName;
		graph_pin_list.NbPixPerCol = graph_pin_list.NbPixPerPinName + (NB_PIX_PER_CHECKBOX_H * NB_CHECKBOX) + NB_PIX_PER_CHAR_H;
		graph_pin_list.NbCol = (rt.right - rt.left) / graph_pin_list.NbPixPerCol;
		graph_pin_list.NbLines = (rt.bottom - rt.top) / NB_PIX_PER_CHECKBOX_V;

		// colon names
		for (i = 0; i < graph_pin_list.NbCol; i++)
		{
			objlist[nb_objs++] = CreateWindowEx(
						0, (LPCSTR)"STATIC", (LPCSTR)"O", WS_VISIBLE | WS_CHILD,
						0 + (i*graph_pin_list.NbPixPerCol) + graph_pin_list.NbPixPerPinName + (20 * 0), 0, NB_PIX_PER_CHECKBOX_H, NB_PIX_PER_CHECKBOX_V, hWnd, (HMENU)(BASE_COL_NAME + 0 + (i * 4)), NULL, NULL
					);

			objlist[nb_objs++] = CreateWindowEx(
						0, (LPCSTR)"STATIC", (LPCSTR)"OE", WS_VISIBLE | WS_CHILD,
						0 + (i*graph_pin_list.NbPixPerCol) + graph_pin_list.NbPixPerPinName + (15 * 1), 0, NB_PIX_PER_CHECKBOX_H, NB_PIX_PER_CHECKBOX_V, hWnd, (HMENU)(BASE_COL_NAME + 1 + (i * 4)), NULL, NULL
					);

			objlist[nb_objs++] = CreateWindowEx(
						0, (LPCSTR)"STATIC", (LPCSTR)"I", WS_VISIBLE | WS_CHILD,
						0 + (i*graph_pin_list.NbPixPerCol) + graph_pin_list.NbPixPerPinName + (22 * 2), 0, NB_PIX_PER_CHECKBOX_H, NB_PIX_PER_CHECKBOX_V, hWnd, (HMENU)(BASE_COL_NAME + 2 + (i * 4)), NULL, NULL
					);

			objlist[nb_objs++] = CreateWindowEx(
						0, (LPCSTR)"STATIC", (LPCSTR)"T", WS_VISIBLE | WS_CHILD,
						0 + (i*graph_pin_list.NbPixPerCol) + graph_pin_list.NbPixPerPinName + (21 * 3), 0, NB_PIX_PER_CHECKBOX_H, NB_PIX_PER_CHECKBOX_V, hWnd, (HMENU)(BASE_COL_NAME + 3 + (i * 4)), NULL, NULL
					);
		}

		// Checkbox generation

		pin_id = 0;
		i = 0;
		j = 0;
		while (pin_id < number_of_pins)
		{
			ckeckboxnb = 0;

			tmp_name[0] = 0;
			tmp_type = 0x00;

			jtagcore_get_pin_properties(jc, device, pin_id, tmp_name, sizeof(tmp_name), &tmp_type);

			if (tmp_type)
			{
				// output data checkbox
				if (tmp_type & JTAG_CORE_PIN_IS_OUTPUT)
				{
					objlist[nb_objs++] = CreateWindowEx(
							0, (LPCSTR)"BUTTON", 0, BS_AUTOCHECKBOX | WS_VISIBLE | WS_CHILD,
							0 + (i*graph_pin_list.NbPixPerCol) + graph_pin_list.NbPixPerPinName + (NB_PIX_PER_CHECKBOX_H * 0), NB_PIX_FIRST_LINE_OFFS + (j*NB_PIX_PER_CHECKBOX_V), NB_PIX_PER_CHECKBOX_H, NB_PIX_PER_CHECKBOX_V, hWnd, (HMENU)(BASE_CHECKBOX_ID + ((pin_id << 2) + 0)), NULL, NULL
						);

					ckeckboxnb++;

					objlist[nb_objs++] = CreateWindowEx(
							0, (LPCSTR)"BUTTON", 0, BS_AUTOCHECKBOX | WS_VISIBLE | WS_CHILD,
							0 + (i*graph_pin_list.NbPixPerCol) + graph_pin_list.NbPixPerPinName + (NB_PIX_PER_CHECKBOX_H * 3), NB_PIX_FIRST_LINE_OFFS + (j*NB_PIX_PER_CHECKBOX_V), NB_PIX_PER_CHECKBOX_H, NB_PIX_PER_CHECKBOX_V, hWnd, (HMENU)(BASE_CHECKBOX_ID + ((pin_id << 2) + 3)), NULL, NULL);

					ckeckboxnb++;
				}

				// output enable checkbox
				if (tmp_type & JTAG_CORE_PIN_IS_TRISTATES)
				{
					objlist[nb_objs++] = CreateWindowEx(
							0, (LPCSTR)"BUTTON", 0, BS_AUTOCHECKBOX | WS_VISIBLE | WS_CHILD,
							0 + (i*graph_pin_list.NbPixPerCol) + graph_pin_list.NbPixPerPinName + (NB_PIX_PER_CHECKBOX_H * 1), NB_PIX_FIRST_LINE_OFFS + (j*NB_PIX_PER_CHECKBOX_V), NB_PIX_PER_CHECKBOX_H, NB_PIX_PER_CHECKBOX_V, hWnd, (HMENU)(BASE_CHECKBOX_ID + ((pin_id << 2) + 1)), NULL, NULL
						);

					ckeckboxnb++;
				}

				// input checkbox
				if (tmp_type & JTAG_CORE_PIN_IS_INPUT)
				{
					objlist[nb_objs++] = CreateWindowEx(
							0, (LPCSTR)"BUTTON", 0, BS_AUTOCHECKBOX | WS_VISIBLE | WS_CHILD,
							0 + (i*graph_pin_list.NbPixPerCol) + graph_pin_list.NbPixPerPinName + (NB_PIX_PER_CHECKBOX_H * 2), NB_PIX_FIRST_LINE_OFFS + (j*NB_PIX_PER_CHECKBOX_V), NB_PIX_PER_CHECKBOX_H, NB_PIX_PER_CHECKBOX_V, hWnd, (HMENU)(BASE_CHECKBOX_ID + ((pin_id << 2) + 2)), NULL, NULL
						);

					ckeckboxnb++;
				}

				// placement du nom de la broche
				if (ckeckboxnb)
				{
					objlist[nb_objs++] = CreateWindowEx(
							0, (LPCSTR)"STATIC", tmp_name, WS_VISIBLE | WS_CHILD,
							0 + (i*graph_pin_list.NbPixPerCol), NB_PIX_FIRST_LINE_OFFS + (j*NB_PIX_PER_CHECKBOX_V), graph_pin_list.NbPixPerPinName, NB_PIX_PER_CHECKBOX_V, hWnd, (HMENU)(BASE_PINNAME_ID + ((pin_id << 2) + 0)), NULL, NULL
						);

					i++;
					if (i >= graph_pin_list.NbCol)
					{
						i = 0;
						j++;
					}
				}
			}

			pin_id++;

		};

		// j contains number of "character lines".
		// Update min and max range info and page size to scroll bar.
		si.cbSize = sizeof(si);
		si.fMask = SIF_ALL;
		GetScrollInfo(hWnd, SB_VERT, &si);
		si.nMin = 0;
		si.nMax = j;
		si.nPage = graph_pin_list.NbLines - 1;
		SetScrollInfo(hWnd, SB_VERT, &si, TRUE);


		// Checkbox state init

		scanposition = 0;
		i = 0;
		pin_id = 0;

		while (pin_id < number_of_pins)
		{
			state = jtagcore_get_pin_state(jc, device, pin_id, JTAG_CORE_OUTPUT);

			if(state >= 0 )
			{
				if(state)
				{
					SendDlgItemMessage(hWnd, (BASE_CHECKBOX_ID + ((pin_id << 2) + 0)), BM_SETCHECK, BST_CHECKED, 0);
				}
				else
				{
					SendDlgItemMessage(hWnd, (BASE_CHECKBOX_ID + ((pin_id << 2) + 0)), BM_SETCHECK, BST_UNCHECKED, 0);
				}
			}

			state = jtagcore_get_pin_state(jc, device, pin_id, JTAG_CORE_OE);

			if (state >= 0)
			{
				if(state)
				{
					SendDlgItemMessage(hWnd, (BASE_CHECKBOX_ID + ((pin_id << 2) + 1)), BM_SETCHECK, BST_CHECKED, 0);
				}
				else
				{
					SendDlgItemMessage(hWnd, (BASE_CHECKBOX_ID + ((pin_id << 2) + 1)), BM_SETCHECK, BST_UNCHECKED, 0);
				}
			}

			pin_id++;
		};

		GetClientRect(hWnd, &rt);
		InvalidateRect(hWnd, &rt, 1);
	}

	return 0;
}

int update_jtag_ids_menu(HWND hWnd)
{
	int i;
	int number_of_devices, dev_nb;
	int loaded_bsdl;
	char idstring[DEFAULT_BUFLEN];
	char szExecPath[MAX_PATH + 1];
	char filename[MAX_PATH + 1];
	char file[MAX_PATH + 1];
	char entityname[DEFAULT_BUFLEN];
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;
	unsigned long chip_id;

	loaded_bsdl = 0;

	// BSDL Auto load : check which bsdl file match with the device
	// And load it.

	jtagcore_scan_and_init_chain(jc);

	number_of_devices = jtagcore_get_number_of_devices(jc);

	for (i = 0; i < 64; i++)
	{
		RemoveMenu(GetSubMenu(GetMenu(hWnd), 2), BASE_DEV_ID + i, MF_BYCOMMAND);
	}

	// Get the bsdl_files folder path
	GetModuleFileName(NULL, szExecPath, MAX_PATH + 1);
	i = strlen(szExecPath);
	while(i && szExecPath[i]!='\\')
		i--;
	szExecPath[i] = 0;
	strcpy(filename,szExecPath);
	strcat(filename,"\\bsdl_files\\*.*");

	// Scan and check files in the folder.
	hFind = FindFirstFile(filename, &FindFileData);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			strcpy(filename,szExecPath);
			strcat(filename,"\\bsdl_files\\");
			strcat(filename,FindFileData.cFileName);

			if ( !(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
			{
				chip_id = jtagcore_get_bsdl_id(jc, filename);
				if( chip_id )
				{
					for(dev_nb=0;dev_nb < number_of_devices;dev_nb++)
					{
						if( chip_id == jtagcore_get_dev_id(jc, dev_nb) )
						{
							// The BSDL ID match with the device.
							if(jtagcore_loadbsdlfile(jc, filename, dev_nb) == JTAG_CORE_NO_ERROR)
							{
							}
						}
					}
				}
			}
		}while(FindNextFile(hFind,&FindFileData));

		FindClose(hFind);
	}

	loaded_bsdl = 0;
	// Count the loaded bsdl
	for(dev_nb=0;dev_nb < number_of_devices;dev_nb++)
	{
		entityname[0] = 0;
		jtagcore_get_dev_name(jc, dev_nb, entityname, file);
		sprintf(idstring, "Device %d: 0x%.8X - %s (%s)", dev_nb, jtagcore_get_dev_id(jc, dev_nb),entityname,file);

		AppendMenu(GetSubMenu(GetMenu(hWnd), 2), MF_STRING | MF_POPUP, BASE_DEV_ID + dev_nb, idstring);
	}

	return 0;
}


//
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent,i,j,c,cnt;
	PAINTSTRUCT ps;
	HPEN hPen;
	unsigned long current_io_id;
	HDC hdc;
	char szExecPath[MAX_PATH + 1];
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;
	int bsdl_file_found;

	char filename[MAX_PATH + 1];
	char tempstring[DEFAULT_BUFLEN];
	char idstring[DEFAULT_BUFLEN];
	char *tempstring2;
	int ret,state, tmp_type;
	unsigned long chip_id;
	static unsigned char togglebit=0;
	HANDLE hDlgModeless;

	SCROLLINFO si;
	static int yPos;        // current vertical scrolling position.
	int fwKeys, zDelta;

	int nb_of_drivers,nb_of_probes;

	switch (message)
	{
		case WM_KEYDOWN:
			wmId    = LOWORD(wParam);
			switch (wmId)
			{
				case VK_F5:
					timertick=0;
					break;
				case VK_F4:
					break;
			}
		break;

		case WM_SIZE:
			SetScrollPos(hWnd,SB_VERT,0,1);
			build_checkbox_map(hWnd, last_selected_dev_index);
		break;

		case WM_COMMAND:
			wmId    = LOWORD(wParam);
			wmEvent = HIWORD(wParam);

			switch (wmEvent)
			{

				case BN_CLICKED: //-> une check box a été modifiée
					if ((wmId >= BASE_CHECKBOX_ID) && (wmId < (jtagcore_get_number_of_pins(jc, last_selected_dev_index) * 4) + BASE_CHECKBOX_ID))
					{
						current_io_id = (wmId - BASE_CHECKBOX_ID) >> 0x2;
						////////////////////////////////////////////////////////////////////////////////////////
						//output enable
						////////////////////////////////////////////////////////////////////////////////////////
						if ((wmId & 0x3) == 1)
						{
							if (SendDlgItemMessage(hWnd, wmId, BM_GETCHECK, 0, 0))
							{
								jtagcore_set_pin_state(jc, last_selected_dev_index, (wmId - BASE_CHECKBOX_ID) >> 0x2, JTAG_CORE_OE, 0x01);
							}
							else
							{
								jtagcore_set_pin_state(jc, last_selected_dev_index, (wmId - BASE_CHECKBOX_ID) >> 0x2, JTAG_CORE_OE, 0x00);
							}

							c = jtagcore_get_number_of_pins(jc, last_selected_dev_index);
							for (i = 0; i < c; i++)
							{
								state = jtagcore_get_pin_state(jc, last_selected_dev_index, i, JTAG_CORE_OE);

								if (state >= 0)
								{
									if (state)
									{
										SendDlgItemMessage(hWnd, (BASE_CHECKBOX_ID + ((i << 2) + 1)), BM_SETCHECK, BST_CHECKED, 0);
									}
									else
									{
										SendDlgItemMessage(hWnd, (BASE_CHECKBOX_ID + ((i << 2) + 1)), BM_SETCHECK, BST_UNCHECKED, 0);
									}
								}
							}
						}

						////////////////////////////////////////////////////////////////////////////////////////
						//data
						////////////////////////////////////////////////////////////////////////////////////////

						if ((wmId & 0x3) == 0)
						{
							if (SendDlgItemMessage(hWnd, wmId, BM_GETCHECK, 0, 0))
							{
								jtagcore_set_pin_state(jc, last_selected_dev_index, (wmId - BASE_CHECKBOX_ID) >> 0x2, JTAG_CORE_OUTPUT, 0x01);
							}
							else
							{
								jtagcore_set_pin_state(jc, last_selected_dev_index, (wmId - BASE_CHECKBOX_ID) >> 0x2, JTAG_CORE_OUTPUT, 0x00);
							}
						}
					}
					break;
			}

			if (  wmId >= BASE_DEV_ID && wmId <= BASE_DEV_ID + 0x80)
			{
				bsdl_file_found = 0;

				if(GetMenuState(GetMenu(hWnd),ID_JTAGCHAIN_BSDLAUTOLOAD,MF_BYCOMMAND))
				{
					// BSDL Auto load : check which bsdl file match with the device
					// And load it.

					// Get the bsdl_files folder path
					GetModuleFileName(NULL, szExecPath, MAX_PATH + 1);
					i = strlen(szExecPath);
					while(i && szExecPath[i]!='\\')
						i--;
					szExecPath[i] = 0;
					strcpy(filename,szExecPath);
					strcat(filename,"\\bsdl_files\\*.*");

					// Scan and check files in the folder.
					hFind = FindFirstFile(filename, &FindFileData);
					if (hFind != INVALID_HANDLE_VALUE)
					{
						do
						{
							strcpy(filename,szExecPath);
							strcat(filename,"\\bsdl_files\\");
							strcat(filename,FindFileData.cFileName);

							if ( !(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
							{
								chip_id = jtagcore_get_bsdl_id(jc, filename);
								if( chip_id )
								{
									if( chip_id == jtagcore_get_dev_id(jc, wmId - BASE_DEV_ID) )
									{
										// The BSDL ID match with the device.

										bsdl_file_found = 1;
										last_selected_dev_index = wmId - BASE_DEV_ID;

										for (i = 0; i < 0x80; i++)
										{
											CheckMenuItem(GetMenu(hWnd), BASE_DEV_ID + i, MF_BYCOMMAND | MF_UNCHECKED);
										}

										jtagcore_loadbsdlfile(jc, filename, wmId - BASE_DEV_ID);
										build_checkbox_map(hWnd, wmId - BASE_DEV_ID);

										CheckMenuItem(GetMenu(hWnd), wmId, MF_BYCOMMAND | MF_CHECKED);
									}
								}
							}
						}
						while(FindNextFile(hFind,&FindFileData) && !bsdl_file_found);

						FindClose(hFind);
					}

				}

				if(!bsdl_file_found)
				{	// Manual BSDL selection
					memset(filename, 0, sizeof(filename));
					if (fileselector(hWnd, 0, 0, filename, "Read bsdl file", TEXT("BSDL File\0*.BS*;*.TXT\0\0"), TEXT("BS*")))
					{
						chip_id = jtagcore_get_bsdl_id(jc, filename);

						if (chip_id != jtagcore_get_dev_id(jc, wmId - BASE_DEV_ID))
						{
							sprintf(tempstring, "BSDL Chip ID doesn't match. Load it anyway ?\n\n");
							strcat(tempstring, "BSDL ID   : ");
							bsdl_id_str(chip_id, idstring);
							strcat(tempstring, idstring);
							strcat(tempstring, "\n");

							strcat(tempstring, "Device ID : ");
							bsdl_id_str(jtagcore_get_dev_id(jc, wmId - BASE_DEV_ID), idstring);
							strcat(tempstring, idstring);
							strcat(tempstring, "\n");

							last_selected_dev_index = wmId - BASE_DEV_ID;

							if (MessageBox(NULL, tempstring, "BSDL Device ID Mismatch", MB_ICONWARNING | MB_YESNO) == IDYES)
							{
								for (i = 0; i < 0x80; i++)
								{
									CheckMenuItem(GetMenu(hWnd), BASE_DEV_ID + i, MF_BYCOMMAND | MF_UNCHECKED);
								}

								jtagcore_loadbsdlfile(jc, filename, wmId - BASE_DEV_ID);
								build_checkbox_map(hWnd, wmId - BASE_DEV_ID);

								CheckMenuItem(GetMenu(hWnd), wmId, MF_BYCOMMAND | MF_CHECKED);
							}
						}
						else
						{
							last_selected_dev_index = wmId - BASE_DEV_ID;

							for (i = 0; i < 0x80; i++)
							{
								CheckMenuItem(GetMenu(hWnd), BASE_DEV_ID + i, MF_BYCOMMAND | MF_UNCHECKED);
							}

							jtagcore_loadbsdlfile(jc, filename, wmId - BASE_DEV_ID);
							build_checkbox_map(hWnd, wmId - BASE_DEV_ID);

							CheckMenuItem(GetMenu(hWnd), wmId, MF_BYCOMMAND | MF_CHECKED);
						}
					}
				}
			}
			else
			{
				if (wmId >= BASE_PROBE_ID && wmId < ( BASE_PROBE_ID + 0x800 ) )
				{
					for (i = 0; i < 0x800; i++)
					{
						CheckMenuItem(GetMenu(hWnd), BASE_PROBE_ID + i, MF_BYCOMMAND | MF_UNCHECKED);
					}

					if (jtagcore_select_and_open_probe(jc, wmId - BASE_PROBE_ID) >= 0)
					{
						jtagcore_scan_and_init_chain(jc);

						CheckMenuItem(GetMenu(hWnd), wmId, MF_BYCOMMAND | MF_CHECKED);
					}
					else
					{
						MessageBox(hWnd, "Error: Probe Init error", "Probe", MB_OK);
					}
				}
				else
				{
					switch (wmId)
					{
						case ID_HELP:
							hDlgModeless = CreateDialog(hInst, (LPCTSTR)IDD_HELP, hWnd, (DLGPROC)DialogProc);
							ShowWindow (hDlgModeless, SW_SHOW);
						break;
						case ID_TOOLS_I2CTOOL:
							hDlgModeless = CreateDialog(hInst, (LPCTSTR)IDD_I2CTOOL, hWnd, (DLGPROC)DialogProc_I2CTOOL);
							ShowWindow (hDlgModeless, SW_SHOW);
						break;
						case ID_TOOLS_SPITOOL:
							hDlgModeless = CreateDialog(hInst, (LPCTSTR)IDD_SPITOOL, hWnd, (DLGPROC)DialogProc_SPITOOL);
							ShowWindow (hDlgModeless, SW_SHOW);
						break;
						case ID_TOOLS_MDIOTOOL:
							hDlgModeless = CreateDialog(hInst, (LPCTSTR)IDD_MDIOTOOL, hWnd, (DLGPROC)DialogProc_MDIOTOOL);
							ShowWindow (hDlgModeless, SW_SHOW);
						break;
						case ID_TOOLS_MEMTOOL:
							hDlgModeless = CreateDialog(hInst, (LPCTSTR)IDD_MEMTOOL, hWnd, (DLGPROC)DialogProc_MEMTOOL);
							ShowWindow (hDlgModeless, SW_SHOW);
						break;
						case ID_TOOLS_SCRIPTEXECUTION:
							memset(filename, 0, sizeof(filename));
							if (fileselector(hWnd, 0, 0, filename, "Select script", TEXT("*.TXT\0\0"), TEXT("TXT")))
							{
								openconsole();
								printf("Starting %s...\n", filename);

								jtagcore_execScriptFile(jc,filename);

								printf("Press enter to exit\n");

								getchar();
								closeconsole();
							}
						break;
						case ID_TOOLS_PINSSTATEEXPORT:
							memset(filename, 0, sizeof(filename));
							if (fileselector(hWnd, 1, 0, filename, "Select script", TEXT("*.TXT\0\0"), TEXT("TXT")))
							{
								jtagcore_savePinsStateScript(jc,last_selected_dev_index,filename);
							}
						break;
						case IDM_ABOUT:
							hDlgModeless = CreateDialog(hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, (DLGPROC)DialogProc);
							ShowWindow (hDlgModeless, SW_SHOW);
						break;
						case IDM_LOGS:
							hDlgModeless = CreateDialog(hInst, (LPCTSTR)IDD_LOGS, hWnd, (DLGPROC)DialogProc_Logs);
							ShowWindow (hDlgModeless, SW_SHOW);
						break;
						case IDM_EXIT:
							//jtag_close();
							DestroyWindow(hWnd);
						break;

						case ID_FILE_OPENBOUNDARYSCANFILE:
							memset(filename, 0, sizeof(filename));
							if (fileselector(hWnd, 0, 0, filename, "Read bsdl file", TEXT("BSDL File\0*.BS*;*.TXT\0\0"), TEXT("BS*")))
							{
								if (jtagcore_loadbsdlfile(jc, filename, -1) >= 0)
								{
									build_checkbox_map(hWnd, 0);
								}
								else
								{
									sprintf(tempstring, "File open & parsing error !");
									MessageBox(NULL, (LPCSTR)tempstring, (LPCSTR)"File open error!", MB_ICONERROR);
								}
							}
						break;

						///////////////////////////

						case ID_BOUNDARYSCAN_NOREFRESH:
							timertickcfg = 0xFFFFFFFF;
							timertick = 0xFFFFFFFF;
							CheckMenuItem(GetMenu(hWnd), ID_BOUNDARYSCAN_NOREFRESH, MF_BYCOMMAND | MF_CHECKED);
							CheckMenuItem(GetMenu(hWnd), ID_BOUNDARYSCAN_10MSREFRESH, MF_BYCOMMAND | MF_UNCHECKED);
							CheckMenuItem(GetMenu(hWnd), ID_BOUNDARYSCAN_100MSREFRESH, MF_BYCOMMAND | MF_UNCHECKED);
							CheckMenuItem(GetMenu(hWnd), ID_BOUNDARYSCAN_1SREFRESH, MF_BYCOMMAND | MF_UNCHECKED);
						break;

						case ID_BOUNDARYSCAN_10MSREFRESH:
							timertickcfg = 0x0;
							timertick = 0x0;
							CheckMenuItem(GetMenu(hWnd), ID_BOUNDARYSCAN_NOREFRESH, MF_BYCOMMAND | MF_UNCHECKED);
							CheckMenuItem(GetMenu(hWnd), ID_BOUNDARYSCAN_10MSREFRESH, MF_BYCOMMAND | MF_CHECKED);
							CheckMenuItem(GetMenu(hWnd), ID_BOUNDARYSCAN_100MSREFRESH, MF_BYCOMMAND | MF_UNCHECKED);
							CheckMenuItem(GetMenu(hWnd), ID_BOUNDARYSCAN_1SREFRESH, MF_BYCOMMAND | MF_UNCHECKED);
						break;

						case ID_BOUNDARYSCAN_100MSREFRESH:
							timertickcfg = 10;
							timertick = 10;
							CheckMenuItem(GetMenu(hWnd), ID_BOUNDARYSCAN_NOREFRESH, MF_BYCOMMAND | MF_UNCHECKED);
							CheckMenuItem(GetMenu(hWnd), ID_BOUNDARYSCAN_10MSREFRESH, MF_BYCOMMAND | MF_UNCHECKED);
							CheckMenuItem(GetMenu(hWnd), ID_BOUNDARYSCAN_100MSREFRESH, MF_BYCOMMAND | MF_CHECKED);
							CheckMenuItem(GetMenu(hWnd), ID_BOUNDARYSCAN_1SREFRESH, MF_BYCOMMAND | MF_UNCHECKED);
						break;

						case ID_BOUNDARYSCAN_1SREFRESH:
							timertickcfg = 50;
							timertick = 50;
							CheckMenuItem(GetMenu(hWnd), ID_BOUNDARYSCAN_NOREFRESH, MF_BYCOMMAND | MF_UNCHECKED);
							CheckMenuItem(GetMenu(hWnd), ID_BOUNDARYSCAN_10MSREFRESH, MF_BYCOMMAND | MF_UNCHECKED);
							CheckMenuItem(GetMenu(hWnd), ID_BOUNDARYSCAN_100MSREFRESH, MF_BYCOMMAND | MF_UNCHECKED);
							CheckMenuItem(GetMenu(hWnd), ID_BOUNDARYSCAN_1SREFRESH, MF_BYCOMMAND | MF_CHECKED);
						break;

						case ID_BOUNDARYSCAN_EXTESTMODE:
							CheckMenuItem(GetMenu(hWnd), ID_BOUNDARYSCAN_EXTESTMODE, MF_BYCOMMAND | MF_CHECKED);
							CheckMenuItem(GetMenu(hWnd), ID_BOUNDARYSCAN_SAMPLEMODE, MF_BYCOMMAND | MF_UNCHECKED);
							jtagcore_set_scan_mode(jc, last_selected_dev_index, JTAG_CORE_EXTEST_SCANMODE);
						break;

						case ID_BOUNDARYSCAN_SAMPLEMODE:
							CheckMenuItem(GetMenu(hWnd), ID_BOUNDARYSCAN_EXTESTMODE, MF_BYCOMMAND | MF_UNCHECKED);
							CheckMenuItem(GetMenu(hWnd), ID_BOUNDARYSCAN_SAMPLEMODE, MF_BYCOMMAND | MF_CHECKED);
							jtagcore_set_scan_mode(jc, last_selected_dev_index, JTAG_CORE_SAMPLE_SCANMODE);
						break;

						case ID_BOUNDARYSCAN_GETID:

							jtagcore_scan_and_init_chain(jc);
							ret = jtagcore_get_number_of_devices(jc);
							if (ret > 0)
							{
								tempstring2 = get_id_str(ret);
								if (tempstring)
								{
									MessageBox(hWnd, tempstring2, "Device ID", MB_OK);

									update_jtag_ids_menu(hWnd);

									free(tempstring2);
								}
							}
							else
							{
								MessageBox(hWnd, "Error: No device found", "Device ID", MB_OK);
							}
							break;

						case ID_JTAGCHAIN_BSDLAUTOLOAD:
							if(GetMenuState(GetMenu(hWnd),ID_JTAGCHAIN_BSDLAUTOLOAD,MF_BYCOMMAND))
							{
								CheckMenuItem(GetMenu(hWnd), ID_JTAGCHAIN_BSDLAUTOLOAD, MF_BYCOMMAND | MF_UNCHECKED);
							}
							else
							{
								CheckMenuItem(GetMenu(hWnd), ID_JTAGCHAIN_BSDLAUTOLOAD, MF_BYCOMMAND | MF_CHECKED);
							}
						break;

						default:
							return DefWindowProc(hWnd, message, wParam, lParam);
					}
				}
			}

			break;

		case WM_MOUSEWHEEL :
			// Get mouse well event.

			fwKeys = (LOWORD(wParam));
			zDelta = ((short)HIWORD(wParam));

			// Get all the vertial scroll bar information
			si.cbSize = sizeof (si);
			si.fMask  = SIF_ALL;
			GetScrollInfo (hWnd, SB_VERT, &si);

			// Save the position for comparison later on
			yPos = si.nPos;

			// Change vertical position.
			si.nPos -= (zDelta / WHEEL_DELTA);

			// Set the position and then retrieve it.  Due to adjustments
			//   by Windows it may not be the same as the value set.
			si.fMask = SIF_POS;
			SetScrollInfo (hWnd, SB_VERT, &si, TRUE);
			GetScrollInfo (hWnd, SB_VERT, &si);

			// If the position has changed, scroll window and update it
			if (si.nPos != yPos)
			{
				ScrollWindow(hWnd, 0, NB_PIX_PER_CHECKBOX_V * (yPos - si.nPos), NULL, NULL);
				UpdateWindow (hWnd);
			}
		break;

		case WM_VSCROLL :

			// Get all the vertial scroll bar information
			si.cbSize = sizeof (si);
			si.fMask  = SIF_ALL;
			GetScrollInfo (hWnd, SB_VERT, &si);

			// Save the position for comparison later on
			yPos = si.nPos;
			switch (LOWORD (wParam))
			{
				// user clicked the HOME keyboard key
				case SB_TOP:
					si.nPos = si.nMin;
				break;

				// user clicked the END keyboard key
				case SB_BOTTOM:
					si.nPos = si.nMax;
				break;

				// user clicked the top arrow
				case SB_LINEUP:
					si.nPos -= 1;
				break;

				// user clicked the bottom arrow
				case SB_LINEDOWN:
					si.nPos += 1;
				break;

				// user clicked the scroll bar shaft above the scroll box
				case SB_PAGEUP:
					si.nPos -= si.nPage;
				break;

				// user clicked the scroll bar shaft below the scroll box
				case SB_PAGEDOWN:
					si.nPos += si.nPage;
				break;

				// user dragged the scroll box
				case SB_THUMBTRACK:
					si.nPos = si.nTrackPos;
				break;

				default:
				break;
			}

			// Set the position and then retrieve it.  Due to adjustments
			//   by Windows it may not be the same as the value set.
			si.fMask = SIF_POS;
			SetScrollInfo (hWnd, SB_VERT, &si, TRUE);
			GetScrollInfo (hWnd, SB_VERT, &si);

			// If the position has changed, scroll window and update it
			if (si.nPos != yPos)
			{
				ScrollWindow(hWnd, 0, NB_PIX_PER_CHECKBOX_V * (yPos - si.nPos), NULL, NULL);
				UpdateWindow (hWnd);
			}
		break;

		case WM_PAINT:
			hdc = BeginPaint(hWnd, &ps);
			if (hdc)
			{
				hPen = CreatePen(PS_SOLID, 1, RGB(25, 25, 255));
				if (hPen)
				{
					SelectObject(hdc, hPen);
					// lignes de séparations
					for (i = 0; i < graph_pin_list.NbCol; i++)
					{
						MoveToEx(hdc, 0 + (i*graph_pin_list.NbPixPerCol) + graph_pin_list.NbPixPerPinName + (NB_PIX_PER_CHECKBOX_H*NB_CHECKBOX), 0, NULL);
						LineTo(hdc, 0 + (i*graph_pin_list.NbPixPerCol) + graph_pin_list.NbPixPerPinName + (NB_PIX_PER_CHECKBOX_H*NB_CHECKBOX), (graph_pin_list.NbLines + 1) * NB_PIX_PER_CHECKBOX_V);
					}
					DeleteObject(hPen);
				}

				EndPaint(hWnd, &ps);
			}
			break;

		case WM_TIMER:
			if(timertick==0)
			{
				timertick=timertickcfg;

				c = jtagcore_get_number_of_pins(jc, last_selected_dev_index);

				/////////////////////////////////////////////////////
				// desactivation des checkbox output en mode auto
				i=0;
				cnt=0;
				while( i < c )
				{
					tmp_type = 0;
					jtagcore_get_pin_properties(jc, last_selected_dev_index, i, 0, 0, &tmp_type);

					if(tmp_type & JTAG_CORE_PIN_IS_OUTPUT)
					{
						if (SendDlgItemMessage(hWnd, (BASE_CHECKBOX_ID + ((i << 2) + 3)), BM_GETCHECK, 0, 0))
						{
							SendDlgItemMessage(hWnd, (BASE_CHECKBOX_ID + ((i << 2) + 0)), BM_SETCHECK, BST_UNCHECKED, 0);
							jtagcore_set_pin_state(jc, last_selected_dev_index, i, JTAG_CORE_OUTPUT, 0x00);
							cnt++;
						}
					}
					i++;
				};

				// routine de balayage automatique
				if(scanposition>=cnt) scanposition=0;
				j=0;
				i=0;
				while( i < c)
				{
					tmp_type = 0;
					jtagcore_get_pin_properties(jc, last_selected_dev_index, i, 0, 0, &tmp_type);

					if (tmp_type & JTAG_CORE_PIN_IS_OUTPUT)
					{
						if (SendDlgItemMessage(hWnd, (BASE_CHECKBOX_ID + ((i << 2) + 3)), BM_GETCHECK, 0, 0))
						{
							jtagcore_set_pin_state(jc, last_selected_dev_index, i, JTAG_CORE_OUTPUT, 0x00);

							if(scanposition==j)
							{
								// si une case seulement en mode auto -> toggle
								if(cnt==1)
								{
									jtagcore_set_pin_state(jc, last_selected_dev_index, i, JTAG_CORE_OUTPUT, togglebit & 1);
									if(togglebit&1)
										SendDlgItemMessage(hWnd, (BASE_CHECKBOX_ID + ((i << 2) + 0)), BM_SETCHECK, BST_CHECKED, 0);
									togglebit++;
								}
								else
								{
									jtagcore_set_pin_state(jc, last_selected_dev_index, i, JTAG_CORE_OUTPUT, 1);
									SendDlgItemMessage(hWnd, (BASE_CHECKBOX_ID + ((i << 2) + 0)), BM_SETCHECK, BST_CHECKED, 0);
								}

								//case suivante le prochaine fois
								scanposition++;
								if(scanposition>=c)
								{
									scanposition=0;
								}
								break;
							}
							j++;
						}
					}

					i++;
				};

				// transfert physique sur la ligne jtag
				if(jtagcore_push_and_pop_chain(jc, JTAG_CORE_WRITE_READ) != JTAG_CORE_NO_ERROR )
				{
					timertickcfg = 0xFFFFFFFF;
					timertick = 0xFFFFFFFF;

					MessageBox(hWnd, "JTAG chain error !", "Error", MB_OK | MB_ICONERROR);
				}

				// recuperation des etat d''entrée et mise à jour des checkbox.
				i=0;
				while (i<c)
				{
					tmp_type = 0;
					jtagcore_get_pin_properties(jc, last_selected_dev_index, i, 0, 0, &tmp_type);

					if (tmp_type & JTAG_CORE_PIN_IS_INPUT)
					{
						state = jtagcore_get_pin_state(jc, last_selected_dev_index, i, JTAG_CORE_INPUT);
						if (state >= 0)
						{
							if (state)
							{
								//SetBkColor(GetDlgItem(hWnd,(BASE_CHECKBOX_ID + ((i << 2) + 2))),RGB(255,0,0));

								if (!SendDlgItemMessage(hWnd, (BASE_CHECKBOX_ID + ((i << 2) + 2)), BM_GETCHECK, 0, 0))
								{
									SendDlgItemMessage(hWnd, (BASE_CHECKBOX_ID + ((i << 2) + 2)), BM_SETCHECK, BST_CHECKED, 0);
								}
							}
							else
							{
								if (SendDlgItemMessage(hWnd, (BASE_CHECKBOX_ID + ((i << 2) + 2)), BM_GETCHECK, 0, 0))
								{
									SendDlgItemMessage(hWnd, (BASE_CHECKBOX_ID + ((i << 2) + 2)), BM_SETCHECK, BST_UNCHECKED, 0);
								}
							}
						}
					}

					if (tmp_type & JTAG_CORE_PIN_IS_TRISTATES)
					{
						state = jtagcore_get_pin_state(jc, last_selected_dev_index, i, JTAG_CORE_INPUT);
						if (state >= 0)
						{
							if (state)
							{
								if (!SendDlgItemMessage(hWnd, (BASE_CHECKBOX_ID + ((i << 2) + 2)), BM_GETCHECK, 0, 0))
								{
									SendDlgItemMessage(hWnd, (BASE_CHECKBOX_ID + ((i << 2) + 2)), BM_SETCHECK, BST_CHECKED, 0);
								}
							}
							else
							{
								if (SendDlgItemMessage(hWnd, (BASE_CHECKBOX_ID + ((i << 2) + 2)), BM_GETCHECK, 0, 0))
								{
									SendDlgItemMessage(hWnd, (BASE_CHECKBOX_ID + ((i << 2) + 2)), BM_SETCHECK, BST_UNCHECKED, 0);
								}
							}
						}


						state = jtagcore_get_pin_state(jc, last_selected_dev_index, i, JTAG_CORE_OUTPUT);
						if (state >= 0)
						{
							if (state)
							{
								if (!SendDlgItemMessage(hWnd, (BASE_CHECKBOX_ID + ((i << 2) + 0)), BM_GETCHECK, 0, 0))
								{
									SendDlgItemMessage(hWnd, (BASE_CHECKBOX_ID + ((i << 2) + 0)), BM_SETCHECK, BST_CHECKED, 0);
								}
							}
							else
							{
								if (SendDlgItemMessage(hWnd, (BASE_CHECKBOX_ID + ((i << 2) + 0)), BM_GETCHECK, 0, 0))
								{
									SendDlgItemMessage(hWnd, (BASE_CHECKBOX_ID + ((i << 2) + 0)), BM_SETCHECK, BST_UNCHECKED, 0);
								}
							}
						}


						state = jtagcore_get_pin_state(jc, last_selected_dev_index, i, JTAG_CORE_OE);
						if (state >= 0)
						{
							if (state)
							{
								if (!SendDlgItemMessage(hWnd, (BASE_CHECKBOX_ID + ((i << 2) + 1)), BM_GETCHECK, 0, 0))
								{
									SendDlgItemMessage(hWnd, (BASE_CHECKBOX_ID + ((i << 2) + 1)), BM_SETCHECK, BST_CHECKED, 0);
								}
							}
							else
							{
								if (SendDlgItemMessage(hWnd, (BASE_CHECKBOX_ID + ((i << 2) + 1)), BM_GETCHECK, 0, 0))
								{
									SendDlgItemMessage(hWnd, (BASE_CHECKBOX_ID + ((i << 2) + 1)), BM_SETCHECK, BST_UNCHECKED, 0);
								}
							}
						}

					}

					i++;
				};
			}
			else
			{
				timertick--;
			}
			break;

		case WM_HELP:

			break;

		case WM_CREATE:
			CheckMenuItem(GetMenu(hWnd),ID_JTAGCHAIN_BSDLAUTOLOAD,MF_BYCOMMAND | MF_CHECKED );
			CheckMenuItem(GetMenu(hWnd),ID_BOUNDARYSCAN_NOREFRESH,MF_BYCOMMAND | MF_CHECKED );
			CheckMenuItem(GetMenu(hWnd),ID_BOUNDARYSCAN_10MSREFRESH,MF_BYCOMMAND | MF_UNCHECKED );
			CheckMenuItem(GetMenu(hWnd),ID_BOUNDARYSCAN_100MSREFRESH,MF_BYCOMMAND | MF_UNCHECKED );
			CheckMenuItem(GetMenu(hWnd),ID_BOUNDARYSCAN_1SREFRESH,MF_BYCOMMAND | MF_UNCHECKED );

			CheckMenuItem(GetMenu(hWnd),ID_BOUNDARYSCAN_SAMPLEMODE,MF_BYCOMMAND | MF_CHECKED );

			nb_of_drivers = jtagcore_get_number_of_probes_drv(jc);
			j = 0;
			while (j < nb_of_drivers)
			{
				nb_of_probes = jtagcore_get_number_of_probes(jc, j);
				i = 0;
				while( i < nb_of_probes )
				{
					jtagcore_get_probe_name(jc, PROBE_ID(j,i), tempstring);
					sprintf(idstring, "Probe: %s", tempstring);
					AppendMenu(GetSubMenu(GetMenu(hWnd), 3), MF_STRING | MF_POPUP, BASE_PROBE_ID + PROBE_ID(j,i), idstring);
					i++;
				}
				j++;
			}
			break;

		case WM_DESTROY:
			PostQuitMessage(0);
			break;

		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Mesage handler for dialog box.
LRESULT CALLBACK DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
				return TRUE;

		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
			{
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			}
			break;
	}
    return FALSE;
}

LRESULT CALLBACK DialogProc_I2CTOOL(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent,number_of_pins,pin_id;
	HWND combo1,combo2;
	unsigned char tmp_buffer[DEFAULT_BUFLEN];
	unsigned char tmp_buffer2[DEFAULT_BUFLEN];
	unsigned char tmp_buffer3[16];
	int ItemIndex,i2cadr,size;
	int i,ret;

	switch (message)
	{
		case WM_INITDIALOG:
			combo1 = GetDlgItem(hDlg,IDC_SDAPIN);
			combo2 = GetDlgItem(hDlg,IDC_SCLPIN);

			number_of_pins = jtagcore_get_number_of_pins(jc, last_selected_dev_index);
			if (number_of_pins>0)
			{
				for (pin_id = 0; pin_id < number_of_pins; pin_id++)
				{
					tmp_buffer[0] = 0;
					jtagcore_get_pin_properties(jc, last_selected_dev_index, pin_id, tmp_buffer, sizeof(tmp_buffer), 0);
					SendMessage(combo1,(UINT) CB_ADDSTRING,(WPARAM) 0,(LPARAM) tmp_buffer);
					SendMessage(combo2,(UINT) CB_ADDSTRING,(WPARAM) 0,(LPARAM) tmp_buffer);
				}

				SendMessage(combo1, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
				SendMessage(combo2, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
			}

			return TRUE;
			break;


		case WM_COMMAND:

			wmId    = LOWORD(wParam);
			wmEvent = HIWORD(wParam);

			switch (wmId)
			{
				case IDWRITE:
				case IDREAD:
				case IDAUTOSCAN:

					combo1 = GetDlgItem(hDlg,IDC_SDAPIN);
					combo2 = GetDlgItem(hDlg,IDC_SCLPIN);

					memset(tmp_buffer,0,sizeof(tmp_buffer));

					ItemIndex = SendMessage((HWND) combo1, (UINT) CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0);

					SendMessage((HWND) combo1, (UINT) CB_GETLBTEXT,  (WPARAM) ItemIndex, (LPARAM) tmp_buffer);

					pin_id = jtagcore_get_pin_id(jc, last_selected_dev_index, tmp_buffer);

					if(pin_id >= 0)
					{
						jtagcore_i2c_set_sda_pin(jc, last_selected_dev_index, pin_id);
					}

					memset(tmp_buffer,0,sizeof(tmp_buffer));

					ItemIndex = SendMessage((HWND) combo2, (UINT) CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0);

					SendMessage((HWND) combo2, (UINT) CB_GETLBTEXT,  (WPARAM) ItemIndex, (LPARAM) tmp_buffer);

					pin_id = jtagcore_get_pin_id(jc, last_selected_dev_index, tmp_buffer);

					if(pin_id >= 0)
					{
						jtagcore_i2c_set_scl_pin(jc, last_selected_dev_index, pin_id);
					}

					memset(tmp_buffer,0,sizeof(tmp_buffer));
					GetWindowText(GetDlgItem(hDlg,IDC_I2CADR), tmp_buffer, sizeof(tmp_buffer));

					i2cadr = strtol(tmp_buffer,0,16);

					if(wmId == IDWRITE)
					{
						memset(tmp_buffer,0,sizeof(tmp_buffer));
						GetWindowText(GetDlgItem(hDlg,IDC_I2CWRDATA), tmp_buffer, sizeof(tmp_buffer));

						size  = strlen(tmp_buffer);
						size = size / 2;
						for(i = 0; i<size; i++)
						{
							tmp_buffer3[0] = tmp_buffer[i*2];
							tmp_buffer3[1] = tmp_buffer[i*2 + 1];
							tmp_buffer3[2] = 0;

							tmp_buffer2[i] = (char)strtol(tmp_buffer3,0,16);
						}

						ret = jtagcore_i2c_write_read(jc, i2cadr, 0, size, tmp_buffer2, 0, 0);
						if(ret < 0)
						{
							MessageBox(hWnd, "JTAG chain error !", "Error", MB_OK | MB_ICONERROR);
						}

						if (ret == 0)
						{
							MessageBox(hWnd, "Warning : Device Ack not detected !", "I2C Tool", MB_OK | MB_ICONWARNING);
						}

					}
					else
					{
						if (wmId == IDAUTOSCAN)
						{
							openconsole();
							do
							{
								printf("Trying 0x%.2X / 0x%.2X...", i2cadr, i2cadr>>1);
								size = 1;
								ret = jtagcore_i2c_write_read(jc, i2cadr, 0, 0, tmp_buffer2, size, tmp_buffer2);
								switch (ret)
								{
									case 0:
										printf(" No answer...\n");
									break;
									case 1:
										printf(" 0x%.2X / 0x%.2X Acknowledged !\n", i2cadr, i2cadr>>1);
									break;
									case JTAG_CORE_I2C_BUS_NOTFREE:
										printf(" Bus not free !\n");
									break;
									default:
										printf(" Internal error 0x%X!\n", ret);
									break;
								}

								i2cadr = (i2cadr + 2) & ~1;
							} while ( (i2cadr & 0xff) != 0x00);

							printf("Press enter to exit\n");

							getchar();
							closeconsole();
						}
						else
						{
							size = GetDlgItemInt(hDlg, IDC_NBBYTESTOREAD, 0, 0);

							ret = jtagcore_i2c_write_read(jc, i2cadr, 0, 0, tmp_buffer2, size, tmp_buffer2);
							if(ret < 0)
							{
								MessageBox(hWnd, "JTAG chain error !", "Error", MB_OK | MB_ICONERROR);
							}

							if ( ret == 0 )
							{
								MessageBox(hWnd, "Warning : Device Ack not detected !", "I2C Tool", MB_OK | MB_ICONWARNING);
							}

							memset(tmp_buffer, 0, sizeof(tmp_buffer));
							for (i = 0; i<size; i++)
							{
								sprintf(tmp_buffer3, "%.2x", tmp_buffer2[i]);
								strcat(tmp_buffer, tmp_buffer3);
							}

							SetWindowText(GetDlgItem(hDlg, IDC_I2CRDDATA), (LPCSTR)tmp_buffer);
						}


					}
				break;

				case IDCANCEL:
				case IDOK:
					EndDialog(hDlg, LOWORD(wParam));
					return TRUE;
				break;
			}

			break;
	}
    return FALSE;
}


LRESULT CALLBACK DialogProc_SPITOOL(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent,number_of_pins,pin_id;
	HWND combo1,combo2,combo3,combo4;
	LRESULT check;
	unsigned char tmp_buffer[DEFAULT_BUFLEN];
	unsigned char tmp_buffer2[DEFAULT_BUFLEN];
	unsigned char tmp_buffer4[DEFAULT_BUFLEN];
	unsigned char tmp_buffer3[16];
	int ItemIndex,size;
	int i,ret;

	switch (message)
	{
		case WM_INITDIALOG:
			combo1 = GetDlgItem(hDlg,IDC_MOSIPIN);
			combo2 = GetDlgItem(hDlg,IDC_MISOPIN);
			combo3 = GetDlgItem(hDlg,IDC_CSPIN);
			combo4 = GetDlgItem(hDlg,IDC_CLKPIN);

			number_of_pins = jtagcore_get_number_of_pins(jc, last_selected_dev_index);
			if (number_of_pins>0)
			{
				for (pin_id = 0; pin_id < number_of_pins; pin_id++)
				{
					tmp_buffer[0] = 0;
					jtagcore_get_pin_properties(jc, last_selected_dev_index, pin_id, tmp_buffer, sizeof(tmp_buffer), 0);
					SendMessage(combo1,(UINT) CB_ADDSTRING,(WPARAM) 0,(LPARAM) tmp_buffer);
					SendMessage(combo2,(UINT) CB_ADDSTRING,(WPARAM) 0,(LPARAM) tmp_buffer);
					SendMessage(combo3,(UINT) CB_ADDSTRING,(WPARAM) 0,(LPARAM) tmp_buffer);
					SendMessage(combo4,(UINT) CB_ADDSTRING,(WPARAM) 0,(LPARAM) tmp_buffer);
				}

				SendMessage(combo1, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
				SendMessage(combo2, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
				SendMessage(combo3, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
				SendMessage(combo4, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
			}

			return TRUE;
			break;


		case WM_COMMAND:

			wmId    = LOWORD(wParam);
			wmEvent = HIWORD(wParam);

			switch (wmId)
			{

				case IDWRITE:
					combo1 = GetDlgItem(hDlg,IDC_MOSIPIN);
					combo2 = GetDlgItem(hDlg,IDC_MISOPIN);
					combo3 = GetDlgItem(hDlg,IDC_CSPIN);
					combo4 = GetDlgItem(hDlg,IDC_CLKPIN);

					memset(tmp_buffer,0,sizeof(tmp_buffer));

					ItemIndex = SendMessage((HWND) combo1, (UINT) CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0);

					SendMessage((HWND) combo1, (UINT) CB_GETLBTEXT,  (WPARAM) ItemIndex, (LPARAM) tmp_buffer);

					check  = SendDlgItemMessage(hDlg, IDC_POLMOSI, BM_GETCHECK, 0, 0);
					pin_id = jtagcore_get_pin_id(jc, last_selected_dev_index, tmp_buffer);

					if(pin_id >= 0)
					{
						jtagcore_spi_set_mosi_pin(jc, last_selected_dev_index, pin_id,check);
					}

					memset(tmp_buffer,0,sizeof(tmp_buffer));

					ItemIndex = SendMessage((HWND) combo2, (UINT) CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0);

					SendMessage((HWND) combo2, (UINT) CB_GETLBTEXT,  (WPARAM) ItemIndex, (LPARAM) tmp_buffer);

					check  = SendDlgItemMessage(hDlg, IDC_POLMISO, BM_GETCHECK, 0, 0);
					pin_id = jtagcore_get_pin_id(jc, last_selected_dev_index, tmp_buffer);

					if(pin_id >= 0)
					{
						jtagcore_spi_set_miso_pin(jc, last_selected_dev_index, pin_id,check);
					}

					memset(tmp_buffer,0,sizeof(tmp_buffer));

					ItemIndex = SendMessage((HWND) combo3, (UINT) CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0);

					SendMessage((HWND) combo3, (UINT) CB_GETLBTEXT,  (WPARAM) ItemIndex, (LPARAM) tmp_buffer);

					check  = SendDlgItemMessage(hDlg, IDC_POLCS, BM_GETCHECK, 0, 0);
					pin_id = jtagcore_get_pin_id(jc, last_selected_dev_index, tmp_buffer);

					if(pin_id >= 0)
					{
						jtagcore_spi_set_cs_pin(jc, last_selected_dev_index, pin_id,check);
					}

					memset(tmp_buffer,0,sizeof(tmp_buffer));

					ItemIndex = SendMessage((HWND) combo4, (UINT) CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0);

					SendMessage((HWND) combo4, (UINT) CB_GETLBTEXT,  (WPARAM) ItemIndex, (LPARAM) tmp_buffer);

					check  = SendDlgItemMessage(hDlg, IDC_POLCLK, BM_GETCHECK, 0, 0);
					pin_id = jtagcore_get_pin_id(jc, last_selected_dev_index, tmp_buffer);

					if(pin_id >= 0)
					{
						jtagcore_spi_set_clk_pin(jc, last_selected_dev_index, pin_id,check);
					}

					check  = SendDlgItemMessage(hDlg, IDC_LSB_FIRST, BM_GETCHECK, 0, 0);
					jtagcore_spi_set_bitorder(jc, check);

					memset(tmp_buffer,0,sizeof(tmp_buffer));
					GetWindowText(GetDlgItem(hDlg, IDC_SPIWRDATA), (LPSTR)tmp_buffer, sizeof(tmp_buffer));

					size  = strlen(tmp_buffer);
					size = size / 2;
					for(i = 0; i<size; i++)
					{
						tmp_buffer3[0] = tmp_buffer[i*2];
						tmp_buffer3[1] = tmp_buffer[i*2 + 1];
						tmp_buffer3[2] = 0;

						tmp_buffer2[i] = (char)strtol(tmp_buffer3,0,16);
					}

					ret = jtagcore_spi_write_read(jc, size,tmp_buffer2,tmp_buffer4, 0);
					if(ret < 0)
					{
						MessageBox(hWnd, "JTAG chain error !", "Error", MB_OK | MB_ICONERROR);
					}

					memset(tmp_buffer,0,sizeof(tmp_buffer));
					for(i = 0; i<size; i++)
					{
						sprintf(tmp_buffer3,"%.2x",tmp_buffer4[i]);
						strcat(tmp_buffer,tmp_buffer3);
					}

					SetWindowText(GetDlgItem(hDlg, IDC_SPIRDDATA), (LPCSTR)tmp_buffer);
				break;

				case IDCANCEL:
				case IDOK:
					EndDialog(hDlg, LOWORD(wParam));
					return TRUE;
				break;

			}

			break;
	}
    return FALSE;
}


LRESULT CALLBACK DialogProc_MDIOTOOL(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent, number_of_pins, pin_id;
	HWND combo1, combo2;
	unsigned char tmp_buffer[DEFAULT_BUFLEN];
	unsigned char tmp_buffer3[16];
	int ItemIndex, devadr, regadr, datatowrite, dataread;
	int ret;

	switch (message)
	{
	case WM_INITDIALOG:
		combo1 = GetDlgItem(hDlg, IDC_MDCPIN);
		combo2 = GetDlgItem(hDlg, IDC_MDIOPIN);

		number_of_pins = jtagcore_get_number_of_pins(jc, last_selected_dev_index);
		if (number_of_pins>0)
		{
			for (pin_id = 0; pin_id < number_of_pins; pin_id++)
			{
				tmp_buffer[0] = 0;
				jtagcore_get_pin_properties(jc, last_selected_dev_index, pin_id, tmp_buffer, sizeof(tmp_buffer), 0);
				SendMessage(combo1, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)tmp_buffer);
				SendMessage(combo2, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)tmp_buffer);
			}

			SendMessage(combo1, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
			SendMessage(combo2, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
		}

		return TRUE;
		break;


	case WM_COMMAND:

		wmId = LOWORD(wParam);
		wmEvent = HIWORD(wParam);

		switch (wmId)
		{
		case IDWRITE:
		case IDREAD:
		case IDAUTOSCAN:

			combo1 = GetDlgItem(hDlg, IDC_MDCPIN);
			combo2 = GetDlgItem(hDlg, IDC_MDIOPIN);

			memset(tmp_buffer, 0, sizeof(tmp_buffer));

			ItemIndex = SendMessage((HWND)combo1, (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);

			SendMessage((HWND)combo1, (UINT)CB_GETLBTEXT, (WPARAM)ItemIndex, (LPARAM)tmp_buffer);

			pin_id = jtagcore_get_pin_id(jc, last_selected_dev_index, tmp_buffer);

			if (pin_id >= 0)
			{
				jtagcore_mdio_set_mdc_pin(jc, last_selected_dev_index, pin_id);
			}

			memset(tmp_buffer, 0, sizeof(tmp_buffer));

			ItemIndex = SendMessage((HWND)combo2, (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);

			SendMessage((HWND)combo2, (UINT)CB_GETLBTEXT, (WPARAM)ItemIndex, (LPARAM)tmp_buffer);

			pin_id = jtagcore_get_pin_id(jc, last_selected_dev_index, tmp_buffer);

			if (pin_id >= 0)
			{
				jtagcore_mdio_set_mdio_pin(jc, last_selected_dev_index, pin_id);
			}

			memset(tmp_buffer, 0, sizeof(tmp_buffer));
			GetWindowText(GetDlgItem(hDlg, IDC_DEVADR), (LPSTR)tmp_buffer, sizeof(tmp_buffer));

			devadr = strtol(tmp_buffer, 0, 16);

			memset(tmp_buffer, 0, sizeof(tmp_buffer));
			GetWindowText(GetDlgItem(hDlg, IDC_REGADR), (LPSTR)tmp_buffer, sizeof(tmp_buffer));

			regadr = strtol(tmp_buffer, 0, 16);

			memset(tmp_buffer, 0, sizeof(tmp_buffer));
			GetWindowText(GetDlgItem(hDlg, IDC_WRDATA), (LPSTR)tmp_buffer, sizeof(tmp_buffer));

			datatowrite = strtol(tmp_buffer, 0, 16);

			if (wmId == IDWRITE)
			{
				ret = jtagcore_mdio_write(jc, devadr, regadr, datatowrite);
				if(ret < 0)
				{
					MessageBox(hWnd, "JTAG chain error !", "Error", MB_OK | MB_ICONERROR);
				}
			}
			else
			{
				if (wmId == IDAUTOSCAN)
				{
					openconsole();
					do
					{
						dataread = jtagcore_mdio_read(jc, devadr, regadr);
						if(dataread < 0)
						{
							MessageBox(hWnd, "JTAG chain error !", "Error", MB_OK | MB_ICONERROR);
						}
						else
						{
							printf(" Dev 0x%.2X : [0x%.2X] = 0x%.4X\n", devadr ,regadr, dataread);

							devadr++;
						}
					} while ( ((devadr & 0x1f) != 0x00) && ( dataread >= 0 ) );

					printf("Press enter to exit\n");
					getchar();
					closeconsole();
				}
				else
				{
					dataread = jtagcore_mdio_read(jc, devadr, regadr);
					sprintf(tmp_buffer3, "%.4x", dataread);
					SetWindowText(GetDlgItem(hDlg, IDC_RDDATA), (LPCSTR)tmp_buffer3);
				}
			}

			break;

		case IDCANCEL:
		case IDOK:
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
			break;
		}

		break;
	}
	return FALSE;
}

int set_mem_pins(char * filename)
{
	char pinline[DEFAULT_BUFLEN];
	char * ptr;
	int index,i,pin_id;

	FILE * f;

	f = fopen(filename,"rb");
	if(f)
	{
		jtagcore_memory_clear_pins(jc);

		while(!feof(f))
		{
			fgets(pinline,sizeof(pinline),f);
			ptr = strchr(pinline,'=');
			if(ptr)
			{
				switch(pinline[0])
				{
					case 'A':
					case 'D':
						index = atoi(&pinline[1]);


						ptr++;
						while(*ptr==' ')
							ptr++;

						i = 0;
						while(ptr[i])
						{
							if(ptr[i] == '\r' || ptr[i] == '\n')
							{
								ptr[i]=0;
							}
							i++;
						}

						pin_id = jtagcore_get_pin_id(jc, 0, ptr);

						if(pin_id >= 0)
						{
							switch(pinline[0])
							{
								case 'A':
									jtagcore_memory_set_address_pin(jc, index, last_selected_dev_index, pin_id);
								break;
								case 'D':
									jtagcore_memory_set_data_pin(jc, index, last_selected_dev_index, pin_id);
								break;
							}
						}
					break;
					case 'C':

						ptr++;
						while(*ptr==' ')
							ptr++;

						i = 0;
						while(ptr[i])
						{
							if(ptr[i] == '\r' || ptr[i] == '\n')
							{
								ptr[i]=0;
							}
							i++;
						}

						pin_id = jtagcore_get_pin_id(jc, 0, ptr);

						if(pin_id >= 0)
						{
							if(!strncmp(pinline,"CTRL_RD",7))
							{
								jtagcore_memory_set_ctrl_pin(jc, JTAG_CORE_RAM_RD_CTRL, 0, last_selected_dev_index, pin_id);
							}

							if(!strncmp(pinline,"CTRL_WR",7))
							{
								jtagcore_memory_set_ctrl_pin(jc, JTAG_CORE_RAM_WR_CTRL, 0, last_selected_dev_index, pin_id);
							}

							if(!strncmp(pinline,"CTRL_CS",7))
							{
								jtagcore_memory_set_ctrl_pin(jc, JTAG_CORE_RAM_CS_CTRL, 0, last_selected_dev_index, pin_id);
							}
						}
					break;
					default:
					break;
				}

				ptr++;

			}

		}

		fclose(f);
	}

	return 0;
}

LRESULT CALLBACK DialogProc_MEMTOOL(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	unsigned char tmp_buffer[DEFAULT_BUFLEN];
	unsigned long tmp_buffer2[DEFAULT_BUFLEN];
	unsigned char tmp_buffer3[16];
	char filename[DEFAULT_BUFLEN];
	int memadr,size;
	int i;

	switch (message)
	{
		case WM_INITDIALOG:
			return TRUE;
			break;

		case WM_COMMAND:

			wmId    = LOWORD(wParam);
			wmEvent = HIWORD(wParam);

			switch (wmId)
			{
				case IDWRITE:
				case IDREAD:
					memset(tmp_buffer,0,sizeof(tmp_buffer));
					GetWindowText(GetDlgItem(hDlg,IDC_MEMADR), tmp_buffer, sizeof(tmp_buffer));

					memadr = strtol(tmp_buffer,0,16);

					if(wmId == IDWRITE)
					{
						memset(tmp_buffer,0,sizeof(tmp_buffer));
						GetWindowText(GetDlgItem(hDlg,IDC_MEMWRDATA), tmp_buffer, sizeof(tmp_buffer));

						size  = strlen(tmp_buffer);
						size = size / 2;

						if (size >= sizeof(tmp_buffer2)/sizeof(unsigned long))
							size = (sizeof(tmp_buffer2) / sizeof(unsigned long)) - 1;

						for(i = 0; i<size; i++)
						{
							tmp_buffer3[0] = tmp_buffer[i*2];
							tmp_buffer3[1] = tmp_buffer[i*2 + 1];
							tmp_buffer3[2] = 0;

							tmp_buffer2[i] = (char)strtol(tmp_buffer3,0,16);
						}

						for(i=0;i<size;i++)
						{
							jtagcore_memory_write(jc, memadr + i, tmp_buffer2[i]);
						}
					}
					else
					{
						size = GetDlgItemInt(hDlg, IDC_NBBYTESTOREAD, 0, 0);

						if (size >= sizeof(tmp_buffer) / sizeof(unsigned long))
							size = (sizeof(tmp_buffer) / sizeof(unsigned long)) - 1;

						for(i=0;i<size;i++)
						{
							tmp_buffer2[i] = jtagcore_memory_read(jc, memadr + i);
						}

						memset(tmp_buffer, 0, sizeof(tmp_buffer));
						for (i = 0; i<size; i++)
						{
							sprintf(tmp_buffer3, "0x%.8X ", tmp_buffer2[i]);
							strcat(tmp_buffer, tmp_buffer3);
						}

						SetWindowText(GetDlgItem(hDlg, IDC_MEMRDDATA), (LPCSTR)tmp_buffer);
					}
				break;

				case IDLOADPINCFG:
					memset(filename,0,sizeof(filename));
					if (fileselector(hWnd, 0, 0, filename, "Read pin cfg file", TEXT("TXT File\0*.txt;\0\0"), TEXT("txt")))
					{
						set_mem_pins(filename);
					}
				break;

				case IDCANCEL:
				case IDOK:
					EndDialog(hDlg, LOWORD(wParam));
					return TRUE;
				break;
			}

			break;
	}
    return FALSE;
}

LRESULT CALLBACK DialogProc_Logs(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;

	switch (message)
	{
		case WM_INITDIALOG:
			hDlg_logs = hDlg;
			SendMessage(GetDlgItem(hDlg, IDC_EDIT1),EM_SETLIMITTEXT,100000000,0);
			SendDlgItemMessage(hDlg, IDC_SLIDER1, TBM_SETRANGE, 1, MAKELONG(0, 5));
			SendDlgItemMessage(hDlg, IDC_SLIDER1, TBM_SETPOS, 1, 4);
			jtagcore_set_logs_level(jc,4);
			return TRUE;
			break;

		case WM_COMMAND:

			wmId    = LOWORD(wParam);
			wmEvent = HIWORD(wParam);

			switch (wmId)
			{

				case IDCANCEL:
				case IDOK:
					hDlg_logs = 0;
					EndDialog(hDlg, LOWORD(wParam));
					return TRUE;
				break;
			}

			break;

		case WM_HSCROLL:
			jtagcore_set_logs_level(jc,SendMessage(GetDlgItem(hDlg, IDC_SLIDER1), TBM_GETPOS, 0, 0));
			break;

	}
    return FALSE;
}
