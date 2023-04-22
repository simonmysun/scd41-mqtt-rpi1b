#include <linux/i2c-dev.h>
#include <stdio.h>
#include <unistd.h>

extern int device;

int scd4x_send_command(__u16 command);
int scd4x_write(__u16 command, __u16 datum);
int scd4x_read(__u16 command, __u8* data, int length);
int scd4x_send_and_fetch(__u16 command, __u16 datum,
                          __u8* data, int length);