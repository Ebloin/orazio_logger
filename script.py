f= open("output.txt", "r")
out= open("modified_output.txt", "w")

x=0.000
y=0.000
for l in f:
    l=l.replace("x:0.000", "x:"+str(x))
    l=l.replace("y:0.000", "y:"+str(y))
    x= x+0.01
    y= y+0.01
    out.write(l)
out.close()
f.close()
