#!/usr/bin/env python3

import pcbnew

filename = 'usb-led-matrix.kicad_pcb'
COUNT = (8, 8)
STARTPOS = (142, 82)
PITCH = (3.5, 3.5)

pcb = pcbnew.LoadBoard(filename)
pcb: pcbnew.BOARD

positions = {f'D{n + 1}': (STARTPOS[0] + PITCH[0] * (n // COUNT[1]),
                           STARTPOS[1] + PITCH[1] * (n % COUNT[1]))
             for n in range(COUNT[0] * COUNT[1])}

for ref, pos in positions.items():
    m = pcb.FindModuleByReference(ref)
    assert m is not None
    m: pcbnew.MODULE
    m.SetPosition(pcbnew.wxPointMM(*pos))
    m.Value().SetVisible(False)
    m.Reference().SetVisible(False)
    m.SetOrientationDegrees(45)


pcb.Save(filename)
