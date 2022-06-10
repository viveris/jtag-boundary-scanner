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
/*
typedef struct _pin_ctrl
{
	char pinname[MAX_ELEMENT_SIZE];
	int  pintype;

	char physical_pin[MAX_ELEMENT_SIZE];

	int ctrl_bit_number;
	int out_bit_number;
	int in_bit_number;
}pin_ctrl;
*/

extern std::vector<std::string> pyBSDLPinSequence(std::string pathToBSDL) {
    std::vector<std::string> result;
    if(pContext) {
        unsigned bytesPerElement=0, elementCount=0;
        std::vector<pin_ctrl> pinSequence = epBSDLPinSequence(pContext, (char*)pathToBSDL.c_str(), &bytesPerElement);
        if( bytesPerElement == sizeof(pin_ctrl) ) {
            for(unsigned i = 0; i<pinSequence.size(); i++) {
                static char formatter[] = {
                    "{"
                    "\"name\":\"%0.64s\","
                    "\"physical\":\"%0.64s\","
                    "\"type\": %8i,"
                    "\"ctrl_bit\": %8i,"
                    "\"out_bit\": %8i,"
                    "\"in_bit\": %8i"
                    "}"
                };
                char bfJsonConversion[256]="";
                sprintf(bfJsonConversion,formatter,
                    pinSequence[i].pinname,
                    pinSequence[i].physical_pin,
                    pinSequence[i].pintype,
                    pinSequence[i].ctrl_bit_number,
                    pinSequence[i].out_bit_number,
                    pinSequence[i].in_bit_number
                );
                result.push_back(bfJsonConversion);
            }
        }        
    }

    return result;
}

/*
typedef struct _jtag_chain
{
	int bit_index;

	int bit_cell_type;                // BC_1,BC_2,...

	char pinname[MAX_ELEMENT_SIZE];   // Pin name.

	int bit_type;                     // None , ctrl , in, out.

	int safe_state;                   // Default - Safe state. (0,1,-1)

	int control_bit_index; // Indicate the associated control bit. -1 if no control bit.
	int control_disable_state;
	int control_disable_result;

}jtag_chain;
*/

extern std::vector<std::string> pyBSDLChainSequence(std::string pathToBSDL) {
    std::vector<std::string> result;
    if(pContext) {
        unsigned bytesPerElement=0, elementCount=0;
        std::vector<jtag_chain> chainSequence = epBSDLChainSequence(pContext, (char*)pathToBSDL.c_str(), &bytesPerElement);
        if( bytesPerElement == sizeof(jtag_chain) ) {
            for(unsigned i = 0; i < chainSequence.size(); i++) {
                static char formatter[] = {
                    "{"
                    "\"name\":\"%0.64s\","
                    "\"index\": %8i,"
                    "\"cell_type\": %8i,"
                    "\"bit_type\": %8i"
                    "}"
                };
                char bfJsonConversion[256]="";
                sprintf(bfJsonConversion,formatter,
                    chainSequence[i].pinname,
                    chainSequence[i].bit_index,
                    chainSequence[i].bit_cell_type,
                    chainSequence[i].bit_type
                );
                result.push_back(bfJsonConversion);
            }
        }        
    }

    return result;
}
