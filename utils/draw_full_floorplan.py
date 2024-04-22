import sys
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.patches as patches
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

def draw_block(ax, x, y, width, height, color):
    # color = "#BBB"
    ax.add_patch(
        patches.Rectangle(
            (x, y),
            width,
            height,
            fill=True,
            edgecolor="#000",
            facecolor=color,
            alpha=1.0  # 0.3 original
        )
    )


def draw_circle(ax, x, y, radius, color):
    # color = "#FCC"
    ax.add_patch(
        patches.Circle(
            (x, y),
            radius,
            fill=True,
            edgecolor="#000",
            facecolor=color,
            alpha=1.0  # 0.3 original
        )
    )


def draw_polygon(ax, corners, color="#FCC"):
    ax.add_patch(
        patches.Polygon(corners,
                        closed=True,
                        fill=True,
                        facecolor=color,
                        edgecolor="#000",
                        alpha=1.0
                        )
    )

########################################################
#                       main 
########################################################
parser = argparse.ArgumentParser(description="This program can draw the rectilinear floorplan")
parser.add_argument("txt_name", help="The name of rectilinear circuit input", type=str)
parser.add_argument("png_name", help="The name of output png", type=str)
parser.add_argument("-l", "--line", help="Show connections between blocks", action="store_true")
args = parser.parse_args()

floorplan_txt_name = args.txt_name
png_name = args.png_name
show_connection = args.line
file_read_floorplan = open(floorplan_txt_name, 'r')
fin_floorplan = file_read_floorplan.read().split("\n")

floorplan_name = fin_floorplan[-1]
total_block_number = int(fin_floorplan[0].split(" ")[1])

# total_connection_number = int(f[0].split(" ")[3])
window_width = int(fin_floorplan[1].split(" ")[0])
window_height = int(fin_floorplan[1].split(" ")[1])
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

name2pos = {}

# draw soft blocks
soft_block_number = int(fin_floorplan[2].split(" ")[1])
i = 3

for block in range(soft_block_number):
    ss = fin_floorplan[i].split(" ")

    mod_name = ss[0]
    corner_count = int(ss[1])
    corners = np.array([])
    max_x, max_y, min_x, min_y = 0, 0, 1e10, 1e10
    i += 1

    for corner in range(corner_count):
        ss = fin_floorplan[i].split(" ")
        corner_x = int(ss[0])
        corner_y = int(ss[1])
        max_x, max_y = max(max_x, corner_x), max(max_y, corner_y)
        min_x, min_y = min(min_x, corner_x), min(min_y, corner_y)

        corners = np.append(corners, [corner_x, corner_y])
        i += 1

    name2pos[mod_name] = ((max_x + min_x) / 2, (max_y + min_y) / 2)
    x = min_x
    y = min_y
    w = max_x - min_x
    h = max_y - min_y
    corners = corners.reshape(-1, 2)
    draw_polygon(ax, corners)
    plt.text(x+w/2, y+h/2, mod_name, horizontalalignment='center', verticalalignment='center')

# draw fixed blocks
fixed_block_number = int(fin_floorplan[i].split(" ")[1])
i += 1

for block in range(fixed_block_number):
    ss = fin_floorplan[i].split(" ")
    
    mod_name = ss[0]
    max_x, max_y, min_x, min_y = 0, 0, 1e10, 1e10
    i += 1

    for corner in range(0,4):
        ss = fin_floorplan[i].split(" ")
        corner_x = int(ss[0])
        corner_y = int(ss[1])
        max_x, max_y = max(max_x, corner_x), max(max_y, corner_y)
        min_x, min_y = min(min_x, corner_x), min(min_y, corner_y)

        corners = np.append(corners, [corner_x, corner_y])
        i += 1
    
    name2pos[mod_name] = ((max_x + min_x) / 2, (max_y + min_y) / 2)
    x = min_x
    y = min_y
    w = max_x - min_x
    h = max_y - min_y
    draw_block(ax, x, y, w, h, color="#BBB")
    plt.text(x + w / 2, y + h / 2, mod_name, horizontalalignment='center', verticalalignment='center')

if show_connection == True:
    is_hyper = False
    connection_number = int(fin_floorplan[i].split(" ")[1])
    i += 1
    j = i
    max_value = 1
    min_value = 1e10
    for connection in range(connection_number):
        ss = fin_floorplan[j].split(" ")
        value = int(ss[-1])
        if value > max_value:
            max_value = value
        if value < min_value:
            min_value = value
        if len(ss) > 3:
            is_hyper = True
        j += 1

    for connection in range(connection_number):
        ss = fin_floorplan[i].split(" ")
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

plt.savefig(png_name)
