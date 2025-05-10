#ifndef BLECUSTAM_H_
#define BLECUSTAM_H_

#include "bluefruit_common.h"

#include "BLECharacteristic.h"
#include "BLEService.h"


#include "ble_callbacks.h"


#define  CUSTAM_UUID_SERVICE  "0000ff14-0000-1000-8000-00805f9b34fb"
#define  CUSTAM_UUID_INPUT  "0000ff15-0000-1000-8000-00805f9b34fb"
#define  CUSTAM_UUID_OUTPUT  "0000ff16-0000-1000-8000-00805f9b34fb"


extern BLECharacteristic *_characteristic_input;
extern BLECharacteristic *_characteristic_output;

class BLECustam : public BLEService
{
  public:
    BLECustam(void);

    virtual err_t begin(void);

    bool write (const char* str);

    bool notify(uint8_t level);
    bool notify(uint16_t conn_hdl, uint8_t level);
    static void onCommandWritten(uint16_t conn_hdl, BLECharacteristic* characteristic, uint8_t* data, uint16_t data_length);
};



#endif /* BLECUSTAM_H_ */
