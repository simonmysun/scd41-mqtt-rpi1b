# CO2 Sensor on Raspberry Pi with MQTT and Home Assistant Integration

This project measures the CO2 concentration in your environment using a Raspberry Pi and an SCD4x sensor. The data is sent to a server via MQTT and integrated with Home Assistant for monitoring and alerting.

## Requirements

- Raspberry Pi with Raspbian (tested on Raspberry Pi 1B)
- SCD4x CO2 sensor
- MQTT service (e.g., Mosquitto)
- Home Assistant service
- Prometheus, Grafana, and Alertmanager services (optional)

## Installation

1. Install the required packages:

```bash
apt install i2c-tools libmosquitto-dev
```

2. Connect sensor enable I2C interface in `raspi-config`

3. Clone the repository and build:

```bash
make
```

4. Configure the MQTT credentials in `scd4x-mqtt.service` (or use `systemctl edit scd4x-mqtt` after the installation)

5. Install:

```bash
make install
```

6. Start the service:

```bash
systemctl enable --now scd4x-mqtt
```

## Configuration in Home Assistant

Goto Configuration -> Integrations -> Add Integration -> MQTT and enter the MQTT server details.

![]()

## Integration with Prometheus, Grafana, and Alertmanager

Enable the Prometheus integration in Home Assistant and configure the Prometheus server to scrape the metrics: https://www.home-assistant.io/integrations/prometheus/

Configuraton of Grafana and Alertmanager is out of scope of this project. You can find the configuration details in the respective documentations.

![]()

## License

[LICENSE](LICENSE)