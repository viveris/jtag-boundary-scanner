// py3SwigHere.cpp
// compile with SWIG

#include "py3SwigHere.h"
#include "epSwigHere.h"
#include <cstddef>

static SWIGHERE_CONTEXT pContext=NULL;

extern void pyUninitSwigHere() {
    if(pContext != NULL) {
        epUninitSwigHere(pContext);
        pContext=NULL;
    }
}

extern void pyInitSwigHere() {
    pyUninitSwigHere();
    pContext=epInitSwigHere();
    //return pContext;
}

extern unsigned long int pyBSDLDeviceId(std::string pathToBSDL) {
    if(pContext) {
        return epBSDLDeviceId(pContext,(char*)pathToBSDL.c_str());
    }
    return 0;
}

extern std::vector<int> pyBSDLPinSequence(std::string pathToBSDL) {
    std::vector<int> result;
    if(pContext) {
        unsigned bytesPerElement=0, elementCount=0;
        std::vector<pin_ctrl> pinSequence = epBSDLPinSequence(pContext, (char*)pathToBSDL.c_str(), &bytesPerElement);
        if( bytesPerElement == sizeof(pin_ctrl) ) {
            for(unsigned i = 0; i<pinSequence.size(); i++) {
                result.push_back(i);
            }
        }        
    }

    return result;
}
