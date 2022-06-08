#include <stddef.h>
#include <stdio.h>
#include "libjtag_bsdl.h"

#include "epSwigHere.h"
#include <memory>
#include <type_traits>
#include <vector>


static void logger(char * string) {
    printf("%s",string);
}

static class CLitterPile : private std::vector<SWIGHERE_CAST> {
private:
    JTAGCORE_PRINT_FUNC logger;
public:
    CLitterPile() {logger=::logger;}
    void ensureInit() {
    }
    void deinit() {
    }
    void clear() {
        //std::vector &self=*this;
        for(SWIGHERE_CAST typedPtr : *this) {
            switch(typedPtr.first) {
            case SWIGHERE_TYPE::INTPTR: {
                int * litter=(int*)typedPtr.second;
            	delete [] litter;
            }
            break;
            default: // don't scope the default block
            break;}
        }
        std::vector<SWIGHERE_CAST>::clear();
    }
    SWIGHERE_CAST push_back(const int * p) {
    	SWIGHERE_CAST tail(SWIGHERE_TYPE::INTPTR,(void*)p);
        std::vector<SWIGHERE_CAST>::push_back(tail);
        return tail;
    }
    ~CLitterPile() {
    	clear();
    }
} garbageCollector;

extern SWIGHERE_CONTEXT epInitSwigHere() {
    garbageCollector.ensureInit();
    return (SWIGHERE_CONTEXT) &garbageCollector;
}

extern void epUninitSwigHere(SWIGHERE_CONTEXT oContext) {
    if(oContext == (SWIGHERE_CONTEXT) &garbageCollector) {
        garbageCollector.clear();
        garbageCollector.deinit();
    }
}

extern long unsigned int epBSDLDeviceId(SWIGHERE_CONTEXT oContext, char * pathToBSDL) {
    long int result = -1;
    if(oContext == (SWIGHERE_CONTEXT)&garbageCollector) {
    	jtag_bsdl * details = jtag_bsdl_load_file(logger,MSG_DEBUG, 0, pathToBSDL);
        if ( details != NULL ) {
            result = details->chip_id;
            jtag_bsdl_unload_file(details);
        } else {
            result = 0;
        }
    }
    return (long unsigned int) result;
}
