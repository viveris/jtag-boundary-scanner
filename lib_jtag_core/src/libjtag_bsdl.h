#ifndef _libjtag_bsdl_
#define _libjtag_bsdl_

#ifdef __cplusplus
extern "C" {
#endif

// Output message types/levels
enum MSGTYPE
{
	MSG_NONE = 0,
	MSG_INFO_0,
	MSG_INFO_1,
	MSG_WARNING,
	MSG_ERROR,
	MSG_DEBUG
};

#define MAX_NUMBER_BITS_IN_CHAIN ( 256 * 1024 )
#define MAX_NUMBER_PINS_PER_DEV  ( 64 * 1024 )
#define MAX_BSDL_FILE_SIZE ( 1024 * 1024 )
#define MAX_NUMBER_OF_BSDL_LINES ( 64 * 1024 )

#include "bsdl_parser/jtag_bsdl.h"

#ifdef __cplusplus
}
#endif

#endif
