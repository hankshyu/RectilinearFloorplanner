'''
Program: prettyRender.py
November 5th, 2023
Iris Lab

A program to visiualize and interact with attributes of a floorplanning
The program shall include side buttons to toggle the visualization of certain attributes
Legends are interactive to select/deselet certain Tessera's visibility
Buttons to rapidly show and hide all tessera

'''

import sys
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.patches as patches
import math
import time
from distinctColours import ColourGenerator
import random
from random import seed

from typing import Optional, Literal
import matplotlib as mpl
import matplotlib.patches as mpatches
import matplotlib.pyplot as plt
from matplotlib.text import Annotation
from matplotlib.transforms import Transform, Bbox
from matplotlib.widgets import CheckButtons
from matplotlib.widgets import Button

COLORRST    = "\u001b[0m"
BLACK       = "\u001b[30m"
RED         = "\u001b[31m"
GREEN       = "\u001b[32m"
YELLOW      = "\u001b[33m"
BLUE        = "\u001b[34m"
MAGENTA     = "\u001b[35m"
CYAN        = "\u001b[36m"
WHITE       = "\u001b[37m"

# This is a global variable to store myChip instance
global_myChip = 0

# Class Tessera is the basic unit of showing info, representing a rectilinear shape
class Tessera:

    def __init__(self, name, TessTypeIn):
        self.TessName = name
        self.TessType = TessTypeIn
        self.Corners = np.array([])
        self.Area = 0
        
        self.Direction = 0
        self.Centre = [0, 0]
        self.Vector = [0, 0]

        self.BBX = 0
        self.BBY = 0
        self.BBW = 0
        self.BBH = 0

        self.initLLX = 0
        self.initLLY = 0

        self.connMask = []

        self.dShape = [True, -1]
        self.dName = [True, -1]
        self.dBBox = [True, -1]
        self.dVector = [True, -1]
        self.dInfo = [True, -1]

    def writeBoundingBox(self, x, y , w, h):
        self.BBX = x
        self.BBY = y
        self.BBW = w
        self.BBH = h

# A Chip class is the data structure that holds all attributes, instantiate and link to global variable for
# full-scope access
class Chip:

    def __init__(self):
        self.chipName = ""
        self.chipWidth = -1
        self.chipHeight = -1
        self.hardModuleCount = 0
        self.hardModuleArr = []
        self.softModuleCount = 0
        self.softModuleArr = []
        
        self.connCount = 0
        self.connArr = []
        self.dConn = []
        self.largestConn = 0
        self.connDict = dict()

        self.avName = True
        self.avVector = True
        self.avXArea = True
        self.avConn = True

        self.lineRead = 0

    def showChip(self):
        if self.chipName != "":
            print(r"CHIP Name = {}, ".format(self.chipName), end = "")
        else:
            print("CHIP ", end = "")
        print(r"W = {}, H = {}, ".format(self.chipWidth, self.chipHeight), end="")
        print(r"F/S = {}/{}".format(len(self.hardModuleArr), len(self.softModuleArr)))

        print(r"There are {}{} Hard-Modules{}".format(GREEN, self.hardModuleCount, COLORRST))
        for hardM in self.hardModuleArr:
            print(r"{}: C{}, Area = {}, ".format(hardM.TessName, len(hardM.Corners), hardM.Area), end = "")
            print(r"BB = [({}, {}), H = {}, W = {}]".format(hardM.BBX, hardM.BBY, hardM.BBW, hardM.BBH))

        print(r"There are {}{} Soft-Modules{}".format(CYAN, self.softModuleCount, COLORRST))
        for softM in self.softModuleArr:
            print(r"{}: C{}, Area = {}, ".format(softM.TessName, len(softM.Corners), softM.Area), end = "")
            print(r"BB = [({}, {}), H = {}, W = {}]".format(softM.BBX, softM.BBY, softM.BBW, softM.BBH))
            print(r"initLL = ({}, {})".format(softM.BBX, softM.BBY, softM.BBW, softM.BBH))
            print(r"Centre@({}, {}), Orient = {:.4f}, Vector = ({}, {})".format(softM.Centre[0], softM.Centre[1], softM.Direction, softM.Vector[0], softM.Vector[1]))

        print(r"There are {}{} Connections{}".format(BLUE, self.connCount, COLORRST))
        for idx, con in enumerate(self.connArr):
            print(r"{}: {} -- {} = {}".format(idx, con[0], con[1], con[2]))

    def readChip(self, finCase):
        self.__readChipContour(finCase)
        self.__readHardModules(finCase)
        self.__readSoftModules(finCase)
        self.__readConnections(finCase)
        self.__processBoundingBox()
        self.__processConnections()

    def __readChipContour(self, finCase):
        chipCounterLine = finCase[self.lineRead].split(" ")
        self.lineRead += 1
        self.chipWidth = int(chipCounterLine[1])
        self.chipHeight = int(chipCounterLine[2])
    
    def __readHardModules(self, finCase):
        hardModulesHead = finCase[self.lineRead].split(" ")
        self.lineRead += 1
        if hardModulesHead[0] != "HARDMODULE":
            sys.exit(r"{}Format Incorrect!{} Missing {}HARDMODULE{}".format(RED, COLORRST, YELLOW, COLORRST))
    
        self.hardModuleCount = int(hardModulesHead[1])
        for i in range(self.hardModuleCount):
            hHead = finCase[self.lineRead].split(" ")
            cornerNums = int(hHead[1])
            self.lineRead += 1
            self.hardModuleArr.append(Tessera(hHead[0], "HardTessera"))
            storeStartX = 0
            storeStartY = 0
            for cn in range(cornerNums):
                hardCorners = finCase[self.lineRead].split(" ")
                self.lineRead += 1
                self.hardModuleArr[-1].Corners = np.append(self.hardModuleArr[-1].Corners, [int(hardCorners[0]), int(hardCorners[1])])
                if cn == 0:
                    storeStartX = int(hardCorners[0])
                    storeStartY = int(hardCorners[1])
            self.hardModuleArr[-1].Corners = np.append(self.hardModuleArr[-1].Corners, [storeStartX, storeStartY])
            
            # Start Reading Tessera Specific Attributes
            self.hardModuleArr[-1].Area = int(finCase[self.lineRead])
            self.lineRead += 1

            hardBBRaw = finCase[self.lineRead].split(" ")
            self.lineRead += 1
            self.hardModuleArr[-1].writeBoundingBox(int(hardBBRaw[0]), int(hardBBRaw[1]), int(hardBBRaw[2]), int(hardBBRaw[3]))

    def __readSoftModules(self, finCase):
        softModulesHead = finCase[self.lineRead].split(" ")
        self.lineRead += 1
        if softModulesHead[0] != "SOFTMODULE":
            sys.exit(r"{}Format Incorrect!{} Missing {}SOFTMODULE{}".format(RED, COLORRST, YELLOW, COLORRST))
    
        self.softModuleCount = int(softModulesHead[1])
        for i in range(self.softModuleCount):
            hHead = finCase[self.lineRead].split(" ")
            cornerNums = int(hHead[1])
            self.lineRead += 1
            self.softModuleArr.append(Tessera(hHead[0], "SoftTessera"))
            storeStartX = 0
            storeStartY = 0
            for cn in range(cornerNums):
                softCorners = finCase[self.lineRead].split(" ")
                self.lineRead += 1
                self.softModuleArr[-1].Corners = np.append(self.softModuleArr[-1].Corners, [int(softCorners[0]), int(softCorners[1])])
                if cn == 0:
                    storeStartX = int(softCorners[0])
                    storeStartY = int(softCorners[1])

            self.softModuleArr[-1].Corners = np.append(self.softModuleArr[-1].Corners, [storeStartX, storeStartY])

            # Start Reading Tessera Specific Attributes
            self.softModuleArr[-1].Area = int(finCase[self.lineRead])
            self.lineRead += 1

            softBBRaw = finCase[self.lineRead].split(" ")
            self.lineRead += 1
            self.softModuleArr[-1].writeBoundingBox(int(softBBRaw[0]), int(softBBRaw[1]), int(softBBRaw[2]), int(softBBRaw[3]))

            softInitLL = finCase[self.lineRead].split(" ")
            self.lineRead += 1
            self.softModuleArr[-1].initLLX = int(softInitLL[0])
            self.softModuleArr[-1].initLLY = int(softInitLL[1])
            self.softModuleArr[-1].Direction = random.uniform(-math.pi, math.pi)

    def __readConnections(self, finCase):
        connHead = finCase[self.lineRead].split(" ")
        self.lineRead += 1
        if connHead[0] != "CONNECTIONS":
            sys.exit(r"{}Format Incorrect!{} Missing {}CONNECTIONS{}".format(RED, COLORRST, YELLOW, COLORRST))
        self.connCount = int(connHead[1])
        for i in range(self.connCount):
            connInfo = finCase[self.lineRead].split(" ")
            self.lineRead += 1
            connNum = int(connInfo[2])
            self.connArr.append([connInfo[0], connInfo[1], connNum])
            if connNum > self.largestConn:
                self.largestConn = connNum

    def __processBoundingBox(self):
        for hm in self.hardModuleArr:
            hm.Centre[0] = float(hm.BBX + (hm.BBW)/2)
            hm.Centre[1] = float(hm.BBY + (hm.BBH)/2)

        
        for sm in self.softModuleArr:
            sm.Centre[0] = float(sm.BBX + (sm.BBW)/2)
            sm.Centre[1] = float(sm.BBY + (sm.BBH)/2)
            vectorlength = 0.2 * min(sm.BBW, sm.BBH)
            sm.Vector[0] = vectorlength * float(math.cos(sm.Direction))
            sm.Vector[1] = vectorlength * float(math.sin(sm.Direction))

    def __processConnections(self):
        # create dictionary to link module name <-> module object
        for hm in self.hardModuleArr:
            self.connDict[hm.TessName] = hm
            hm.connMask = [True] * self.connCount

        for sm in self.softModuleArr:
            self.connDict[sm.TessName] = sm
            sm.connMask = [True] * self.connCount

        # finish the connMask
        for connIdx, conn in enumerate(self.connArr):
            self.connDict[conn[0]].connMask[connIdx] = True
            self.connDict[conn[1]].connMask[connIdx] = True
            if self.largestConn < conn[2]:
                self.largestConn = conn[2]
      
# function to create a text element(block) which has adjustable size
def text_with_autofit(
    ax: plt.Axes,
    txt: str,
    xy: tuple[float, float],
    width: float, height: float,
    *,
    transform: Optional[Transform] = None,
    ha: Literal['left', 'center', 'right'] = 'center',
    va: Literal['bottom', 'center', 'top'] = 'center',
    show_rect: bool = False,
    **kwargs,
):
    if transform is None:
        transform = ax.transData

    #  Different alignments give different bottom left and top right anchors.
    x, y = xy
    xa0, xa1 = {
        'center': (x - width / 2, x + width / 2),
        'left': (x, x + width),
        'right': (x - width, x),
    }[ha]
    ya0, ya1 = {
        'center': (y - height / 2, y + height / 2),
        'bottom': (y, y + height),
        'top': (y - height, y),
    }[va]
    a0 = xa0, ya0
    a1 = xa1, ya1

    x0, y0 = transform.transform(a0)
    x1, y1 = transform.transform(a1)
    # rectangle region size to constrain the text in pixel
    rect_width = x1 - x0
    rect_height = y1 - y0

    fig: plt.Figure = ax.get_figure()
    dpi = fig.dpi
    rect_height_inch = rect_height / dpi
    # Initial fontsize according to the height of boxes
    fontsize = rect_height_inch * 72

    text: Annotation = ax.annotate(txt, xy, ha=ha, va=va, xycoords=transform, **kwargs)

    # Adjust the fontsize according to the box size.
    text.set_fontsize(fontsize)
    bbox: Bbox = text.get_window_extent(fig.canvas.get_renderer())
    adjusted_size = fontsize * rect_width / bbox.width
    text.set_fontsize(adjusted_size)

    if show_rect:
        rect = mpatches.Rectangle(a0, width, height, fill=False, ls='--')
        ax.add_patch(rect)

    return text, Annotation

# This is the Handler for checkboxes that could toggle attributes of the Tessera
def handleCheckButtonsClick(label):
    print(r"handleCheckButtonsClick: {}".format(label))

    if label == "Bounding Box":
        global_myChip.avBBox = not global_myChip.avBBox
        for sm in global_myChip.softModuleArr:
            sm.dBBox[0] = global_myChip.avBBox and sm.dShape[0]
            sm.dBBox[1].set_visible(sm.dBBox[0])
    elif label == "Vector":
        global_myChip.avVector = not global_myChip.avVector
        for sm in global_myChip.softModuleArr:
            sm.dVector[0] = global_myChip.avVector and sm.dShape[0]
            sm.dVector[1].set_visible(sm.dVector[0])
    elif label == "Name":
        global_myChip.avName = not global_myChip.avName
        for sm in global_myChip.softModuleArr:
            sm.dName[0] = global_myChip.avName and sm.dShape[0]
            sm.dName[1].set_visible(sm.dName[0])
    elif label == "Connections":
        global_myChip.avConn = not global_myChip.avConn
        for cnIdx, cn in enumerate(global_myChip.dConn):
            global_myChip.dConn[cnIdx][1][0].set_visible(global_myChip.avConn)

    plt.draw()

# Handler for interactive legend, finds the corresponding element and to visibility
def legnedPickTessHandler(event):
    legTess = event.artist
    origTess = ClickElementdictionary[legTess]
    

    newVisibility = not origTess.dShape[0]
    origTess.dShape[0] = newVisibility
    
    origTess.dShape[1].set_visible(newVisibility)

    if newVisibility == True:
        origTess.dName[0] = global_myChip.avName
        origTess.dBBox[0] = global_myChip.avBBox
        origTess.dVector[0] = global_myChip.avVector
    else:
        origTess.dName[0] = False
        origTess.dBBox[0] = False
        origTess.dVector[0] = False

    origTess.dName[1].set_visible(origTess.dName[0])
    origTess.dBBox[1].set_visible(origTess.dBBox[0])
    origTess.dVector[1].set_visible(origTess.dVector[0])
    
    if newVisibility:
        legTess.set_alpha(1.0)
    else:
        legTess.set_alpha(0.1)
    
    fig.canvas.draw()

# Handler function for showing all Teseerae button
def showAllTesserae(event):
    visibilityGoal = True
    for Tess in global_myChip.softModuleArr:
        Tess.dShape[0] = visibilityGoal
        Tess.dShape[1].set_visible(Tess.dShape[0])

        Tess.dName[0] = global_myChip.avName
        Tess.dBBox[0] = global_myChip.avBBox
        Tess.dVector[0] = global_myChip.avVector

        Tess.dName[1].set_visible(Tess.dName[0])
        Tess.dBBox[1].set_visible(Tess.dBBox[0])
        Tess.dVector[1].set_visible(Tess.dVector[0])

    fig.canvas.draw()

# Handler function for Hiding all Teseerae button
def HideAllTesserae(event):
    visibilityGoal = False
    for Tess in global_myChip.softModuleArr:
        Tess.dShape[0] = visibilityGoal
        Tess.dShape[1].set_visible(Tess.dShape[0])

        Tess.dName[0] = False
        Tess.dBBox[0] = False
        Tess.dVector[0] = False

        Tess.dName[1].set_visible(Tess.dName[0])
        Tess.dBBox[1].set_visible(Tess.dBBox[0])
        Tess.dVector[1].set_visible(Tess.dVector[0])
    
    fig.canvas.draw()

if __name__ == '__main__':

    # check args
    if len(sys.argv)!= 2:
        print("usage:\tpython3 prettyRender.py [input file]")
        sys.exit(4)
        


    seed(17)
    print("Parser Program building ...")

    # A ColourGenerator class generates colors that are visually seperate, using precomputed LUT
    colourG = ColourGenerator()

    
    rawData_fileName = sys.argv[1]
    file_read_case = open(rawData_fileName, 'r')
    fin_case = file_read_case.read().split("\n")

    myChip = Chip()
    myChip.readChip(fin_case)
    myChip.showChip()

    fig, ax = plt.subplots()
    BORDER = int((myChip.chipHeight + myChip.chipWidth)/20)
    ax.set_xbound(0, myChip.chipWidth)
    ax.set_ybound(0, myChip.chipHeight)
    ax.set_xlim([-0.2 * myChip.chipWidth, 1.2 * myChip.chipWidth])
    ax.set_ylim([-0.2 * myChip.chipHeight, 1.2 * myChip.chipHeight])
    
    global_myChip = myChip
    
    # This the chip's contour
    ax.add_patch(
        patches.Rectangle(
            (0, 0),
            myChip.chipWidth,
            myChip.chipHeight,
            fill=False,
            edgecolor="#000000",
            alpha=1.0
        )
    )

    # Plot the hard module's contour
    for hmIdx, hm in enumerate(myChip.hardModuleArr):
        hm.dShape[1] = patches.Polygon(hm.Corners.reshape(-1, 2),
                            closed=True,
                            fill=True,
                            facecolor="#808080",
                            edgecolor="#000000",
                            alpha=1.0
                        )

        ax.add_patch(hm.dShape[1])

    # This is the array that collects patches for legend toggle
    ClickElements = []

    # Plot the soft module's Information
    for smIdx, sm in enumerate(myChip.softModuleArr):
        ClickElements.append(sm)
        sm.dShape[1] = patches.Polygon(sm.Corners.reshape(-1, 2),
                            closed=True,
                            fill=True,
                            facecolor=colourG.getColour(),
                            edgecolor="#000000",
                            alpha=0.63,
                            label = sm.TessName
                        )
        ax.add_patch(sm.dShape[1])
        
        sm.dBBox[1] = patches.Rectangle((sm.BBX, sm.BBY),
                sm.BBW,
                sm.BBH,
                fill=False,
                edgecolor="#ff0000",
                alpha=0.35,  # 0.3 original
                linestyle = '--',
                linewidth = 1.0
                
            )

        ax.add_patch(sm.dBBox[1])

        sm.dName[1], _ = text_with_autofit(ax, sm.TessName, (int(sm.BBX+0.5*sm.BBW), int(sm.BBY+0.6*sm.BBH)), 0.7*sm.BBW, 0.3*sm.BBH, show_rect = False)
        sm.dVector[1] = plt.arrow(sm.Centre[0], sm.Centre[1], sm.Vector[0], sm.Vector[1], width = 10.0, head_width = 150.0, color = 'g')
    
    # Plot connections
    for connIdx, conn in enumerate(myChip.connArr):
        leftCentre = myChip.connDict[conn[0]].Centre
        rightCentre = myChip.connDict[conn[1]].Centre
        lw = (conn[2]/myChip.largestConn)*2.5
        newLine = ax.plot([leftCentre[0], rightCentre[0]], [leftCentre[1], rightCentre[1]], linewidth=(1+lw), color='b')
        newConn = [True, newLine]
        myChip.dConn.append(newConn)

    
    # Create checkboxes to select/unselect certain Tessera attributes
    rax = fig.add_axes([0.6, 0.05, 0.15, 0.15])
    CBlabels = ['Name', 'Bounding Box', 'Vector', 'Connections', 'Info']
    CBvisible = [myChip.avName, myChip.avBBox, myChip.avVector, myChip.avConn, myChip.avInfo]
    check = CheckButtons(rax, CBlabels, CBvisible)
    check.on_clicked(handleCheckButtonsClick)
    

    # Legedns could toggle individual tessera to show/hide
    leg = ax.legend(loc='upper left', bbox_to_anchor=(1.05, 1), ncol= 3, borderaxespad=0)
    fig.subplots_adjust(right=0.55, bottom = 0.2)
    plt.suptitle("Interactive Visualisation Tool", va='top', size='large')

    
    ClickElementdictionary = dict()
    for legTess, origTess in zip(leg.get_patches(), ClickElements):
        legTess.set_picker(10) # 10 pts tolerance
        ClickElementdictionary[legTess] = origTess
    
    fig.canvas.mpl_connect('pick_event', legnedPickTessHandler)

    # Buttons to Show or Hide all Tesserae
    axShowAll = fig.add_axes([0.6, 0.25, 0.09, 0.05])
    buttonShowAll = Button(axShowAll, 'Show all')
    buttonShowAll.on_clicked(showAllTesserae)

    axHideAll = fig.add_axes([0.75, 0.25, 0.09, 0.05])
    buttonHideAll = Button(axHideAll, 'Hide all')
    buttonHideAll.on_clicked(HideAllTesserae)

    plt.show()