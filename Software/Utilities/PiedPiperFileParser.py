import pathlib
import os
import sys
import matplotlib.pyplot as plt

def plotData(path):
    file = open(path, 'r')
    allLines = []
    lines = file.readlines()
    for line in lines:
        line = line.replace("\n", "")
        #line = line.replace(",", "")
        allLines.append(int(line))
    file.close()
    plt.ylim((0, 4095))
    plt.plot(allLines)
    plt.show()

def plotFreqData(path):
    file = open(path, 'r')
    amplitude = ""
    amps = []
    while 1:
        char = file.read(1)
        if not char:
            break
        if char != ',':
            amplitude += char
        else:
            amps.append(int(amplitude))
            amplitude = ""
    file.close()
    plt.ylim((-7500, 20000))
    plt.plot(amps)
    plt.show()

def printDets(path):
    file = open(path, 'r')
    allLines = ""
    allLines += file.read()
    file.close()
    print(allLines)


try:
    dir_path = sys.argv[1]
    path = pathlib.Path(dir_path)
    try:
        if path.name == "DETS.txt":
            printDets(path)
        elif path.name[0] == 'F':
            plotFreqData(path)
        else:
            plotData(path)
    except:
        print("Passed directory is invalid!")
except:
    print("Pass directory and file as argument")