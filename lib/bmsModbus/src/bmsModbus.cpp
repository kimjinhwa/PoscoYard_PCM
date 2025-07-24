#include "bmsModbus.h"
#include "readAmpere.h"
#include "EEPROM.h"
#include "../../src/main.h"
#include "../../version.h"
ModbusServerRTU MBserver(2000,GPIO_NUM_33);
int16_t sendBuffer[100];
void getVersion(int16_t *desc){
  String ver = String(VERSION);
  int16_t firstDot = ver.indexOf(".");
  int16_t secondDot = ver.indexOf(".",firstDot+1);
  int16_t underScore= ver.indexOf("_");
  desc[0] = ver.substring(underScore+1,firstDot).toInt();
  desc[1] = ver.substring(firstDot+1,secondDot).toInt();
  desc[2] = ver.substring(secondDot+1).toInt();


}
ModbusMessage BmsModbus::FC03(ModbusMessage request)
{
  uint16_t address;           // requested register address
  uint16_t words;             // requested number of registers
  ModbusMessage response;     // response message to be sent back
  int16_t firmVersion[3];
  getVersion(firmVersion);
  EEPROM.readBytes(0, (byte *)&nvmSet, sizeof(nvmSystemSet)); //Read from EEPROM

  String ver = String(VERSION);

  memset(sendBuffer, 0, sizeof(sendBuffer));
  EEPROM.readBytes(0, (byte *)&nvmSet, sizeof(nvmSystemSet)); //Read from EEPROM
  sendBuffer[0] = nvmSet.ModbusAddress; // modbus address
  sendBuffer[1] = nvmSet.InstalledCells; // installed cells
  sendBuffer[2] = nvmSet.Max1161_RefVolt;  // reference voltage
  sendBuffer[3] = nvmSet.Max1161_CellGain;  // gain
  sendBuffer[4] = nvmSet.Max1161_CellOffset;  // offset
  sendBuffer[5] = firmVersion[0];  // major version
  sendBuffer[6] = firmVersion[1];  // minor version
  sendBuffer[7] = firmVersion[2];  // patch version
  sendBuffer[8] = 0;
  sendBuffer[9] = nvmSet.UseHoleCTRatio;
  sendBuffer[10] = nvmSet.TempCutOff;
  sendBuffer[11] = nvmSet.AmpereOffset;
  sendBuffer[12] = nvmSet.AmpereGain;
  sendBuffer[13] = nvmSet.TotalVoltageOffset;
  sendBuffer[14] = nvmSet.TotalVoltageGain;
  sendBuffer[15] = 0; //nvmSet.spiBalance01;
  sendBuffer[16] = 0; //nvmSet.spiBalance02;
  sendBuffer[17] = 0; //nvmSet.CanIBalance;
  sendBuffer[18] = 0; //nvmSet.BalanceTargetVoltage;
  sendBuffer[30] = (digitalRead(RELAY1_DISCHARGE) << 0) | 
                   (digitalRead(RELAY2_CHARGE) << 1) | 
                   (digitalRead(RELAY3_RESERVE) << 2);
  sendBuffer[31] = 0;
  sendBuffer[32] = nvmSet.CutOffChargeAmpere;
  sendBuffer[33] = nvmSet.CutOffDischargeAmpere;
  sendBuffer[34] = nvmSet.ResumeGapAmpere;
  sendBuffer[35] = nvmSet.RelayDelayTime;
  sendBuffer[36] = nvmSet.HoleCTdirection;
  // get request values
  request.get(2, address);
  request.get(4, words);

  // Address and words valid? We assume 10 registers here for demo
  if (words && (address + words) < 100) {
    // Looks okay. Set up message with serverID, FC and length of data
    response.add(request.getServerID(), request.getFunctionCode(), (uint8_t)(words * 2));
    // Fill response with requested data
    for (uint16_t i = address; i < address + words; ++i) {
      response.add(sendBuffer[i]);
    }
  } else {
    // No, either address or words are outside the limits. Set up error response.
    response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
  }
  return response;
}
ModbusMessage BmsModbus::FC04(ModbusMessage request)
{
  uint16_t address;           // requested register address
  uint16_t words;             // requested number of registers
  ModbusMessage response;     // response message to be sent back

  if (xSemaphoreTake(ReadAmpereClass::dataMutex, portMAX_DELAY) == pdPASS)
  {
    for (int i = 0; i < nvmSet.InstalledCells; i++)
    {
      sendBuffer[i] = 0;
    }
    sendBuffer[16] = 0;
    sendBuffer[17] = 0;
    sendBuffer[18] = _ReadAmpereClass.ampereAverage; // current
    sendBuffer[19] = 0;
    sendBuffer[20] = 0;
    sendBuffer[21] = 0;
    sendBuffer[22] = 0;
    xSemaphoreGive(ReadAmpereClass::dataMutex);
  }
  // get request values
  request.get(2, address);
  request.get(4, words);

  // Address and words valid? We assume 10 registers here for demo
  if (words && (address + words) < 100) {
    // Looks okay. Set up message with serverID, FC and length of data
    response.add(request.getServerID(), request.getFunctionCode(), (uint8_t)(words * 2));
    // Fill response with requested data
    for (uint16_t i = address; i < address + words; ++i) {
      response.add(sendBuffer[i]);
    }
  } else {
    // No, either address or words are outside the limits. Set up error response.
    response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
  }
  return response;
}
ModbusMessage BmsModbus::FC06(ModbusMessage request)
{
  uint16_t address;           // requested register address
  uint16_t words;             // requested number of registers
  ModbusMessage response;     // response message to be sent back
  // get request values
  request.get(2, address);
  request.get(4, words);

  if (address  < 100) {
    // Looks okay. Set up message with serverID, FC and length of data
    switch(address){
      case 0:
        // nvmSet.ModbusAddress = words; // modbus address
        // updateModbusAddress((uint8_t)words);
        break;
      case 1:
        //nvmSet.InstalledCells = words; // installed cells
        break;
      case 2:
        // nvmSet.Max1161_RefVolt = words;  // reference voltage
        // _ReadAmpereClass.VREF = float(nvmSet.Max1161_RefVolt)/1000.0;
        break;
      case 3:
        // nvmSet.Max1161_CellGain = words;  // gain
        break;
      case 4:
        // nvmSet.Max1161_CellOffset = words;  // offset
        break;
      case 9:
        nvmSet.UseHoleCTRatio = words;
        break;
      case 10:
        nvmSet.TempCutOff = words;
        break;
      case 11:
        nvmSet.AmpereOffset = words;
        break;
      case 12:
        nvmSet.AmpereGain = words;
        break;
      case 13:
        //nvmSet.TotalVoltageOffset = words;
        break;
      case 14:
        //nvmSet.TotalVoltageGain = words;
        break;
      case 15:
        //spiBalance01
        break;
      case 16:
        //spiBalance02
        break;
      case 17:
        //Can I balance?
        break;
      case 18:
        //Balance TargetVoltage
        break;
      case 30:  // PCM Board Status  
        break;
      case 31:  // PCM Board Controll
        sendBuffer[31] = (words & 0x01) | ((words & 0x02) << 1) | ((words & 0x04) << 2);
        relayControl.setRelayStatus(words);
        break;
      case 32:  // PCM Board Cut off Charge Ampere
        nvmSet.CutOffChargeAmpere = words;
        break;
      case 33:  // PCM Board Cut off Discharge Ampere
        nvmSet.CutOffDischargeAmpere = words;
        break;
      case 34:  // PCM Board Resume Gap Ampere 
        nvmSet.ResumeGapAmpere = words;
        break;
      case 35:  // Relay Delay time 
        nvmSet.RelayDelayTime = words;
        break;
      case 36:  // Hole CT direction
        nvmSet.HoleCTdirection = words;
        break;
      default:
        break;
    }
    EEPROM.writeBytes(0, (byte *)&nvmSet, sizeof(nvmSystemSet)); //Write to EEPROM
    EEPROM.commit(); //Commit EEPROM
    EEPROM.readBytes(0, (byte *)&nvmSet, sizeof(nvmSystemSet)); //Read from EEPROM
    response.add(request.getServerID(), request.getFunctionCode(), address, words);
    // Fill response with requested data
  } else {
    // No, either address or words are outside the limits. Set up error response.
    response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
  }
  return response;
}
void BmsModbus::FC06_Broadcast(ModbusMessage request)
{
  uint16_t address;           // requested register address
  uint16_t words;             // requested number of registers
  // get request values
  request.get(2, address);
  request.get(4, words);
  if(request.getServerID() == ANY_SERVER){
    Serial.printf("BROADCAST\n");
  }
  if( request.getFunctionCode() != WRITE_HOLD_REGISTER){
    Serial.printf("Not WRITE_HOLD_REGISTER\n");
    return;
  }

  if (address  < 13) {
    // Looks okay. Set up message with serverID, FC and length of data
    switch(address){
      case 0:
        nvmSet.ModbusAddress = words; // modbus address
        updateModbusAddress((uint8_t)words);
        break;
      case 1:
        nvmSet.InstalledCells = words; // installed cells
        break;
      case 2:
        nvmSet.Max1161_RefVolt = words;  // reference voltage
        _ReadAmpereClass.VREF = float(nvmSet.Max1161_RefVolt)/1000.0;
        break;
      case 3:
        nvmSet.Max1161_CellGain = words;  // gain
        break;
      case 4:
        nvmSet.Max1161_CellOffset = words;  // offset
        break;
      case 9:
        nvmSet.UseHoleCTRatio = words;
        break;
      case 10:
        nvmSet.TempCutOff = words;
        break;
      case 11:
        nvmSet.AmpereOffset = words;
        break;
      case 12:
        nvmSet.AmpereGain = words;
        break;
      default:
        break;
    }
    EEPROM.writeBytes(0, (byte *)&nvmSet, sizeof(nvmSystemSet)); //Write to EEPROM
    EEPROM.commit(); //Commit EEPROM
    EEPROM.readBytes(0, (byte *)&nvmSet, sizeof(nvmSystemSet)); //Read from EEPROM
  } 
}

void BmsModbus::updateModbusAddress(uint8_t newAddress) {
    // 기존 서비스 중지
    Serial.printf("\nupdateModbusAddress %d",newAddress);
    EEPROM.readBytes(0, (byte *)&nvmSet, sizeof(nvmSystemSet));
    nvmSet.ModbusAddress = newAddress;
    EEPROM.writeBytes(0, (byte *)&nvmSet, sizeof(nvmSystemSet));
    EEPROM.commit();
    EEPROM.readBytes(0, (byte *)&nvmSet, sizeof(nvmSystemSet));
    Serial.println("\nSystem Restart");
    ESP.restart();
    
    // // 새 주소로 서비스 재시작
    // Serial1.begin(nvmSet.BAUDRATE, SERIAL_8N1, GPIO_NUM_26, GPIO_NUM_22);
    
    // // 새 주소로 워커 재등록
    // MBserver.registerWorker(nvmSet.ModbusAddress, READ_HOLD_REGISTER, &FC03);
    // MBserver.registerWorker(nvmSet.ModbusAddress, READ_INPUT_REGISTER, &FC04);
    // MBserver.registerWorker(nvmSet.ModbusAddress, WRITE_HOLD_REGISTER, &FC06);
    
    // // ModbusRTU 다시 시작
    // MBserver.begin(Serial1);
}
void BmsModbus::registerWorker(uint8_t address)
{
  MBserver.unregisterWorker(address, READ_HOLD_REGISTER);
  MBserver.unregisterWorker(address, READ_INPUT_REGISTER);
  MBserver.unregisterWorker(address, WRITE_HOLD_REGISTER);

  MBserver.registerWorker(address, READ_HOLD_REGISTER, &FC03);
  MBserver.registerWorker(address, READ_INPUT_REGISTER, &FC04);
  MBserver.registerWorker(address, WRITE_HOLD_REGISTER, &FC06);
}
void BmsModbus::setup()
{

    Serial1.begin(nvmSet.BAUDRATE, SERIAL_8N1, GPIO_NUM_26, GPIO_NUM_22);

    MBserver.registerBroadcastWorker( &FC06_Broadcast);
    // Register served function code worker for server 1, FC 0x03
    MBserver.registerWorker(nvmSet.ModbusAddress, READ_HOLD_REGISTER, &FC03);
    MBserver.registerWorker(nvmSet.ModbusAddress, READ_INPUT_REGISTER, &FC04);
    MBserver.registerWorker(nvmSet.ModbusAddress, WRITE_HOLD_REGISTER, &FC06);

    // Start ModbusRTU background task
    MBserver.begin(Serial1);
}
