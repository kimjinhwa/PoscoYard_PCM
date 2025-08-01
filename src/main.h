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
#define LED_TOGGLE digitalWrite(LED_PORT, digitalRead(LED_PORT) == HIGH ? LOW : HIGH)

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
  int16_t TempCutOff;
  int16_t AmpereOffset;
  uint16_t AmpereGain;
  uint16_t TotalVoltageOffset;
  uint16_t TotalVoltageGain;
  int16_t CutOffChargeAmpere;
  int16_t CutOffDischargeAmpere;
  uint16_t ResumeGapAmpere;
  uint16_t RelayDelayTime;
  int8_t HoleCTdirection;
  char SSID[32];
  char PASS[32];
  uint8_t EpromValidEnd;
} nvmSystemSet;
extern nvmSystemSet nvmSet;

enum F03_Addr{
  MODBUSADDRESS = 0,
  INSTALLEDCELLS = 1,
  MAX1161_REFVOLT = 2,
  MAX1161_CELLGAIN = 3,
  MAX1161_CELLOFFSET = 4,
  MAJORVERSION = 5,
  MINORVERSION = 6,
  PATCHVERSION = 7,
  OPENWIRESTATUS = 8,
  USEHOLECTRATIO = 9,
  TEMPCUTOFF = 10,
  AMPEREOFFSET = 11,
  AMPEREGAIN = 12,
  TOTALVOLTAGEOFFSET = 13,
  TOTALVOLTAGEGAIN = 14,
  SPIBALANCE01 = 15,
  SPIBALANCE02 = 16,
  MODBUSCANBALANCE= 17,
  BALANCETARGETVOLTAGE= 18,
  ACCUMULATEDDISCHARGECURRENT = 19,
  ACCUMULATEDDISCHARGETIME = 20,
  ACCUMULATEDCHARGECURRENT = 21,
  ACCUMULATEDCHARGETIME = 22,
  FULLCHARGESET = 23,
  RATED_BATTERY_CURRENT= 24,
  STATE_OF_CHARGE= 25,
  RESERVER26 = 26,
  RESERVER27 = 27,
  RESERVER28 = 28,
  RESERVER29 = 29,
  PCMBOARSTATUS= 30,
  PCMBOARDSET= 31,
  CUTOFFCHARGEAMPERE = 32,
  CUTOFFDISCHARGEAMPERE = 33,
  RESUMEGAPAMPERE = 34,
  RELAYDELAYTIME = 35,
  HOLECTDIRECTION = 36,
  RESERVER37 = 37,
  RESERVER38 = 38,
  RESERVER39 = 39,
  RESERVER40 = 40,
};
typedef union 
{
    struct{
        uint8_t relay1 : 1;
        uint8_t relay2 : 1;
        uint8_t relay3 : 1;
    }bit;
    uint8_t value;
}RelayStatus;
//extern BluetoothSerial SerialBT;
class RelayControl
{
public:
    long userRelayOnTime = 0;
    RelayStatus nowRelayStatus;
    RelayStatus lastRelayStatus;
    void relayAutoOnControl() {};
    void relayAutoOffControl() {};
    void all_off(){
      if(userRelayOnTime > nvmSet.RelayDelayTime * 1000){
        digitalWrite(RELAY2_CHARGE, LOW); 
        digitalWrite(RELAY1_DISCHARGE, LOW);
      }
      if(isRelayChange()) userRelayOnTime =0 ; // 릴레이 상태가 변경되면 릴레이 온타임을 초기화
    }
    void all_charge_discharge(){
      if(userRelayOnTime > nvmSet.RelayDelayTime * 1000){
        digitalWrite(RELAY2_CHARGE, HIGH); 
        digitalWrite(RELAY1_DISCHARGE, HIGH);
      }
      if(isRelayChange()) userRelayOnTime =0 ;
    }
    void only_charge(){
      if(userRelayOnTime > nvmSet.RelayDelayTime * 1000){
        digitalWrite(RELAY2_CHARGE, HIGH); 
        digitalWrite(RELAY1_DISCHARGE, LOW);
      }
      if(isRelayChange()) userRelayOnTime =0 ;
    }
    void only_discharge(){
      if(userRelayOnTime > nvmSet.RelayDelayTime * 1000){
        digitalWrite(RELAY2_CHARGE, LOW); 
        digitalWrite(RELAY1_DISCHARGE, HIGH);
      }
      if(isRelayChange()) userRelayOnTime =0 ;
    }
    void setRelayStatus(uint8_t status){
      if(userRelayOnTime > nvmSet.RelayDelayTime * 1000)
      {
        digitalWrite(RELAY2_CHARGE, status & 0x02);
        digitalWrite(RELAY1_DISCHARGE, status & 0x01);
        digitalWrite(RELAY3_RESERVE, status & 0x04);
      }
      if(isRelayChange())
      {
        ESP_LOGI("RelayControl", "RelayStatus Changed %d", status);
        userRelayOnTime =0 ;
      } 
    }
    uint8_t readRelayStatus()
    {
        nowRelayStatus.bit.relay1 = digitalRead(RELAY1_DISCHARGE);
        nowRelayStatus.bit.relay2 = digitalRead(RELAY2_CHARGE);
        nowRelayStatus.bit.relay3 = digitalRead(RELAY3_RESERVE);
        return nowRelayStatus.value;
    }
    /* 릴레이 상태가 변경되었는지 확인 */
    /* 변경되면 true 반환 */
    bool isRelayChange()
    {
        readRelayStatus(); //  현재 상태를 읽는다.
        if (nowRelayStatus.value != lastRelayStatus.value)
        {
            lastRelayStatus.value = nowRelayStatus.value;
            return true;
        }
        return false;
    }
    void OnOffbyAmpere()
    {
        // ESP_LOGI("RelayControl", "change %3.2f %3.2f %3.2f",
        //         (float)nvmSet.CutOffChargeAmpere/10.0,
        //         (float)nvmSet.CutOffDischargeAmpere/10.0,
        //         _ReadAmpereClass.ampereAverage);
        if (_ReadAmpereClass.getAmpereAverage()> (float)nvmSet.CutOffChargeAmpere/10.0)
        {
                only_discharge();
        }
        else if (_ReadAmpereClass.getAmpereAverage()< (float)nvmSet.CutOffDischargeAmpere/10.0)
        {
                only_charge();
        }
        else
        {
                all_charge_discharge();
        }
    }
};
extern RelayControl relayControl;

#endif