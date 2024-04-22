'''
This is the python script that generates visually distinct colours
'''

import distinctipy
import copy
from tqdm import tqdm

T_COLORRST    = "\u001b[0m"
T_YELLOW      = "\u001b[33m"
T_CYAN        = "\u001b[36m"


# HyperParameters
TARGET_COLOR_NUM = 4096
OUTPUTFILE = "./distinctColours.txt"

# PASTEL_FACTOR - float between 0 and 1. If pastel_factor>0 paler colours will be generated. (default 0)
PASTEL_FACTOR = 0

# ATTEMPTS - number of random colours to generate to find most distinct colour
ATTEMPTS = 17000




# Colors to forbid generating, Shall be placed under the "FORBIDDEN_COLORS" Array

RED = (1, 0, 0)
GREEN = (0, 1, 0)
BLUE = (0, 0, 1)
THE_GRAY = (0.5, 0.5, 0.5)
WHITE = (1, 1, 1)
BLACK = (0, 0, 0)
forbidden_colors = [RED, GREEN, BLUE, THE_GRAY, WHITE, BLACK]

# Start generation

print(r"Generating {}{}{} Distinct Colors with attempts {}{}{} to {}".format(T_YELLOW,TARGET_COLOR_NUM, T_COLORRST, T_CYAN, ATTEMPTS, T_COLORRST, OUTPUTFILE))

exist_colors = copy.deepcopy(forbidden_colors)

color_bank = []
for i in tqdm(range(TARGET_COLOR_NUM)):
    new_color = distinctipy.distinctipy.distinct_color(exist_colors, pastel_factor=PASTEL_FACTOR, n_attempts=ATTEMPTS)
    color_bank.append(distinctipy.distinctipy.get_hex(new_color))
    exist_colors.append(new_color)



# Output Generating Results


print("FORBIDDEN_COLORS = [")
for idx, color in enumerate(forbidden_colors):
    print(r'"{}"'.format(distinctipy.distinctipy.get_hex(color)), end="")
    if idx != (len(forbidden_colors)-1):
        print(", ", end = "")
    else:
        print(",")

print("]")

print("COLOR_BANK = [")
for idx, color in enumerate(color_bank):
    print(r'"{}"'.format(distinctipy.distinctipy.get_hex(color)), end="")
    if (idx == (len(color_bank)-1)):
        print("")
    elif ((idx%8) != 7) :
        print(", ", end = "")
    else:
        print(",")

print("]")


with open(OUTPUTFILE, 'w') as f:
    f.write("FORBIDDEN_COLORS = [\n")
    for idx, color in enumerate(forbidden_colors):
        f.write(r'"{}"'.format(distinctipy.distinctipy.get_hex(color)))
        if idx != (len(forbidden_colors)-1):
            f.write(", ")
        else:
            f.write("\n")

    f.write("]\n")


    f.write("COLOR_BANK = [\n")
    for idx, color in enumerate(color_bank):
        f.write(r'"{}"'.format(distinctipy.distinctipy.get_hex(color)))
        if (idx == (len(color_bank)-1)):
            f.write("\n")
        elif ((idx%8) != 7):
            f.write(", ",)
        else:
            f.write(",\n")
    f.write("]")



distinctipy.color_swatch(exist_colors)
