// xcSwigHere.cpp

#include "xcSwigHere.h"
#include <cstddef>
#include "stdio.h" // for debugging

static SWIGHERE_CONTEXT pContext=NULL;

extern void xcUninitSwigHere() {
    if(pContext != NULL) {
        epUninitSwigHere(pContext);
        pContext=NULL;
    }
}

extern void xcInitSwigHere() {
    xcUninitSwigHere();
    pContext=epInitSwigHere();
    //return pContext;
}

extern unsigned long int xcBSDLDeviceId(char * pathToBSDL) {
    return epBSDLDeviceId(pContext,pathToBSDL);
}

extern std::vector<pin_ctrl> xcBSDLPinSequence(char * pathToBSDL) {
    std::vector<pin_ctrl> noresult;
    unsigned bytesPerElement=0;
    std::vector<pin_ctrl> pinSequence = epBSDLPinSequence(pContext, pathToBSDL,&bytesPerElement);
    if( bytesPerElement == sizeof(pin_ctrl)) {
        return pinSequence;
    }
    return noresult;
}
