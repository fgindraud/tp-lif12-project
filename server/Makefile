#CC = clang
CFLAGS = -Wall -Wextra -O2 

BIN=server
OBJ=main.o server.o

.PHONY: all clean mrproper

all: $(BIN)

$(BIN): $(OBJ)

server.o: server.c server.h ../protocol/protocol.h

main.o: main.c server.h

clean:
	rm -f $(OBJ)

mrproper: clean
	rm -f $(BIN)

