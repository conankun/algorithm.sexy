"""
    Programming autograder Server by Junghyun Kim
    written for python 3 >
    May 19th 2016
"""
import subprocess
import time
import os
import resource
from exceptions import *
from modules import CompileInstance, ClearInstance

DEBUG=1

def compile_user_submission(runid, lang):
    try:
        compiled = CompileInstance(runid, lang)
    except CompileFailed:
        return False
    else:
        return True

def fetch_problem_information(problem):
    """
    return in the format of list
    1. The number of subtasks
    2. The number of test datas per subtasks(list)
    3. Time limit per subtasks(MS, list)
    4. Memory limit per subtasks(MB, list)
    """
    return [1, [10], [2500], [32]]

def grading(problem, runid, lang):
    compiled = compile_user_submission(runid, lang)
    if not compiled:
        #grading finished
        if DEBUG:
            print("Compile Failed")
            f = open("RUN/"+str(runid)+"/compile.txt")
            logs = f.readlines()
            for line in logs:
                print(line)
            f.close()
        return
    problemInfo = fetch_problem_information(problem)
    subtasks = problemInfo[0]
    for subtask in xrange(subtasks):
        subtaskNum = subtask + 1
        testdataNum = problemInfo[1][subtask]
        timeLimit = problemInfo[2][subtask]
        memoryLimit = problemInfo[3][subtask]
        data_directory = "problems/"+problem+"/subtask"+str(subtaskNum)+"/"
        if DEBUG:
            print ("========== SUBTASK #"+str(subtaskNum)+" ==========")
        for data in xrange(testdataNum):
            dataNum = data + 1
            if DEBUG:
                print ("=====data"+str(dataNum)+"=====")
            inputfile = data_directory+"input"+str(dataNum)+".txt"
            outputfile = data_directory+"output"+str(dataNum)+".txt"
            cmd = ['python', 'judge_client.py', problem, runid, lang, str(subtaskNum), str(dataNum), str(timeLimit), str(memoryLimit)]
            proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=False)
            out, err = proc.communicate()
            proc.wait()
            if DEBUG:
                print (out)
                if err:
                    print(err)
    ClearInstance().remove_dummies(lang, "RUN/"+str(runid)+"/")
grading('1000', '1', 'C++')
