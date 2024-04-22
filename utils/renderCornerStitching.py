'''
Program: renderCornerStitching.py
January 20th, 2024
Iris Lab

A program to visiualize and interact with cornerStitching tiles distribution, an useful debugging tool
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
# Class Tile is the basic unit of showing info, representing the Tile data structure in cornerStitching
class Tile:
    def __init__(self, blockType, xl, yl, width, height, xh, yh):

        self.blockType = blockType
        self.xl = xl
        self.yl = yl
        self.xh = xh
        self.yh = yh

        self.width = width
        self.height = height

        self.LL = Cord(xl, yl)
        self.LR = Cord(xh, yl)
        self.UL = Cord(xl, yh)
        self.UR = Cord(xh, yh)
        self.Corner = [[xl, yl], [xl, yh], [xh, yh], [xh, yl]]
        self.centre = Cord(int((self.xl + self.xh)/2), int((self.yl + self.yh)/2))

        self.rt = 0
        self.tr = 0
        self.bl = 0
        self.lb = 0

        self.dShape = [True, -1]
        self.dName = [True, -1]
        self.drt = [True, -1]
        self.dtr = [True, -1]
        self.dbl = [True, -1]
        self.dlb = [True, -1]


    
    def __str__(self) -> str:
        MeStr = "@({}, {})".format(self.xl, self.yl) + " W = {}, H = {}".format(self.width, self.height) + " @({}, {})".format(self.xh, self.yh)

        if self.rt == False:
            rtStr = "rt: NIL"
        else:
            rtStr = "rt :@({}, {})".format(self.rt.x, self.rt.y)

        if self.tr == False:
            trStr = "tr: NIL"
        else:
            trStr = "tr: @({}, {})".format(self.tr.x, self.tr.y)

        if self.bl == False:
            blStr = "bl: NIL"
        else:
            blStr = "bl: @({}, {})".format(self.bl.x, self.bl.y)

        if self.lb == False:
            lbStr = "lb: NIL"
        else:
            lbStr = "lb: @({}, {})".format(self.lb.x, self.lb.y)
        
        return MeStr + "\n" + rtStr + "\n" + trStr + "\n" + blStr + "\n" + lbStr
        
# A Chip class is the data structure that holds all attributes, instantiate and link to global variable for
# full-scope access
class Chip:

    def __init__(self):
        self.chipName = ""
        self.chipWidth = -1
        self.chipHeight = -1
        self.tileCount = 0
        self.tileArr = []
        self.tileDict = dict()
        
        self.connCount = 0
        self.connArr = []
        self.dConn = []
        self.largestConn = 0
        self.connDict = dict()

        self.avName = True
        self.avrt = True
        self.avtr = True
        self.avbl = True
        self.avlb = True

        self.lineRead = 0

    def readChip(self, finCase):
        # Read the total number of Tiles 
        self.tileCount = int(finCase[self.lineRead])
        self.lineRead += 1

        contourStr = finCase[self.lineRead]
        self.lineRead += 1
        contourStr = contourStr.split(" ")
        self.chipWidth = int(contourStr[0])
        self.chipHeight = int(contourStr[1])
        
        for tileIdx in range(self.tileCount):
            tileHead = finCase[self.lineRead]
            self.lineRead += 1
            
            rtStr = finCase[self.lineRead]
            self.lineRead += 1
            trStr = finCase[self.lineRead]
            self.lineRead += 1
            blStr = finCase[self.lineRead]
            self.lineRead += 1
            lbStr = finCase[self.lineRead]
            self.lineRead += 1

            bt, llx, lly, w, h, urx, ury = parseTile(tileHead)
            tile = Tile(bt, llx, lly, w, h, urx, ury)

            if rtStr.endswith("nullptr"):
                tile.rt = False
            else:
                _, llx, lly, _, _, _, _ = parseTile(rtStr)
                tile.rt = Cord(llx, lly)

            if trStr.endswith("nullptr"):
                tile.tr = False
            else:
                _, llx, lly, _, _, _, _ = parseTile(trStr)
                tile.tr = Cord(llx, lly)

            if blStr.endswith("nullptr"):
                tile.bl = False
            else:
                _, llx, lly, _, _, _, _ = parseTile(blStr)
                tile.bl = Cord(llx, lly)

            if lbStr.endswith("nullptr"):
                tile.lb = False
            else:
                _, llx, lly, _, _, _, _ = parseTile(lbStr)
                tile.lb = Cord(llx, lly)

            self.tileArr.append(tile)
            self.tileDict[tile.LL] = tile

    def showChip(self):
        print("Printing attributes of the cornerStitching visualization:")
        print("Chip Contour W = {}, H = {}".format(self.chipWidth, self.chipHeight))
        print("There are total {} tiles to show".format(self.tileCount))
        for tile in self.tileArr:
            print(tile)


def parseTile(tileInfo):
    arr = tileInfo.split(',')
    blockType = arr[0].split('[')[1]
    llx = int(arr[1].split('(')[1])
    lly = int(arr[2].split(')')[0])

    width = int(arr[3])
    height = int(arr[4])

    urx = int(arr[5].split('(')[1])
    ury = int(arr[6].split(')')[0])

    return blockType, llx, lly, width, height, urx, ury

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

    if label == "LowerLeft":
        global_myChip.avName = not global_myChip.avName
        for tl in global_myChip.tileArr:
            tl.dName[0] = global_myChip.avName and tl.dName[0] 
            tl.dName[1].set_visible(tl.dName[0])
    elif label == "rt":
        global_myChip.avrt = not global_myChip.avrt
        for tl in global_myChip.tileArr:
            tl.drt[0] = global_myChip.avrt and tl.drt[0] 
            tl.drt[1].set_visible(tl.drt[0])
    elif label == "tr":
        global_myChip.avtr = not global_myChip.avtr
        for tl in global_myChip.tileArr:
            tl.dtr[0] = global_myChip.avtr and tl.dtr[0] 
            tl.dtr[1].set_visible(tl.dtr[0])
    elif label == "bl":
        global_myChip.avbl = not global_myChip.avbl
        for tl in global_myChip.tileArr:
            tl.dbl[0] = global_myChip.avbl and tl.dbl[0] 
            tl.dbl[1].set_visible(tl.dbl[0])
    elif label == "lb":
        global_myChip.avlb = not global_myChip.avlb
        for tl in global_myChip.tileArr:
            tl.dlb[0] = global_myChip.avlb and tl.dlb[0] 
            tl.dlb[1].set_visible(tl.dlb[0])

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
        origTess.drt[0] = global_myChip.avrt
        origTess.dtr[0] = global_myChip.avtr
        origTess.dbl[0] = global_myChip.avbl
        origTess.dlb[0] = global_myChip.avlb

    else:
        origTess.dName[0] = False
        origTess.drt[0] = False
        origTess.dtr[0] = False
        origTess.dbl[0] = False
        origTess.dlb[0] = False

    origTess.dName[1].set_visible(origTess.dName[0])
    origTess.drt[1].set_visible(origTess.dtr[0])
    origTess.dtr[1].set_visible(origTess.drt[0])
    origTess.dbl[1].set_visible(origTess.dbl[0])
    origTess.dlb[1].set_visible(origTess.dlb[0])
    
    if newVisibility:
        legTess.set_alpha(1.0)
    else:
        legTess.set_alpha(0.1)
    
    fig.canvas.draw()

# Handler function for showing all Teseerae button
def showAllTiles(event):
    visibilityGoal = True
    for Tess in global_myChip.tileArr:
        Tess.dShape[0] = visibilityGoal
        Tess.dShape[1].set_visible(Tess.dShape[0])

        Tess.dName[0] = global_myChip.avName
        Tess.drt[0]= global_myChip.avrt
        Tess.dtr[0]= global_myChip.avtr
        Tess.dbl[0]= global_myChip.avbl
        Tess.dlb[0]= global_myChip.avlb

        Tess.dName[1].set_visible(Tess.dName[0])
        Tess.drt[1].set_visible(Tess.drt[0])
        Tess.dtr[1].set_visible(Tess.dtr[0])
        Tess.dbl[1].set_visible(Tess.dbl[0])
        Tess.dlb[1].set_visible(Tess.dlb[0])

    fig.canvas.draw()

# Handler function for Hiding all Teseerae button
def hideAllTiles(event):
    visibilityGoal = False
    for Tess in global_myChip.tileArr:
        Tess.dShape[0] = visibilityGoal
        Tess.dShape[1].set_visible(Tess.dShape[0])

        Tess.dName[0] = False
        Tess.drt[0]= False
        Tess.dtr[0]= False
        Tess.dbl[0]= False
        Tess.dlb[0]= False

        Tess.dName[1].set_visible(Tess.dName[0])
        Tess.drt[1].set_visible(Tess.drt[0])
        Tess.dtr[1].set_visible(Tess.dtr[0])
        Tess.dbl[1].set_visible(Tess.dbl[0])
        Tess.dlb[1].set_visible(Tess.dlb[0])

    fig.canvas.draw()

if __name__ == '__main__':

    # A ColourGenerator class generates colors that are visually seperate, using precomputed LUT
    colourG = ColourGenerator()

    # Parse input arguments
    parser = ArgumentParser()
    parser.add_argument('inFile', help='visualization input file')
    parser.add_argument('-o', help='visualization output file', dest='outFile')
    parser.add_argument('-g', '--gui', action='store_true', help='GUI mode', dest='guiFlag')
    parser.add_argument('-a', '--arrow', action='store_true', help='show arrow mode', dest='arrowFlag')
    parser.add_argument('-v', '--verbose', action='store_true', help='verbosely show input parsing results', dest='verboseFlag')
    parser.add_argument('-w', '--white_blank', action='store_true', help='BLANK Tile pale white mode', dest='whiteBlankFlag')
    args = parser.parse_args()

    print(CYAN, "IRISLAB Corner-Stitching Rendering Program", COLORRST)
    print("Input File: ", args.inFile)

    if args.outFile == None:
        print("Rendering Results saved to: ", RED, "Not saved", COLORRST)
    else:
        print("Rendering Results saved to: ", args.outFile)

    if args.guiFlag == True:
        print("GUI Interactive mode: ", GREEN, "ON", COLORRST)
    else:
        print("GUI Interactive mode: ", RED, "OFF", COLORRST)

    if args.arrowFlag == True:
        print("Rendering Result with arrows ", GREEN, "ON", COLORRST)
    else:
        print("Rendering Result with arrows ", RED, "OFF", COLORRST)

    if args.verboseFlag == True:
        print("Verbose mode: ", GREEN, "ON", COLORRST)
    else:
        print("Verbose mode: ", RED, "OFF", COLORRST)

    if args.whiteBlankFlag == True:
        print("BLANK Tile pale white mode: ", GREEN, "ON", COLORRST)
    else:
        print("BLANK Tile pale white mode: ", RED, "OFF", COLORRST)

    # start parsing inputs 
    file_read_case = open(args.inFile, 'r')
    fin_case = file_read_case.read().split("\n")

    myChip = Chip()
    myChip.readChip(fin_case)
    if args.verboseFlag == True:
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

    ClickElements = []
    faceColorChoice = 0
    # Plot the tiles
    for tIdx, tl in enumerate(myChip.tileArr):
        if tl.blockType == "BLANK":
            if args.whiteBlankFlag == True:
                faceColorChoice = '#FFFFFF'
            else:
                faceColorChoice = colourG.getColour()
            adaptAlpha = 0.10
            representColour = 'black'
        elif tl.blockType == "BLOCK":
            faceColorChoice = colourG.getColour()
            adaptAlpha = 0.60
            representColour = 'blue'
        else:
            faceColorChoice = colourG.getColour()
            adaptAlpha = 0.95
            representColour = 'red'
        
        ClickElements.append(tl)

        tl.dShape[1] = patches.Polygon(tl.Corner,
                                       closed=True,
                                       fill=True,
                                       facecolor=faceColorChoice,
                                       edgecolor="#000000",
                                       alpha=adaptAlpha,
                                       label=tl.LL.__str__())
        
        ax.add_patch(tl.dShape[1])

        if tl.width > tl.height:
            Twidth = 0.7*tl.width
            Theight = 0.3*tl.height
            if(Twidth > 3*Theight):
                Twidth = 3*Theight
        else:
            Twidth = 0.95 * tl.width
            Theight = 0.3*tl.height
            if(Theight > Twidth):
                Theight = Twidth
        representLenghIdx = min(Twidth, Theight)
        
        tl.dName[1], _ = text_with_autofit(ax, tl.LL.__str__(), (int(tl.xl + 0.5*tl.width), int(tl.yl + 0.5*tl.height)), Twidth, Theight, show_rect = False, color=representColour)
        # sm.dVector[1] = plt.arrow(sm.Centre[0], sm.Centre[1], sm.Vector[0], sm.Vector[1], width = 10.0, head_width = 150.0, color = 'g')

        rtRoot = Cord(int(0.25*tl.xl + 0.75*tl.xh), int(0.125*tl.yl + 0.875*tl.yh))
        if tl.rt == False:
            # tl.drt[1] = plt.arrow(rtRoot.x, rtRoot.y, 0, (tl.yh - rtRoot.y), width=0.7, head_width = 10.0, color="r", linestyle="-")
            tl.drt[1] = plt.scatter(rtRoot.x, rtRoot.y, s = representLenghIdx*1.5, c ='g', marker = 'x', clip_on=False)
        else:
            rtTile = myChip.tileDict[tl.rt]
            tl.drt[1] = plt.arrow(rtRoot.x, rtRoot.y, (rtTile.centre.x - rtRoot.x), (rtTile.centre.y - rtRoot.y), width=representLenghIdx*0.1, head_width = representLenghIdx*0.4, color="g", linestyle="-")
             
        trRoot = Cord(int(0.125*tl.xl + 0.875*tl.xh), int(0.25*tl.yl + 0.75*tl.yh))
        if tl.tr == False:
            tl.dtr[1] = plt.scatter(trRoot.x, trRoot.y, s = representLenghIdx*1.5, c ='b', marker = 'x', clip_on=False)
        else:
            trTile = myChip.tileDict[tl.tr]
            tl.dtr[1] = plt.arrow(trRoot.x, trRoot.y, (trTile.centre.x - trRoot.x), (trTile.centre.y - trRoot.y), width=representLenghIdx*0.1, head_width = representLenghIdx*0.4, color="b", linestyle="-")
    
        blRoot = Cord(int(0.125*tl.xh + 0.875*tl.xl), int(0.25*tl.yh + 0.75*tl.yl))
        if tl.bl == False:
            tl.dbl[1] = plt.scatter(blRoot.x, blRoot.y, s = representLenghIdx*1.5, c ='deepskyblue', marker = 'x', clip_on=False)
        else:
            blTile = myChip.tileDict[tl.bl]
            tl.dbl[1] = plt.arrow(blRoot.x, blRoot.y, (blTile.centre.x - blRoot.x), (blTile.centre.y - blRoot.y), width=representLenghIdx*0.1, head_width =representLenghIdx*0.4, color="deepskyblue", linestyle="-")

        lbRoot = Cord(int(0.25*tl.xh + 0.75*tl.xl), int(0.125*tl.yh + 0.875*tl.yl))
        if tl.lb == False:
            tl.dlb[1] = plt.scatter(lbRoot.x, lbRoot.y, s = representLenghIdx*1.5, c ='slategray', marker = 'x', clip_on=False)
        else:
            lbTile = myChip.tileDict[tl.lb]
            tl.dlb[1] = plt.arrow(lbRoot.x, lbRoot.y, (lbTile.centre.x - lbRoot.x), (lbTile.centre.y - lbRoot.y), width=representLenghIdx*0.1, head_width=representLenghIdx*0.4, color="slategray", linestyle="-")

    # Adjustments for "-a", arrow option
    if args.arrowFlag == False:
        global_myChip.avrt = not global_myChip.avrt
        for tl in global_myChip.tileArr:
            tl.drt[0] = global_myChip.avrt and tl.drt[0] 
            tl.drt[1].set_visible(tl.drt[0])

        global_myChip.avtr = not global_myChip.avtr
        for tl in global_myChip.tileArr:
            tl.dtr[0] = global_myChip.avtr and tl.dtr[0] 
            tl.dtr[1].set_visible(tl.dtr[0])
        
        global_myChip.avbl = not global_myChip.avbl
        for tl in global_myChip.tileArr:
            tl.dbl[0] = global_myChip.avbl and tl.dbl[0] 
            tl.dbl[1].set_visible(tl.dbl[0])

        global_myChip.avlb = not global_myChip.avlb
        for tl in global_myChip.tileArr:
            tl.dlb[0] = global_myChip.avlb and tl.dlb[0] 
            tl.dlb[1].set_visible(tl.dlb[0])
        plt.draw()

    # For using '-o' flag to save output file
    if args.outFile != None:
        plt.savefig(args.outFile, dpi = 1200)

    # Create checkboxes to select/unselect certain Tile attributes
    rax = fig.add_axes([0.6, 0.05, 0.15, 0.15])
    CBlabels = ['LowerLeft', 'rt', 'tr', 'bl', 'lb']
    CBvisible = [myChip.avName, myChip.avrt, myChip.avtr, myChip.avbl, myChip.avlb]
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
    buttonShowAll.on_clicked(showAllTiles)

    axHideAll = fig.add_axes([0.75, 0.25, 0.09, 0.05])
    buttonHideAll = Button(axHideAll, 'Hide all')
    buttonHideAll.on_clicked(hideAllTiles)

  
    if args.guiFlag == True:
        plt.show()

