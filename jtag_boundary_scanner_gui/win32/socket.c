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
* @file   socket.c
* @brief  TCP control support.
* @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
*/

#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#include "socket.h"

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#include "jtag_core.h"
#include "bsdl_parser/bsdl_loader.h"

extern jtag_core * jc;
int line_index;
SOCKET ClientSocket = INVALID_SOCKET;

int Printf_socket(void * ctx, int MSGTYPE,char * chaine, ...)
{
	char temp[DEFAULT_BUFLEN];
	char textbuf[DEFAULT_BUFLEN];
	int iSendResult,i,j;

	textbuf[0] = 0;

	if(MSGTYPE!=MSG_DEBUG)
	{
		va_list marker;
		va_start( marker, chaine );

		switch(MSGTYPE)
		{
			case MSG_NONE:
				sprintf(textbuf,"");
			break;
			case MSG_INFO_0:
				sprintf(textbuf,"OK : ");
			break;
			case MSG_INFO_1:
				sprintf(textbuf,"OK : ");
			break;
			case MSG_WARNING:
				sprintf(textbuf,"WARNING : ");
			break;
			case MSG_ERROR:
				sprintf(textbuf,"ERROR : ");
			break;
			case MSG_DEBUG:
				sprintf(textbuf,"DEBUG : ");
			break;
		}

		vsprintf(temp,chaine,marker);
		//strcat(textbuf,"\n");

		j = strlen(textbuf);
		i = 0;
		while(temp[i])
		{

			if(temp[i]=='\n')
			{
				textbuf[j++] = '\r';
				textbuf[j++] = '\n';
			}
			else
				textbuf[j++] = temp[i];

			i++;
		}
		textbuf[j] = 0;

		// Echo the buffer back to the sender
		iSendResult = send( ClientSocket, textbuf, strlen(textbuf), 0 );
		if (iSendResult == SOCKET_ERROR) {
			printf("send failed with error: %d\n", WSAGetLastError());
			closesocket(ClientSocket);
			WSACleanup();
			return 1;
		}

		va_end( marker );
	}
	return 0;
}

int launch_server(int port)
{
	WSADATA wsaData;
	int iResult,i;

	struct addrinfo hints;
	char recvbuf[DEFAULT_BUFLEN];
	char fullline[DEFAULT_BUFLEN];
	char port_string[16];

	script_ctx * ctx;

	int recvbuflen = DEFAULT_BUFLEN;
	SOCKET ListenSocket = INVALID_SOCKET;
	struct addrinfo *result = NULL;

	printf("Start server...\n");

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	sprintf(port_string,"%d",port);
	iResult = getaddrinfo(NULL, port_string, &hints, &result);
	if ( iResult != 0 ) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	// Create a SOCKET for connecting to server
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	// Setup the TCP listening socket
	iResult = bind( ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	freeaddrinfo(result);

	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	ctx = jtagcore_initScript(jc);

	jtagcore_setScriptOutputFunc( ctx, Printf_socket );

	do
	{

		// Accept a client socket
		ClientSocket = accept(ListenSocket, NULL, NULL);
		if (ClientSocket == INVALID_SOCKET) {
			printf("accept failed with error: %d\n", WSAGetLastError());
			closesocket(ListenSocket);
			WSACleanup();
			return 1;
		}

		line_index = 0;
		// Receive until the peer shuts down the connection
		do {

			iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
			if (iResult > 0) {

				i = 0;
				do
				{
					while( recvbuf[i]!='\n' && i < iResult )
					{
						if(line_index<DEFAULT_BUFLEN)
						{
							fullline[line_index] = recvbuf[i];
							line_index++;
						}
						i++;
					}

					if( recvbuf[i] == '\n' &&  i != iResult )
					{
						fullline[line_index] = 0;

						if( !strncmp( fullline, "kill_server", 11 ) )
						{
							printf("Exiting !\n");
							closesocket(ClientSocket);
							WSACleanup();
							return 1;
						}
						else
						{
							jtagcore_execScriptLine(ctx,fullline);
							line_index = 0;
						}
						i++;
					}
				}while(i < iResult);
			}
			else if (iResult == 0)
				printf("Closing connection...\n");
			else  {
				if( WSAGetLastError() == 10054 )
					printf("Connection closed !\n");
				else
					printf("recv failed with error: %d\n", WSAGetLastError());
			}

		} while (iResult > 0);

		// shutdown the connection since we're done
		iResult = shutdown(ClientSocket, SD_SEND);
		if (iResult == SOCKET_ERROR) {
			printf("shutdown failed with error: %d\n", WSAGetLastError());
			closesocket(ClientSocket);
			WSACleanup();
			return 1;
		}

	} while(1);

	// cleanup
	closesocket(ClientSocket);
	WSACleanup();

	return 0;
}
