
#ifndef READAMPERE_H
#define READAMPERE_H

#include "Arduino.h"
#include <inttypes.h>

#define SEL_MAX11163 23    // chip-select MAX11163
#define DATAOUT_MOSI 13   // MOSI 
#define DATAIN_MISO 12    // MISO 
#define SPICLOCK 14  // Clock 
#define SPI_SPEED 1000000
#define MAX_EN 18


#define SEL_MAX14921 5 // chip-select MAX14921
#define SAMPLPIN_MAX14921 19 // SAMPLE pin MAX14921  
/*
ECS 16 R/W 0
0: Cell selection is disabled
1: Cell selection is enabled
SC0 17 R/W 0
[ECS, SC0, SC1, SC2, SC3]
1 – SC0, SC1, SC2, SC3: Selects the cell for voltage readout during hold phase.**
The selected cell voltage is routed to AOUT after the rising CS edge. See Table 2.
0 – 0, 0, 0, 0: AOUT is three-stated and sampling switches are configured for
parasitic capacitance error calibration.
0 – 1, 0, 0, 0: AOUT is three-stated and self-calibration of buffer amplifier offset
voltage is initiated after the following rising CS.
0 – SC0, SC1, 0, 1: Switches the T1, T2. T2 analog inputs directly to AOUT. See
Table 3.
0 – 0, 0, 1, 1: VP/12 (MAX14920) or VP/16 (MAX14921) voltage is routed to AOUT
on the next rising CS
0 – SC0, SC1, 1, 1: Routes and buffers the T1, T2. T3 to AOUT. See Table 3.
SC1 18 R/W 0
SC2 19 R/W 0
SC3 20 R/W 0
SMPLB 21 R/W 0
0: Device in sample phase if SAMPL input is logic-high
1: Device in hold phase
DIAG 22 R/W 0
0: Normal operation
1: Diagnostic enable, 10FA leakage is sunk on all CV_ inputs (CV0–CV16).
LOPW 23 R/W 0
0: Normal operation
1: Low-power mode enabled. Current into LDOIN is reduced to 125FA. Current
into VP is reduced to 1FA.*/
// MAX14921 명령어 비트 구조체 정의
typedef union {
  struct {
    uint8_t ECS : 1;    // Cell selection enabled
    uint8_t SC0 : 1;    // Self-calibration mode 0
    uint8_t SC1 : 1;    // Self-calibration mode 1
    uint8_t SC2 : 1;    // Self-calibration mode 2
    uint8_t SC3 : 1;    // Self-calibration mode 3
    uint8_t SAMPLB : 1; // Sample buffer enable
    uint8_t DIAG : 1;   // Diagnostic mode
    uint8_t LOPW : 1;   // Low power mode
  } bits;
  uint8_t cmd;
} MAX14921_Command_t;



#define FIFO_SIZE 30


class ReadAmpereClass{
private:
  float ampereFIFO[FIFO_SIZE];
  float updateAmpereFIFO(float newvalue);
  int head;
  int count;
  float ampereAverage;
  static SemaphoreHandle_t dataMutex;
public:
  float VREF;
	ReadAmpereClass();
  void initFIFO();
  float getAmpereAverage()
  {
    float retAmpere = 0.0f;
    if (xSemaphoreTake(ReadAmpereClass::dataMutex, portMAX_DELAY) == pdPASS)
    {
      retAmpere = ampereAverage;
      xSemaphoreGive(ReadAmpereClass::dataMutex);
    }
    return retAmpere;
  };
  float readAmpereAdc();
};
extern ReadAmpereClass _ReadAmpereClass;
#endif
// // 비트 순서 뒤집기 매크로 (SPI MSB First 전송 순서에 맞춤)
// #define REVERSE_BITS_8(x) (((x & 0x01) << 7) | \
//                            ((x & 0x02) << 5) | \
//                            ((x & 0x04) << 3) | \
//                            ((x & 0x08) << 1) | \
//                            ((x & 0x10) >> 1) | \
//                            ((x & 0x20) >> 3) | \
//                            ((x & 0x40) >> 5) | \
//                            ((x & 0x80) >> 7))



//typedef enum {TT1=0, TT2=1, AMPERE=2} T_NUMBER;
// typedef enum {
//     CELL_NORMAL = 0,
//     CELL_OPEN = 1,
//     CELL_SHORT = 2
// } CellStatus;