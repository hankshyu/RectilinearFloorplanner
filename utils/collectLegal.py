import os
import sys
import tqdm
import multiprocessing

EXPERIMENT_ROOT_DIR = "./experiments/0502_213059_ami49_10%"
LEGAL_CONFIGS = [
    ["case01",6000,30,100,10,-500,200,-500,70,10,150,-500,100,-500,20,5], 
    ["case02", 6000,30,100,70,-500,1000,-500,70,10,500,-500,500,-500,30,5], 
    ["case03", 6000,30,100,70,-500,1000,-500,100,10,500,-500,500,-500,100,5], 
    ["util effort low",6000,30,100,10,-500,2000,-500,70,10,750,-500,750,-500,20,15], 
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

if __name__ == "__main__":
    besthpwl = sys.float_info.max
    besthpwlPunishment = 0
    besthpwlPath = ""
    besthpwlLegalConfig = ""
    besthpwlLegalMode = -1
    allTaskParam=[]
    for punishmentTest in os.listdir(EXPERIMENT_ROOT_DIR):
        punishmentPath = os.path.join(EXPERIMENT_ROOT_DIR, punishmentTest)
        for exp in os.listdir(punishmentPath):
            experimentPath = os.path.join(punishmentPath, exp)
            resultFilePath = ""
            configFilePath = ""
            for file in os.listdir(experimentPath):
                if "output" in file:
                    resultFilePath = os.path.join(experimentPath, file)
                elif "log" not in file:
                    configFilePath = os.path.join(experimentPath, file)
            
            if experimentPath != "": # there is result
                if os.path.getsize(experimentPath) != 0 and resultFilePath != "":
                    with open(resultFilePath, "r") as rfile:
                        # print(resultFilePath)
                        sussFail = rfile.readline().rstrip("\n")
                        if len(sussFail) != 0:
                            sussFail = int(sussFail)
                            if sussFail == 1:
                                hpwl = float(rfile.readline().rstrip("\n"))
                                time = float(rfile.readline())
                                # read the used hyperparameters
                                SPEC_ASPECT_RATIO_MIN = float(rfile.readline().split(" ")[2])
                                SPEC_ASPECT_RATIO_MAX = float(rfile.readline().split(" ")[2]) 
                                SPEC_UTILIZATION_MIN = float(rfile.readline().split(" ")[2]) 
                                HYPERPARAM_GLOBAL_PUNISHMENT = float(rfile.readline().split(" ")[2])
                                HYPERPARAM_GLOBAL_MAX_ASPECTRATIO_RATE = float(rfile.readline().split(" ")[2])
                                HYPERPARAM_GLOBAL_LEARNING_RATE = float(rfile.readline().split(" ")[2])
                                HYPERPARAM_POR_USAGE = float(rfile.readline().split(" ")[2])
                                HYPERPARAM_LEG_MAX_COST_CUTOFF = float(rfile.readline().split(" ")[2])
                                HYPERPARAM_LEG_OB_AREA_WEIGHT = float(rfile.readline().split(" ")[2])
                                HYPERPARAM_LEG_OB_UTIL_WEIGHT = float(rfile.readline().split(" ")[2])
                                HYPERPARAM_LEG_OB_ASP_WEIGHT = float(rfile.readline().split(" ")[2])
                                HYPERPARAM_LEG_OB_UTIL_POS_REIN = float(rfile.readline().split(" ")[2])
                                HYPERPARAM_LEG_BW_UTIL_WEIGHT = float(rfile.readline().split(" ")[2])
                                HYPERPARAM_LEG_BW_UTIL_POS_REIN = float(rfile.readline().split(" ")[2])
                                HYPERPARAM_LEG_BW_ASP_WEIGHT = float(rfile.readline().split(" ")[2])
                                HYPERPARAM_LEG_BB_AREA_WEIGHT = float(rfile.readline().split(" ")[2])
                                HYPERPARAM_LEG_BB_FROM_UTIL_WEIGHT = float(rfile.readline().split(" ")[2])
                                HYPERPARAM_LEG_BB_FROM_UTIL_POS_REIN = float(rfile.readline().split(" ")[2])
                                HYPERPARAM_LEG_BB_TO_UTIL_WEIGHT = float(rfile.readline().split(" ")[2])
                                HYPERPARAM_LEG_BB_TO_UTIL_POS_REIN = float(rfile.readline().split(" ")[2])
                                HYPERPARAM_LEG_BB_ASP_WEIGHT = float(rfile.readline().split(" ")[2])
                                HYPERPARAM_LEG_BB_FLAT_COST = float(rfile.readline().split(" ")[2])
                                HYPERPARAM_LEG_LEGALMODE = float(rfile.readline().split(" ")[2])
                                REFINE_ENGINE_GRID_SEARCH = float(rfile.readline().split(" ")[2])
                                HYPERPARAM_REF_MAX_ITERATION = float(rfile.readline().split(" ")[2])
                                HYPERPARAM_REF_USE_GRADIENT_ORDER = float(rfile.readline().split(" ")[2])
                                HYPERPARAM_REF_INITIAL_MOMENTUM = float(rfile.readline().split(" ")[2])
                                HYPERPARAM_REF_MOMENTUM_GROWTH = float(rfile.readline().split(" ")[2])
                                HYPERPARAM_REF_USE_GRADIENT_GROW = float(rfile.readline().split(" ")[2])
                                HYPERPARAM_REF_USE_GRADIENT_SHRINK = float(rfile.readline().split(" ")[2])
                                hyperConfig = "config ?"
                                for cf in LEGAL_CONFIGS:
                                    if(cf[1] != HYPERPARAM_LEG_MAX_COST_CUTOFF):
                                        continue
                                    if(cf[2] != HYPERPARAM_LEG_OB_AREA_WEIGHT):
                                        continue
                                    if(cf[3] != HYPERPARAM_LEG_OB_UTIL_WEIGHT):
                                        continue
                                    if(cf[4] != HYPERPARAM_LEG_OB_ASP_WEIGHT):
                                        continue
                                    if(cf[5] != HYPERPARAM_LEG_OB_UTIL_POS_REIN):
                                        continue
                                    if(cf[6] != HYPERPARAM_LEG_BW_UTIL_WEIGHT):
                                        continue
                                    if(cf[7] != HYPERPARAM_LEG_BW_UTIL_POS_REIN):
                                        continue
                                    if(cf[8] != HYPERPARAM_LEG_BW_ASP_WEIGHT):
                                        continue
                                    if(cf[9] != HYPERPARAM_LEG_BB_AREA_WEIGHT):
                                        continue
                                    if(cf[10] != HYPERPARAM_LEG_BB_FROM_UTIL_WEIGHT):
                                        continue
                                    if(cf[11] != HYPERPARAM_LEG_BB_FROM_UTIL_POS_REIN):
                                        continue
                                    if(cf[12] != HYPERPARAM_LEG_BB_TO_UTIL_WEIGHT):
                                        continue
                                    if(cf[13] != HYPERPARAM_LEG_BB_TO_UTIL_POS_REIN):
                                        continue
                                    if(cf[14] != HYPERPARAM_LEG_BB_ASP_WEIGHT):
                                        continue
                                    if(cf[15] != HYPERPARAM_LEG_BB_FLAT_COST):
                                        continue
                                    hyperConfig = cf[0]
                                    break
                                if hpwl < besthpwl:
                                    besthpwl = hpwl
                                    besthpwlLegalConfig = hyperConfig
                                    besthpwlLegalMode = int(HYPERPARAM_LEG_LEGALMODE)
                                    besthpwlPunishment = HYPERPARAM_GLOBAL_PUNISHMENT 
                                    besthpwlPath = experimentPath
                                
                                print("Punishment = {}, Config = {}, Legal Mode = {}, HPWL = {}".format(HYPERPARAM_GLOBAL_PUNISHMENT, hyperConfig, int(HYPERPARAM_LEG_LEGALMODE), hpwl))


    if besthpwl != sys.float_info.max:
        print("Best HPWL found = {}".format(besthpwl))
        print("Best Hyperparameter: Punishment = {}, Config = {}, LegalMode = {}".format(besthpwlPunishment, besthpwlLegalConfig, besthpwlLegalMode))
        print("Folder position: {}".format(besthpwlPath))
    else:
        print("No Legal solution present !!")


            