#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <mosquitto.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>

#include "scd4x.h"

#define mqtt_keep_alive 60
#define mqtt_max_message_size 512

char buff[mqtt_max_message_size];
bool session = true;

int device;

int main(void) {

  char* mqtt_host = getenv("MQTT_HOST");
  char* mqtt_port_s = getenv("MQTT_PORT");
  char* mqtt_username = getenv("MQTT_USERNAME");
  char* mqtt_password = getenv("MQTT_PASSWORD");
  if (mqtt_host == NULL || mqtt_port_s == NULL || mqtt_username == NULL || mqtt_password == NULL) {
    printf("one of the environment varabiles not set: MQTT_HOST, MQTT_PORT, MQTT_USERNAME, MQTT_PASSWORD\n");
    return 1;
  }
  int mqtt_port = strtol(mqtt_port_s, (char **)NULL, 10);
  device = open("/dev/i2c-0", O_RDWR);
  if (device < 0) {
    printf("file open error: %x\n", device);
    exit(1);
  }

  __u8 i2c_addr = 0x62;

  if (ioctl(device, I2C_SLAVE, i2c_addr) < 0) {
    printf("fail to specify i2c_addr %02x", i2c_addr);
    exit(1);
  }

  printf("stop_periodic_measurement and sleep 6s\n");
  stop_periodic_measurement();
  sleep(6);

  __u16 serial_number_0;
  __u16 serial_number_1;
  __u16 serial_number_2;
  char serial_number[12];
  get_serial_number(&serial_number_0, &serial_number_1, &serial_number_2);
  sprintf(serial_number, "%x-%x-%x", serial_number_0, serial_number_1, serial_number_2);
  printf("serial_number: %s\n", serial_number);
  sleep(1);

  __u16 sensor_altitude = 123;
  // printf("setting sensor_altitude to: %d\n", sensor_altitude);
  // set_sensor_altitude(sensor_altitude);
  // sleep(1);
  get_sensor_altitude(&sensor_altitude);
  printf("sensor_altitude: %d\n", sensor_altitude);
  sleep(1);

  float temperature_offset;
  get_temperature_offset(&temperature_offset);
  printf("temperature_offset: %f\n", temperature_offset);
  sleep(1);

  __u16 asc_enabled;
  get_automatic_self_calibration_enabled(&asc_enabled);
  printf("asc_enabled: 0x%04x\n", asc_enabled);
  sleep(1);

  printf("start_periodic_measurement and sleep 6s\n");
  start_periodic_measurement();
  sleep(6);

  char client_id[32];
  sprintf(client_id, "scd4x_%s", serial_number);

  struct mosquitto* mosq = NULL;
  mosquitto_lib_init();
  mosq = mosquitto_new(client_id, session, NULL);
  if (!mosq) {
    printf("failed to create client!\n");
    mosquitto_lib_cleanup();
    return 1;
  }
  mosquitto_username_pw_set(mosq, mqtt_username, mqtt_password);
  if (mosquitto_connect(mosq, mqtt_host, mqtt_port, mqtt_keep_alive)) {
    fprintf(stderr, "Unable to connect.\n");
    return 1;
  }

  int loop = mosquitto_loop_start(mosq);
  if (loop != MOSQ_ERR_SUCCESS) {
    printf("mosquitto loop error\n");
    return 1;
  }

  sprintf(buff, "%s is up\n", client_id);
  printf("%s", buff);
  mosquitto_publish(mosq, NULL, "up:scd4x", strlen(buff) + 1, buff, 0, 0);

  __u16 co2_concentration;
  float temperature;
  float relative_humidity;
  time_t ltime;
  while (1) {
    read_measurement(&co2_concentration, &temperature, &relative_humidity);
    ltime = time(NULL);
    sprintf(buff, "%s:CO_2=%dppm,T=%fC,RH=%f%%\n", strtok(asctime(localtime(&ltime)), "\n"),
            co2_concentration, temperature, relative_humidity);
    printf("%s", buff);
    mosquitto_publish(mosq, NULL, "update:scd4x", strlen(buff) + 1, buff, 0, 0);
    sleep(5);
  }
}
