CC = g++

computer:
	clear
	$(CC) -std=c++20 -o a.exe computer.cpp -pthread
	./a.exe
decoder:
	clear
	$(CC) -std=c++20 -o a.exe decoder.cpp
	./a.exe
clean:
	rm -f a.exe

