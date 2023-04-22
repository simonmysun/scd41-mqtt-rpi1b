#include "scd4x_hal.h"
#include <float.h>

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