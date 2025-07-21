#ifndef MAIN_H
#define MAIN_H

#define LED_PORT GPIO_NUM_15
// 원래 핀 설정
#define ASEL1 GPIO_NUM_32
#define ASEL2 GPIO_NUM_25
#define ASEL3 GPIO_NUM_27
#define AIN_PLUS GPIO_NUM_36  //GPIO36 = ADC1_CH0

#define SENSOR_VN GPIO_NUM_39  //GPIO39 = ADC1_CH3
#define TH3 GPIO_NUM_34  //GPIO34 = ADC1_CH6
#define TH4 GPIO_NUM_35  //GPIO35 = ADC1_CH7
#define IN_TH SENSOR_VN 

#define RELAY1_DISCHARGE GPIO_NUM_19
#define RELAY2_CHARGE GPIO_NUM_18
#define RELAY3_RESERVE GPIO_NUM_5

#define LED_ON digitalWrite(LED_PORT,LOW)
#define LED_OFF digitalWrite(LED_PORT,HIGH)
#define LED_TOGGLE digitalRead(LED_PORT) == HIGH ? LED_ON : LED_OFF

#define HOLE_CT_A 600.0
#define HOLE_CT_V 4000.0

#define MAX_AMPERE_FILTER_FACTOR 0.1

#define HOLE_CT_RATIO (HOLE_CT_A/HOLE_CT_V)
typedef struct
{
  uint8_t EpromValidStart;
  uint8_t InstalledCells;
  uint8_t ModbusAddress;
  uint8_t WEBSERVERPORT;
  uint16_t Max1161_RefVolt;
  int16_t Max1161_CellOffset;
  uint16_t Max1161_CellGain;
  uint32_t BAUDRATE;
  uint32_t IPADDRESS;
  uint32_t GATEWAY;
  uint32_t SUBNETMASK;
  uint16_t UseHoleCTRatio;
  int16_t TempOffset;
  int16_t AmpereOffset;
  uint16_t AmpereGain;
  uint16_t TotalVoltageOffset;
  uint16_t TotalVoltageGain;
  uint16_t CutOffChargeAmpere;
  uint16_t CutOffDischargeAmpere;
  uint16_t ResumeGapAmpere;
  uint16_t RelayDelayTime;
  uint8_t Reserved8;
  char SSID[32];
  char PASS[32];
  uint8_t EpromValidEnd;
} nvmSystemSet;
extern nvmSystemSet nvmSet;


#endif