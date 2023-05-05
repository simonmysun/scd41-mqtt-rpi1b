CC=gcc
LIBS=-lmosquitto

main: main.o scd4x.o scd4x_hal.o
	$(CC) -o main main.o scd4x.o scd4x_hal.o $(LIBS)

main.o: main.c
	$(CC) -c main.c

scd4x.o: scd4x.c scd4x_hal.o
	$(CC) -c scd4x.c

scd4x_hal.o: scd4x_hal.c
	$(CC) -c scd4x_hal.c

.PHONY: clean
clean:
	-rm *.o main
