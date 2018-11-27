from collections import OrderedDict
import numpy as np
import cv2
import vpython

#PACKET EXAMPLE
#{seq:01094, x:0.000, y:0.000, t:0.000, tvm:0.000, tvd:0.600, tva:0.060, rvm:0.000, rvd:0.100, rva:0.100}


def parsepacket(p):
    splitted = p.replace("{", "").replace("}", "").replace(" ", "").split(",")
    dict = {}
    for e in splitted:
        keyval = e.split(":")
        dict[keyval[0]] = keyval[1]
    return dict

file = open("output.txt")
drive_packets= OrderedDict()
images= OrderedDict()
for l in file:
    linea = l.replace("\n", "").split(";\t")
    if (linea[1] == "DRIVE"):
        drive_packets[int(linea[0])]= linea[2]
    elif (linea[1] == "FRAME"):
        images[int(linea[0])]= "./"+linea[2]

back= np.zeros((512,512,3), np.uint8)
cv2.namedWindow('frames', cv2.WINDOW_AUTOSIZE)
cv2.namedWindow('pos', cv2.WINDOW_AUTOSIZE)
for k in drive_packets:
    back = cv2.circle(back, ())
    data = parsepacket(drive_packets[k])
    print("POSIZIONE:\tX: "+data['x']+"\tY: "+ data['y']+ "\n")
    if(images.has_key(k)):
        img = cv2.imread(images[k])
        cv2.imshow('frames', img)
        cv2.imshow('pos', back)
        cv2.waitKey(0)
cv2.destroyAllWindows()
