CC = g++

all:
	$(CC) -std=c++14 -o a.exe decoder.cpp
	./a.exe
clean:
	rm -f a.exe

