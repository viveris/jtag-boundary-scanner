"""
Sample program showing import of external library in Python 3
"""
from __future__ import print_function
import bindings as SWIGHERE
import json

class CSwigHere:
    def __init__(self):
        pass

    def pyBSDLDeviceId(self,pathToBSDL):
        return SWIGHERE.pyBSDLDeviceId(pathToBSDL);

    def pyBSDLPinSequence(self,pathToBSDL):
        return SWIGHERE.pyBSDLPinSequence(pathToBSDL);

    def pyBSDLChainSequence(self,pathToBSDL):
        return SWIGHERE.pyBSDLChainSequence(pathToBSDL);

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
                chainMembers = []
                pinList = instance.pyBSDLPinSequence(sys.argv[1])
                chainList = instance.pyBSDLChainSequence(sys.argv[1])
                pinListJSON = [json.loads(s) for s in pinList]
                chainListJSON = [ json.loads(s) for s in chainList]
                chainListDict = { chain["index"]: chain for chain in chainListJSON}
                for pin in pinListJSON:
                    chain_positions=[pin[key] for key in ["ctrl_bit","out_bit","in_bit"] if pin[key]>=0]
                    if any(chain_positions):
                        chainMembers.append(pin)
                print(chainListDict)

