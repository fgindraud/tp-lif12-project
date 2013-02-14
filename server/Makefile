CFLAGS = -Wall -Wextra -O2 -g
EXEC = server

.PHONY: all clean mrproper

all: $(EXEC)

$(EXEC): main.c ../protocol.h
	gcc $(CFLAGS) $< -o $@

clean:

mrproper: clean
	rm -f $(EXEC)

