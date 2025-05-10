

#include "ble_callbacks.h"
#include "az_common.h"


#include <bluefruit.h>
#include "CustamService.h"

// HID 
extern BLEDis bledis;
extern BLEHidAdafruit blehid;
extern BLECustam blecus;


// BLEキーボードクラス
class BleKeyboardJIS
{
  public:
    uint8_t batteryLevel; // バッテリーレベル 0-100
    hid_keyboard_report_t _keyReport;
    MediaKeyReport _mediaKeyReport;
    uint8_t _MouseButtons; // マウスボタン情報

    /* メソッド */
    BleKeyboardJIS(void); // コンストラクタ
    void begin(char *deviceName);
    void startAdv(void);
    static void set_keyboard_led(uint16_t conn_handle, uint8_t led_bitmap);
    bool isConnected(void);
    unsigned short modifiers_press(unsigned short k);
    unsigned short modifiers_release(unsigned short k);
    void shift_release(); // Shiftを離す
    unsigned short modifiers_media_press(unsigned short k);
    unsigned short modifiers_media_release(unsigned short k);
    void sendReport(hid_keyboard_report_t* keys);
    void sendReport(MediaKeyReport* keys);
    void mouse_click(uint8_t b);
    void mouse_press(uint8_t b);
    void mouse_release(uint8_t b);
    void mouse_move(signed char x, signed char y, signed char wheel, signed char hWheel);
    size_t press_set(uint8_t k); // 指定したキーだけ押す
    size_t press_raw(unsigned short k);
    size_t release_raw(unsigned short k);
    void releaseAll(void);
    bool onShift(); // Shiftが押されている状態かどうか(物理的に)
    void setConnInterval(int interval_type);
};


