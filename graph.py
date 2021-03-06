from matplotlib import pyplot as plt
import numpy as np
import csv
import matplotlib

f = open("serverlog.txt",'r');
data = {}
w=5
for line in f:
	message = line.strip().split('|')
	if message[0] not in data:
		data[message[0]] =len(message[3])
	else:
		data[message[0]]=data[message[0]]+len(message[3])
for key in data:
	print key, data[key]

objects = data.keys()
y_pos = np.arange(len(objects))
performance = data.values()
 
plt.bar(y_pos, performance, align='center', alpha=0.5)
plt.xticks(y_pos, objects)
plt.xlabel('Clients')
plt.ylabel('Bytes/Characters send')
plt.title('Server Load Plot')
plt.savefig("barplot.png") 
#plt.show()
plt.close()
f.close()

#generating 2D matrix
mat= [[0 for x in range(w)] for y in range(w)] 
f=open("serverlog.txt",'r')
for line in f:
	line = line.strip().split('|')
	src= line[0].strip().split('t')
	dest= line[1].strip().split('t')
	mat[int(src[1])-1][int(dest[1])-1] =mat[int(src[1])-1][int(dest[1])-1]+ len(line[3])
f.close()
print mat

#generating csv file
with open("messagelog.csv", "wb") as f:
    writer = csv.writer(f)
    writer.writerows(mat)

#generating heapmap
m =np.array(mat)
h  = plt.pcolor(m, cmap = matplotlib.cm.Blues)
plt.xticks(y_pos, objects)
plt.yticks(y_pos, objects)
plt.colorbar()
plt.savefig("heatmap.png")
plt.close()
#plt.show()
