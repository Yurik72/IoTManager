#include "Sensors/BME280Sensor.h"

#include "Global.h"
#include "Events.h"
#include "MqttClient.h"
#include "Utils/PrintMessage.h"

namespace BME280Sensor {
Adafruit_BME280 bme;
Adafruit_Sensor *bme_temp = bme.getTemperatureSensor();
Adafruit_Sensor *bme_pressure = bme.getPressureSensor();
Adafruit_Sensor *bme_humidity = bme.getHumiditySensor();

String bme280T_value_name = "";
String bme280P_value_name = "";
String bme280H_value_name = "";
String bme280A_value_name = "";

void bme280T_reading() {
    float value = 0;
    value = bme.readTemperature();

    liveData.writeFloat(bme280T_value_name, value);
    Events::fire(bme280T_value_name);
    MqttClient::publishStatus(bme280T_value_name, String(value));
}

void bme280P_reading() {
    float value = 0;
    value = bme.readPressure();
    value = value / 1.333224;

    liveData.writeFloat(bme280P_value_name, value);
    Events::fire(bme280P_value_name);
    MqttClient::publishStatus(bme280P_value_name, String(value));
}

void bme280H_reading() {
    float value = 0;
    value = bme.readHumidity();

    liveData.writeFloat(bme280H_value_name, value);
    Events::fire(bme280H_value_name);
    MqttClient::publishStatus(bme280H_value_name, String(value));
}

void bme280A_reading() {
    float value = bme.readAltitude(1013.25);

    liveData.writeFloat(bme280A_value_name, value);
    Events::fire(bme280A_value_name);
    MqttClient::publishStatus(bme280A_value_name, String(value));
}

}  // namespace BME280Sensor