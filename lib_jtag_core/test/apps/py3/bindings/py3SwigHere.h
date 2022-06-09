// py3SwigHere.h
#pragma once

#include <vector>
#include <string>

extern void pyInitSwigHere();
extern void pyUninitSwigHere();
extern unsigned long int pyBSDLDeviceId(std::string pathToBSDL);
extern std::vector<int> pyBSDLPinSequence(std::string pathToBSDL);
