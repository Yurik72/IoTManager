#include "Base/ParamFile.h"

#include <Arduino.h>

#include "FS.h"

#ifdef ESP32
#include "LITTLEFS.h"
#define LittleFS LITTLEFS
#endif
#ifdef ESP8266
#include <LittleFS.h>
#endif

#define FILE_READ "r"
#define FILE_WRITE "w"
#define FILE_APPEND "a"

ParamFile::ParamFile(const String& name) : ParamStore() {
    _name = name;
}

ParamFile::~ParamFile() {}

const String ParamFile::getFilename() const {
    return _name;
}

bool ParamFile::save() {
    bool res = false;
    auto file = LittleFS.open(getFilename(), FILE_WRITE);
    if (file) {
        DynamicJsonBuffer buf;
        JsonObject& obj = buf.createObject();
        KeyValueStore::save(obj);
        obj.printTo(file);
        res = true;
    }
    file.close();
    return res;
}

bool ParamFile::load() {
    bool res = false;
    auto file = LittleFS.open(getFilename(), FILE_READ);
    if (file) {
        if (file.available()) {
            DynamicJsonBuffer buf;
            JsonObject& obj = buf.parse(file);
            KeyValueStore::load(obj);
            res = true;
        }
        file.close();
    };
    return res;
}