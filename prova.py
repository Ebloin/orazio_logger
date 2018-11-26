import cv2
import numpy as np

img = cv2.imread("output_frames/88.jpg")
cv2.imshow("test", img)
cv2.waitKey(0)
cv2.destroyAllWindows()
