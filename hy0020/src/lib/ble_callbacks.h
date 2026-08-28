#ifndef BleCallbacks_h
#define BleCallbacks_h

#include <bluefruit.h>
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>


#include "hid_common.h"
#include "az_common.h"


using namespace Adafruit_LittleFS_Namespace;

#define INPUT_REPORT_RAW_MAX_LEN 20
#define OUTPUT_REPORT_RAW_MAX_LEN 20

// HID キーボード
extern BLEDis bledis; // BLE 情報サービス
extern BLEHidAdafruit blehid; // キーボードサービス
extern BLEClientDis  clientDis;  // device information client
extern BLEUart bleuart; // uart over ble

// HID BLE クライアント
extern BLEClientUart clientUart; // bleuart client

extern BLECharacteristic *_characteristic_input;
extern BLECharacteristic *_characteristic_output;


bool addr_check(uint8_t *addr_a, uint8_t *addr_b);
void addr_copy(uint8_t *addr_a, uint8_t *addr_b);

// HidrawCallback
void HidrawCallbackExec(int data_length);

// BLE クライアント(左手用) コールバック
void scan_callback(ble_gap_evt_adv_report_t* report); // スキャン
void client_connect_callback(uint16_t conn_handle); // コネクト
void client_disconnect_callback(uint16_t conn_handle, uint8_t reason); // ディスコネクト
void bleuart_rx_callback(BLEClientUart& uart_svc); // RX コールバック

#endif
