CC = gcc
CFLAGS = -std=c17 -Wall -Werror -pedantic -g -lm

all: Thermo

Thermo: Thermocouple.c
	$(CC) $(CFLAGS) -o $@ $^

clean: Thermo
	rm -f *.0
	rm $^