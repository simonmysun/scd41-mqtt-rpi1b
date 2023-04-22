#include "scd4x.h"

int start_periodic_measurement() {
  int ret = scd4x_send_command(0x21b1);
  if(ret != 0) {
    printf("start_periodic_measurement failed\n");
    return 1;
  }
  return 0;
}

int read_measurement(__u16* co2_concentration, float* temperature, float* relative_humidity) {
  __u8 buffer[10];
  int ret = scd4x_read(0xec05, buffer, 9);
  if(ret != 0) {
    printf("read_measurement failed\n");
    return 1;
  }
  *co2_concentration = buffer[0];
  *co2_concentration <<= 8;
  *co2_concentration += buffer[1];
  *temperature = buffer[3];
  *temperature *= 256;
  *temperature += buffer[4];
  *temperature = -45 + (175 * *temperature) / 0x10000;
  *relative_humidity = buffer[6];
  *relative_humidity *= 256;
  *relative_humidity += buffer[7];
  *relative_humidity = (*relative_humidity * 100) / 0x10000;
  return 0;
}

int stop_periodic_measurement() {
  int ret = scd4x_send_command(0x3f86);
  if(ret != 0) {
    printf("stop_periodic_measurement failed\n");
    return 1;
  }
  return 0;
}

int set_temperature_offset(float temperature_offset) {
  int ret = scd4x_write(0x241d, (__u16)(0x10000 * temperature_offset / 175));
  if(ret != 0) {
    printf("set_temperature_offset failed\n");
    return 1;
  }
  return 0;
}

int get_temperature_offset(float* temperature_offset) {
  __u8 buffer[4];
  int ret = scd4x_read(0x2318, buffer, 3);
  if(ret != 0) {
    printf("get_temperature_offset failed\n");
    return 1;
  }
  *temperature_offset = buffer[0];
  *temperature_offset *= 256;
  *temperature_offset += buffer[1];
  *temperature_offset = (__u16)(175 * *temperature_offset / 0x10000);
  return 0;
}

int set_sensor_altitude(__u16 altitude) {
  int ret = scd4x_write(0x2427, altitude);
  if(ret != 0) {
    printf("set_sensor_altitude failed\n");
    return 1;
  }
  return 0;
}

int get_sensor_altitude(__u16* altitude) {
  __u8 buffer[4];
  int ret = scd4x_read(0x2322, buffer, 3);
  if(ret != 0) {
    printf("get_sensor_altitude failed\n");
    return 1;
  }
  *altitude = buffer[0];
  *altitude <<= 8;
  *altitude += buffer[1];
  return 0;
}

int set_ambient_pressure(__u16 ambient_pressure) {
  int ret = scd4x_write(0xe000, ambient_pressure);
  if(ret != 0) {
    printf("set_ambient_pressure failed\n");
    return 1;
  }
  return 0;
}

int perform_forced_recalibration(__u16 target_co2_concentration, __u16* frc_correction) {
  __u8 buffer[4];
  int ret = scd4x_send_and_fetch(0x362f, target_co2_concentration, buffer, 3);
  if(ret != 0) {
    printf("perform_forced_recalibration failed\n");
    return 1;
  }
  *frc_correction = buffer[0];
  *frc_correction <<= 8;
  *frc_correction += buffer[1];
  return 0;
}

int set_automatic_self_calibration_enabled(__u16 asc_enabled) {
  int ret = scd4x_write(0x2416, asc_enabled);
  if(ret != 0) {
    printf("set_automatic_self_calibration_enabled failed\n");
    return 1;
  }
  return 0;
}

int get_automatic_self_calibration_enabled(__u16* asc_enabled) {
  __u8 buffer[4];
  int ret = scd4x_read(0x2313, buffer, 3);
  if(ret != 0) {
    printf("get_automatic_self_calibration_enabled failed\n");
    return 1;
  }
  *asc_enabled = buffer[0];
  *asc_enabled <<= 8;
  *asc_enabled += buffer[1];
  return 0;
}

int start_low_power_periodic_measurement() {
  int ret = scd4x_send_command(0x21ac);
  if(ret != 0) {
    printf("start_low_power_periodic_measurement failed\n");
    return 1;
  }
  return 0;
}

int get_data_ready_status(__u16* signal) {
  __u8 buffer[4];
  int ret = scd4x_read(0xe4b8, buffer, 3);
  if(ret != 0) {
    printf("get_data_ready_status failed\n");
    return 1;
  }
  *signal = buffer[0];
  *signal <<= 8;
  *signal += buffer[1];
  return 0;
}

int persist_settings() {
  int ret = scd4x_send_command(0x3615);
  if(ret != 0) {
    printf("persist_settings failed\n");
    return 1;
  }
  return 0;
}

int get_serial_number(__u16* serial_number_0, __u16* serial_number_1, __u16* serial_number_2) {
  __u8 buffer[10];
  int ret = scd4x_read(0x3682, buffer, 9);
  if(ret != 0) {
    printf("get_serial_number failed\n");
    return 1;
  }
  *serial_number_0 = buffer[0];
  *serial_number_0 <<= 8;
  *serial_number_0 += buffer[1];
  *serial_number_1 = buffer[3];
  *serial_number_1 *= 256;
  *serial_number_1 += buffer[4];
  *serial_number_2 = buffer[6];
  *serial_number_2 *= 256;
  *serial_number_2 += buffer[7];
  return 0;
}

int perform_self_test(__u16* sensor_status) {
  __u8 buffer[4];
  int ret = scd4x_read(0x3639, buffer, 3);
  if(ret != 0) {
    printf("perform_self_test failed\n");
    return 1;
  }
  *sensor_status = buffer[0];
  *sensor_status <<= 8;
  *sensor_status += buffer[1];
  return 0;
}

int perform_factory_reset() {
  int ret = scd4x_send_command(0x3632);
  if(ret != 0) {
    printf("perform_factory_reset failed\n");
    return 1;
  }
  return 0;
}

int reinit() {
  int ret = scd4x_send_command(0x3646);
  if(ret != 0) {
    printf("reinit failed\n");
    return 1;
  }
  return 0;
}

// SCD41 only
int measure_single_shot() {
  int ret = scd4x_send_command(0x219d);
  if(ret != 0) {
    printf("measure_single_shot failed\n");
    return 1;
  }
  return 0;
}

int measure_single_shot_rht_only() {
  int ret = scd4x_send_command(0x2196);
  if(ret != 0) {
    printf("measure_single_shot_rht_only failed\n");
    return 1;
  }
  return 0;
}
