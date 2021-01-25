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
* @file   network.c
* @brief  Basic/generic network functions wrapper.
* @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
*/

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#include <stdint.h>

#ifdef	WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <commctrl.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close (s)
typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
#include <unistd.h>
#include <dirent.h>

#ifdef	OSX
#	include <mach-o/dyld.h>
#endif

#endif

#include "../jtag_core.h"
#include "network.h"
#include "os_interface.h"

void * network_connect(char * address,unsigned short port)
{
	genos_tcp_stat * tcp_stat;
#ifdef WIN32
	int iResult;
#endif

	tcp_stat = malloc(sizeof(genos_tcp_stat));
	if(tcp_stat)
	{
		memset(tcp_stat,0,sizeof(genos_tcp_stat));

#ifdef WIN32
		iResult = WSAStartup(MAKEWORD(2,2), &tcp_stat->wsaData);
		if (iResult != NO_ERROR)
		{
			free(tcp_stat);
			return (void*)NULL;
		}
#endif

		tcp_stat->m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (tcp_stat->m_socket == INVALID_SOCKET)
		{
#ifdef WIN32
			WSACleanup();
#endif
			free(tcp_stat);
			return (void*)NULL;
		}

		tcp_stat->clientService.sin_family = AF_INET;
		tcp_stat->clientService.sin_addr.s_addr = inet_addr(address);
		tcp_stat->clientService.sin_port = htons(port);

		if (connect(tcp_stat->m_socket, (SOCKADDR*)&tcp_stat->clientService, sizeof(tcp_stat->clientService)) == SOCKET_ERROR)
		{
#ifdef WIN32
			WSACleanup();
#endif
			free(tcp_stat);
			return (void*)NULL;
		}

		return (void*)tcp_stat;
	}

	return (void*)NULL;
}

int network_read(void * network_connection, unsigned char * buffer, int size,int timeout)
{
	int bytesRecv;
	int offset;
	genos_tcp_stat * tcp_stat;

	tcp_stat = (genos_tcp_stat *)network_connection;

	offset=0;

	while(offset < size)
	{
		bytesRecv = recv(tcp_stat->m_socket, (char*)&buffer[offset], size - offset, 0);
		if(bytesRecv == SOCKET_ERROR)
		{
			return -1;
		}
		else
		{
			offset += bytesRecv;
		}
	}

	return size;
}

int network_read2(void * network_connection, unsigned char * buffer, int size,int timeout)
{
	int bytesRecv;
	int offset;
	genos_tcp_stat * tcp_stat;

	tcp_stat = (genos_tcp_stat *)network_connection;

	offset=0;

		bytesRecv = recv(tcp_stat->m_socket, (char*)&buffer[offset], size - offset, 0);

	return bytesRecv;
}

int network_write(void * network_connection, unsigned char * buffer, int size,int timeout)
{
	int bytesSent;
	int offset;
	genos_tcp_stat * tcp_stat;

	tcp_stat = (genos_tcp_stat *)network_connection;

	offset = 0;
	while(offset < size)
	{
		bytesSent = send(tcp_stat->m_socket, (char*)&buffer[offset], size - offset, 0);
		if(bytesSent == SOCKET_ERROR)
		{
			return -1;
		}
		else
		{
			offset += bytesSent;
		}
	}

	return size;
}

int network_close(void * network_connection)
{
	genos_tcp_stat * tcp_stat;

	tcp_stat = (genos_tcp_stat *)network_connection;

	closesocket (tcp_stat->m_socket);

#ifdef WIN32
	WSACleanup();
#endif
	free(tcp_stat);

	return 0;
}
