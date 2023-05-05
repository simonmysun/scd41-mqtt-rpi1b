## drivers from source
https://github.com/fastoe drivers doesn't work

## precompiled drivers
using http://downloads.fars-robotics.net/wifi-drivers/8822bu-drivers/

downgrade kernel to 5.10.73+ with `rpi-update 9fe1e973b550019bd6a87966db8443a70b991129` ([ref](https://github.com/Hexxeh/rpi-firmware/blob/9fe1e973b550019bd6a87966db8443a70b991129/uname_string), [ref](https://raspberrypi.stackexchange.com/questions/19959/how-to-get-a-specific-kernel-version-for-raspberry-pi-linux))

lock kernel version by `apt-mark hold libraspberrypi-bin libraspberrypi-dev libraspberrypi-doc libraspberrypi0 raspberrypi-bootloader raspberrypi-kernel raspberrypi-kernel-headers` ([ref](https://forums.raspberrypi.com//viewtopic.php?p=1701547#p1701547))

```bash
wget http://downloads.fars-robotics.net/wifi-drivers/install-wifi -O /usr/bin/install-wifi
chmod +x /usr/bin/install-wifi
install-wifi
```

Failed. system hang after wlan0 read

## older kernel with precompiled drivers
image from https://downloads.raspberrypi.org/raspbian_lite/images/raspbian_lite-2020-02-14/

using `dd` to flash image

using `parted` to resize partition to avoid wasting space

using `e2fsck -f` and `resize2fs` to make size valid

boot into rpi

repeat last chapter

result: wlan0 appears but cannot be brought up.

## connect to pc via ethernet and share pc internet

finally gave up on wifi adapter drivers.

connect cat6 ethernet to a pc with internet access via wifi.

install dhcpd.

``` bash
pacman -S dhcp
```

add static ip for ethernet adapter

``` bash
ip addr add 192.168.155.1/24 dev enp8s0
```

`enp8s0` is my ethernet interface connecting raspberry pi.

backup dhcpd config

``` bash
cp /etc/dhcpd.conf /etc/dhcpd.conf.example
```

add dhcp service configuration

``` conf
/etc/dhcpd.conf
---

...

option domain-name-servers 1.1.1.1, 1.0.0.1;
option subnet-mask 255.255.255.0;
option routers 192.168.155.1;
subnet 192.168.155.0 netmask 255.255.255.0 {
  range 192.168.155.100 192.168.155.150;
}

# avoid serving dhcp under wifi router
subnet 192.168.2.0 netmask 255.255.255.0 {
}
```

start dhcp service

``` bash
systemctl enable dhcpd4
systemctl start dhcpd4
```

the two deviced shoud be able to ping each other now.

share internet

``` bash
echo "1" > /proc/sys/net/ipv4/ip_forward
```

add `net.ipv4.ip_forward=1` to `/etc/sysctl.conf` to make this change permanent

enable NAT with `iptables`

``` bash
iptables -F
iptables -P INPUT ACCEPT
iptables -P FORWARD ACCEPT
iptables -t nat -A POSTROUTING -o wlp4s0 -j MASQUERADE
```

`wlp4s0` is my wireless adapter with internet connection

the Pi shoud be able to access internet now.

([ref](http://linux-wiki.cn/wiki/zh-hans/%E5%85%B1%E4%BA%AB%E4%B8%8A%E7%BD%91))

## read data from sensor

enable i2c in `sudo raspi-config`, connect to corresponding pins (I'm using Pin 3, 4, 5, 6) ([ref](https://pinout.xyz/pinout/i2c)), restart pi

``` bash

ls /dev/i2c*
apt install i2c-tools
i2cdetect 0
i2cdump 0 0x62
```

however scd4x doesn't work that way ([ref](https://cdn.sparkfun.com/assets/d/4/9/a/d/Sensirion_CO2_Sensors_SCD4x_Datasheet.pdf))

we can read write directly ([ref](https://www.kernel.org/doc/Documentation/i2c/dev-interface))

Include `linux/i2c-dev.h` to get definition of `I2C_SLAVE`, include`fcntl.h` to open file, include `unistd.h` to `read`, `write` and `sleep`, and include `stdio.h` to print message to stdout.

```c
#include <linux/i2c-dev.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
```

open i2c device (mine is `/dev/i2c-0`, it can be elsewhere):

```c
int device = open("/dev/i2c-0", O_RDWR);
if (device < 0) {
  printf("file open error: %x\n", device);
  exit(1);
}
```

specify scd4x sensor address:

```c
__u8 addr = 0x62;

if (ioctl(device, I2C_SLAVE, addr) < 0) {
  printf("fail to specify i2c addr %02x", addr);
  exit(1);
}
```

to send command `start_periodic_measurement`:

```c
__u8 command[2] = { 0x21, 0xb1, };
if (write(device, command, 2) != 2) {
  printf("fail to write\n");
}
```

the result of `read_measurement` has a CRC checksum, let's copy and adapt the CRC function in the scd4x datasheet.

```c
__u8 crc(__u8* data, __u16 count) {
  __u16 current_byte;
  __u8 crc = CRC8_INIT;
  __u8 crc_bit;
  /* calculates 8-Bit checksum with given polynomial */
  for (current_byte = 0; current_byte < count; ++current_byte) {
    crc ^= (data[current_byte]);
    for (crc_bit = 8; crc_bit > 0; --crc_bit) {
      if (crc & 0x80)
        crc = (crc << 1) ^ CRC8_POLYNOMIAL;
      else
        crc = (crc << 1);
    }
  }
  return crc;
}
```

to read measurement:

```c
__u8 command[2] = { 0xec, 0x05, };
__u8 result[10];
if (write(device, command, 2) != 2) {
  printf("fail to write\n");
}
if (read(device, result, 9) != 9) {
  printf("read fails\n");
}
for (int i = 0; i < length; i += 3) {
  if (crc(data + i, 2) != data[i + 2]) {
    printf("crc fails on %d", i);
  }
}
int co2 = data[0];
co2 <<= 8;
co2 += data[1];
float t = data[3];
t *= 256;
t += data[4];
t = -45 + (175 * t) / 0x10000;
float rh = data[6];
rh *= 256;
rh += data[7];
rh = (rh * 100) / 0x10000;;
printf("CO_2: %dppm; T: %fC; RH: %f%%\n", co2, t, rh);
```

but this is ugly. let's add an abstract layer and define:

```c
int scd4x_send_command(int device, __u8 addr, __u16 command);
int scd4x_write(int device, __u8 addr, __u16 command, __u16 datum);
int scd4x_read(int device, __u8 addr, __u16 command, __u8* data, int length);
int scd4x_send_and_fetch(int device, __u8 addr, __u16 command, __u16 datum,
                          __u8* data, int length);
```

then we can implement all the funtions of scd41 happily:

```c
int start_periodic_measurement();
int read_measurement(__u16* co2_concentration, float* temperature, float* relative_humidity);
int stop_periodic_measurement();
int set_temperature_offset(float temperature_offset);
int get_temperature_offset(float* temperature_offset);
int set_sensor_altitude(__u16 altitude);
int get_sensor_altitude(__u16* altitude);
int set_ambient_pressure(__u16 ambient_pressure);
int perform_forced_recalibration(__u16 target_co2_concentration, __u16* frc_correction);
int set_automatic_self_calibration_enabled(__u16 asc_enabled);
int get_automatic_self_calibration_enabled(__u16* asc_enabled);
int start_low_power_periodic_measurement();
int get_data_ready_status(__u16* signal);
int persist_settings();
int get_serial_number(__u16* serial_number_0, __u16* serial_number_1, __u16* serial_number_2);
int perform_self_test(__u16* sensor_status);
int perform_factory_reset();
int reinit();
// SCD41 only
int measure_single_shot();
int measure_single_shot_rht_only();
```

Note: It seems that writing data to the sensor does not work now. But I currently don't have much need to do so. Will fix it in the future.




## ref

https://github.com/mqtt/mqtt.org/wiki/libraries
https://github.com/eclipse/paho.mqtt.c
