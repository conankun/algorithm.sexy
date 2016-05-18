import subprocess
import time
import os
import resource
from exceptions import *

class CompileInstance:
    root_dir = ""
    def __init__(self, runid, lang):
        root_dir = "RUN/"+runid+"/"
        IsCompileSuccessful = self.compile(lang, root_dir)
        if not IsCompileSuccessful:
            raise CompileFailed("Compile Failed")

    def compile(self, lang, directory):
        """
        lang : programming language to compile
        directory : a string that indicates directory in which compile files exist

        return True if compilable
        False if not
        """
        #set compile command
        compile_command = []
        if lang == "C":
            compile_command = ['gcc', directory+'Main.c', '-o', directory+'Main', '-O2', '-Wall', '-lm', '-std=c99']
        elif lang == "C++":
            compile_command = ['g++', directory+'Main.cc', '-o', directory+'Main', '-O2', '-Wall', '-lm']
        elif lang == "Java":
            compile_command = ['javac', '-J-Xms32m', '-J-Xmx256m', directory+'Main.java']
        elif lang == "Python2":
            compile_command = ["python", "-m", "py_compile", directory+"Main.py"]
        elif lang == "Python3":
            compile_command = ["python3", "-m", "py_compile", directory+"Main.py"]
        else:
            return False

        #set compile log directory
        compile_log = directory + "compile.txt"
        compile_log_file = open(compile_log,"w")
        proc = subprocess.Popen(compile_command, stdin=subprocess.PIPE, stdout=compile_log_file, stderr=compile_log_file)
        proc.wait()
        compile_log_file.close()
        #check whether compile succeeds
        #check returned value
        returnvalue = proc.returncode
        if returnvalue is not 0:
            #compile failed
            return False;

        #compile successful
        return True
class ClearInstance:
    def __init__(self):
        pass
    def remove_dummies(self, lang, directory):
        try:
            if lang == "C" or lang == "C++":
                os.remove(directory + 'Main')
            elif lang == "Python2":
                os.remove(directory + 'Main.pyc')
            elif lang == "Python3":
                os.remove(directory + 'Main.pyc')
            elif lang == "Java":
                os.remove(directory + 'Main.class')
        except FileNotFoundError:
            pass
