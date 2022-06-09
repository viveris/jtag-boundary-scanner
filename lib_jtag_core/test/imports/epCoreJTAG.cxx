#include <stddef.h>
#include <stdio.h>

#include "epSwigHere.h"
#include <memory>
#include <type_traits>

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

extern std::vector<pin_ctrl> epBSDLPinSequence(SWIGHERE_CONTEXT oContext, char * pathToBSDL, unsigned * pBytesPerElement) {
    int * p=NULL;
    std::vector<pin_ctrl> result;

    if(pBytesPerElement != NULL) {
       *pBytesPerElement = sizeof(pin_ctrl);
    }
    if(oContext==(SWIGHERE_CONTEXT)&garbageCollector) {
    	jtag_bsdl * details = jtag_bsdl_load_file(logger,MSG_DEBUG, 0, pathToBSDL);
        if ( details != NULL ) {
            int number_of_pins = details->number_of_pins;
            int i = 0;
            if(number_of_pins>0) {
                while(i<number_of_pins) {
                    pin_ctrl * pins_list = details->pins_list;
                    result.push_back(pins_list[i]);
                    i++;
                }
            }
        }
    }

    return result;
};

//extern std::vector<jtag_chain> epBSDLChainSequence(SWIGHERE_CONTEXT oContext, char * pathToFile, unsigned * pBytesPerElement,unsigned * pElementCount);
//extern std::vector<pin_ctrl>  epBSDLPinSequence(SWIGHERE_CONTEXT oContext, char * pathToBSDL, unsigned * pBytesPerElement,unsigned * pElementCount) {
