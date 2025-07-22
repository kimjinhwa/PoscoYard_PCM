#include "readAmpere.h"
#include "Arduino.h"
#include <SPI.h>
#include <ThreeWire.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "../../src/main.h"
#include "esp_adc_cal.h"

SemaphoreHandle_t ReadAmpereClass::dataMutex = NULL;

void ReadAmpereClass::initFIFO() 
{
  head = 0;
  count = 0;
  for (int j = 0; j < FIFO_SIZE; j++)  // 20 samples per cell
    ampereFIFO[j] = 0;
}
float ReadAmpereClass::updateAmpereFIFO(float newvalue) {
  ampereFIFO[head] = newvalue;
  head = (head + 1) % FIFO_SIZE;
  if(count < FIFO_SIZE) {
    count++;
  }
  float sum = 0.0f;
  for(int i=0; i<count; i++) {
    sum += ampereFIFO[i];
  }
  ampereAverage = sum / count;
  return ampereAverage;
}


ReadAmpereClass::ReadAmpereClass()
{
    if(dataMutex == NULL) dataMutex = xSemaphoreCreateMutex();
}

float ReadAmpereClass::readAmpereAdc()
{
    uint32_t adc_voltage1;
    int holeCTdirection = nvmSet.HoleCTdirection == 0 ? 1 : -1;
    esp_adc_cal_characteristics_t adc_chars;
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12, 1100, &adc_chars);
    //SENSOR_VP = GPIO36 = ADC1_CH0
    esp_adc_cal_get_voltage(ADC_CHANNEL_0, &adc_chars, &adc_voltage1);  // IN_TH = GPIO39 = ADC1_CH3
    //uint16_t adcData = 0;
    //ESP_LOGI("AMPERE", "adc_voltage1: %d\n", (int32_t)adc_voltage1);
    adc_voltage1 += nvmSet.AmpereOffset;
    //ESP_LOGI("AMPERE", "adc_voltage1: %d\n", (int32_t)adc_voltage1);
    adc_voltage1 = adc_voltage1 * nvmSet.AmpereGain / 1000.0;
    //ESP_LOGI("AMPERE", "adc_voltage1: %d\n", (int32_t)adc_voltage1);
    float voltage = (adc_voltage1 - 2000.0) * 2.0;  // 읽은 전압에서 중간값인 2.0V를 빼서 중간값을 0V로 만든다.
    float ampere = voltage * nvmSet.UseHoleCTRatio /HOLE_CT_V ;
    ampere *= holeCTdirection;
    //ESP_LOGI("AMPERE", "ampere: %d\n", (int32_t)ampere);
    if (xSemaphoreTake(ReadAmpereClass::dataMutex, portMAX_DELAY) == pdPASS)
    {
      updateAmpereFIFO(ampere);
      xSemaphoreGive(ReadAmpereClass::dataMutex);
    }
    //ESP_LOGI("AMPERE", "ampereAverate: %3.2f\n", ampereAverage);
    return ampere;
}