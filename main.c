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

int device;

int main(void) {
  time_t start_time;
  time(&start_time);

  char* mqtt_host = getenv("MQTT_HOST");
  char* mqtt_port_s = getenv("MQTT_PORT");
  char* mqtt_username = getenv("MQTT_USERNAME");
  char* mqtt_password = getenv("MQTT_PASSWORD");
  if (mqtt_host == NULL || mqtt_port_s == NULL || mqtt_username == NULL ||
      mqtt_password == NULL) {
    printf(
        "one of the environment varabiles not set: MQTT_HOST, MQTT_PORT, "
        "MQTT_USERNAME, MQTT_PASSWORD\n");
    exit(1);
  }
  int mqtt_port = strtol(mqtt_port_s, (char**)NULL, 10);

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
  if (stop_periodic_measurement() != 0) {
    exit(1);
  }
  sleep(6);

  __u16 serial_number_0;
  __u16 serial_number_1;
  __u16 serial_number_2;
  char serial_number[12];
  if (get_serial_number(&serial_number_0, &serial_number_1, &serial_number_2) !=
      0) {
    exit(1);
  }
  sprintf(serial_number, "%x-%x-%x", serial_number_0, serial_number_1,
          serial_number_2);
  printf("serial_number: %s\n", serial_number);
  sleep(1);

  __u16 sensor_altitude = 123;
  // printf("setting sensor_altitude to: %d\n", sensor_altitude);
  // set_sensor_altitude(sensor_altitude);
  // sleep(1);
  if (get_sensor_altitude(&sensor_altitude) != 0) {
    exit(1);
  }
  printf("sensor_altitude: %d\n", sensor_altitude);
  sleep(1);

  float temperature_offset;
  if (get_temperature_offset(&temperature_offset) != 0) {
    exit(1);
  }
  printf("temperature_offset: %f\n", temperature_offset);
  sleep(1);

  __u16 asc_enabled;
  if (get_automatic_self_calibration_enabled(&asc_enabled) != 0) {
    exit(1);
  }
  printf("asc_enabled: 0x%04x\n", asc_enabled);
  sleep(1);

  printf("start_periodic_measurement and sleep 6s\n");
  if (start_periodic_measurement() != 0) {
    exit(1);
  }
  sleep(6);

  char mqtt_client_id[32];
  sprintf(mqtt_client_id, "scd4x_%s", serial_number);
  char topic[64];

  struct mosquitto* mosq = NULL;
  if (mosquitto_lib_init() != MOSQ_ERR_SUCCESS) {
    fprintf(stderr, "mqtt:failed to init mosquitto lib\n");
    exit(1);
  }
  mosq = mosquitto_new(mqtt_client_id, true, NULL);
  if (!mosq) {
    fprintf(stderr, "mqtt:failed to create client!\n");
    mosquitto_lib_cleanup();
    exit(1);
  }
  if (mosquitto_username_pw_set(mosq, mqtt_username, mqtt_password) !=
      MOSQ_ERR_SUCCESS) {
    fprintf(stderr, "mqtt:failed set username password\n");
    mosquitto_lib_cleanup();
    exit(1);
  }
  if (mosquitto_connect(mosq, mqtt_host, mqtt_port, mqtt_keep_alive) !=
      MOSQ_ERR_SUCCESS) {
    fprintf(stderr, "mqtt:Unable to connect.\n");
    mosquitto_lib_cleanup();
    exit(1);
  }

  if (mosquitto_loop_start(mosq) != MOSQ_ERR_SUCCESS) {
    fprintf(stderr, "mqtt:failed to start mosquitto loop\n");
    mosquitto_lib_cleanup();
    exit(1);
  }

  __u16 co2_concentration;
  float temperature;
  float relative_humidity;
  time_t now;
  int counter = 0;

  while (1) {
    counter++;
    if (counter > 150) {  // 150 * 6s = 15min
      counter = 0;
      sprintf(topic, "homeassistant/sensor/%s/LWT", mqtt_client_id);
      sprintf(buff, "Offline");
      printf("mqtt:will_topic='%s',msg='%s'\n", topic, buff);
      if (mosquitto_will_set(mosq, topic, strlen(buff), buff, 0, 0) !=
          MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "mqtt:failed to set will message\n");
        mosquitto_lib_cleanup();
        exit(1);
      }
      sleep(1);

      sprintf(topic, "homeassistant/sensor/%s/LWT", mqtt_client_id);
      sprintf(buff, "Online");
      printf("mqtt:topic='%s',msg='%s'\n", topic, buff);
      if (mosquitto_will_set(mosq, topic, strlen(buff), buff, 0, 0) !=
          MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "mqtt:failed to set will message\n");
        mosquitto_lib_cleanup();
        exit(1);
      }
      sleep(1);

      sprintf(topic, "homeassistant/sensor/%s_CO2/config", mqtt_client_id);
      sprintf(
          buff,
          "{\"device_class\":\"carbon_dioxide\",\"name\":\"CO2 "
          "Concentration\",\"state_class\":\"measurement\",\"unique_id\":\"%s_"
          "CO2\",\"state_topic\":\"homeassistant/sensor/%s/"
          "state\",\"unit_of_measurement\":\"ppm\",\"value_template\":\"{{ "
          "value_json.co2_concentration }}\"}",
          mqtt_client_id, mqtt_client_id);
      printf("mqtt:topic='%s',msg='%s'\n", topic, buff);
      if (mosquitto_publish(mosq, NULL, topic, strlen(buff), buff, 0, 0) !=
          MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "mqtt:failed to publish\n");
        mosquitto_lib_cleanup();
        exit(1);
      }
      sleep(1);

      sprintf(topic, "homeassistant/sensor/%s_T/config", mqtt_client_id);
      sprintf(
          buff,
          "{\"device_class\":\"temperature\",\"name\":\"Temperature\",\"state_"
          "class\":\"measurement\",\"unique_id\":\"%s_T\",\"state_topic\":"
          "\"homeassistant/sensor/%s/"
          "state\",\"unit_of_measurement\":\"°C\",\"value_template\":\"{{ "
          "value_json.temperature }}\"}",
          mqtt_client_id, mqtt_client_id);
      printf("mqtt:topic='%s',msg='%s'\n", topic, buff);
      if (mosquitto_publish(mosq, NULL, topic, strlen(buff), buff, 0, 0) !=
          MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "mqtt:failed to publish\n");
        mosquitto_lib_cleanup();
        exit(1);
      }
      sleep(1);

      sprintf(topic, "homeassistant/sensor/%s_RH/config", mqtt_client_id);
      sprintf(
          buff,
          "{\"device_class\":\"humidity\",\"name\":\"Relative "
          "Humidity\",\"state_class\":\"measurement\",\"unique_id\":\"%s_RH\","
          "\"state_topic\":\"homeassistant/sensor/%s/"
          "state\",\"unit_of_measurement\":\"%%\",\"value_template\":\"{{ "
          "value_json.relative_humidity }}\"}",
          mqtt_client_id, mqtt_client_id);
      printf("mqtt:topic='%s',msg='%s'\n", topic, buff);
      if (mosquitto_publish(mosq, NULL, topic, strlen(buff), buff, 0, 0) !=
          MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "mqtt:failed to publish\n");
        mosquitto_lib_cleanup();
        exit(1);
      }
      sleep(1);

      sprintf(topic, "homeassistant/sensor/%s_ts/config", mqtt_client_id);
      sprintf(buff,
              "{\"device_class\":\"timestamp\",\"name\":\"Timestamp\",\"unique_"
              "id\":\"%s_ts\",\"state_topic\":\"homeassistant/sensor/%s/"
              "state\",\"unit_of_measurement\":\"\",\"value_template\":\"{{ "
              "value_json.timestamp }}\"}",
              mqtt_client_id, mqtt_client_id);
      printf("mqtt:topic='%s',msg='%s'\n", topic, buff);
      if (mosquitto_publish(mosq, NULL, topic, strlen(buff), buff, 0, 0) !=
          MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "mqtt:failed to publish\n");
        mosquitto_lib_cleanup();
        exit(1);
      }
      sleep(1);
    } else {
      sleep(6);
    }
    if (read_measurement(&co2_concentration, &temperature,
                         &relative_humidity) != 0) {
      exit(1);
    }
    time(&now);
    char timestr[21];
    sprintf(topic, "homeassistant/sensor/%s/state", mqtt_client_id);
    strftime(timestr, sizeof timestr, "%Y-%m-%dT%H:%M:%SZ", gmtime(&now));
    sprintf(buff,
            "{\"timestamp\":\"%s\",\"co2_concentration\":%d,\"temperature\":%f,"
            "\"relative_humidity\":%f}",
            timestr, co2_concentration, temperature, relative_humidity);
    printf("mqtt:topic='%s',msg='%s'\n", topic, buff);
    int err = mosquitto_publish(mosq, NULL, topic, strlen(buff), buff, 0, 0);
    if (err != MOSQ_ERR_SUCCESS) {
      fprintf(stderr, "mqtt:failed to publish\n");
      if (err == MOSQ_ERR_NO_CONN) {
        fprintf(stderr, "mqtt:not connected\n");
      } else {
        fprintf(stderr, "mqtt: %d\n", err);
        exit(1);
      }
    }
  }
}
