import sys
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.patches as patches
import math
import time
import random

SOFT_BLOCK_COLOR = "#FCC"
HIGHLIGHT_COLOR = "#EF0"
FIXED_BLOCK_COLOR = "#BBB"
CONNECTION_COLOR = "#00F"

used_color = ["#FFE", "#FFF", "#F00"]

def set_rand_color():
    (R, G, B) = (random.randint(0, 15), random.randint(0, 15), random.randint(0, 15))
    while R+G+B < 24:
        (R, G, B) = (random.randint(0, 15),
                     random.randint(0, 15), random.randint(0, 15))
    (Rc, Gc, Bc) = (hex(R)[2], hex(G)[2], hex(B)[2])
    color = "#" + Rc + Gc + Bc

    while color in used_color:
        (R, G, B) = (random.randint(0, 15),
                     random.randint(0, 15), random.randint(0, 15))
        while R+G+B < 20:
            (R, G, B) = (random.randint(0, 15),
                         random.randint(0, 15), random.randint(0, 15))
        (Rc, Gc, Bc) = (hex(R)[2], hex(G)[2], hex(B)[2])
        color = "#" + Rc + Gc + Bc

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
            # linewidth=0.5,
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
                        # linewidth=0.5,
                        alpha=0.8
                        )
    )


case_txt_name = sys.argv[1]
floorplan_txt_name = sys.argv[2]
png_name = sys.argv[3]
file_read_case = open(case_txt_name, 'r')
fin_case = file_read_case.read().split("\n")
file_read_floorplan = open(floorplan_txt_name, 'r')
fin_floorplan = file_read_floorplan.read().split("\n")


# total_block_number = int(f[0].split(" ")[1])
# total_connection_number = int(f[0].split(" ")[3])
window_width = int(fin_case[0].split(" ")[1])
window_height = int(fin_case[0].split(" ")[2])
aspect_ratio = window_height / window_width

png_size = (16, 15*aspect_ratio)
fig = plt.figure(figsize=png_size)

ax = fig.add_subplot(111)
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

# draw fixed blocks
fixed_block_number = 0
i = 0

while True:
    i += 1
    ss = fin_case[i].split(" ")
    if ss[0] == "FIXEDMODULE":
        fixed_block_number = int(ss[1])
        break

i += 1
for block in range(fixed_block_number):
    ss = fin_case[i].split(" ")
    mod_name = ss[0]
    x, y, w, h = int(ss[1]), int(ss[2]), int(ss[3]), int(ss[4])

    draw_block(ax, x, y, w, h, color=FIXED_BLOCK_COLOR)
    # plt.text(x + w / 2 - 20, y + h / 2 - 20, mod_name)
    name2pos[mod_name] = (x + w / 2, y + h / 2)
    i += 1

# draw soft blocks
highlight_soft_block_name = fin_floorplan[1]
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
        # plt.text(corner_x, corner_y, [corner_x, corner_y])
        name2pos[mod_name] = ((max_x + min_x) / 2, (max_y + min_y) / 2)
        i += 1

    corners = corners.reshape(-1, 2)
    if mod_name == highlight_soft_block_name:
        draw_polygon(ax, corners, color=HIGHLIGHT_COLOR)
    else:
        draw_polygon(ax, corners, color=SOFT_BLOCK_COLOR)

    # plt.text((min_x + max_x) / 2 - 20, (min_y + max_y) / 2 - 20, mod_name)

# draw connection
connection_number = 0
i = 0

while True:
    i += 1
    ss = fin_case[i].split(" ")
    if ss[0] == "CONNECTION":
        connection_number = int(ss[1])
        break
i += 1
j = i
max_value = 1
min_value = 1e10
for connection in range(connection_number):
    ss = fin_case[j].split(" ")
    value = int(ss[2])
    if value > max_value:
        max_value = value
    if value < min_value:
        min_value = value
    j += 1

HPWL = 0

for connection in range(connection_number):
    ss = fin_case[i].split(" ")
    x_values = [name2pos[ss[0]][0], name2pos[ss[1]][0]]
    y_values = [name2pos[ss[0]][1], name2pos[ss[1]][1]]
    value = float(ss[2])
    width = (value - min_value) / (max_value - min_value) * 14 + 1
    HPWL += value * (abs(name2pos[ss[0]][0] - name2pos[ss[1]][0]) + abs(name2pos[ss[0]][1] - name2pos[ss[1]][1]))
    plt.plot(x_values, y_values, color=CONNECTION_COLOR,
             linestyle="-", linewidth=width, alpha=0.5)
    i += 1

ax.axis('off')  # Turn off the axes
plt.axis('tight')  # Automatically adjust the axis limits to fit the data

# plt.savefig(str(sys.argv[1])[:-4]+".png")
# print("HPWL =", HPWL)
plt.savefig(png_name, dpi=400, bbox_inches='tight')
