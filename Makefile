CC=gcc
LIBS=-lmosquitto

scd4x-mqtt: scd4x-mqtt.o scd4x.o scd4x_hal.o
	$(CC) -o scd4x-mqtt scd4x-mqtt.o scd4x.o scd4x_hal.o $(LIBS)

scd4x-mqtt.o: scd4x-mqtt.c
	$(CC) -c scd4x-mqtt.c

scd4x.o: scd4x.c scd4x_hal.o
	$(CC) -c scd4x.c

scd4x_hal.o: scd4x_hal.c
	$(CC) -c scd4x_hal.c

.PHONY: clean
clean:
	-rm *.o scd4x-mqtt

.PHONY: install
install:
	cp scd4x-mqtt /usr/local/bin/scd4x-mqtt
	cp scd4x-mqtt.service /etc/systemd/system

.PHONY: uninstall
uninstall:
	rm /usr/local/bin/scd4x-mqtt
	rm /etc/systemd/system/scd4x-mqtt.service
	systemctl daemon-reload
	systemctl reset-failed
	systemctl stop scd4x-mqtt
	systemctl disable scd4x-mqtt