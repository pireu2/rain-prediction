#include <Wire.h>
#include <Math.h>

#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_TSL2561_U.h>
#include <Adafruit_RGBLCDShield.h>

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#define SEA_LEVEL_RESSURE (1013.25)
#define B_DEWPOINT (17.62)
#define C_DEWPOINT (243.12)

const char *ssid = "TP-Link_0C10_5G";
const char *password = "48845946";
const char *server = "http://192.168.1.213:5000/predict";

Adafruit_BME280 bme;
Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

unsigned long delayTime = 1000;

struct SensorData
{
    float temperature;
    float pressure;
    float humidity;
    float luminosity;
    float dewpoint;
};

unsigned int predictionTypeIndex = 0;
const unsigned int PREDICTION_TYPES = 4;
const unsigned int predictionTypes[] = {1, 6, 12, 24};

SensorData sensorData = {0, 0, 0, 0, 0};

void configureTSL()
{
    tsl.enableAutoRange(true);
    tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_101MS);
}

bool validateData()
{
    if (sensorData.temperature < -20 || sensorData.temperature > 50)
    {
        return false;
    }
    if (sensorData.pressure < 800 || sensorData.pressure > 1100)
    {
        return false;
    }
    if (sensorData.humidity < 0 || sensorData.humidity > 100)
    {
        return false;
    }
    if (sensorData.luminosity < 0)
    {
        return false;
    }
    if (sensorData.dewpoint < 0 || sensorData.dewpoint > 50)
    {
        return false;
    }

    return true;
}

void getSensorData()
{
    sensorData.temperature = bme.readTemperature();
    sensorData.pressure = bme.readPressure() / 100.0F;
    sensorData.humidity = bme.readHumidity();

    float gamma = (B_DEWPOINT * sensorData.temperature / (C_DEWPOINT + sensorData.temperature)) + log(sensorData.humidity / 100.0);
    sensorData.dewpoint = (C_DEWPOINT * gamma) / (B_DEWPOINT - gamma);

    sensors_event_t event;
    tsl.getEvent(&event);
    if (event.light)
    {
        sensorData.luminosity = event.light;
    }
    else
    {
        sensorData.luminosity = -1;
    }

    if (validateData())
    {
        lcd.clear();
        lcd.print("Data OK");
    }
    else
    {
        lcd.clear();
        lcd.print("Data Error");
    }
}

const char *getPredictionType()
{
    switch (predictionTypeIndex)
    {
    case 0:
        return "ONE_HOUR";
    case 1:
        return "SIX_HOURS";
    case 2:
        return "TWELVE_HOURS";
    case 3:
        return "TWENTY_FOUR_HOURS";
    default:
        return "ONE_HOUR";
    }
}

void sendPredictionRequest()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        HTTPClient http;
        http.begin(server);
        http.addHeader("Content-Type", "application/json");

        StaticJsonDocument<200> jsonDoc;
        jsonDoc["temperature"] = sensorData.temperature;
        jsonDoc["humidity"] = sensorData.humidity;
        jsonDoc["pressure"] = sensorData.pressure;
        jsonDoc["luminosity"] = sensorData.luminosity;
        jsonDoc["dewpoint"] = sensorData.dewpoint;
        jsonDoc["prediction_type"] = getPredictionType();

        String requestBody;
        serializeJson(jsonDoc, requestBody);

        int httpResponseCode = http.POST(requestBody);

        if (httpResponseCode > 0)
        {
            String response = http.getString();
            lcd.print("Response: ");
            lcd.setCursor(0, 1);
            lcd.print(response);
        }
        else
        {
            lcd.print("Error: ");
            lcd.setCursor(0, 1);
            lcd.print(httpResponseCode);
        }

        http.end();
    }
    else
    {
        Serial.println("WiFi Disconnected");
    }
}

void setup()
{
    Serial.begin(9600);
    delay(delayTime);

    lcd.begin(16, 2);
    lcd.print("LCD test");

    bool status;
    status = bme.begin(0x76);
    if (!status)
    {
        Serial.println("Could not find BME280 sensor!");
        while (1)
            ;
    }
    Serial.println("BME280 sensor found!");
    if (!tsl.begin())
    {
        Serial.println("Could not find TSL2561 sensor!");
        while (1)
            ;
    }
    Serial.println("TSL2561 sensor found!");
    configureTSL();

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi: ");
    Serial.println(ssid);
    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) // 10 seconds timeout
    {
        delay(delayTime);
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("Connected to the WiFi network");
    }
    else
    {
        Serial.println("Failed to connect to WiFi");
        Serial.print("SSID: ");
        Serial.println(ssid);
        Serial.print("Password: ");
        Serial.println(password);
    }
}

void loop()
{

    uint8_t buttons = lcd.readButtons();
    if (buttons)
    {
        lcd.clear();
        if (buttons & BUTTON_UP)
        {
            getSensorData();
        }
        if (buttons & BUTTON_DOWN)
        {
            sendPredictionRequest();
        }
        if (buttons & BUTTON_SELECT)
        {
            lcd.print("Temperature: ");
            lcd.setCursor(0, 1);
            lcd.print(sensorData.temperature);
            delay(delayTime);
            lcd.clear();
            lcd.print("Pressure: ");
            lcd.setCursor(0, 1);
            lcd.print(sensorData.pressure);
            delay(delayTime);
            lcd.clear();
            lcd.print("Humidity: ");
            lcd.setCursor(0, 1);
            lcd.print(sensorData.humidity);
            delay(delayTime);
            lcd.clear();
            lcd.print("Dewpoint: ");
            lcd.setCursor(0, 1);
            lcd.print(sensorData.dewpoint);
            delay(delayTime);
            lcd.clear();
            lcd.print("Luminosity: ");
            lcd.setCursor(0, 1);
            lcd.print(sensorData.luminosity);
            delay(delayTime);
            lcd.clear();
        }
        if (buttons & BUTTON_LEFT)
        {
            predictionTypeIndex = (predictionTypeIndex - 1) % PREDICTION_TYPES;
            lcd.print("Prediction Type: ");
            lcd.setCursor(0, 1);
            lcd.print(predictionTypes[predictionTypeIndex]);
            delay(delayTime);
        }
        if (buttons & BUTTON_RIGHT)
        {
            predictionTypeIndex = (predictionTypeIndex + 1) % PREDICTION_TYPES;
            lcd.print("Prediction Type: ");
            lcd.setCursor(0, 1);
            lcd.print(predictionTypes[predictionTypeIndex]);
            delay(delayTime);
        }
    }
}
