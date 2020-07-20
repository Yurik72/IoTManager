#include "Cmd.h"

#include "Collection/Buttons.h"
#include "Collection/Devices.h"
#include "Collection/Logger.h"
#include "Collection/Sensors.h"
#include "Collection/Servos.h"
#include "Collection/Switches.h"
#include "Collection/Timers.h"
#include "Collection/Widgets.h"

#include "Sensors/AnalogSensor.h"
#include "Sensors/OneWireBus.h"

#include "Objects/Pwm.h"

#include "Objects/Terminal.h"
#include "Objects/Telnet.h"

#include "PrintMessage.h"
#include "Runtime.h"
#include "StringConsts.h"
#include "MqttClient.h"
#include "WebClient.h"
#include "Scenario.h"

static const char *MODULE = "Cmd";

StringQueue _orders;

StringCommand sCmd;

Terminal *term = nullptr;
Telnet *telnet = nullptr;

#ifdef ESP8266
SoftwareSerial *mySerial = nullptr;
#else
HardwareSerial *mySerial = nullptr;
#endif

unsigned long parsePeriod(const String &str, unsigned long default_time_syfix) {
    unsigned long res = 0;
    if (str.indexOf("digit") != -1) {
        res = runtime.readInt(str);
    } else {
        if (str.endsWith("ms")) {
            res = str.substring(0, str.indexOf("ms")).toInt();
        } else if (str.endsWith("s")) {
            res = str.substring(0, str.indexOf("s")).toInt() * ONE_SECOND_ms;
        } else if (str.endsWith("m")) {
            res = str.substring(0, str.indexOf("m")).toInt() * ONE_MINUTE_ms;
        } else if (str.endsWith("h")) {
            res = str.substring(0, str.indexOf("h")).toInt() * ONE_HOUR_ms;
        } else {
            res = str.toInt() * default_time_syfix;
        }
    }
    return res;
}

const String getObjectName(const char *type, const char *id) {
    return String(type) + id;
}

void cmd_init() {
    switches.setOnChangeState([](Switch *obj) {
        String name = String("switch") + obj->getName();
        runtime.writeAsInt(name, obj->getValue());
    });

    sCmd.addCommand("button", cmd_button);
    sCmd.addCommand("buttonSet", cmd_buttonSet);
    sCmd.addCommand("buttonChange", cmd_buttonChange);

    sCmd.addCommand("switch", cmd_switch);

    sCmd.addCommand("pinSet", cmd_pinSet);
    sCmd.addCommand("pinChange", cmd_pinChange);

    sCmd.addCommand("pwm", cmd_pwm);
    sCmd.addCommand("pwmSet", cmd_pwmSet);

    sCmd.addCommand("analog", cmd_analog);

    sCmd.addCommand("levelPr", cmd_levelPr);
    sCmd.addCommand("ultrasonicCm", cmd_ultrasonicCm);
    sCmd.addCommand("dallas", cmd_dallas);
    sCmd.addCommand("dhtT", cmd_dhtT);
    sCmd.addCommand("dhtH", cmd_dhtH);
    sCmd.addCommand("dhtPerception", cmd_dhtPerception);
    sCmd.addCommand("dhtComfort", cmd_dhtComfort);
    sCmd.addCommand("dhtDewpoint", cmd_dhtDewpoint);

    sCmd.addCommand("bmp280T", cmd_bmp280T);
    sCmd.addCommand("bmp280P", cmd_bmp280P);

    sCmd.addCommand("bme280T", cmd_bme280T);
    sCmd.addCommand("bme280P", cmd_bme280P);
    sCmd.addCommand("bme280H", cmd_bme280H);
    sCmd.addCommand("bme280A", cmd_bme280A);

    sCmd.addCommand("stepper", cmd_stepper);
    sCmd.addCommand("stepperSet", cmd_stepperSet);

    sCmd.addCommand("servo", cmd_servo);
    sCmd.addCommand("servoSet", cmd_servoSet);

    sCmd.addCommand("serialBegin", cmd_serialBegin);
    sCmd.addCommand("serialEnd", cmd_serialEnd);
    sCmd.addCommand("serialWrite", cmd_serialWrite);

    sCmd.addCommand("serialLog", cmd_serialLog);

    sCmd.addCommand("telnet", cmd_telnet);

    sCmd.addCommand("logging", cmd_logging);

    sCmd.addCommand("inputDigit", cmd_inputDigit);
    sCmd.addCommand("digitSet", cmd_digitSet);

    sCmd.addCommand("inputTime", cmd_inputTime);
    sCmd.addCommand("timeSet", cmd_timeSet);

    sCmd.addCommand("timerStart", cmd_timerStart);
    sCmd.addCommand("timerStop", cmd_timerStop);

    sCmd.addCommand("text", cmd_text);
    sCmd.addCommand("textSet", cmd_textSet);

    sCmd.addCommand("mqtt", cmd_mqtt);
    sCmd.addCommand("http", cmd_http);

    sCmd.addCommand("push", cmd_push);

    sCmd.addCommand("firmwareUpdate", cmd_firmwareUpdate);
    sCmd.addCommand("firmwareVersion", cmd_firmwareVersion);

    sCmd.addCommand("get", cmd_get);

    sCmd.addCommand("reboot", cmd_reboot);

    sCmd.addCommand("oneWire", cmd_oneWire);
}

void cmd_inputDigit() {
    String name = sCmd.next();
    String number = name.substring(5);
    String descr = sCmd.next();
    descr.replace("#", " ");
    String page_name = sCmd.next();
    page_name.replace("#", " ");
    String value = sCmd.next();
    String order = sCmd.next();

    String objName = "digit" + number;

    runtime.writeAsInt(objName, value);

    Widgets::createWidget(descr, page_name, order, "inputNum", objName);
}

void cmd_digitSet() {
    String name = sCmd.next();
    String value = sCmd.next();

    String objName = "digit" + name;

    runtime.writeAsInt(objName, value);
}

void cmd_inputTime() {
    String name = sCmd.next();
    String number = name.substring(4);
    String widget_name = sCmd.next();
    widget_name.replace("#", " ");
    String page_name = sCmd.next();
    page_name.replace("#", " ");
    String state = sCmd.next();
    String order = sCmd.next();

    String objName = "time" + name;

    runtime.write(objName, state);

    Widgets::createWidget(widget_name, page_name, order, "inputTime", "time" + number);
}

void cmd_timeSet() {
    String name = sCmd.next();
    String value = sCmd.next();

    String objName = "time" + name;
    runtime.write(objName, value);
}

void cmd_serialBegin() {
    uint32_t baud = atoi(sCmd.next());
    int8_t rx = atoi(sCmd.next());
    int8_t tx = atoi(sCmd.next());

    cmd_serialEnd();
#ifdef ESP8266
    mySerial = new SoftwareSerial(rx, tx);
    mySerial->begin(baud);
#else
    mySerial = new HardwareSerial(2);
    mySerial->begin(rx, tx);
#endif
    term = new Terminal(mySerial);
    term->setOnReadLine([](const char *str) {
        addCommand(str);
    });
}

void cmd_serialEnd() {
    if (mySerial) {
        mySerial->end();
        delete mySerial;
    }
}

void cmd_serialWrite() {
    String payload = sCmd.next();
    if (term) {
        term->println(payload.c_str());
    }
}

void cmd_telnet() {
    bool enabled = atoi(sCmd.next());
    uint16_t port = atoi(sCmd.next());

    if (enabled) {
        pm.info("telnet: enabled");
        if (!telnet) {
            telnet = new Telnet(port);
        }
        telnet->setOutput(pm.getOutput());
        telnet->start();
    } else {
        pm.info("telnet: disabled");
        telnet->stop();
        telnet->end();
    }
}

void cmd_get() {
    String obj = sCmd.next();
    String param = sCmd.next();
    String res = "";

    if (!obj.isEmpty()) {
        if (obj.equalsIgnoreCase("state")) {
            if (param.isEmpty()) {
                res = runtime.asJson();
            } else {
                res = runtime.read(param);
            }
        } else if (obj.equalsIgnoreCase("devices")) {
            Devices::get(res, param.toInt());
        } else {
            res = F("unknown param");
        }
    } else {
        res = F("unknown obj");
    }
    pm.info(res);
    if (term) {
        term->println(res.c_str());
    }
}

void cmd_mqtt() {
    String topic = sCmd.next();
    String data = sCmd.next();

    MqttClient::publishOrder(topic, data);
}

// ultrasonicCm cm 14 12 Дистанция,#см Датчики fillgauge 1
void cmd_ultrasonicCm() {
    String measure_unit = sCmd.next();

    String trig = sCmd.next();
    String echo = sCmd.next();
    String widget = sCmd.next();
    String page = sCmd.next();
    String type = sCmd.next();
    String empty_level = sCmd.next();
    String full_level = sCmd.next();
    String order = sCmd.next();

    Ultrasonic::ultrasonicCm_value_name = measure_unit;

    // options.write("trig", trig);
    // options.write("echo", echo);
    // pinMode(trig.toInt(), OUTPUT);
    // pinMode(echo.toInt(), INPUT);

    Widgets::createWidget(widget, page, order, type, measure_unit);
}

void cmd_oneWire() {
    String assign = sCmd.next();
    onewire.attach(assign.toInt());
}

// dallas temp1 0x14 Температура Датчики anydata 1
// dallas temp2 0x15 Температура Датчики anydata 2
void cmd_dallas() {
    if (!onewire.attached()) {
        pm.error("attach bus first");
        return;
    }

    String name = sCmd.next();
    String address = sCmd.next();
    String descr = sCmd.next();
    String page = sCmd.next();
    String widget = sCmd.next();
    String order = sCmd.next();

    Sensors::add(SENSOR_DALLAS, name, address);

    Widgets::createWidget(descr, page, order, widget, name);
}

//levelPr p 14 12 Вода#в#баке,#% Датчики fillgauge 125 20 1
void cmd_levelPr() {
    String name = sCmd.next();
    String trig = sCmd.next();
    String echo = sCmd.next();
    String widget_name = sCmd.next();
    String page_name = sCmd.next();
    String type = sCmd.next();
    String empty_level = sCmd.next();
    String full_level = sCmd.next();
    String order = sCmd.next();

    Ultrasonic::levelPr_value_name = name;

    // options.write("e_lev", empty_level);
    // options.write("f_lev", full_level);
    // options.write("trig", trig);
    // options.write("echo", echo);
    // pinMode(trig.toInt(), OUTPUT);
    // pinMode(echo.toInt(), INPUT);
    Widgets::createWidget(widget_name, page_name, order, type, name);
}

//dhtH h 2 dht11 Влажность#DHT,#t°C Датчики any-data 1
void cmd_dhtH() {
    String name = sCmd.next();
    String pin = sCmd.next();
    String sensor_type = sCmd.next();
    String widget_name = sCmd.next();
    String page_name = sCmd.next();
    String type = sCmd.next();
    String order = sCmd.next();
    DHTSensor::dhtH_value_name = name;
    if (sensor_type == "dht11") {
        DHTSensor::dht.setup(pin.toInt(), DHTesp::DHT11);
    }
    if (sensor_type == "dht22") {
        DHTSensor::dht.setup(pin.toInt(), DHTesp::DHT22);
    }
    Widgets::createWidget(widget_name, page_name, order, type, name);
}

// dhtT t 2 dht11 Температура#DHT,#t°C Датчики any-data 1
void cmd_dhtT() {
    String name = sCmd.next();
    String pin = sCmd.next();
    String sensor_type = sCmd.next();
    String widget_name = sCmd.next();
    String page_name = sCmd.next();
    String type = sCmd.next();
    String order = sCmd.next();
    DHTSensor::dhtT_value_name = name;
    if (sensor_type == "dht11") {
        DHTSensor::dht.setup(pin.toInt(), DHTesp::DHT11);
    }
    if (sensor_type == "dht22") {
        DHTSensor::dht.setup(pin.toInt(), DHTesp::DHT22);
    }

    Widgets::createWidget(widget_name, page_name, order, type, name);
}

//dhtDewpoint Точка#росы: Датчики 5
void cmd_dhtDewpoint() {
    String widget_name = sCmd.next();
    String page_name = sCmd.next();
    String order = sCmd.next();

    Widgets::createWidget(widget_name, page_name, order, "anydata", "dhtDewpoint");
}

// dhtPerception Восприятие: Датчики 4
void cmd_dhtPerception() {
    String widget_name = sCmd.next();
    String page_name = sCmd.next();
    String order = sCmd.next();

    Widgets::createWidget(widget_name, page_name, order, "any-data", "dhtPerception");
}

// dhtComfort Степень#комфорта: Датчики 3
void cmd_dhtComfort() {
    String widget_name = sCmd.next();
    String page_name = sCmd.next();
    String order = sCmd.next();

    Widgets::createWidget(widget_name, page_name, order, "anydata", "dhtComfort");
}

// bmp280T temp1 0x76 Температура#bmp280 Датчики any-data 1
void cmd_bmp280T() {
    String name = sCmd.next();
    String address = sCmd.next();
    String widget_name = sCmd.next();
    String page_name = sCmd.next();
    String type = sCmd.next();
    String order = sCmd.next();
    BMP280Sensor::bmp280T_value_name = name;

    BMP280Sensor::bmp.begin(hexStringToUint8(address));
    BMP280Sensor::bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */

    Widgets::createWidget(widget_name, page_name, order, type, name);
}

//bmp280P press1 0x76 Давление#bmp280 Датчики any-data 2
void cmd_bmp280P() {
    String name = sCmd.next();
    String address = sCmd.next();
    String widget_name = sCmd.next();
    String page_name = sCmd.next();
    String type = sCmd.next();
    String order = sCmd.next();
    BMP280Sensor::bmp280P_value_name = name;

    BMP280Sensor::bmp.begin(hexStringToUint8(address));
    BMP280Sensor::bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */

    Widgets::createWidget(widget_name, page_name, order, type, name);
}

//=========================================================================================================================================
//=============================================Модуль сенсоров bme280======================================================================
//bme280T temp1 0x76 Температура#bmp280 Датчики any-data 1
void cmd_bme280T() {
    String name = sCmd.next();
    String address = sCmd.next();
    String descr = sCmd.next();
    String page = sCmd.next();
    String type = sCmd.next();
    String order = sCmd.next();

    Widgets::createWidget(descr, page, order, type, name);
}

//bme280P pres1 0x76 Давление#bmp280 Датчики any-data 1
void cmd_bme280P() {
    String name = sCmd.next();
    String address = sCmd.next();
    String widget_name = sCmd.next();
    String page_name = sCmd.next();
    String type = sCmd.next();
    String order = sCmd.next();

    Widgets::createWidget(widget_name, page_name, order, type, name);
}

//bme280H hum1 0x76 Влажность#bmp280 Датчики any-data 1
void cmd_bme280H() {
    String name = sCmd.next();
    String address = sCmd.next();
    String widget_name = sCmd.next();
    String page_name = sCmd.next();
    String type = sCmd.next();
    String order = sCmd.next();

    Widgets::createWidget(widget_name, page_name, order, type, name);
}

//bme280A altit1 0x76 Высота#bmp280 Датчики any-data 1
void cmd_bme280A() {
    String name = sCmd.next();
    String address = sCmd.next();
    String descr = sCmd.next();
    String page = sCmd.next();
    String templateMame = sCmd.next();
    String order = sCmd.next();

    Widgets::createWidget(descr, page, order, templateMame, name);
}

void cmd_firmwareUpdate() {
    perform_upgrade();
}

void cmd_firmwareVersion() {
    String widget = sCmd.next();
    String page = sCmd.next();
    String order = sCmd.next();

    Widgets::createWidget(widget, page, order, "anydata", TAG_FIRMWARE);
}

bool extractCommand(const String &buf, size_t &startIndex, String &block) {
    int endIndex = buf.indexOf("\n", startIndex);
    if (endIndex < 0) {
        return false;
    }
    block = buf.substring(startIndex, endIndex);
    block.replace("\r\n", "");
    block.replace("\r", "");
    if (block.startsWith("//")) {
        block = "";
    }
    startIndex = endIndex + 1;
    return true;
}

void addCommands(const String &str) {
    size_t pos = 0;
    while (pos < str.length()) {
        String buf;
        if (!extractCommand(str, pos, buf)) {
            break;
        }
        if (!buf.isEmpty()) {
            addCommand(buf);
        }
    }
}

void addCommand(const String &str) {
    _orders.push(str);
}

void executeCommand(const String &str) {
    pm.info("Execute: " + str);
    sCmd.readStr(str);
}

void loop_cmd() {
    if (_orders.available()) {
        String cmd;
        _orders.pop(cmd);
        executeCommand(cmd);
    }
}

void loop_items() {
    if (term) {
        term->loop();
    }
    if (telnet) {
        telnet->loop();
    }

    switches.loop();
}
