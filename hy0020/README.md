



# 動作確認手順書を参考にarduino開発環境を作成
https://akizukidenshi.com/catalog/g/g131342/
```
https://adafruit.github.io/arduino-board-index/package_adafruit_index.json
```



# 下記のファイルの g_ADigitalPinMap で digitalWrite とかで使用する番号の定義がある
```
C:\Users\user\AppData\Local\Arduino15\packages\adafruit\hardware\nrf52\1.7.0\variants\feather_nrf52832\variant.cpp
```


# ライブラリ
```
ArduinoJson by Benoit Blanchon version 7.4.3
Adafruit SSD 1306 by Adafruit version 2.5.17
Adafruit NeoPixel by Adafruit version 1.15.5
Adafruit MCP23017 Arduino Library by Adafruit version 2.3.2
```


# ピン番号
渡す番号がどのGPIOか g_ADigitalPinMap[] に定義してある。ボードによって中身が違うぽいんだけど、XIAO nRF52832 の定義どこにしてあるのか分からなかったから中に何が入ってるか調べた結果をメモしておく

```
const uint32_t g_ADigitalPinMap[] = {
  // D0 - D7
  0,  // xtal 1
  1,  // xtal 2
  2,  // a0
  3,  // a1
  4,  // a2
  5,  // a3
  6,  // TXD
  7,  // GPIO #7

  // D8 - D13
  8,  // RXD

  9,  // NFC1
  10, // NFC2

  11, // GPIO11

  12, // SCK
  13, // MOSI
  14, // MISO

  15, // GPIO #15
  16, // GPIO #16

  // function set pins
  17, // LED #1 (red)
  18, // SWO
  19, // LED #2 (blue)
  20, // DFU
  21, // Reset
  22, // Factory Reset
  23, // N/A
  24, // N/A

  25, // SDA
  26, // SCL
  27, // GPIO #27
  28, // A4
  29, // A5
  30, // A6
  31, // A7
};

```

