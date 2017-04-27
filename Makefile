all: cimgTest.o

cimgTest.o : 
	g++ -o cimgTest cimgTest.cpp -O2 -lm -lpthread -I/usr/X11/include -L/usr/X11/lib -lm -lpthread -lX11
clean :
	rm -f *.o cimgTest