// BLEキーボード
#include "ble_keyboard_jis.h"

// BLEキーボードクラス
BleKeyboardJIS bleKeyboard = BleKeyboardJIS();

short btn_last = 0;

void setup() {
  bleKeyboard.begin("nrf52832");
  pinMode(7, INPUT_PULLUP);

}

void loop() {
  if (digitalRead(7)) {
    if (btn_last) {
      bleKeyboard.releaseAll();
      btn_last = 0;
    }
  } else {
    if (!btn_last) {
      bleKeyboard.press_set(0x61);
      btn_last = 1;
    }
  }
  delay(5);
}
