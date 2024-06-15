import os
import tqdm
import multiprocessing
import subprocess

PARALLEL_TASKS = 8
EXPERIMENT_ROOT_DIR = "./experiments/0502_213059_ami49_10%"
INPUT_SOURCE = "./inputs/rawInputs/ami49_10%-input.txt"
EXECUTABLE = "./bin/rfrun"

def execute_cpp(parameters):
    # Command to execute the C++ program
    command = [EXECUTABLE] + ["-i"] + [parameters[0]] + ["-c"] + [parameters[1]] + [ "-o"] + [parameters[2]]
     # Construct the log file name based on the parameters
    
    # Open the log file for writing
    with open(parameters[3], "w") as log:
        # Execute the command and redirect output to the log file
        subprocess.run(command, stdout=log)
    # Execute the command

if __name__ == "__main__":
    allTaskParam=[]
    for punishmentTest in os.listdir(EXPERIMENT_ROOT_DIR):
        punishmentPath = os.path.join(EXPERIMENT_ROOT_DIR, punishmentTest)
        for exp in os.listdir(punishmentPath):
            experimentPath = os.path.join(punishmentPath, exp)
            configFilePath = 0
            for configFile in os.listdir(experimentPath):
                configFilePath = os.path.join(experimentPath, configFile)
            
            # print(configFilePath)
            outputFilePath = configFilePath[:-4] + "-output.txt"
            logFilePath = configFilePath[:-4] + ".log.txt"
            allTaskParam.append([INPUT_SOURCE, configFilePath, outputFilePath, logFilePath])


    progress_bar = tqdm.tqdm(total=len(allTaskParam))
    with multiprocessing.Pool(processes = PARALLEL_TASKS) as pool:
        for _ in pool.imap_unordered(execute_cpp, allTaskParam):
            progress_bar.update(1)

            


