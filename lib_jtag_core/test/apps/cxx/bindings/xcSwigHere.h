// xcSwigHere.h
#pragma once

#include <vector>
#include "epSwigHere.h"

extern void xcInitSwigHere();
extern unsigned long int xcBSDLDeviceId(char * pathToBSDL);
extern std::vector<pin_ctrl> xcBSDLPinSequence(char * pathToBSDL);
extern void xcUninitSwigHere();
