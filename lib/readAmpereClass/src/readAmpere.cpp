#include "readAmpere.h"
#include "Arduino.h"
#include <SPI.h>
#include <ThreeWire.h>
#include <math.h>
#include <BluetoothSerial.h>
#include "freertos/FreeRTOS.h"
#include "../../src/main.h"
#include "esp_adc_cal.h"
#include "driver/adc.h"

extern BluetoothSerial SerialBT;
SemaphoreHandle_t ReadAmpereClass::dataMutex = NULL;
static esp_adc_cal_characteristics_t adc_chars;
static bool adc_initialized = false;

ReadAmpereClass::ReadAmpereClass()
{
    if(dataMutex == NULL) dataMutex = xSemaphoreCreateMutex();
    ratedBatteryCurrent = 100;  // 먼저 설정
    accumulatedDischargeCurrent = 0;
    accumulatedDischargeTime = 0;
    accumulatedChargeCurrent = 0;  // 100Ah로 초기화 (100% SOC)
    accumulatedChargeTime = 0;
    isNowChargeing = false;
    isNowDischarging = false;
    systemStartTime = millis();
    ChargeStartTime = 0;
    DischargeStartTime = 0;
    StateOfCharge = 100.0f;  // 초기 SOC를 100%로 설정
        
    // ADC 초기화 (한 번만)
    if(!adc_initialized) {
        adc1_config_width(ADC_WIDTH_BIT_12);  // 12bit 해상도
        adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_12);  // GPIO36, 0~3.3V 범위
        esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12, 1100, &adc_chars);
        adc_initialized = true;
        Serial.println("ADC1_CH0 (GPIO36) initialized for current sensor");
    }
}
void ReadAmpereClass::initFIFO() 
{
  head = 0;
  count = 0;
  for (int j = 0; j < FIFO_SIZE; j++)  // 20 samples per cell
    ampereFIFO[j] = 0;
}
float ReadAmpereClass::updateAmpereFIFO(float newvalue)
{
  ampereFIFO[head] = newvalue;
  head = (head + 1) % FIFO_SIZE;
  if (count < FIFO_SIZE) count++;
  if (xSemaphoreTake(ReadAmpereClass::dataMutex, portMAX_DELAY) == pdPASS)
  {
    float sum = 0.0f;
    for (int i = 0; i < count; i++)
      sum += ampereFIFO[i];
    ampereAverage = sum / count;
    xSemaphoreGive(ReadAmpereClass::dataMutex);
  }
  return ampereAverage;
}


static long every1000ms = 0;
void ReadAmpereClass::accumulateCurrent()
{
  // 1초마다 실행
  if (millis() - every1000ms >= 1000)
  {
    every1000ms = millis();
    
    // 충전/방전 상태 판단 (3A 이상)
    if (ampereAverage > 3.0f)
    {
      isNowChargeing = true;
      isNowDischarging = false;
      accumulatedDischargeTime = 0;
      DischargeStartTime = millis();
    }
    else if (ampereAverage < -3.0f)
    {
      isNowDischarging = true;
      isNowChargeing = false;
      accumulatedChargeTime = 0;
      ChargeStartTime = millis();
    }
    else
    {
      isNowChargeing = false;
      isNowDischarging = false;
    }
    
    // 1초간의 전류를 Ah 단위로 변환 (1초 = 1/3600 시간)
    float currentAh = ampereAverage / 3600.0f;
    
    // 충전/방전 전류 누적
    if (isNowChargeing)
    {
      accumulatedChargeCurrent += currentAh;
      accumulatedChargeTime = millis() - ChargeStartTime;
      ESP_LOGI("\nReadAmpereClass", "충전 전류: %f Ah, 누적 충전량: %f Ah, 누적 충전시간: %ld ms", 
        currentAh, accumulatedChargeCurrent, accumulatedChargeTime);
    }
    else if (isNowDischarging)
    {
      accumulatedDischargeCurrent += abs(currentAh); // 절댓값으로 방전량 누적
      accumulatedDischargeTime = millis() - DischargeStartTime;
      ESP_LOGI("\nReadAmpereClass", "방전 전류: %f Ah, 누적 방전량: %f Ah, 누적 방전시간: %ld ms", 
        currentAh, accumulatedDischargeCurrent, accumulatedDischargeTime);
    }
    
    // 만충전 설정 시 SOC를 100%로 설정
    if (isFullChargeSet)
    {
      accumulatedChargeCurrent = ratedBatteryCurrent; // Ah 단위 그대로 사용
      accumulatedDischargeCurrent = 0;
      isFullChargeSet = false;
      StateOfCharge = 100.0f;
    }
    else
    {
      // SOC 계산: (초기 용량 - 누적 방전량 + 누적 충전량) / 총 용량 × 100%
      float totalCapacity = ratedBatteryCurrent; // Ah 단위 그대로 사용
      float currentCapacity = totalCapacity - accumulatedDischargeCurrent + accumulatedChargeCurrent;
      
      // SOC 범위 제한 (0~100%)
      StateOfCharge = constrain(currentCapacity / totalCapacity * 100.0f, 0.0f, 100.0f);
      if(StateOfCharge == 100.0f){
        accumulatedChargeCurrent = 0;
        accumulatedDischargeCurrent = 0;
      }
      ESP_LOGI("\nReadAmpereClass", "SOC: %f %f %f", StateOfCharge, accumulatedDischargeCurrent, accumulatedChargeCurrent);
      SerialBT.printf("\nSOC: %3.2f %3.2f %3.2f\n", StateOfCharge, accumulatedDischargeCurrent, accumulatedChargeCurrent);
    }
  }
}
float ReadAmpereClass::readAmpereAdc()
{
    uint32_t adc_voltage1;
    int holeCTdirection = nvmSet.HoleCTdirection == 0 ? 1 : -1;
    // AMP_OUT 읽기: GPIO36 = ADC1_CH0
    // ADC는 생성자에서 이미 초기화됨
    esp_adc_cal_get_voltage(ADC_CHANNEL_0, &adc_chars, &adc_voltage1);  // AMP_OUT = GPIO36 = ADC1_CH0
    adc_voltage1 += nvmSet.AmpereOffset;
    adc_voltage1 = adc_voltage1 * nvmSet.AmpereGain / 1000.0;
    float voltage = (adc_voltage1 - 2000.0) * 2.0;  // 읽은 전압에서 중간값인 2.0V를 빼서 중간값을 0V로 만든다.
    float ampere = voltage * nvmSet.UseHoleCTRatio /HOLE_CT_V ;
    ampere *= holeCTdirection;
    updateAmpereFIFO(ampere);
    accumulateCurrent();
    return ampere;
}