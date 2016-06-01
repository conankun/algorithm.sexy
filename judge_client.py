"""
    Programming autograder Client by Junghyun Kim
    written for python 3 >
    May 18th 2016
"""
import subprocess
import time
import os
import resource
import sys
import psutil
from exceptions import *
from modules import *
"""
tr = compile('C++','RUN/')
if not tr:
    print ("Compile Failed")
    exit()
input_file = open('input.txt','r')
output_file = open('output.txt','w')
run_instance(run_command('C++','RUN/'),input_file, output_file)
output_file.close()
input_file.close()
#remove_dummies('C++', 'RUN/')

"""
class RunInstance:
    def __init__(self, problem, runid, lang, subtask, data, timelimit, memorylimit):
        self.directory = "RUN/"+runid+"/"
        self.problem = problem
        self.lang = lang
        self.subtask = subtask
        self.data = data
        self.timelimit = int(timelimit);
        self.memorylimit = int(memorylimit);
        self.run_testdata()

    def run_testdata(self):
        input_file = open('problems/'+self.problem+'/subtask'+self.subtask+'/input'+self.data+'.txt','r')
        output_file = open(self.directory+'output.txt','w')
        self.run_instance(self.run_command(self.lang),input_file, output_file)
        output_file.close()
        input_file.close()

    def run_command(self, lang):
        """
            lang : programming language to compile
            self.directory : a string that indicates self.directory in which compile files exist

            return a command(list) to run a program compiled by lang
        """
        if lang == "C" or lang == "C++":
            return ["./"+self.directory+"Main"]
        elif lang == "Java":
            return ['java','-classpath', 'RUN', self.directory+'Main']
        elif lang == "Python2":
            return ['python', self.directory+'Main.py']
        elif lang == "Python3":
            return ['python3', self.directory+'Main.py']
        else:
            return []
    def sandbox(self):
        ### TIME LIMIT
        resource.setrlimit(resource.RLIMIT_CPU, (int(self.timelimit/1000) + 1, 20))
        ### LIMIT MEMORY USAGE
        #overall memory
        resource.setrlimit(resource.RLIMIT_AS, (1024*1024*self.memorylimit, 1024*1024*1024))
        ### BLOCK ATTACKS
        #file size
        resource.setrlimit(resource.RLIMIT_FSIZE, (1024*1024*1024, 1024*1024*1024))
        #fork
        resource.setrlimit(resource.RLIMIT_NPROC, (0, 0))
        #pass

    def run_instance(self, cmd, input_file, output_file):
        """
        cmd : list of strings that contain command to run_program
        e.g) python add.py <=> ['python', 'add.py']

        input_file : file stream of input file(where program will read from)
        output_file : file stream of output file(where program's ouput will be)
        """
        ####################Run####################
        #start time stamp
        start_time = time.time()
        proc = subprocess.Popen(cmd, stdin=input_file, stdout=output_file, stderr=subprocess.PIPE, preexec_fn=self.sandbox, shell=False,  close_fds=True)
        ####################Memory Usage####################
        pid = proc.pid
        process = psutil.Process(pid)
        topmemory = process.get_memory_info()[0] 
        while proc.poll() is None:
            process = psutil.Process(pid)
            mem = process.get_memory_info()[0] 
            if mem > topmemory:
                topmemory = mem
        out, err = proc.communicate()
        rc = proc.wait()

        #adjustment
        if topmemory is 0:
            topmemory = 356*1024

        #end time stamp
        end_time = time.time()
        if 1000*(end_time - start_time) > self.timelimit:
            #TLE
            print("Time Limit Exceeded")
        else:
            memoryUsed = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss
            if rc == 0:
                print("Running Time : {:.4f} seconds".format(end_time - start_time))
                print("Memory Usage : "+str(int(topmemory/1024))+" KB")
                output_file.flush()
            elif rc == -8:
                print("Divide-By-Zero")
            elif rc == -9:
                print("Memory Limit Exceeded::")
            elif rc == -11:
                print("Segmentation Fault")
            else:
                print("Runtime Error")

#######################################
argc = len(sys.argv)
if argc is not 8:
    #TODO : throw exception
    print("The number of arguments is incorrect :: " + str(argc))
    print (str(sys.argv))
    exit()
argv = sys.argv
inst = RunInstance(argv[1], argv[2], argv[3], argv[4], argv[5], argv[6], argv[7])
