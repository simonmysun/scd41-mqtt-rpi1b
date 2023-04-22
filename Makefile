main: main.o scd4x.o scd4x_hal.o
	gcc -o main main.o scd4x.o scd4x_hal.o

main.o: main.c
	gcc -c main.c

scd4x.o: scd4x.c scd4x_hal.o
	gcc -c scd4x.c

scd4x_hal.o: scd4x_hal.c
	gcc -c scd4x_hal.c

.PHONY: clean
clean:
	-rm *.o main