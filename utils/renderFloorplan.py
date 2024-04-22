'''
Program: renderFloorplan.py
April 10th, 2024
Iris Lab

A program to visiualize and interact with Floorplan outline
'''

import sys
sys.path.append("./lib/")
from argparse import ArgumentParser
import re

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

# GLOBAL VARIABLES
USE_DEGUB = 0

# This is a global variable to store myChip instance
global_myChip = 0

# Class Cord represent a 2D coordinate system
class Cord:
    def __init__(self, x, y):
        self.x = x
        self.y = y
    def __str__(self) -> str:
        return "({}, {})".format(self.x, self.y)
    def __eq__(self, other):
        if isinstance(other, Cord):
            return ((self.x == other.x) and (self.y == other.y))
        return False
    def __hash__(self) -> int:
        return hash((self.x, self.y))

# Class Rectilinear is the basic unit of showing info, representing the Rectilinear data structure in cornerStitching
class Rectilnear:
    def __init__(self, rectName, rectType):
        self.name = rectName
        self.type = rectType
        self.xarea = 0
        self.bbox = np.array([])
        self.centreX= 0
        self.centreY = 0
        self.optCentreX = 0
        self.optCentreY = 0
        self.corners = np.array([])

        self.dShape = [True, -1]
        self.dName = [True, -1]
        self.dXArea = [True, -1]
        self.dVector = [True, -1]
        self.dBBox = [True, -1]

    def __str__(self) -> str:
        MeStr = self.name + " "

        if self.type == "SOFT":
            MeStr += "Type: SOFT "
            MeStr += ("ExArea = " + str(self.xarea) + " " )
            MeStr += "BBOX[({}, {}) ({}, {})] ".format(self.bbox[0].x, self.bbox[0].y, self.bbox[2].x, self.bbox[2].y)
            MeStr += "Vector ({}, {}) -> ({}, {})".format(self.centreX, self.centreY, self.optCentreX, self.optCentreY)
            MeStr += "\n"
            for cord in self.corners:
                MeStr += "({}, {}) ".format(cord.x, cord.y)
            MeStr += '\n'
        elif self.type == "PREPLACED":
            MeStr += "Type: PREPLACED "
            for cord in self.corners:
                MeStr += "({}, {}) ".format(cord.x, cord.y)
            MeStr += '\n'
        elif self.type == "PIN":
            MeStr += "Type: PIN Centre = ({}, {})".format(self.centre.x, self.centre.y)
        else:
            MeStr += "Type: Error "

        return MeStr
  
# A Chip class is the data structure that holds all attributes, instantiate and link to global variable for
# full-scope access
class Chip:

    def __init__(self):
        self.chipName = ""
        self.chipWidth = -1
        self.chipHeight = -1
        self.softModuleCount = 0
        self.softModuleArr = []
        self.prePlaceddModuleCount = 0
        self.prePlacedModuleArr = []
        self.pinModuleCount = 0
        self.pinModuleArr = []
        
        self.connCount = 0
        self.connArr = []
        self.dConn = []
        self.largestConn = 0
        self.connDict = dict()

        self.avShape = True
        self.avName = True
        self.avBBox = True
        self.avVector = True
        self.avXarea = True
        self.avConn = True

        self.lineRead = 0


    def readChip(self, finCase):
        self.__readChipContour(finCase)
        self.__readSoftModules(finCase)
        self.__readPrePlacedModules(finCase)
        self.__readPinModules(finCase)
        self.__readConnections(finCase)
        self.__processConnections()

    def __readChipContour(self, finCase):
        chipCounterLine = finCase[self.lineRead].split(" ")
        self.lineRead += 1
        self.chipWidth = int(chipCounterLine[1])
        self.chipHeight = int(chipCounterLine[2])

    def __readSoftModules(self, finCase):
        softModulesHead = finCase[self.lineRead].split(" ")
        self.lineRead += 1
        if softModulesHead[0] != "SOFTBLOCK":
            sys.exit(r"{}Format Incorrect!{} Missing {}SOFTBLOCK{}".format(RED, COLORRST, YELLOW, COLORRST))
        self.softModuleCount = int(softModulesHead[1])

        for i in range(self.softModuleCount):
            hHead = finCase[self.lineRead].split(" ")
            self.lineRead += 1
            self.softModuleArr.append(Rectilnear(hHead[0], "SOFT"))
            if USE_DEGUB == 1:
                self.softModuleArr[-1].xarea = hHead[1]
            else:
                self.softModuleArr[-1].xarea = int(hHead[1])

            # read the bounding box
            for j in range(4):
                bbstr = finCase[self.lineRead].split(" ")
                self.lineRead += 1
                self.softModuleArr[-1].bbox = np.append(self.softModuleArr[-1].bbox, Cord(int(bbstr[0]), int(bbstr[1])))
            
            # read the arrow 
            arrowstr = finCase[self.lineRead].split(" ")
            self.lineRead += 1
            self.softModuleArr[-1].centreX = int(arrowstr[0])
            self.softModuleArr[-1].centreY = int(arrowstr[1])
            self.softModuleArr[-1].optCentreX = int(arrowstr[2])
            self.softModuleArr[-1].optCentreY = int(arrowstr[3])
            # read the contour

            contourPoints = finCase[self.lineRead].split(" ")
            self.lineRead += 1
            contourPoints = int(contourPoints[0])
            storeStartX = 0
            storeStartY = 0
            for cn in range(contourPoints):
                softCorners = finCase[self.lineRead].split(" ")
                self.lineRead += 1
                self.softModuleArr[-1].corners = np.append(self.softModuleArr[-1].corners, Cord(int(softCorners[0]), int(softCorners[1])))
                if cn == 0:
                    storeStartX = int(softCorners[0])
                    storeStartY = int(softCorners[1])
            self.softModuleArr[-1].corners = np.append(self.softModuleArr[-1].corners, Cord(storeStartX, storeStartY))

    def __readPrePlacedModules(self, finCase):
        prePlacedModulesHead = finCase[self.lineRead].split(" ")
        self.lineRead += 1
        if prePlacedModulesHead[0] != "PREPLACEDBLOCK":
            sys.exit(r"{}Format Incorrect!{} Missing {}PREPLACEDBLOCK{}".format(RED, COLORRST, YELLOW, COLORRST))
        self.prePlaceddModuleCount = int(prePlacedModulesHead[1])

        for i in range(self.prePlaceddModuleCount):
            hHead = finCase[self.lineRead].split(" ")
            self.lineRead += 1
            self.prePlacedModuleArr.append(Rectilnear(hHead[0], "PREPLACED"))

            # read the contour
            contourPoints = finCase[self.lineRead].split(" ")
            self.lineRead += 1
            contourPoints = int(contourPoints[0])
            storeStartX = 0
            storeStartY = 0
            for cn in range(contourPoints):
                prePlacedCorners = finCase[self.lineRead].split(" ")
                self.lineRead += 1
                self.prePlacedModuleArr[-1].corners = np.append(self.prePlacedModuleArr[-1].corners, Cord(int(prePlacedCorners[0]), int(prePlacedCorners[1])))
                if cn == 0:
                    storeStartX = int(prePlacedCorners[0])
                    storeStartY = int(prePlacedCorners[1])
            self.prePlacedModuleArr[-1].corners = np.append(self.prePlacedModuleArr[-1].corners, Cord(storeStartX, storeStartY))
            # calculate the centre coordinate for preplace modules
            xl = xh = self.prePlacedModuleArr[-1].corners[0].x
            yl = yh = self.prePlacedModuleArr[-1].corners[0].y
            for co in self.prePlacedModuleArr[-1].corners:
                if co.x < xl:
                    xl = co.x
                elif co.x > xh:
                    xh = co.x
                if co.y < yl:
                    yl = co.y
                elif co.y > yh:
                    yh = co.y
            self.prePlacedModuleArr[-1].centreX = int((xl + xh) /2)
            self.prePlacedModuleArr[-1].centreY = int((yl + yh) /2)

    def __readPinModules(self, finCase):
        pinModulesHead = finCase[self.lineRead].split(" ")
        self.lineRead += 1
        if pinModulesHead[0] != "PIN":
            sys.exit(r"{}Format Incorrect!{} Missing {}PIN{}".format(RED, COLORRST, YELLOW, COLORRST))
        self.pinModuleCount = int(pinModulesHead[1])
        for i in range(self.pinModuleCount):
            hHead = finCase[self.lineRead].split(" ")
            self.lineRead += 1
            self.pinModuleArr.append(Rectilnear(hHead[0], "PIN"))
            self.pinModuleArr[-1].centreX = int(hHead[1])
            self.pinModuleArr[-1].centreY = int(hHead[2])
    
    def __readConnections(self, finCase):
        connHead = finCase[self.lineRead].split(" ")
        self.lineRead += 1
        if connHead[0] != "CONNECTION":
            sys.exit(r"{}Format Incorrect!{} Missing {}CONNECTION{}".format(RED, COLORRST, YELLOW, COLORRST))
        connectionCount = int(connHead[1])
        for cnnIdx in range(connectionCount):
            connInfo = finCase[self.lineRead].split(" ")
            self.lineRead += 1
            connNum = int(connInfo[-1])
            connModules = int(connInfo[0])
            for i in range(connModules -1):
                BlockI = connInfo[i + 1]
                j = i+1
                for j in range(i+1, connModules):
                    BlockJ = connInfo[j + 1]
                    self.connArr.append([BlockI, BlockJ, connNum])
                    self.connCount += 1
            if connNum > self.largestConn:
                self.largestConn = connNum

    def __processConnections(self):
        # create dictionary to link module name <-> module object
        for hm in self.prePlacedModuleArr:
            self.connDict[hm.name] = hm
            hm.connMask = [True] * self.connCount

        for sm in self.softModuleArr:
            self.connDict[sm.name] = sm
            sm.connMask = [True] * self.connCount

        for sm in self.pinModuleArr:
            self.connDict[sm.name] = sm
            sm.connMask = [True] * self.connCount
            
        # finish the connMask
        for connIdx, conn in enumerate(self.connArr):
            self.connDict[conn[0]].connMask[connIdx] = True
            self.connDict[conn[1]].connMask[connIdx] = True
            if self.largestConn < conn[2]:
                self.largestConn = conn[2]

    def showChip(self):
        print("Printing attributes of the cornerStitching visualization:")
        print("Chip Contour W = {}, H = {}".format(self.chipWidth, self.chipHeight))

        print("There are total {} Soft Modules ".format(self.softModuleCount))
        for tile in self.softModuleArr:
            print(tile)

        print("There are total {} Preplaced Modules ".format(self.prePlaceddModuleCount))
        for tile in self.prePlacedModuleArr:
            print(tile)

        print("There are total {} Pin Modules ".format(self.pinModuleCount))
        for tile in self.pinModuleArr:
            print(tile)


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

    if label == "contour":
        global_myChip.avName = not global_myChip.avName
        for tl in global_myChip.softModuleArr:
            tl.dShape[0] = global_myChip.avShape and tl.dShape[0] 
            tl.dShape[1].set_visible(tl.dShape[0])
    elif label == "arrow":
        global_myChip.avVector = not global_myChip.avVector
        for tl in global_myChip.softModuleArr:
            tl.dVector[0] = global_myChip.avVector and tl.dVector[0] 
            tl.dVector[1].set_visible(tl.dVector[0])
    elif label == "connection":
        global_myChip.avConn = not global_myChip.avConn
        for tl in global_myChip.dConn:
            tl[0] = global_myChip.avConn
            tl[1][0].set_visible(tl[0])
    plt.draw()

# Handler for interactive legend, finds the corresponding element and to visibility
def legnedPickTessHandler(event):
    legTess = event.artist
    origTess = ClickElementdictionary[legTess]
    

    newVisibility = not origTess.dShape[0]
    origTess.dShape[0] = newVisibility
    
    origTess.dShape[1].set_visible(newVisibility)

    if newVisibility == True:
        origTess.dShape[0] = global_myChip.avShape
        origTess.dName[0] = global_myChip.avName
        # origTess.dXArea[0] = global_myChip.avXarea
        origTess.dVector[0] = global_myChip.avVector
    else:
        origTess.dShape[0] = False
        origTess.dName[0] = False
        # origTess.dXArea[0] = False
        origTess.dVector[0] = False

    origTess.dShape[1].set_visible(origTess.dShape[0])
    origTess.dName[1].set_visible(origTess.dName[0])
    # origTess.dXArea[1].set_visible(origTess.dXArea[0])
    origTess.dVector[1].set_visible(origTess.dVector[0])
    
    if newVisibility:
        legTess.set_alpha(1.0)
    else:
        legTess.set_alpha(0.1)
    
    fig.canvas.draw()

# Handler function for showing all Teseerae button
def showAllTiles(event):
    visibilityGoal = True
    for Tess in global_myChip.softModuleArr:
        Tess.dShape[0] = visibilityGoal
        Tess.dShape[1].set_visible(Tess.dShape[0])

        Tess.dName[0] = True
        Tess.dXArea[0] = True
        Tess.dVector[0] = True


        Tess.dName[1].set_visible(Tess.dName[0])
        Tess.dVector[1].set_visible(Tess.dVector[0])

    fig.canvas.draw()

# Handler function for Hiding all Teseerae button
def hideAllTiles(event):
    visibilityGoal = False
    for Tess in global_myChip.softModuleArr:
        Tess.dShape[0] = visibilityGoal
        Tess.dShape[1].set_visible(Tess.dShape[0])

        Tess.dName[0] = False
        Tess.dXArea[0] = False
        Tess.dVector[0] = False

        Tess.dName[1].set_visible(Tess.dName[0])
        Tess.dVector[1].set_visible(Tess.dVector[0])

    fig.canvas.draw()

if __name__ == '__main__':

    # A ColourGenerator class generates colors that are visually seperate, using precomputed LUT
    colourG = ColourGenerator()

    # Parse input arguments
    parser = ArgumentParser()
    parser.add_argument('inFile', help='visualization input file')
    parser.add_argument('-o', help='visualization output file', dest='outFile')
    parser.add_argument('-g', '--gui', action='store_true', help='GUI mode', dest='guiFlag')
    parser.add_argument('-v', '--verbose', action='store_true', help='verbosely show input parsing results', dest='verboseFlag')
    parser.add_argument('-a', '--arrow', action='store_true', help='show arrow mode', dest='arrowFlag')
    parser.add_argument('-c', '--connection', action='store_true', help='show connections', dest='connFlag')
    args = parser.parse_args()

    print(CYAN, "IRISLAB Floorplan Rendering Program", COLORRST)
    print("Input File: ", args.inFile)

    if args.outFile == None:
        print("Rendering Results saved to: ", RED, "Not saved", COLORRST)
    else:
        print("Rendering Results saved to: ",GREEN, args.outFile, COLORRST)

    if args.guiFlag == True:
        print("GUI Interactive mode: ", GREEN, "ON", COLORRST)
    else:
        print("GUI Interactive mode: ", RED, "OFF", COLORRST)

    if args.verboseFlag == True:
        print("Verbose mode: ", GREEN, "ON", COLORRST)
    else:
        print("Verbose mode: ", RED, "OFF", COLORRST)

    if args.arrowFlag == True:
        print("Rendering Result with arrows ", GREEN, "ON", COLORRST)
    else:
        print("Rendering Result with arrows ", RED, "OFF", COLORRST)

    if args.connFlag == True:
        print("Rendering Result with connections ", GREEN, "ON", COLORRST)
    else:
        print("Rendering Result with connections ", RED, "OFF", COLORRST)


    # start parsing inputs 
    file_read_case = open(args.inFile, 'r')
    fin_case = file_read_case.read().split("\n")

    myChip = Chip()
    myChip.readChip(fin_case)
    if args.verboseFlag == True:
        myChip.showChip()

    fig, ax = plt.subplots()
    BORDER = int((myChip.chipHeight + myChip.chipWidth)/10)
    ax.set_xbound(0, myChip.chipWidth)
    ax.set_ybound(0, myChip.chipHeight)
    ax.set_xlim([-0.00 * myChip.chipWidth, 1.00 * myChip.chipWidth])
    ax.set_ylim([-0.00 * myChip.chipHeight, 1.00 * myChip.chipHeight])
    
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

    ClickElements = []
    faceColorChoice = 0
    # Plot the tiles
    for tIdx, tl in enumerate(myChip.softModuleArr):
        faceColorChoice = colourG.getColour()
        ClickElements.append(tl)
        workingcorners = []
        for c in tl.corners:
            workingcorners.append([c.x, c.y])
        tl.dShape[1] = patches.Polygon(workingcorners,
                                       closed=True,
                                       fill=True,
                                       facecolor=faceColorChoice,
                                    #    edgecolor="#000000",
                                       alpha=0.6,
                                       label=tl.name
                                       )
        
        ax.add_patch(tl.dShape[1])
        tlxl = tl.bbox[0].x
        tlyl = tl.bbox[0].y
        tlxh = tl.bbox[2].x
        tlyh = tl.bbox[2].y
        tlwidth = tlxh - tlxl
        tlheight = tlyh - tlyl

        if tlwidth > tlheight:
            Twidth = 0.7*tlwidth
            Theight = 0.3*tlheight
            if(Twidth > 3*Theight):
                Twidth = 3*Theight
        else:
            Twidth = 0.95 * tlwidth
            Theight = 0.3*tlheight
            if(Theight > Twidth):
                Theight = Twidth
        representLenghIdx = min(Twidth, Theight)

        tl.dName[1], _ = text_with_autofit(ax, tl.name, (int(tlxl + 0.5*tlwidth), int(tlyl + 0.7*tlheight)), tlwidth*0.4, tlheight*0.4, show_rect = False, color="#000000")
        xareaColor = "#000000"
        if USE_DEGUB == 0:
            if tl.xarea < 0:
                xareaColor = "#FF0000"
            
        tl.dXArea[1], _ = text_with_autofit(ax, tl.xarea, (int(tlxl + 0.5*tlwidth), int(tlyl + 0.25*tlheight)), tlwidth*0.35, tlheight*0.35, show_rect = False, color=xareaColor)

        if tl.optCentreX == -1 and tl.optCentreY == -1:
            tl.dVector[1] = plt.scatter(tl.centre.x, tl.centre.y, s = representLenghIdx*1.5, c ='g', marker = 'x', clip_on=False)
        else:
            tl.dVector[1] = plt.arrow(tl.centreX, tl.centreY, tl.optCentreX - tl.centreX, tl.optCentreY - tl.centreY, width=representLenghIdx*0.1, head_width = representLenghIdx*0.4, color="g", linestyle="-")



    for tIdx, tl in enumerate(myChip.prePlacedModuleArr):
        faceColorChoice = "#808080"
        ClickElements.append(tl)
        workingcorners = []
        for c in tl.corners:
            workingcorners.append([c.x, c.y])
        tl.dShape[1] = patches.Polygon(workingcorners,
                                       closed=True,
                                       fill=True,
                                       facecolor=faceColorChoice,
                                       edgecolor="#000000",
                                       alpha=0.80,
                                       label=tl.name)
        
        ax.add_patch(tl.dShape[1])
        tlxl = tl.corners[0].x
        tlxh = tl.corners[0].x
        tlyl = tl.corners[0].y
        tlyh = tl.corners[0].y
        for i in tl.corners:
            if i.x < tlxl:
                tlxl = i.x
            elif i.x > tlxh:
                tlxh = i.x
            
            if i.y < tlyl:
                tlyl = i.y
            elif i.y > tlyh:
                tlyh = i.y

        tlwidth = tlxh - tlxl
        tlheight = tlyh - tlyl 
        tl.dName[1], _ = text_with_autofit(ax, tl.name, (int(tlxl + 0.5*tlwidth), int(tlyl + 0.5*tlheight)), tlwidth*0.75, tlheight*0.5, show_rect = False, color="#000000")

    
    # Plot connections
    for connIdx, conn in enumerate(myChip.connArr):
        leftCentre =[myChip.connDict[conn[0]].centreX, myChip.connDict[conn[0]].centreY]
        rightCentre =[myChip.connDict[conn[1]].centreX, myChip.connDict[conn[1]].centreY]
        lw = (conn[2]/myChip.largestConn)*2.5
        newLine = ax.plot([leftCentre[0], rightCentre[0]], [leftCentre[1], rightCentre[1]], linewidth=.75, color='royalblue', alpha = 0.45)
        newConn = [True, newLine]
        myChip.dConn.append(newConn)


    # Adjustments for "-a", arrow option
    if args.arrowFlag == False:
        global_myChip.avVector = not global_myChip.avVector
        for tl in global_myChip.softModuleArr:
            tl.dVector[0] = global_myChip.avVector and tl.dVector[0]
            tl.dVector[1].set_visible(tl.dVector[0])
        plt.draw()
    
    # Adjustment for "-c", connection option
    if args.connFlag == False:
        global_myChip.avConn = not global_myChip.avConn
        for cn in global_myChip.dConn:
            cn[0] = global_myChip.avConn and cn[0]
            cn[1][0].set_visible(cn[0])
        plt.draw()

    # For using '-o' flag to save output file
    if args.outFile != None:
        plt.savefig(args.outFile, dpi = 1600)

    # Create checkboxes to select/unselect certain Tile attributes
    rax = fig.add_axes([0.6, 0.05, 0.15, 0.15])
    CBlabels = ['contour', 'arrow', 'connection']
    CBvisible = [myChip.avShape, myChip.avVector, myChip.avConn]
    check = CheckButtons(rax, CBlabels, CBvisible)
    check.on_clicked(handleCheckButtonsClick)
    

    # Legedns could toggle individual Rectilinear to show/hide
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
    buttonShowAll.on_clicked(showAllTiles)

    axHideAll = fig.add_axes([0.75, 0.25, 0.09, 0.05])
    buttonHideAll = Button(axHideAll, 'Hide all')
    buttonHideAll.on_clicked(hideAllTiles)

  
    if args.guiFlag == True:
        plt.show()

