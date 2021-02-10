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
    graphRedChannel = pygame.surfarray.pixels_red(graphSurface)
    
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
                
                    if event.key == K_1:
                        print(ser.write((0).to_bytes(1, byteorder='little',signed=True)))
                        
                    elif event.key == K_2:
                        print(ser.write((1).to_bytes(1, byteorder='little',signed=True)))
                        
                    elif event.key == K_3:
                        print(ser.write((2).to_bytes(1, byteorder='little',signed=True)))
                    
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
            inData = ser.read(round(math.pow(2, packetSize)))
            
            graphRedChannel = np.roll(graphRedChannel, -1, 0)
            verticalScale = round(math.pow(2, graphHeightPow - packetSize))
            
            for i in range(len(inData)):
                graphRedChannel[processedDataBufferSize - 1, (i * verticalScale):((i + 1) * verticalScale)] = int(inData[i])
                #for v in range(verticalScale):
                #    graphRedChannel[processedDataBufferSize - 1, i * verticalScale + v] = int(inData[i])
        
        if time.time() - lastBlitTime > 1/60:
            lastBlitTime = time.time()
            pygame.surfarray.pixels_red(graphSurface)[:,:] = graphRedChannel[:,:]
            win.blit(graphSurface, (32, 32))
            pygame.display.flip()
            
    pygame.quit()
    ser.close()
    #Loop:
        #Get data from pipe
        #

main()