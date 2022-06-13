#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xcSwigHere.h"

int main(int argc, char * argv[]) {
    int lastResult=-1;
    unsigned cbInt=0;
    unsigned nInts=0;
    const int * ptr;
    xcInitSwigHere();
    if(argc>1) {
	    long unsigned int id = xcBSDLDeviceId(argv[1]);
        if(id > 0 ) {
    	    std::vector<pin_ctrl> pinSequence = xcBSDLPinSequence(argv[1]);
            printf("BSDL file describes device id %lx\r\n",id);
            for(unsigned i = 0; i<pinSequence.size(); i++) {
                printf("%i\t",i);
            }
            printf("\r\n");
        }
    }
    xcUninitSwigHere();
    return lastResult;
}
