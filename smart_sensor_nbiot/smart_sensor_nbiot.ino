/* Project : 
 * Send  moisture, temperature, EC, PH, Nitrogen, Phosphorus, Potassium data from 7 in 1 Sensor to NB-IOT
 * Date : 20/11/2021
*/

#include <ModbusMaster.h>
#include <HardwareSerial.h>
#include "HardwareSerial_NB_BC95.h"
#include <ArduinoOTA.h>
#include <WiFiManager.h>
#include "REG_CONFIG.h"
WiFiManager wifiManager;

// OTA
#define HOSTNAME "SmartSensorNB-IOT02"
#define PASSWORD "12345678"

HardwareSerial modbus(2);
HardwareSerial_NB_BC95 AISnb;

ModbusMaster node;
// thingcontrol.io setup
String deviceToken = "4grx6H0zcurf7AAV2RIW";
String serverIP = "147.50.151.130";
String serverPort = "9956";

struct Seveninone
{
  String moisture;
  String temperature;
  String EC;
  String PH;
  String Nit;
  String Pho;
  String Pot;
};

Seveninone sensor ;

unsigned long currentMillis;
unsigned long previousMillis;
const unsigned long interval = 60000; // Interval Time
//unsigned int previous_check = 0;

signal meta;

void setup() {
  Serial.begin(115200);
  modbus.begin(4800);
  AISnb.debug = true;
  AISnb.setupDevice(serverPort);
  String ip1 = AISnb.getDeviceIP();
  pingRESP pingR = AISnb.pingIP(serverIP);
  // OTA
  wifiManager.setTimeout(180);
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setAPClientCheck(true);
  String wifiName = "@ESP32-";
  wifiName.concat(String((uint32_t)ESP.getEfuseMac(), HEX));
  if (!wifiManager.autoConnect(wifiName.c_str())) {
    //Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    //    ESP.reset();
    //delay(1000);
    ESP.restart();
    delay(1);
  }
  setupWIFI();
  setupOTA();
}

void loop() {
    ArduinoOTA.handle();
  currentMillis = millis();
  if (currentMillis - previousMillis >= interval)
  {
    readSensor();
    sendViaNBIOT();
    previousMillis = currentMillis;
  }
}

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void sendViaNBIOT() {
  meta = AISnb.getSignal();
  String json = "";
  json.concat("{\"Tn\":\"");
  json.concat(deviceToken);
  json.concat("\",\"moisture\":");
  json.concat(sensor.moisture);
  json.concat(",\"temperature\":");
  json.concat(sensor.temperature);
  json.concat(",\"EC\":");
  json.concat(sensor.EC);
  json.concat(",\"PH\":");
  json.concat(sensor.PH);
  json.concat(",\"N\":");
  json.concat(sensor.Nit);
  json.concat(",\"P\":");
  json.concat(sensor.Pho);
  json.concat(",\"K\":");
  json.concat(sensor.Pot);
  json.concat("}");
  Serial.println(json);
  UDPSend udp = AISnb.sendUDPmsgStr(serverIP, serverPort, json);
  UDPReceive resp = AISnb.waitResponse();
}

long readModbus(char addr, uint16_t  REG)
{
  uint8_t j, result;
  uint16_t data;
  // communicate with Modbus slave ID 1 over Hardware Serial (port 1)
  node.begin(addr, modbus);
  result = node.readHoldingRegisters(REG, 1);
  // do something with data if read is successful
  if (result == node.ku8MBSuccess)
  {
    data = node.getResponseBuffer(0);
    //Serial.println("Connec modbus Ok.");
    return data;
  } else
  {
    Serial.print("Connec modbus ID: ");
    Serial.print(addr);
    Serial.print(" Sensor fail. REG >>> ");
    Serial.println(REG); // Debug
    delay(100);
    return 0;
  }
}

void readSensor()
{
  sensor.moisture = readModbus(ID_SENSOR, Address[0]);
  sensor.temperature = readModbus(ID_SENSOR, Address[1]);
  sensor.EC = readModbus(ID_SENSOR, Address[2]);
  sensor.PH = readModbus(ID_SENSOR, Address[3]);
  sensor.Nit = readModbus(ID_SENSOR, Address[4]);
  sensor.Pho = readModbus(ID_SENSOR, Address[5]);
  sensor.Pot = readModbus(ID_SENSOR, Address[6]);

  Serial.print("Moisture : ");  Serial.println(sensor.moisture);
  Serial.print("Temperature : ");  Serial.println(sensor.temperature);
  Serial.print("EC : ");  Serial.println(sensor.EC);
  Serial.print("PH : ");  Serial.println(sensor.PH);
  Serial.print("Nitrogen : ");  Serial.println(sensor.Nit);
  Serial.print("Phosphorus : ");  Serial.println(sensor.Pho);
  Serial.print("Potassium : ");  Serial.println(sensor.Pot);
  Serial.println("");
  delay(2000);
}

// OTA
void setupOTA()
{
  //Port defaults to 8266
  //ArduinoOTA.setPort(8266);
  //Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(HOSTNAME);

  //No authentication by default
  ArduinoOTA.setPassword(PASSWORD);

  //Password can be set with it's md5 value as well
  //MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  //ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]()
  {
    Serial.println("Start Updating....");
    Serial.printf("Start Updating....Type:%s\n", (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem");
  });
  ArduinoOTA.onEnd([]()
  {
    Serial.println("Update Complete!");
    ESP.restart();
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
  {
    String pro = String(progress / (total / 100)) + "%";
    int progressbar = (progress / (total / 100));
    //int progressbar = (progress / 5) % 100;
    //int pro = progress / (total / 100);
    //    drawUpdate(progressbar, 265, 195);
    //    tft.drawString(title5, 310, 235, GFXFF); // Print the test text in the custom font
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error)
  {
    Serial.printf("Error[%u]: ", error);
    String info = "Error Info:";
    switch (error)
    {
      case OTA_AUTH_ERROR:
        info += "Auth Failed";
        Serial.println("Auth Failed");
        break;

      case OTA_BEGIN_ERROR:
        info += "Begin Failed";
        Serial.println("Begin Failed");
        break;

      case OTA_CONNECT_ERROR:
        info += "Connect Failed";
        Serial.println("Connect Failed");
        break;

      case OTA_RECEIVE_ERROR:
        info += "Receive Failed";
        Serial.println("Receive Failed");
        break;

      case OTA_END_ERROR:
        info += "End Failed";
        Serial.println("End Failed");
        break;
    }
    Serial.println(info);
    ESP.restart();
  });
  ArduinoOTA.begin();
}

void setupWIFI()
{
  WiFi.setHostname(HOSTNAME);
  byte count = 0;
  while (WiFi.status() != WL_CONNECTED && count < 10)
  {
    count ++;
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED)
    Serial.println("Connecting...OK.");
  else
    Serial.println("Connecting...Failed");
}
