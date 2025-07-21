#include "readAmpere.h"
#include "Arduino.h"
#include <SPI.h>
#include <ThreeWire.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "../../src/main.h"

SemaphoreHandle_t ReadAmpereClass::dataMutex = NULL;

void ReadAmpereClass::initFIFO() 
{
  head = 0;
  count = 0;
  for (int j = 0; j < FIFO_SIZE; j++)  // 20 samples per cell
    ampereFIFO[j] = 0;
}
float ReadAmpereClass::updateAmpereFIFO(int16_t newvalue) {
  ampereFIFO[head] = newvalue;
  head = (head + 1) % FIFO_SIZE;
  if(count < FIFO_SIZE) {
    count++;
  }
  int32_t sum = 0;
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


uint16_t ReadAmpereClass::readAmpereAdc()
{
    uint16_t adcData = 0;
    adcData = adcData + nvmSet.AmpereOffset;
    adcData = adcData * nvmSet.AmpereGain / 1000.0;
    float Vntc = adcData * VREF / 65536.0;
    Vntc = Vntc - 2.0;
    float voltage = 1000.0 * Vntc * 2.0; // mV 단위로 변환
    float ampere = 0.0;

    if (xSemaphoreTake(ReadAmpereClass::dataMutex, portMAX_DELAY) == pdPASS)
    {
      updateAmpereFIFO((int16_t)ampere);
      xSemaphoreGive(ReadAmpereClass::dataMutex);
    }
    return adcData;
}
