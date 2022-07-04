CC=g++
CFLAGS=-Wall -Wextra -Wno-unused-parameter -pthread
EXEC=./tp2

$(EXEC): main.cpp
	$(CC) $(CFLAGS) main.cpp -o $(EXEC)