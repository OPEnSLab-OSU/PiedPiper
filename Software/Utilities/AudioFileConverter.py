# This is a Python script used for converting wave files to the audio file format used by the Pied Piper traps.
# The traps essentially use 16-bit unsigned PCM audio files, with the sample values limited to the range [0, 2^12 - 1]
# The restriction to this range is done because the DAC on the Adafruit Feather M4 Express has a 12-bit resolution, and the
# general motivation behind this file formatting is to simplify the process of loading and playing back the sounds on the Arduino:
# the values of each audio sample written to the file are the exact values that are passed to the analogWrite() function.

#################################################### IMPORTANT #####################################################

# Before using this tool you must first convert whatever audio you're using to a 16-bit PCM wave file with a 4096 Hz sample frequency.
# I usually just use Audacity to do this:
    # 1. Select all the audio in the file (Ctrl + A)
    # 2. Go to Tracks > Resample, type in 4096 Hz, and press Enter.
    # 3. Set the Project Rate (lower left corner) to 4096 Hz.
    # 4. Go to File > Export > Export as WAV
    # 5. In the "Encoding" dropdown at the bottom of the export window, select "Signed 16-bit PCM"
    # 6. Save the exported audio.
    
# Additionally, make sure that the audio file is no longer than eight seconds, because only (4096 Hz) x (8 s) = 32 kB of RAM is
# allocated for storing playback audio.

#################################################### TO USE THIS UTILITY: #####################################################

# 0. Make sure you read the text immediately above this procedure.
# 1. Define the string 'inFile' as the name of whatever audio file you're converting. You can specify a directory in the string,
#    or you can just move the audio file to the same location as this script.
# 2. Run the script.
# 3. The newly formatted audio file will be generated and saved with whatever name is stored in the variable 'outFileName'.

import os, wave, sys

inFile = 'FPBT2.wav'

wfile = wave.open(inFile, 'rb')

outFileName = 'FPBX.PAD'

ratio = (2**12) / (2**16)
offset = (2**12) / 2
maxVal = 2**12 - 1
outFile = open(outFileName, 'wb+')

maxS = 0
minS = 10000000
avg = 0;

print(wfile.getnframes())

for i in range(wfile.getnframes()):
    s = wfile.readframes(1)
    s = int.from_bytes(s, sys.byteorder, signed=True)
    s = s * ratio
    s = s + offset
    s = round(s)
    
    if s > maxS:
        maxS = s
    
    if s < minS:
        minS = s
    
    avg = avg + s
        
    s = max(min(s, maxVal), 0)
    s = s.to_bytes(2, byteorder='little', signed=False)
    outFile.write(s)
    
print(maxS)
print(minS)
print(avg / wfile.getnframes())

wfile.close()
outFile.close()