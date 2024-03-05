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
* @file   os_interface.c
* @brief  Basic/generic OS functions wrapper.
* @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
*/

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#ifdef WIN32
	#include <windows.h>
	#include <direct.h>
#else
	#include <sys/time.h>
	#include <pthread.h>
	#include <sched.h>
#endif

#include <errno.h>

#include <stdint.h>

#include "../drivers/drv_loader.h"
#include "../jtag_core_internal.h"
#include "../script/script.h"
#include "../jtag_core.h"

#include "os_interface.h"

#ifdef WIN32
	HANDLE eventtab[256];
	CRITICAL_SECTION criticalsectiontab[256];
#else
	typedef struct _EVENT_HANDLE{
		pthread_cond_t eCondVar;
		pthread_mutex_t eMutex;
		int iVar;
	} EVENT_HANDLE;

	EVENT_HANDLE * eventtab[256];

	pthread_mutex_t criticalsectiontab[256];
#endif

#ifdef WIN32

DWORD WINAPI ThreadProc( LPVOID lpParameter)
{
	threadinit *threadinitptr;
	THREADFUNCTION thread;
	jtag_core* jtag_ctx;
	void * hw_context;

	if( lpParameter )
	{
		//SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

		threadinitptr=(threadinit*)lpParameter;
		thread=threadinitptr->thread;
		jtag_ctx = threadinitptr->jtag_ctx;
		hw_context=threadinitptr->hwcontext;
		thread(jtag_ctx,hw_context);

		free(threadinitptr);
	}

	return 0;
}
#else
void * ThreadProc( void *lpParameter)
{
	threadinit *threadinitptr;
	THREADFUNCTION thread;
	jtag_core* jtag_ctx;
	void * hw_context;

	threadinitptr=(threadinit*)lpParameter;
	if( threadinitptr )
	{

		thread=threadinitptr->thread;
		jtag_ctx = threadinitptr->jtag_ctx;
		hw_context=threadinitptr->hwcontext;
		thread(jtag_ctx,hw_context);

		free(threadinitptr);
	}

	return 0;
}
#endif

int genos_setevent(jtag_core* jtag_ctx,unsigned char id)
{
#ifdef WIN32
	SetEvent(eventtab[id]);
#else
	pthread_mutex_lock(&eventtab[id]->eMutex);
	pthread_cond_signal(&eventtab[id]->eCondVar);
	pthread_mutex_unlock(&eventtab[id]->eMutex);
#endif
	return 0;
}

uintptr_t genos_createevent(jtag_core* jtag_ctx,unsigned char id)
{
#ifdef WIN32

	eventtab[id] = CreateEvent(NULL,FALSE,FALSE,NULL);
	return (uintptr_t)eventtab[id];

#else

	eventtab[id]=(EVENT_HANDLE*)malloc(sizeof(EVENT_HANDLE));
	pthread_mutex_init(&eventtab[id]->eMutex, NULL);
	pthread_cond_init(&eventtab[id]->eCondVar, NULL);
	return (uintptr_t)eventtab[id];
#endif
}

int genos_waitevent(jtag_core* jtag_ctx,int id,int timeout)
{

#ifdef WIN32
	int ret;

	if(timeout==0) timeout=INFINITE;
	ret=WaitForSingleObject(eventtab[id],timeout);

	if(ret==0)
	{
		return 0;
	}
	else
	{
		return 1;
	}
#else
	struct timeval now;
	struct timespec timeoutstr;
	int retcode;

	pthread_mutex_lock(&eventtab[id]->eMutex);
	gettimeofday(&now,0);
	timeoutstr.tv_sec = now.tv_sec + (timeout/1000);
	timeoutstr.tv_nsec = (now.tv_usec * 1000) ;//+(timeout*1000000);
	retcode = 0;

	retcode = pthread_cond_timedwait(&eventtab[id]->eCondVar, &eventtab[id]->eMutex, &timeoutstr);
	if (retcode == ETIMEDOUT)
	{
		pthread_mutex_unlock(&eventtab[id]->eMutex);
		return 1;
	}
	else
	{
		pthread_mutex_unlock(&eventtab[id]->eMutex);
		return 0;
	}
#endif
}

uintptr_t genos_createcriticalsection(jtag_core* jtag_ctx,unsigned char id)
{
#ifdef WIN32

	InitializeCriticalSection(&criticalsectiontab[id]);
	return (uintptr_t)&criticalsectiontab[id];

#else
	//create mutex attribute variable
	pthread_mutexattr_t mAttr;

	pthread_mutexattr_init(&mAttr);

#if defined(__APPLE__)
	// setup recursive mutex for mutex attribute
	pthread_mutexattr_settype(&mAttr, PTHREAD_MUTEX_RECURSIVE);
#else
	pthread_mutexattr_settype(&mAttr, PTHREAD_MUTEX_RECURSIVE_NP);
#endif
	// Use the mutex attribute to create the mutex
	pthread_mutex_init(&criticalsectiontab[id], &mAttr);

	// Mutex attribute can be destroy after initializing the mutex variable
	pthread_mutexattr_destroy(&mAttr);

	return (uintptr_t)&criticalsectiontab[id];
#endif
}


void genos_entercriticalsection(jtag_core* jtag_ctx,unsigned char id)
{
#ifdef WIN32
	EnterCriticalSection( &criticalsectiontab[id] );
#else
	pthread_mutex_lock( &criticalsectiontab[id] );
#endif
}

void genos_leavecriticalsection(jtag_core* jtag_ctx,unsigned char id)
{
#ifdef WIN32
	LeaveCriticalSection( &criticalsectiontab[id] );
#else
	pthread_mutex_unlock( &criticalsectiontab[id] );
#endif
}

void genos_destroycriticalsection(jtag_core* jtag_ctx,unsigned char id)
{
#ifdef WIN32
	DeleteCriticalSection(&criticalsectiontab[id]);
#else
	pthread_mutex_destroy (&criticalsectiontab[id]);
#endif
}


#ifndef WIN32
void genos_msleep (unsigned int ms) {
    int microsecs;
    struct timeval tv;
    microsecs = ms * 1000;
    tv.tv_sec  = microsecs / 1000000;
    tv.tv_usec = microsecs % 1000000;
    select (0, NULL, NULL, NULL, &tv);
}
#endif

void genos_pause(int ms)
{
#ifdef WIN32
	Sleep(ms);
#else
	genos_msleep(ms);
#endif
}

int genos_createthread(jtag_core* jtag_ctx,void* hwcontext,THREADFUNCTION thread,int priority)
{
#ifdef WIN32
	DWORD sit;
	HANDLE thread_handle;
	threadinit *threadinitptr;

	sit = 0;

	threadinitptr=(threadinit*)malloc(sizeof(threadinit));
	if( threadinitptr )
	{
		threadinitptr->thread = thread;
		threadinitptr->jtag_ctx = jtag_ctx;
		threadinitptr->hwcontext = hwcontext;

		thread_handle = CreateThread(NULL,8*1024,&ThreadProc,threadinitptr,0,&sit);

		if(!thread_handle)
		{
			free(threadinitptr);
	//		jtag_ctx->jtagcore_print_callback(MSG_ERROR,"genos_createthread : CreateThread failed -> 0x.8X", GetLastError());
		}
	}
	return sit;
#else
	unsigned long sit;
	int ret;
	pthread_t threadid;
	pthread_attr_t threadattrib;
	threadinit *threadinitptr;
	struct sched_param param;
	JTAGCORE_PRINT_FUNC print_callback;

	sit = 0;

	pthread_attr_init(&threadattrib);

	pthread_attr_setinheritsched(&threadattrib, PTHREAD_EXPLICIT_SCHED);

	if(priority)
	{
		pthread_attr_setschedpolicy(&threadattrib,SCHED_FIFO);
		param.sched_priority = sched_get_priority_max(SCHED_FIFO);
	}
	else
	{
		pthread_attr_setschedpolicy(&threadattrib,SCHED_OTHER);
		param.sched_priority = sched_get_priority_max(SCHED_OTHER);
	}
	/* set the new scheduling param */
	pthread_attr_setschedparam (&threadattrib, &param);

	print_callback = jtag_ctx->jtagcore_print_callback;

	threadinitptr = (threadinit *)malloc(sizeof(threadinit));
	if( threadinitptr )
	{
		threadinitptr->thread = thread;
		threadinitptr->jtag_ctx = jtag_ctx;
		//threadinitptr->hwcontext=hwcontext;

		ret = pthread_create(&threadid, &threadattrib,ThreadProc, threadinitptr);
		if(ret)
		{
			print_callback("genos_createthread : pthread_create failed !");
			free( threadinitptr );
		}
	}
	else
	{
		print_callback("genos_createthread : memory allocation failed !");
	}

	return sit;
#endif

}

#ifndef WIN32
/*void strlwr(char *string)
{
	int i;

	i=0;
	while (string[i])
	{
		string[i] = tolower(string[i]);
		i++;
	}
}*/
#endif

char * genos_strupper(char * str)
{
	int i;

	i=0;
	while(str[i])
	{

		if(str[i]>='a' && str[i]<='z')
		{
			str[i]=str[i]+('A'-'a');
		}
		i++;
	}

	return str;
}

char * genos_strlower(char * str)
{
	int i;

	i=0;
	while(str[i])
	{

		if(str[i]>='A' && str[i]<='Z')
		{
			str[i]=str[i]+('a'-'A');
		}
		i++;
	}

	return str;
}

char * genos_getfilenamebase(char * fullpath,char * filenamebase, int type)
{
	int len,i;
	char separator;

	if(fullpath)
	{
		len=strlen(fullpath);

		separator = DIR_SEPARATOR_CHAR; // system type by default

		switch(type)
		{
			case SYS_PATH_TYPE:  // System based
				separator = DIR_SEPARATOR_CHAR;
			break;

			case UNIX_PATH_TYPE:    // Unix style
				separator = '/';
			break;

			case WINDOWS_PATH_TYPE: // Windows style
				separator = '\\';
			break;
		}

		i=0;
		if(len)
		{
			i=len-1;
			while(i &&	( fullpath[i] != separator && fullpath[i]!=':') )
			{
				i--;
			}

			if( fullpath[i] == separator || fullpath[i]==':' )
			{
				i++;
			}

			if(i>len)
			{
				i=len;
			}
		}

		if(filenamebase)
		{
			strcpy(filenamebase,&fullpath[i]);
		}

		return &fullpath[i];
	}

	return 0;
}

char * genos_getfilenameext(char * fullpath,char * filenameext, int type )
{
	char * filename;
	int len,i;

	filename = genos_getfilenamebase(fullpath,0,type);

	if(filename)
	{
		len=strlen(filename);

		i=0;
		if(len)
		{
			i=len-1;

			while(i &&	( filename[i] != '.' ) )
			{
				i--;
			}

			if( filename[i] == '.' )
			{
				i++;
			}
			else
			{
				i=len;
			}

			if(i>len)
			{
				i=len;
			}
		}

		if(filenameext)
		{
			strcpy(filenameext,&filename[i]);
		}

		return &filename[i];
	}

	return 0;
}

int genos_getfilenamewext(char * fullpath,char * filenamewext, int type)
{
	char * filename;
	char * ext;
	int len;

	len = 0;
	if(fullpath)
	{
		filename = genos_getfilenamebase(fullpath,0,type);
		ext = genos_getfilenameext(fullpath,0,type);

		len = ext-filename;

		if(len && filename[len-1]=='.')
		{
			len--;
		}

		if(filenamewext)
		{
			memcpy(filenamewext,filename,len);
			filenamewext[len]=0;
		}
	}

	return len;
}

int genos_getpathfolder(char * fullpath,char * folder,int type)
{
	int len;
	char * filenameptr;

	len = 0;
	if(fullpath)
	{
		filenameptr = genos_getfilenamebase(fullpath,0,type);

		len = filenameptr-fullpath;

		if(folder)
		{
			memcpy(folder,fullpath,len);
			folder[len]=0;
		}
	}

	return len;
}

int genos_checkfileext(char * path,char *ext,int type)
{
	char pathext[16];
	char srcext[16];
	char * ptr;

	if(path && ext)
	{
		pathext[0] = '\0';
		srcext[0] = ' ';
		srcext[1] = '\0';

		ptr = genos_getfilenameext(path,0,type);
		if(!ptr)
			return 0;

		if( ( strlen(ptr) < 16 )  && ( strlen(ext) < 16 ))
		{
			genos_getfilenameext(path,(char*)&pathext,type);
			genos_strlower(pathext);

			strcpy((char*)srcext,ext);
			genos_strlower(srcext);

			if(!strcmp(pathext,srcext))
			{
				return 1;
			}
		}
	}
	return 0;
}

int genos_getfilesize(char * path)
{
	int filesize;
	FILE * f;

	filesize=-1;

	if(path)
	{
		f=genos_fopen(path,"rb");
		if(f)
		{
			fseek (f , 0 , SEEK_END);
			filesize=ftell(f);

			fclose(f);
		}
	}

	return filesize;
}

#ifdef WIN32

#if defined(_MSC_VER) && _MSC_VER < 1900

#define va_copy(dest, src) (dest = src)

int vsnprintf(char *s, size_t n, const char *fmt, va_list ap)
{
	int ret;
	va_list ap_copy;

	if (n == 0)
		return 0;
	else if (n > INT_MAX)
		return 0;
	memset(s, 0, n);
	va_copy(ap_copy, ap);
	ret = _vsnprintf(s, n - 1, fmt, ap_copy);
	va_end(ap_copy);

	return ret;
}

int snprintf(char *s, size_t n, const char *fmt, ...)
{
	va_list ap;
	int ret;

	va_start(ap, fmt);
	ret = vsnprintf(s, n, fmt, ap);
	va_end(ap);

	return ret;
}

#endif

#endif

char * genos_strndstcat( char *dest, const char *src, size_t maxdestsize )
{
	int i,j;

	i = 0;
	while( ( i < maxdestsize ) && dest[i] )
	{
		i++;
	}

	if( !dest[i] )
	{
		j = 0;
		while( ( i < maxdestsize ) && src[j] )
		{
			dest[i] = src[j];
			i++;
			j++;
		}

		if( i < maxdestsize )
		{
			dest[i] = '\0';
		}
	}

	return dest;
}
