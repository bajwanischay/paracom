import random
import os
import sys



WORDS = open("/usr/share/dict/words").read().splitlines()
def createDir(path,d,b,f,w):
	if len(path.split("/"))==d+1:
		content=""
		for i in range(w):
			content+=(random.choice(WORDS)+" ")
		a=int(pow(10,f)/pow(b,d))
		#print("Making+"+str(a)+" Files")
		if a==0:
			a=1
		for i in range(a):
			f=open(path+"/"+str(i), "w")
			f.write(content)
			f.close()
		return

	for i in range(b):
		newpath=path+"/"+str(i)
		#print(newpath)
		os.mkdir(newpath)
		createDir(newpath,d,b,f,w)

rootfile="D"+sys.argv[1]+"B"+sys.argv[2]+"F"+sys.argv[3]
os.mkdir(rootfile)
createDir(rootfile,int(sys.argv[1]),int(sys.argv[2]),int(sys.argv[3]),int(sys.argv[4]))
