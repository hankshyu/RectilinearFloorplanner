import os
import datetime
import numpy as np
BASE_DIRECTORY = "./experiments/"
CASE_NAME = "ami33_10%"
ROOT_FOLDER = ""

PUNISHMENT_MIN = 0.2
PUNISHMENT_MAX = 3.0
EXPERIMENT_NUM = 200

LEARNING_RATE = 5e-4

USE_PRIMITIVE_OVERLAP_REMOVAL = 0

LEGAL_CONFIGS = [
["case01", 6000,30,100,10,-500,200,-500,70,10,150,-500,100,-500,20,5], 
["case02", 6000,30,100,70,-500,1000,-500,70,10,500,-500,500,-500,30,5], 
["case03", 6000,30,100,70,-500,1000,-500,100,10,500,-500,500,-500,100,5], 
["util effort low", 6000,30,100,10,-500,2000,-500,70,10,750,-500,750,-500,20,15], 
["case05",6000,70,1000,100,-500,1500,-500,90,150,600,-500,1750,-100,30,150], 
["aspectratio effort low",6000,30,200,100,-500,500,-500,250,10,150,-500,200,-500,250,5], 
["case07",6000,30,100,70,-500,300,-500,70,10,200,-500,200,-500,15,5], 
["iccad",6000,150,400,50,-500,1500,-500,90,150,600,-500,1000,-100,30,30], 
["program default (bad)",10000,750,1000,100,-500,1500,-500,90,150,900,-500,1750,-500,30,150], 
["strategy1",10000,1400,750,80,-500,750,-500,80,150,900,-500,1750,-500,30,150], 
["strategy2",10000,700,900,40,-500,2000,-1000,40,150,900,-500,1750,-500,30,150], 
["strategy3",10000,750,1100,300,-500,1000,-500,800,150,900,-500,1750,-500,30,150], 
["strategy4",10000,1000,1250,400,-500,1800,-500,700,100,450,-500,500,-500,10,-30], 
["legal effort medium",10000,30,100,10,-500,1700,-500,500,10,800,-750,1200,-750,70,5], 
["legal effort high",10000,30,300,10,-500,2500,-500,700,10,1200,-900,2000,-1000,300,5], 
["encourage bb",6000,10,200,10,-500,500,-500,100,10,150,-500,100,-500,20,-30], 
["Prioritize area",10000,1000,100,10,-500,200,-500,70,500,150,-500,100,-500,20,5], 
["util effort high",10000,70,1000,70,-500,1500,-500,90,10,600,-500,1750,-100,20,30], 
["aspect ratio effort high",6000,30,100,200,-500,300,-500,700,10,150,-500,100,-500,500,5], 
["ignore util",6000,30,10,70,-500,10,-500,70,10,10,-100,8,-100,30,5], 
]

def create_root_folder():
    # Get the current date and time
    current_time = datetime.datetime.now()
    
    # Format the current date and time
    global ROOT_FOLDER
    ROOT_FOLDER = BASE_DIRECTORY + current_time.strftime("%m%d_%H%M%S") + "_" + CASE_NAME
    
    # Create the folder
    try:
        os.makedirs(ROOT_FOLDER)
        print(f"Folder '{ROOT_FOLDER}' created successfully.")
    except OSError as e:
        print(f"Failed to create folder: {e}")

if __name__ == "__main__":
    create_root_folder()
    stepSize = (PUNISHMENT_MAX - PUNISHMENT_MIN) / EXPERIMENT_NUM
    PUNISHMENT_ARR = np.arange(PUNISHMENT_MIN, PUNISHMENT_MAX + stepSize, stepSize)
    print("Punishment array: ")
    print(PUNISHMENT_ARR)

    for pv in PUNISHMENT_ARR:
        punishmentFolder = ROOT_FOLDER + "/" + str(pv)
        os.makedirs(punishmentFolder)
        for runConfig in range(len(LEGAL_CONFIGS)):
            for runStrategy in [0, 2]:
                experimentFolder = punishmentFolder + "/" + str(runConfig) + "_"+ str(runStrategy)
                os.makedirs(experimentFolder)
                targetFileName = experimentFolder + "/" + str(pv) + "_" + str(runConfig) + "_" + str(runStrategy) + ".txt"
                with open(targetFileName, "w") as file:
                    if "case" in CASE_NAME:
                        file.write("SPEC_ASPECT_RATIO_MIN = 0.5\n")
                        file.write("SPEC_ASPECT_RATIO_MAX = 2.0\n")
                        file.write("SPEC_UTILIZATION_MIN = 0.8\n")
                    else:
                        file.write("SPEC_ASPECT_RATIO_MIN = 0.33333333333333333333\n")
                        file.write("SPEC_ASPECT_RATIO_MAX = 3.0\n")
                        file.write("SPEC_UTILIZATION_MIN = 0.8\n")

                    file.write("\n")
                    file.write("HYPERPARAM_GLOBAL_PUNISHMENT = {}\n".format(pv))
                    file.write("HYPERPARAM_GLOBAL_MAX_ASPECTRATIO_RATE = 0.9\n")
                    file.write("HYPERPARAM_GLOBAL_LEARNING_RATE = {}\n".format(LEARNING_RATE))

                    file.write("\n")
                    file.write("HYPERPARAM_POR_USAGE = {}\n".format(int(USE_PRIMITIVE_OVERLAP_REMOVAL)))
                    file.write("\n")
                    file.write("HYPERPARAM_LEG_MAX_COST_CUTOFF = {}\n".format(LEGAL_CONFIGS[runConfig][1]))
                    file.write("\n")
                    file.write("HYPERPARAM_LEG_OB_AREA_WEIGHT = {}\n".format(LEGAL_CONFIGS[runConfig][2]))
                    file.write("HYPERPARAM_LEG_OB_UTIL_WEIGHT = {}\n".format(LEGAL_CONFIGS[runConfig][3]))
                    file.write("HYPERPARAM_LEG_OB_ASP_WEIGHT = {}\n".format(LEGAL_CONFIGS[runConfig][4]))
                    file.write("HYPERPARAM_LEG_OB_UTIL_POS_REIN = {}\n".format(LEGAL_CONFIGS[runConfig][5]))
                    file.write("\n")
                    file.write("HYPERPARAM_LEG_BW_UTIL_WEIGHT = {}\n".format(LEGAL_CONFIGS[runConfig][6]))
                    file.write("HYPERPARAM_LEG_BW_UTIL_POS_REIN = {}\n".format(LEGAL_CONFIGS[runConfig][7]))
                    file.write("HYPERPARAM_LEG_BW_ASP_WEIGHT = {}\n".format(LEGAL_CONFIGS[runConfig][8]))
                    file.write("\n")
                    file.write("HYPERPARAM_LEG_BB_AREA_WEIGHT = {}\n".format(LEGAL_CONFIGS[runConfig][9]))
                    file.write("HYPERPARAM_LEG_BB_FROM_UTIL_WEIGHT = {}\n".format(LEGAL_CONFIGS[runConfig][10]))
                    file.write("HYPERPARAM_LEG_BB_FROM_UTIL_POS_REIN = {}\n".format(LEGAL_CONFIGS[runConfig][11]))
                    file.write("HYPERPARAM_LEG_BB_TO_UTIL_WEIGHT = {}\n".format(LEGAL_CONFIGS[runConfig][12]))
                    file.write("HYPERPARAM_LEG_BB_TO_UTIL_POS_REIN = {}\n".format(LEGAL_CONFIGS[runConfig][13]))
                    file.write("HYPERPARAM_LEG_BB_ASP_WEIGHT = {}\n".format(LEGAL_CONFIGS[runConfig][14]))
                    file.write("HYPERPARAM_LEG_BB_FLAT_COST = {}\n".format(LEGAL_CONFIGS[runConfig][15]))
                    file.write("\n")
                    file.write("HYPERPARAM_LEG_LEGALMODE = {}\n".format(runStrategy))
                    file.write("\n")
                    file.write("REFINE_ENGINE_GRID_SEARCH = 1.0\n")
                    file.write("\n")
                    file.write("HYPERPARAM_REF_MAX_ITERATION = 30\n")
                    file.write("HYPERPARAM_REF_USE_GRADIENT_ORDER = 1.0\n")
                    file.write("HYPERPARAM_REF_INITIAL_MOMENTUM = 2.0\n")
                    file.write("HYPERPARAM_REF_MOMENTUM_GROWTH = 2.0\n")
                    file.write("HYPERPARAM_REF_USE_GRADIENT_GROW = 1.0\n")
                    file.write("HYPERPARAM_REF_USE_GRADIENT_SHRINK = 1.0\n")


