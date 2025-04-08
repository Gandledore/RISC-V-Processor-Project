CC = g++

computer:
	clear
	$(CC) -std=c++14 -o a.exe computer.cpp
	./a.exe
decoder:
	clear
	$(CC) -std=c++14 -o a.exe decoder.cpp
	./a.exe
clean:
	rm -f a.exe

