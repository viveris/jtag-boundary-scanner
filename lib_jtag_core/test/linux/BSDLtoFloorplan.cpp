#include <stddef.h>
#include <stdio.h>
#include "jtag_core.h"
#include "bsdl_parser/bsdl_loader.h"
#include "bsdl_parser/bsdl_strings.h"

int main(int argc,char * argv[]) {
    if(argc > 1) {
        jtag_core * jc = jtagcore_init();
        if (jc != NULL ) {
	    jtag_bsdl * details = load_bsdlfile(jc, argv[1]);
            if ( details != NULL ) {
                unsigned long  id = details->chip_id;
		        int irLength = details->number_of_bits_per_instruction;
                if(irLength>0) {
    		        int chainCount = details->number_of_chainbits;
	    	        int pinCount = details->number_of_pins;
                    printf("Chip ID %0lx OK: uses %i-bit OPCODES\r\n",id,irLength);
                    printf("Chip ID %0lx OK: has %i pins with %i chains\r\n",id,pinCount,chainCount);
                   
                } else {
                    printf("BSDL file for chip ID %0lx has unsupported OPCODE length\r\n",id);
                }
                unload_bsdlfile(jc,details);
                details = NULL;
            } else {
                printf("Unable to parse file %s\r\n",argv[1]);
            }

            jtagcore_deinit(jc);
            return 0;
        }
        jtagcore_deinit(NULL);
        return 1;
   }
   return -1;
}
