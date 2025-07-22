#include "Arduino.h"
#include <readAmpere.h>
#include <SPI.h>
#include <BluetoothSerial.h>
#include "bluetoothtask.h"
#include <HardwareSerial.h>
#include <driver/uart.h>
#include <WebServer.h>
#include <EEPROM.h>
#include "bmsModbus.h"
#include "main.h"
#include <esp_task_wdt.h>
#include "esp32SelfUploder.h"
#include "../../../version.h"
#include <esp_adc_cal.h>

#define EVERY_100 100
#define EVERY_200 200
#define EVERY_250 250
#define EVERY_500 500
#define EVERY_1000 1000
#define EVERY_2000 2000

ESP32SelfUploder selfUploder;

TaskHandle_t *h_pxblueToothTask;
ReadAmpereClass _ReadAmpereClass;

// ThreeWire max11163wire(DATAIN_MISO, SPICLOCK, SEL_MAX11163 );

RelayControl relayControl;
extern WebServer webServer;
BmsModbus bmsModbus;
nvmSystemSet nvmSet;
long elaspedTime100 = 0;
long elaspedTime250 = 0;
long elaspedTime500 = 0;
long elaspedTime1000 = 0;
long elaspedTime2000 = 0;
void readAndWriteEprom(){
  EEPROM.readBytes(0, (byte *)&nvmSet, sizeof(nvmSystemSet));
  if (nvmSet.EpromValidStart != 0x55 && nvmSet.EpromValidEnd != 0xAA)
  {
     Serial.println("Initializing default values...");
      memset(&nvmSet, 0, sizeof(nvmSystemSet));
      nvmSet.EpromValidStart = 0x55;
      nvmSet.InstalledCells = 16;
      nvmSet.ModbusAddress = 1;
      nvmSet.WEBSERVERPORT = 80;
      nvmSet.Max1161_RefVolt = 4096;
      nvmSet.Max1161_CellOffset = 0;
      nvmSet.Max1161_CellGain = 1220;
      nvmSet.BAUDRATE = 9600;
      nvmSet.IPADDRESS = (uint32_t)IPAddress(192, 168, 1, 100);
      nvmSet.GATEWAY = (uint32_t)IPAddress(192, 168, 1, 1);
      nvmSet.SUBNETMASK = (uint32_t)IPAddress(255, 255, 255, 0);
      nvmSet.UseHoleCTRatio = 100;
      nvmSet.TempOffset = 4600;
      nvmSet.AmpereOffset = 0; 
      nvmSet.AmpereGain = 1220;
      nvmSet.TotalVoltageOffset = 0;
      nvmSet.TotalVoltageGain = 1220;
      nvmSet.CutOffChargeAmpere = 500; // 50A
      nvmSet.CutOffDischargeAmpere = 1000; // 100A
      nvmSet.ResumeGapAmpere = 100; // 10A
      nvmSet.RelayDelayTime = 1;  //second
      nvmSet.HoleCTdirection = 0;
      nvmSet.EpromValidEnd = 0xAA;
      EEPROM.writeBytes(0, (byte *)&nvmSet, sizeof(nvmSystemSet));
      EEPROM.commit();
  }
}
void upLoder(){
    selfUploder.begin("AndroidHotspot1953", "87654321", "https://raw.githubusercontent.com/kimjinhwa/IP-Fineder-For-ESP32/main/dist/poscoYardPCM");

    Serial.println("Booting Sketch...");
    WiFi.mode(WIFI_AP_STA);
    if(strlen(nvmSet.SSID) == 0){
        strncpy(nvmSet.SSID, selfUploder.ssid, sizeof(nvmSet.SSID));
    }
    if(strlen(nvmSet.PASS) == 0){
        strncpy(nvmSet.PASS, selfUploder.password, sizeof(nvmSet.PASS));
    }
    EEPROM.writeBytes(0, (byte *)&nvmSet, sizeof(nvmSystemSet));
    WiFi.begin(nvmSet.SSID, nvmSet.PASS);

    int retry = 0;
    
    while (WiFi.waitForConnectResult() != WL_CONNECTED  && retry  < (5 + nvmSet.ModbusAddress%5) ) {
        esp_task_wdt_reset();
        WiFi.begin(selfUploder.ssid, selfUploder.password);
        delay(100);  // 5초 대기
        Serial.println("WiFi failed, retrying.");
        retry++;
    }
    if(WiFi.isConnected()){
        Serial.println("WiFi connected");
        Serial.println(WiFi.localIP());
        Serial.println(WiFi.subnetMask());
        Serial.println(WiFi.gatewayIP());
        Serial.println(WiFi.dnsIP());
        #if AUTOUPDATE == 1
        Serial.printf("Free heap before SSL: %d\n", ESP.getFreeHeap());
        if (selfUploder.checkNewVersion(selfUploder.update_url)) {
            Serial.println("New version available!");
            if (selfUploder.tryAutoUpdate(selfUploder.updateFile_url.c_str())) {
                return;
            }
        } else {
            Serial.println("Already on latest version");
        }
        #endif
    }
    else{
        Serial.println("WiFi connection failed");
    }   
    // selfUploder.httpUpdater.setup(&selfUploder.httpServer);
    // selfUploder.httpServer.begin();
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(100);
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(100);
    Serial.println("\nWiFi Offed");
}
uint16_t timerCount = 0;
void IRAM_ATTR onTimer(){
    relayControl.userRelayOnTime++ ;
    timerCount++;
}
void useInterrupt(){
    hw_timer_t *timer = NULL;
    timer = timerBegin(0, 80, true);
    timerAttachInterrupt(timer, &onTimer, true);
    timerAlarmWrite(timer, 1000, true); // 1000us = 1ms
    timerAlarmEnable(timer);
}
void initPin(){
    pinMode(LED_PORT, OUTPUT);
    LED_ON;
    pinMode(SPICLOCK, OUTPUT);
    digitalWrite(SPICLOCK, LOW);
    // ASEL 핀들을 INPUT_PULLUP으로 설정
    pinMode(ASEL1, INPUT_PULLUP);
    pinMode(ASEL2, INPUT_PULLUP);
    pinMode(ASEL3, INPUT_PULLUP);

    pinMode(RELAY1_DISCHARGE, OUTPUT);
    digitalWrite(RELAY1_DISCHARGE, LOW);
    pinMode(RELAY2_CHARGE, OUTPUT);
    digitalWrite(RELAY2_CHARGE, LOW);
    pinMode(RELAY3_RESERVE, OUTPUT);
    digitalWrite(RELAY3_RESERVE, LOW);
    // 초기 상태 확인을 위한 디버깅
    delay(100); // 핀 상태 안정화 대기
}



void setup()
{
    Serial.begin(115200);
    initPin();
    EEPROM.begin(sizeof(nvmSystemSet));
    readAndWriteEprom();
    _ReadAmpereClass.VREF = float(nvmSet.Max1161_RefVolt) / 1000.0;

    Serial.printf("\nVersion: %s\n", VERSION);
    Serial.printf("\nInstalledCells: %d\n", nvmSet.InstalledCells);
    Serial.printf("ModbusAddress: %d\n", nvmSet.ModbusAddress);
    Serial.printf("WEBSERVERPORT: %d\n", nvmSet.WEBSERVERPORT);
    Serial.printf("Max1161_RefVolt: %d\n", nvmSet.Max1161_RefVolt);
    Serial.printf("Max1161_CellOffset: %d\n", nvmSet.Max1161_CellOffset);
    Serial.printf("Max1161_CellGain: %d\n", nvmSet.Max1161_CellGain);
    Serial.printf("BAUDRATE: %d\n", nvmSet.BAUDRATE);
    Serial.printf("IPADDRESS: %s\n", IPAddress(nvmSet.IPADDRESS).toString().c_str());
    Serial.printf("GATEWAY: %s\n", IPAddress(nvmSet.GATEWAY).toString().c_str());
    Serial.printf("SUBNETMASK: %s\n", IPAddress(nvmSet.SUBNETMASK).toString().c_str());
    Serial.printf("UseHoleCTRatio : %d\n", nvmSet.UseHoleCTRatio);

    SPI.begin(SPICLOCK, DATAIN_MISO, DATAOUT_MOSI, SEL_MAX14921);
    esp_task_wdt_init(60*10, true);
    esp_task_wdt_add(NULL);
    esp_task_wdt_reset();
    upLoder(); 
    esp_task_wdt_reset();
    void bluetoothTask(void *pvParameters);
    xTaskCreate(bluetoothTask, "bluetoothTask", 1024 * 4,
                NULL, 1, h_pxblueToothTask); // 5120 6144 PCB 패턴문제로 사용하지 않는다.
    bmsModbus.setup();
    useInterrupt();
}
uint8_t readModbusAddress(){
    uint8_t address = 0;
    if(analogRead(ASEL1) > 1000) address += 1;
    if(analogRead(ASEL2) > 1000) address += 2;
    if(digitalRead(ASEL3) == HIGH) address += 4;
    if(nvmSet.ModbusAddress != address){
        nvmSet.ModbusAddress = address;
        EEPROM.writeBytes(0, (byte *)&nvmSet, sizeof(nvmSystemSet));
        EEPROM.commit();
        bmsModbus.registerWorker(nvmSet.ModbusAddress);
        Serial.printf("ModbusAddress Changed to: %d\n", nvmSet.ModbusAddress);
    }
    return address;
}
extern BluetoothSerial SerialBT;
void loop()
{
    esp_task_wdt_reset();
    long currentTime = millis();
   
    if(timerCount > 200)  // 200ms
    {
        _ReadAmpereClass.readAmpereAdc();
        timerCount = 0;
    }
    if (currentTime - elaspedTime250 > EVERY_250)
    {
        elaspedTime250 = currentTime;
    }
    if (currentTime - elaspedTime1000 > EVERY_1000)
    {
        elaspedTime1000 = currentTime;
        LED_TOGGLE;
        relayControl.OnOffbyAmpere();
        readModbusAddress();
    }

    if (currentTime - elaspedTime2000 > EVERY_2000)
    {
        elaspedTime2000 = currentTime;
        float totalVoltage = 0.0;
        uint16_t maxVol;
        uint16_t minVol=65535;
        SerialBT.printf("\r\nR:%d C:%3.1f D:%3.1f A:%3.2f",
                relayControl.readRelayStatus(),
                (float)nvmSet.CutOffChargeAmpere/10.0,
                (float)nvmSet.CutOffDischargeAmpere/10.0,
                _ReadAmpereClass.ampereAverage);
        Serial.printf("\r\nR:%d C:%3.2f D:%3.2f A:%3.1f",
                relayControl.readRelayStatus(),
                (float)nvmSet.CutOffChargeAmpere/10.0,
                (float)nvmSet.CutOffDischargeAmpere/10.0,
                _ReadAmpereClass.ampereAverage);
    }
    delay(5);
}

