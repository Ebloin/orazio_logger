from pylab import *
import matplotlib.pyplot as plt
import cv2
from collections import OrderedDict


def imshow(path):
	subplot(211)
	img = cv2.imread(path,0)
	plt.imshow(img, cmap = 'gray', interpolation = 'bicubic')
	plt.xticks([]), plt.yticks([])  

def parsepacket(p):
    splitted = p.replace("{", "").replace("}", "").replace(" ", "").split(",")
    dict = {}
    for e in splitted:
        keyval = e.split(":")
        dict[keyval[0]] = keyval[1]
    return dict

file = open("output.txt")
drive_packets=[]
images= OrderedDict()
for l in file:
    linea = l.replace("\n", "").split(";\t")
    if (linea[1] == "DRIVE"):
        drive_packets.append((linea[0], linea[2]))
    elif (linea[1] == "FRAME"):
        images[linea[0]]= "./"+linea[2]

subplot(211)
plt.xticks([]), plt.yticks([])  
subplot(212)
axis([-50, 50, -50, 50])
axcolor = 'lightgoldenrodyellow'
sliax = axes([0.15, 0.1, 0.65, 0.03], axisbg=axcolor)
slider = Slider(sliax, 'Timestamp', 0, len(drive_packets)-1, valinit=0)
subplots_adjust(bottom=0.25)

def callback(val):
	pos = int(val)
	subplot(211)
	if(images.has_key(drive_packets[pos][0])):
		img = cv2.imread(images[drive_packets[pos][0]])
		plt.imshow(img)
		plt.xticks([]), plt.yticks([])
	x= []
	y= []
	for i in range(0, pos+1):
		p = parsepacket(drive_packets[i][1])
		x.append(float(p['x']))
		y.append(float(p['y']))
	subplot(212)
	cla()
	axis([-50, 50, -50, 50])
	plot(x, y)

slider.on_changed(callback)
show()
