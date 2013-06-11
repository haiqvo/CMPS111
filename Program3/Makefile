fileSysTest: fileSysTest.o mydisk.o
	gcc -o fileSysTest fileSysTest.o mydisk.o -lfl

fileSysTest.o: fileSysTest.c
	gcc -c fileSysTest.c

mydisk.o: mydisk.c mydisk.h
	gcc -c mydisk.c

clean: 
	rm fileSysTest *.o

