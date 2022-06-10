"""
Sample program showing import of external library in Python 3
"""
from __future__ import print_function
import bindings as SWIGHERE
import json
from tkinter import *

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

def pinsToChains(chainListDict,pincount,chainMembers):
    chaincount = len(chainListDict.keys())
    canvasWidth = max(400,6*chaincount//4)
    canvasHeight = canvasWidth
    app = Tk()
    app.geometry("%ix%i"%(canvasWidth,canvasHeight))
    canvas = Canvas(app, bg='black')
    canvas.pack(anchor='nw', fill='both', expand=1)
    chainMargin=30
    pinMargin=90
    canvas.create_rectangle(
        chainMargin, chainMargin, canvasWidth-chainMargin, canvasHeight-chainMargin,
        outline="grey",
        fill="white"
    )
    canvas.create_rectangle(
        pinMargin, pinMargin, canvasWidth-pinMargin, canvasHeight-pinMargin,
        outline="black",
        fill="grey"
    )
    chainSize = (canvasWidth-2*chainMargin,canvasHeight-2*chainMargin)
    chainSpacing = (chainSize[0]/(chaincount//4),chainSize[1]/(chaincount//4))
    canvas.create_text(
        canvasWidth - chainMargin - chainSpacing[0],chainMargin + chainSpacing[1],
        anchor=NE,
        fill="darkblue",
        font="Times 20 italic bold",
        text="1"
    )

    for chain in range(chaincount):
        if chain in range(chaincount//4):
            x = canvasWidth - chainMargin - chainSpacing[0]//2
            y = chainMargin + chainSpacing[1]//2 + chainSpacing[1]*(chain%(chaincount//4))
        elif chain in range(chaincount//4,chaincount//2):
            y = canvasWidth-chainMargin-chainSpacing[1]//2
            x = canvasWidth - chainMargin - chainSpacing[0]//2 - chainSpacing[0]*(chain%(chaincount//4))
        elif chain in range(chaincount//2,3*chaincount//4):
            x = chainMargin + chainSpacing[0]//2
            y = canvasWidth - chainMargin - chainSpacing[1]//2 - chainSpacing[1]*(chain%(chaincount//4))
        else:
            y = chainMargin + chainSpacing[1]//2
            x = chainMargin + chainSpacing[0]//2 + chainSpacing[0]*(chain%(chaincount//4))

        canvas.create_oval(
            x-2, y-2, x+2, y+2,
            fill="grey",
            tag="chain%i"%chain
        )

    pinoutSize = (canvasWidth-2*pinMargin,canvasHeight-2*pinMargin)
    pinSpacing = (pinoutSize[0]/(pincount//4),pinoutSize[1]/(pincount//4))
    canvas.create_text(
        canvasWidth - pinMargin - pinSpacing[0],pinMargin + pinSpacing[1],
        anchor=NE,
        fill="darkblue",
        font="Times 20 italic bold",
        text="1"
    )

    for pin in range(pincount):
        if pin in range(pincount//4):
            x = canvasWidth - pinMargin - pinSpacing[0]//2
            y = pinMargin + pinSpacing[1]//2 + pinSpacing[1]*(pin%(pincount//4))
        elif pin in range(pincount//4,pincount//2):
            y = canvasWidth-pinMargin-pinSpacing[1]//2
            x = canvasWidth - pinMargin - pinSpacing[0]//2 - pinSpacing[0]*(pin%(pincount//4))
        elif pin in range(pincount//2,3*pincount//4):
            x = pinMargin + pinSpacing[0]//2
            y = canvasWidth - pinMargin - pinSpacing[1]//2 - pinSpacing[1]*(pin%(pincount//4))
        else:
            y = pinMargin + pinSpacing[1]//2
            x = pinMargin + pinSpacing[0]//2 + pinSpacing[0]*(pin%(pincount//4))

        canvas.create_oval(
            x-2.5, y-2.5, x+2.5, y+2.5,
            fill="white",
            tag="pin%i"%pin
        )

    canvas.create_text(
        canvasWidth//2,canvasHeight//2,
        fill="darkblue",
        font="Times 20 italic bold",
        text="Floorplan showing relationship between pins and chains"
    )

    app.mainloop()

if __name__=="__main__":
    import sys
    with CSwigHere() as instance:
        if len(sys.argv) > 1:
            deviceId = instance.pyBSDLDeviceId(sys.argv[1])
            if deviceId > 0:
                chainMembers = {}
                pinList = instance.pyBSDLPinSequence(sys.argv[1])
                chainList = instance.pyBSDLChainSequence(sys.argv[1])
                pinListJSON = [json.loads(s) for s in pinList]
                chainListJSON = [ json.loads(s) for s in chainList]
                chainListDict = { chain["index"]: chain for chain in chainListJSON}
                for pinnumber,pin in enumerate(pinListJSON):
                    chain_positions=[pin[key] for key in ["ctrl_bit","out_bit","in_bit"] if pin[key]>=0]
                    if any(chain_positions):
                        chainMembers[pinnumber]=pin
                pinsToChains(chainListDict,len(pinList),chainMembers)

