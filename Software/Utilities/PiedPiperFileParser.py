import pathlib
import os
import sys
import matplotlib.pyplot as plt
import numpy as np

def plotData(path):
    file = open(path, 'r')
    samples = []
    lines = file.readlines()
    for line in lines:
        line = line.replace("\n", "")
        samples.append(int(line))
    file.close()

    t = np.linspace(0, 9, len(samples))

    plt.plot(t, samples, marker='.')
    plt.ylim((0, 4096))
    plt.show()

def plotDataOld(path):
    file = open(path, 'r')
    allLinesR = []
    lines = file.readlines()
    for line in lines:
        line = line.replace("\n", "")
        #line = line.replace(",", "")
        allLinesR.append(int(line))
    file.close()

    print(path)
    det_num = path.name[1]
    path_len = len(str(path))
    path2 = (str(path)[:path_len - 6])
    path2 += "D" + det_num + ".txt"
    print(path2)

    file = open(path2, 'r')
    allLinesD = []
    linesA = file.readlines()
    for line in linesA:
        line = line.replace("\n", "")
        allLinesD.append(int(line))
    file.close()

    T = 8 # signal time
    S = 4096
    t_r = np.linspace(0, T, len(allLinesR)) 
    t_d = np.linspace(0, T, len(allLinesD))
    
    fig, axs = plt.subplots(2,1)

    axs[0].plot(t_r, allLinesR, label='Original', marker='.')
    axs[0].set_ylim([0, 4095])
    axs[0].legend()
    axs[1].plot(t_d, allLinesD, label='Downsampled', marker='.')
    axs[1].legend()
    axs[1].set_ylim([0, 4095])
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