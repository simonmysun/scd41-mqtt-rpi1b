#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <time.h>

#include "scd4x.h"

int device;

int main(void) {
  device = open("/dev/i2c-0", O_RDWR);
  if (device < 0) {
    printf("file open error: %x\n", device);
    exit(1);
  }

  __u8 addr = 0x62;

  if (ioctl(device, I2C_SLAVE, addr) < 0) {
    printf("fail to specify i2c addr %02x", addr);
    exit(1);
  }
  // scd4x_send_command(device, 0x21b1);
  // sleep(6);

  __u16 co2_concentration;
  float temperature;
  float relative_humidity;
  time_t ltime;
  while (1) {
    if (read_measurement(&co2_concentration, &temperature,
                         &relative_humidity) != 0) {
      printf("fail to read\n");
    } else {
      ltime = time(NULL);
      printf("%s", asctime(localtime(&ltime)));
      printf("CO_2: %dppm; T: %fC; RH: %f%%\n", co2_concentration, temperature,
             relative_humidity);
    }
    sleep(5);
  }
}
