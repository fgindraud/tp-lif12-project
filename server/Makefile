CFLAGS = -Wall -Wextra -O2 
EXEC = server

.PHONY: all clean mrproper

all: $(EXEC)

$(EXEC): server.c main.c server.h ../protocol/protocol.h
	gcc $(CFLAGS) $^ -o $@

clean:

mrproper: clean
	rm -f $(EXEC)

