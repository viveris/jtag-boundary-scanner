// epSWIGHere.h for CoreJTAG
#pragma once
#include <utility>
#include <string>

typedef enum _SWIGHERE_TYPE {
    VOIDPTR=0,
    INTPTR
} SWIGHERE_TYPE;

typedef std::pair<SWIGHERE_TYPE,void *> SWIGHERE_CAST;
typedef std::pair<SWIGHERE_TYPE,int *> SWIGHERE_INTLIST;

typedef volatile void * SWIGHERE_CONTEXT;

extern SWIGHERE_CONTEXT epInitSwigHere();
extern long unsigned int epBSDLDeviceId(SWIGHERE_CONTEXT oContext, char * pathToFile);
extern SWIGHERE_INTLIST epSequence(SWIGHERE_CONTEXT oContext,unsigned * pBytesPerElement,unsigned * pElementCount);
//extern SWIGHERE_INTLIST epBSDLPinMap(SWIGHERE_CONTEXT oContext,unsigned * pBytesPerElement,unsigned * pElementCount);
extern void epUninitSwigHere(SWIGHERE_CONTEXT oContext);
