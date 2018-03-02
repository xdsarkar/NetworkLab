CC = g++

all: packunpack.o serverlib.o clientlib.o serverlib clientlib

serverlib: serverlib.o packunpack.o
	$(CC) -o serverlib serverlib.o packunpack.o

clientlib: clientlib.o packunpack.o
	$(CC) -o clientlib clientlib.o packunpack.o

packunpack.o:
	$(CC) -c packunpack.cpp

serverlib.o:
	$(CC) -c serverlib.cpp

clientlib.o:
	$(CC) -c clientlib.cpp

clean:	
	rm packunpack.o serverlib.o clientlib.o serverlib clientlib