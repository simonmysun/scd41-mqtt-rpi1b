#include "scd4x_hal.h"

#define CRC8_POLYNOMIAL 0x31
#define CRC8_INIT 0xFF

#define printf(fmt, ...) (0)

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

int scd4x_send_command(__u16 command) {
  char buf[2];
  buf[0] = (command & 0xff00) >> 8;
  buf[1] = command & 0x00ff;
  printf("send_command 0x%02x 0x%02x\n", buf[0], buf[1]);
  if (write(device, buf, 2) != 2) {
    printf("send_command fails\n");
    return 1;
  }
  return 0;
}

int scd4x_write(__u16 command, __u16 datum) {
  if (scd4x_send_command(command) != 0) {
    return 1;
  }
  char buf[3];
  buf[0] = (datum & 0xff00) >> 8;
  buf[1] = datum & 0x00ff;
  buf[2] = crc(buf, 2);
  printf("write 0x%02x 0x%02x 0x%02x\n", buf[0], buf[1], buf[2]);
  if (write(device, buf, 3) != 3) {
    printf("write fails\n");
    return 1;
  }
  return 0;
}

int scd4x_read(__u16 command, __u8* data, int length) {
  if (scd4x_send_command(command) != 0) {
    return 1;
  }
  printf("read (length=%d)\n", length);
  if (read(device, data, length) != length) {
    printf("read fails\n");
  }
  for (int i = 0; i < length; i += 1) {
    printf("0x%02x ", data[i]);
  }
  printf("\n");
  for (int i = 0; i < length; i += 3) {
    if (crc(data + i, 2) != data[i + 2]) {
      printf("crc fails on %d", i);
      return 1;
    }
  }
  return 0;
}

int scd4x_send_and_fetch(__u16 command, __u16 datum, __u8* data, int length) {
  // This part is not working. combined transactions of mixing read and write
  // messages are not supported
  if (scd4x_send_command(command) != 0) {
    return 1;
  }
  char buf[3];
  buf[0] = datum & 0xff00 >> 8;
  buf[1] = datum & 0x00ff;
  buf[3] = crc(buf, 2);
  printf("write 0x%02x 0x%02x 0x%02x\n", buf[0], buf[1], buf[2]);
  if (write(device, buf, 2) != 2) {
    printf("write fails during send and fetch\n");
    return 1;
  }
  printf("read (length=%d)\n", length);
  if (read(device, data, length) != length) {
    printf("read fails during send and fetch\n");
    return 1;
  }
  for (int i = 0; i < length; i += 3) {
    if (crc(data + i, 2) != data[i + 2]) {
      printf("crc fails on %d", i);
      return 1;
    }
  }
  return 0;
}