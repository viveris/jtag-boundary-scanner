"""
Sample program showing import of external library in Python 3
"""
from __future__ import print_function
import bindings as SWIGHERE

class CSwigHere:
    def __init__(self):
        pass

    def pyBSDLDeviceId(self,pathToBSDL):
        return SWIGHERE.pyBSDLDeviceId(pathToBSDL);

    def pyBSDLPinSequence(self,pathToBSDL):
        return SWIGHERE.pyBSDLPinSequence(pathToBSDL);

    def __enter__(self):
        SWIGHERE.pyInitSwigHere();
        return self

    def __exit__(self,exType,exValue,trace):
        SWIGHERE.pyUninitSwigHere();

if __name__=="__main__":
    import sys
    with CSwigHere() as instance:
        if len(sys.argv) > 1:
            deviceId = instance.pyBSDLDeviceId(sys.argv[1])
            if deviceId > 0:
                pinList = instance.pyBSDLPinSequence(sys.argv[1])
                print(pinList)

