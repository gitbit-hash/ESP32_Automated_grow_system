#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <Adafruit_BME280.h>
#include <RTClib.h>
#include <TelnetSpy.h>

TelnetSpy SerialAndTelnet;
#define SERIAL SerialAndTelnet

const char *SSID = "MazeRunner";
const char *WIFIPWD = "Withoutme";
const char *HOSTNAME = "MyFirstConnection";

// Soil moisture sensor pins
const int SOIL_MOISTURE_PIN = 34; // Analog pin for soil sensor
const int SOIL_POWER_PIN = 4;     // Soil sensor power control
const int DRY_VALUE = 2380;       // Sensor in air
const int WET_VALUE = 1960;       // Sensor in moist coco
const int WATER_SUBMERGED = 100;  // Full water immersion

const int WATER_PUMP_PIN = 16; // Existing relay's free channel

RTC_DS3231 rtc;
char days[7][4] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

#define CLOCK_PIN 23
const int RELAY_PIN = 26;
// Alarm times
const int ON_HOUR = 8; // 8 AM
const int ON_MINUTE = 0;
const int OFF_HOUR = 2; // 2 AM next day
const int OFF_MINUTE = 0;

volatile bool alarmTriggered = false;
bool lightState = false;

Adafruit_BME280 bme;

void onAlarm()
{
  alarmTriggered = true;
}
void printTime(DateTime t)
{
  SERIAL.printf("%04d-%02d-%02d %02d:%02d:%02d\n",
                t.year(), t.month(), t.day(),
                t.hour(), t.minute(), t.second());
}

void setAlarms()
{
  // Clear any existing alarms
  rtc.clearAlarm(1);
  rtc.clearAlarm(2);

  // Turn off square wave output
  rtc.writeSqwPinMode(DS3231_OFF);

  // Set alarms for today
  DateTime now = rtc.now();
  // Calculate next ON time (today or tomorrow)
  DateTime nextOn(now.year(), now.month(), now.day(), ON_HOUR, ON_MINUTE, 0);
  if (now.hour() >= ON_HOUR)
  {
    nextOn = nextOn + TimeSpan(1, 0, 0, 0); // Move to tomorrow
  }

  // Calculate next OFF time (always after next ON time)
  DateTime nextOff = nextOn + TimeSpan(0, 18, 0, 0); // 18 hours after ON time

  // Set alarms
  rtc.setAlarm1(nextOn, DS3231_A1_Hour);
  rtc.setAlarm2(nextOff, DS3231_A2_Hour);

  SERIAL.println("Alarms set:");
  SERIAL.print("ON at ");
  printTime(nextOn);
  SERIAL.print("OFF at ");
  printTime(nextOff);
}

void updateLight()
{
  DateTime now = rtc.now();

  // Calculate today's ON time
  DateTime todayOn(now.year(), now.month(), now.day(), ON_HOUR, ON_MINUTE, 0);

  // Calculate when the light should turn OFF (18 hours after today's ON time)
  DateTime todayOff = todayOn + TimeSpan(0, 18, 0, 0);

  // If we're in the period between midnight and today's ON time
  if (now < todayOn)
  {
    // Use yesterday's schedule
    DateTime yesterdayOn = todayOn - TimeSpan(1, 0, 0, 0);
    DateTime yesterdayOff = yesterdayOn + TimeSpan(0, 18, 0, 0);

    if (now >= yesterdayOn && now < yesterdayOff)
    {
      digitalWrite(RELAY_PIN, HIGH);
      lightState = true;
      return;
    }
  }

  // Check if we're in today's ON period
  if (now >= todayOn && now < todayOff)
  {
    digitalWrite(RELAY_PIN, HIGH);
    lightState = true;
  }
  else
  {
    digitalWrite(RELAY_PIN, LOW);
    lightState = false;
  }
}

void setupRTC()
{
  if (!rtc.begin())
    while (1)
      ;
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  if (rtc.lostPower())
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  pinMode(CLOCK_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(CLOCK_PIN), onAlarm, FALLING);
}

void setupBME()
{
  if (!bme.begin(0x76))
    while (1)
      ;
}

void printStats()
{
  SERIAL.println(F("\n--- STATUS ---"));
  SERIAL.print(F("Time: "));
  printTime(rtc.now());
  SERIAL.print(F("Light: "));
  SERIAL.println(lightState ? "ON" : "OFF");
  SERIAL.print(F("WiFi: "));
  SERIAL.println(WiFi.status() == WL_CONNECTED ? "OK" : "FAIL");
  if (WiFi.status() == WL_CONNECTED)
  {
    SERIAL.print(F("IP: "));
    SERIAL.println(WiFi.localIP());
  }
  SERIAL.print(F("Uptime: "));
  SERIAL.print(millis() / 1000);
  SERIAL.println(F("s"));
}

void handleAlarm()
{
  DateTime now = rtc.now();

  if (rtc.alarmFired(1))
  {
    SERIAL.print("\n>>> ALARM 1 TRIGGERED at ");
    printTime(now);
    SERIAL.println(": Turning light ON");

    digitalWrite(RELAY_PIN, HIGH);
    lightState = true;
    rtc.clearAlarm(1);

    // Set next OFF alarm (18 hours from now)
    DateTime nextOff = now + TimeSpan(0, 18, 0, 0);
    rtc.setAlarm2(nextOff, DS3231_A2_Hour);
    SERIAL.print("Next OFF alarm set to: ");
    printTime(nextOff);
  }
  else if (rtc.alarmFired(2))
  {
    SERIAL.print("\n>>> ALARM 2 TRIGGERED at ");
    printTime(now);
    SERIAL.println(": Turning light OFF");

    digitalWrite(RELAY_PIN, LOW);
    lightState = false;
    rtc.clearAlarm(2);

    // Set next ON alarm (next day at 8:00 AM)
    DateTime nextOn(now.year(), now.month(), now.day(), ON_HOUR, ON_MINUTE, 0);
    nextOn = nextOn + TimeSpan(1, 0, 0, 0); // Always tomorrow
    rtc.setAlarm1(nextOn, DS3231_A1_Hour);
    SERIAL.print("Next ON alarm set to: ");
    printTime(nextOn);
  }

  printStats();
}

void connectWiFi()
{
  IPAddress local_IP(192, 168, 1, 10); // Your desired static IP
  IPAddress gateway(192, 168, 1, 1);   // Your router's IP
  IPAddress subnet(255, 255, 255, 0);  // Your network's subnet mask
  
  if (!WiFi.config(local_IP, gateway, subnet))
  {
    Serial.println("STA Failed to configure");
  }

  WiFi.begin(SSID, WIFIPWD);
  for (int i = 0; i < 20 && WiFi.status() != WL_CONNECTED; i++)
  {
    delay(500);
    SERIAL.print(".");
  }
  if (WiFi.status() == WL_CONNECTED)
  {
    SERIAL.println(F("\nWiFi OK"));
    SERIAL.print(F("IP: "));
    SERIAL.println(WiFi.localIP());
  }
}

void setupOTA()
{
  ArduinoOTA.setHostname(HOSTNAME);
  ArduinoOTA.setPort(8266);
  ArduinoOTA.begin();
}

int readSoilMoisture()
{
  digitalWrite(SOIL_POWER_PIN, HIGH);
  delay(200); // Stabilization time
  int raw = analogRead(SOIL_MOISTURE_PIN);
  digitalWrite(SOIL_POWER_PIN, LOW);

  // Invert mapping: higher ADC = drier soil
  int moisture = map(raw, DRY_VALUE, WET_VALUE, 0, 100);

  // Constrain to 0-100% range
  return constrain(moisture, 0, 100);
}

void setupWaterPump()
{
  pinMode(WATER_PUMP_PIN, OUTPUT);
  digitalWrite(WATER_PUMP_PIN, HIGH); // Relays OFF initially
}

void setupSoilMoistureSensor()
{
  pinMode(SOIL_POWER_PIN, OUTPUT);
  digitalWrite(SOIL_POWER_PIN, LOW); // Sensor OFF initially
}

void setup()
{
  SERIAL.begin(115200);
  setupRTC();
  setAlarms();
  updateLight();
  setupBME();
  connectWiFi();
  setupOTA();
  setupSoilMoistureSensor();
  setupWaterPump();
}

void loop()
{
  ArduinoOTA.handle();
  SerialAndTelnet.handle();

  if (alarmTriggered)
  {
    alarmTriggered = false;
    handleAlarm();
  }

  static uint32_t lastCheck;
  if (millis() - lastCheck > 30000)
  {
    lastCheck = millis();
    updateLight();
    printStats();
  }

  float h = bme.readHumidity();
  float t = bme.readTemperature();
  DateTime now = rtc.now();

  SERIAL.print(F("H:"));
  SERIAL.print(h);
  SERIAL.print(F("% T:"));
  SERIAL.print(t);
  SERIAL.print(F("C D:"));
  SERIAL.print(days[now.dayOfTheWeek()]);
  SERIAL.print(F(" "));
  printTime(now);

  int moisture = readSoilMoisture();
  SERIAL.println(moisture);

  delay(1000);
}