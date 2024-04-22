import sys
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.patches as patches
import math
import time
import random
import argparse


used_color = ["#FFE", "#FFF", "#F00", "#FCC", "#BBB"]

def set_rand_color(used_color):
    # Generate a random color with full 8-bit RGB values
    R, G, B = random.randint(0, 15), random.randint(0, 15), random.randint(0, 15)
    
    # Convert RGB to hexadecimal format
    color = f"#{R:01x}{G:01x}{B:01x}"
    
    # Ensure the color is not in the used colors and has a diverse intensity
    while color in used_color or (R + G + B) < 10 or (R + G + B) > 35:  # Adjust the sum constraint for diversity
        R, G, B = random.randint(0, 15), random.randint(0, 15), random.randint(0, 15)
        color = f"#{R:01x}{G:01x}{B:01x}"

    return color


def draw_block(ax, x, y, width, height, color="#FFF", aplha=1.0):
    ax.add_patch(
        patches.Rectangle(
            (x, y),
            width,
            height,
            fill=True,
            edgecolor="#DDD",
            facecolor=color,
            alpha=aplha,  # 0.3 original
            linewidth=0.2
        )
    )


def blend_color(colors):
    r = []
    g = []
    b = []
    colorCount = len(colors)
    for c in colors:
        r.append(int(c[1], 16) * 16)
        g.append(int(c[2], 16) * 16)
        b.append(int(c[3], 16) * 16)

    (br, bg, bb) = (0, 0, 0)
    for i in range(colorCount):
        br += r[i]
        bg += g[i]
        bb += b[i]

    (br, bg, bb) = (br//colorCount, bg//colorCount, bb//colorCount)
    (br, bg, bb) = (hex(br)[2:], hex(bg)[2:], hex(bb)[2:])
    if len(br) < 2:
        br += "0"
    if len(bg) < 2:
        bg += "0"
    if len(bb) < 2:
        bb += "0"

    blend = "#" + br + bg + bb

    return blend


########################################################
#                     main part
########################################################

# Create the parser and add arguments
parser = argparse.ArgumentParser(description="This program can draw the layout of the circuit")
parser.add_argument("txt_name", help="The name of circuit spec file", type=str)
parser.add_argument("png_name", help="The name of circuit spec file", type=str)
parser.add_argument("-l", "--line", help="Show connections between blocks", action="store_true")
args = parser.parse_args()

txt_name = args.txt_name
png_name = args.png_name
show_connection = args.line
fread = open(txt_name, 'r')
f = fread.read().split("\n")


total_block_number = int(f[0].split(" ")[1])
window_width = int(f[1].split(" ")[0])
window_height = int(f[1].split(" ")[1])
floorplan_name = f[-1]
aspect_ratio = window_height / window_width

png_size = (16, 15*aspect_ratio)
fig = plt.figure(figsize=png_size, dpi=300)

ax = fig.add_subplot(111)
ax.set_title(floorplan_name)
ax.set_xbound(0, window_width)
ax.set_ybound(0, window_height)

ax.add_patch(
    patches.Rectangle(
        (0, 0),
        window_width,
        window_height,
        fill=False,
        edgecolor="#000",
        facecolor="#FFF",
        alpha=1.0  # 0.3 original
    )
)

i = 2

name2pos = {}
name2color = {}
overlapDict = {}

for block in range(total_block_number):
    ss = f[i].split(" ")
    bx, by, bw, bh = float(ss[2]), float(ss[3]), float(ss[4]), float(ss[5])
    plt.text(bx+bw/2, by+bh/2, ss[0], horizontalalignment='center', verticalalignment='center')
    name2pos[ss[0]] = (bx+bw/2, by+bh/2)
    if ss[6] == "HARD_BLOCK":
        color = "#777"
    else:
        color = set_rand_color(used_color)
    name2color[ss[0]] = color
    i += 1
    ss = f[i].split(" ")
    subblockCount = int(ss[0])
    overlapCount = int(ss[1])
    i += 1
    for subblock in range(subblockCount):
        ss = f[i].split(" ")
        i += 1
        (sbx, sby, sbw, sbh) = (float(ss[0]), float(
            ss[1]), float(ss[2]), float(ss[3]))
        draw_block(ax, sbx, sby, sbw, sbh, color=color)

    for overlap in range(overlapCount):
        ss = f[i].split(" ")
        i += 1
        (sbx, sby, sbw, sbh) = (float(ss[0]), float(
            ss[1]), float(ss[2]), float(ss[3]))
        #draw_block(ax, sbx, sby, sbw, sbh, color=color, aplha=0.3)
        if (sbx, sby, sbw, sbh) in overlapDict:
            overlapDict[(sbx, sby, sbw, sbh)].append(color)
        else:
            overlapDict[(sbx, sby, sbw, sbh)] = [color]

for overlap in overlapDict:
    colors = overlapDict[overlap]
    (sbx, sby, sbw, sbh) = overlap
    blend = blend_color(colors)
    draw_block(ax, sbx, sby, sbw, sbh, color=blend)


while f[i].split(" ")[0] != "CONNECTION":
    ss = f[i].split(" ")
    i += 1
    (sbx, sby, sbw, sbh) = (float(ss[0]),
                            float(ss[1]), float(ss[2]), float(ss[3]))
    draw_block(ax, sbx, sby, sbw, sbh, color="#FFE")


if show_connection:
    is_hyper = False
    total_connection_number = int(f[i].split(" ")[1])
    i += 1
    j = i
    max_value = 1
    min_value = 1e10
    for connection in range(total_connection_number):
        ss = f[j].split(" ")
        value = int(ss[-1])
        if value > max_value:
            max_value = value
        if value < min_value:
            min_value = value
        if len(ss) > 3:
            is_hyper = True
        j += 1


    for connection in range(total_connection_number):
        ss = f[i].split(" ")
        value = float(ss[-1])
        if min_value == max_value:
            width = 2
        else:
            width = (value - min_value) / (max_value - min_value) * 14 + 1
        
        if is_hyper:
            current_color = set_rand_color(used_color)
            for mod_id in range(1, len(ss)-1):
                x_values = [name2pos[ss[0]][0], name2pos[ss[mod_id]][0]]
                y_values = [name2pos[ss[0]][1], name2pos[ss[mod_id]][1]]
                plt.plot(x_values, y_values, color=current_color,
                    linestyle="-", linewidth=width, alpha=0.5)
        else:
            x_values = [name2pos[ss[0]][0], name2pos[ss[1]][0]]
            y_values = [name2pos[ss[0]][1], name2pos[ss[1]][1]]
            
            plt.plot(x_values, y_values, color="blue",
                    linestyle="-", linewidth=width, alpha=0.5)
        i += 1


# while i < len(f):
#     if f[i] == "":
#         break
#     ss = f[i].split(" ")
#     (bx, by, bw, bh) = (float(ss[0]), float(ss[1]), float(ss[2]), float(ss[3]))
#     draw_block(ax, bx, by, bw, bh, color="#F00")
#     i += 1


plt.savefig(png_name)
