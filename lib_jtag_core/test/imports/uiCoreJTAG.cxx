#include "epSwigHere.h"

#include "libjtag_bsdl.h"

int main(int argc, char * argv[], char ** envp) {
    SWIGHERE_CONTEXT context = epInitSwigHere();
    if(context != NULL) {
    if(argc>1) {
	    long unsigned int id = epBSDLDeviceId(context, argv[1]);
	    //SWIGHERE_INTLIST epSequence(SWIGHERE_CONTEXT oContext,unsigned * pBytesPerElement,unsigned * pElementCount);
	    //extern SWIGHERE_INTLIST epBSDLPinMap(SWIGHERE_CONTEXT oContext,unsigned * pBytesPerElement,unsigned * pElementCount);
            printf("BSDL file describes device id %lx\r\n",id);
        }
        epUninitSwigHere(context);
        context = NULL;
    }
    return 0;
}

