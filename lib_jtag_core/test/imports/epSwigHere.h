// epSWIGHere.h for libjtag_bsdl
#pragma once
#include <utility>
#include <vector>
#include <string>

#include "libjtag_bsdl.h"

typedef enum _SWIGHERE_TYPE {
    VOIDPTR=0,
    INTPTR
} SWIGHERE_TYPE;

typedef std::pair<SWIGHERE_TYPE,void *> SWIGHERE_CAST;
typedef std::pair<SWIGHERE_TYPE,int *> SWIGHERE_INTLIST;

typedef volatile void * SWIGHERE_CONTEXT;

extern SWIGHERE_CONTEXT epInitSwigHere();
extern long unsigned int epBSDLDeviceId(SWIGHERE_CONTEXT oContext, char * pathToBSDL);
extern std::vector<pin_ctrl> epBSDLPinSequence(SWIGHERE_CONTEXT oContext, char * pathToBSDL, unsigned * pBytesPerElement);
extern std::vector<jtag_chain> epBSDLChainSequence(SWIGHERE_CONTEXT oContext, char * pathToBSDL, unsigned * pBytesPerElement);
extern void epUninitSwigHere(SWIGHERE_CONTEXT oContext);
