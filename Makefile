all: cimgTest.o

cimgTest.o : 
	g++ -o cimgTest cimgTest.cpp -lrt -O2 -L/usr/X11R6/lib -lm -lpthread -lX11
clean :
	rm -f *.o cimgTest
