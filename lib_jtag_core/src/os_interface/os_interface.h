/*
 * JTAG Boundary Scanner
 * Copyright (c) 2008 - 2024 Viveris Technologies
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
* @file   os_interface.h
* @brief  Basic/generic OS functions wrapper header.
* @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
*/
#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32
#define DIR_SEPARATOR "\\"
#define DIR_SEPARATOR_CHAR '\\'
#if defined(_MSC_VER) && _MSC_VER < 1900
int snprintf(char *outBuf, size_t size, const char *format, ...);
#endif
#else
#define DIR_SEPARATOR "/"
#define DIR_SEPARATOR_CHAR '/'
#endif

#define FILEFOUND_NAMESIZE 256

/////////////// Thread functions ////////////////

typedef int (*THREADFUNCTION) (void* jtag_ctx,void* hw_ctx);

typedef struct threadinit_
{
	THREADFUNCTION thread;
	jtag_core * jtag_ctx;
	void * hwcontext;
}threadinit;

typedef struct filefoundinfo_
{
	int isdirectory;
	char filename[FILEFOUND_NAMESIZE];
	int size;
}filefoundinfo;

int genos_setevent( jtag_core* jtag_ctx, unsigned char id );
uintptr_t genos_createevent( jtag_core* jtag_ctx, unsigned char id );
int genos_waitevent( jtag_core* jtag_ctx, int id, int timeout );
void genos_pause( int ms );
int genos_createthread( jtag_core* jtag_ctx, void* hwcontext, THREADFUNCTION thread, int priority );

uintptr_t genos_createcriticalsection( jtag_core* jtag_ctx, unsigned char id );
void genos_entercriticalsection( jtag_core* jtag_ctx, unsigned char id );
void genos_leavecriticalsection( jtag_core* jtag_ctx, unsigned char id );
void genos_destroycriticalsection( jtag_core* jtag_ctx, unsigned char id );

/////////////// String functions ///////////////

#ifndef WIN32
//void strlwr(char *string)
#endif
char * genos_strupper( char * str );
char * genos_strlower( char * str );
char * genos_strndstcat( char *dest, const char *src, size_t maxdestsize );

/////////////// File functions ////////////////

int genos_open ( const char *filename, int flags, ... );

FILE *genos_fopen ( const char *filename, const char *mode );
int genos_fread( void * ptr, size_t size, FILE *f );
char * genos_fgets( char * str, int num, FILE *f );
int genos_fclose( FILE * f );
int genos_fgetsize( FILE * f );
#ifndef stat
#include <sys/stat.h>
#endif
int genos_stat( const char *filename, struct stat *buf );

void* genos_find_first_file( char *folder, char *file, filefoundinfo* fileinfo );
int genos_find_next_file( void* handleff, char *folder, char *file, filefoundinfo* fileinfo );
int genos_find_close( void* handle );

int  genos_mkdir( char * folder );

char * genos_getcurrentdirectory( char *currentdirectory, int buffersize );

enum
{
	SYS_PATH_TYPE = 0,
	UNIX_PATH_TYPE,
	WINDOWS_PATH_TYPE,
};

char * genos_getfilenamebase( char * fullpath, char * filenamebase, int type );
char * genos_getfilenameext( char * fullpath, char * filenameext, int type );
int genos_getfilenamewext( char * fullpath, char * filenamewext, int type );
int genos_getpathfolder( char * fullpath, char * folder, int type );
int genos_checkfileext( char * path, char *ext, int type );
int genos_getfilesize( char * path );

/////////////// Network functions ////////////////

void * network_connect(char * address,unsigned short port);
int network_read(void * network_connection, unsigned char * buffer, int size,int timeout);
int network_read2(void * network_connection, unsigned char * buffer, int size,int timeout);
int network_write(void * network_connection, unsigned char * buffer, int size,int timeout);
int network_close(void * network_connection);

#ifdef __cplusplus
}
#endif
