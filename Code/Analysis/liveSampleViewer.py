import math, time, serial, pygame
from pygame.locals import *
import numpy as np

def generateWindowTemplate(graphHeightPx):
    s = pygame.Surface((576, 680)).convert()
    s.fill((240, 240, 240))
    #s.fill((0,0,0), pygame.Rect((32, 32), (512, 512)))
    
    renderFont = pygame.font.Font(pygame.font.get_default_font(), 16)
    smallFont = pygame.font.Font(pygame.font.get_default_font(), 12)
    
    graphTitle = "PiedPiper Data Stream"
    textSurface = renderFont.render(graphTitle, 1, (0,0,0));
    s.blit(textSurface, ((576 - textSurface.get_width()) / 2, 8))
    
    axis1Title = "Spectrum: Frequency (kHz)"
    textSurface = renderFont.render(axis1Title, 1, (0,0,0));
    s.blit(pygame.transform.rotate(textSurface, 90), (4, 32 + (512 - textSurface.get_width()) / 2))
    
    axis2Title = "Audio: Time (ms)"
    textSurface = renderFont.render(axis2Title, 1, (0,0,0));
    s.blit(pygame.transform.rotate(textSurface, 90), (576-16, 32 + (512 - textSurface.get_width()) / 2))
    
    xAxisTitle = "Time (s)"
    textSurface = renderFont.render(xAxisTitle, 1, (0,0,0));
    s.blit(textSurface, ((576 - textSurface.get_width()) / 2, 570))
    
    for i in range(6):
        pygame.draw.line(s, (0,0,0), (24,544 - 102 * i), (576 - 24, 544 - 102 * i))
        
        textSurface = smallFont.render(str(i), 1, (0,0,0));
        s.blit(textSurface, (24, 530 - 102 * i))
        
        textSurface = smallFont.render(str(i * 10), 1, (0,0,0));
        s.blit(textSurface, (545, 530 - 102 * i))
    
    for i in range(26):
        pygame.draw.line(s, (0,0,0), (31 + 512 - (20 * i), 552), (31 + 512 - (20 * i), 512))
        
        textSurface = smallFont.render(str(i), 1, (0,0,0));
        s.blit(textSurface, (27 + 512 - (20 * i), 554))
    return s
    
#def animate():

# Packet:
# <0xff><(dataType * 128) + (packetLenPow [0-127])><packet>

def gfxTest():
    pygame.init()
    win = pygame.display.set_mode((576, 680))
    s = generateWindowTemplate(512)
    win.blit(s, (0,0))
    win.fill((0, 0, 0), pygame.Rect((32, 32), (512, 512)))
    pygame.display.flip()
    time.sleep(5)
    pygame.quit()

def main():
    portLocation = ""
    
    while not portLocation.isdigit():
        portLocation = input("COM port: ")
    
    portLocation = "COM" + portLocation
    
    pygame.init()
    win = pygame.display.set_mode((576, 680))
    
    baudrate = 115200
    processedDataBufferSize = 512
    maxPacketSize = 10
    graphHeightPow = 9
    
    print("Setting up serial port...")
    ser = serial.Serial(port=portLocation, baudrate=baudrate)
    
    winTemplate = generateWindowTemplate(graphHeightPow)
    win.blit(winTemplate, (0,0))
    
    graphSurface = pygame.Surface((processedDataBufferSize, math.pow(2, graphHeightPow))).convert()
    dataBuffer = np.zeros(512, int)
    
    pygame.display.set_caption("PiedPiper Live Graphing Utility")
    
    lastBlitTime = 0
    pgrmExit = False
    
    print("Finished initialization")
    
    while not pgrmExit:
        # Input event handler
        for event in pygame.event.get():
            if event.type == KEYDOWN:
                if event.key == K_ESCAPE:
                    pgrmExit = True
                    break
                
                else:
                    print("Writing to port...")
                    sendDat = bytearray()
                
                    if event.key == K_1:
                        sendDat.append(0)
                        print(ser.write(sendDat))
                        
                    elif event.key == K_2:
                        sendDat.append(1)
                        print(ser.write(sendDat))
                        
                    elif event.key == K_3:
                        sendDat.append(2)
                        print(ser.write(sendDat))
                    
            elif event.type == QUIT:
                pgrmExit = True
                break

        # Move to the next packet
        while int(ser.read()[0]) != 255:
            continue
        
        metaDataByte = int(ser.read()[0])
        
        packetSize = metaDataByte % 128
        
        if packetSize > maxPacketSize:
            print("WARNING: Packet size (2^" + str(packetSize) + " bytes) exceeds maximum allowed (2^" + str(maxPacketSize) + " bytes)")
        else:
            packetByteSize = 1 << packetSize

            inData = ser.read(packetByteSize)
            
            #dataBuffer = np.roll(dataBuffer, len(inData), 0)
            
            for i in range(packetByteSize):
                dataBuffer[i] = 512 - int(inData[i]) * 2
        
        if time.time() - lastBlitTime > 1/60:
            lastBlitTime = time.time()
            graphSurface.fill((255,255,255))
            
            for i in range(processedDataBufferSize - 2):
                pygame.draw.line(graphSurface, (255,0,0), (i, dataBuffer[i]), (i+1, dataBuffer[i+1]), 1)
                
            win.blit(graphSurface, (32, 32))
            pygame.display.flip()
            
    pygame.quit()
    ser.close()
    #Loop:
        #Get data from pipe
        #

main()
