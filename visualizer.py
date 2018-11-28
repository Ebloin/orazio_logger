from collections import OrderedDict
import numpy as np
import cv2
from pylab import *

#PACKET EXAMPLE
#{seq:01094, x:0.000, y:0.000, t:0.000, tvm:0.000, tvd:0.600, tva:0.060, rvm:0.000, rvd:0.100, rva:0.100}
back = np.zeros((512,512,3), np.uint8)
img = np.zeros((352,288,3), np.uint8)

def parsepacket(p):
    splitted = p.replace("{", "").replace("}", "").replace(" ", "").split(",")
    dict = {}
    for e in splitted:
        keyval = e.split(":")
        dict[keyval[0]] = keyval[1]
    return dict

def build_cartesian_plane(max_quadrant_range):
    """ The quadrant range controls the range of the quadrants"""
    l = []
    zeros = []
    plt.grid(True, color='b', zorder=0,)
    ax = plt.axes()
    head_width = float(0.05) * max_quadrant_range
    head_length = float(0.1) * max_quadrant_range
    ax.arrow(0, 0, max_quadrant_range, 0, head_width=head_width, head_length=head_length, fc='k', ec='k',zorder=100)
    ax.arrow(0, 0, -max_quadrant_range, 0, head_width=head_width, head_length=head_length, fc='k', ec='k', zorder=100)
    ax.arrow(0, 0, 0, max_quadrant_range, head_width=head_width, head_length=head_length, fc='k', ec='k', zorder=100)
    ax.arrow(0, 0, 0, -max_quadrant_range, head_width=head_width, head_length=head_length, fc='k', ec='k', zorder=100)
    counter_dash_width = max_quadrant_range * 0.02
    dividers = [0,.1,.2,.3,.4, .5, .6, .7, .8, .9, 1]
    for i in dividers:
        plt.plot([-counter_dash_width, counter_dash_width], [i*max_quadrant_range, i*max_quadrant_range], color='k')
        plt.plot([i * max_quadrant_range, i*max_quadrant_range], [-counter_dash_width, counter_dash_width], color='k')
        plt.plot([-counter_dash_width, counter_dash_width], [-i * max_quadrant_range, -i * max_quadrant_range], color='k')
        plt.plot([-i * max_quadrant_range, -i * max_quadrant_range], [-counter_dash_width, counter_dash_width], color='k')
        l.append(i * max_quadrant_range)
        l.append(-i * max_quadrant_range)
        zeros.append(0)
        zeros.append(0)

def callback(value):
    pos = value
    global back
    back = np.zeros((512,512,3), np.uint8)
    for i in range(0, pos+1):
        data = parsepacket(drive_packets[i][1])
        x = int(float(data['x'])*1000)
        y = int(float(data['y'])*1000)
        back = cv2.circle(back, (x,y), 4, (0,0,255), -1)
    cv2.imshow('pos', back)
    print(drive_packets[pos][0])

    #Display dell'immagine
    if(images.has_key(drive_packets[pos][0])):
        img = cv2.imread(images[drive_packets[pos][0]])
        cv2.imshow('frames', img)
    return

file = open("modified_output.txt")
drive_packets=[]
images= OrderedDict()
for l in file:
    linea = l.replace("\n", "").split(";\t")
    if (linea[1] == "DRIVE"):
        drive_packets.append((linea[0], linea[2]))
    elif (linea[1] == "FRAME"):
        images[linea[0]]= "./"+linea[2]

#drivepackets  un array contenente tuple timestamp-paccketto
sliderpos = 0
#back= np.zeros((512,512,3), np.uint8)
#img= np.zeros((352,288,3), np.uint8)
cv2.namedWindow('frames', cv2.WINDOW_AUTOSIZE)
cv2.namedWindow('pos', cv2.WINDOW_AUTOSIZE)
cv2.moveWindow('frames', 0, 0)
cv2.moveWindow('pos', 0, 500)
cv2.createTrackbar('Timestamp', 'pos', sliderpos, len(drive_packets), callback)
cv2.imshow('frames', img)
cv2.imshow('pos', back)
cv2.waitKey(0)
cv2.destroyAllWindows()
