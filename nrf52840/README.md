

# 事前に必要
```
python 3.13
Arduino IDE 2.3.3
```


# arduinoIDE設定に追加するJSON
https://wiki.seeedstudio.com/XIAO_BLE/
```
https://files.seeedstudio.com/arduino/package_seeeduino_boards_index.json
```


# ボード
```
Seed nRF52 Boards by Seeed Studio 1.1.10
選択するボード ＞ Seeed XIAO nRF52840 Plus
```


# 下記ファイルの BLE_GATT_ATT_MTU_DEFAULT がデフォルト 23 を 35 にしないと 送信する時 20 で通知が行ってしまう

```
C:\Users\user\AppData\Local\Arduino15\packages\Seeeduino\hardware\nrf52\1.1.10\cores\nRF5\nordic\softdevice\s140_nrf52_7.3.0_API\include\ble_gatt.h
```

# 下記のファイルの g_ADigitalPinMap で digitalWrite とかで使用する番号の定義がある
```
c:\Users\user\AppData\Local\Arduino15\packages\Seeeduino\hardware\nrf52\1.1.10\variants\Seeed_XIAO_nRF52840\variant.cpp
c:\Users\user\AppData\Local\Arduino15\packages\Seeeduino\hardware\nrf52\1.1.10\variants\Seeed_XIAO_nRF52840\variant.h
```


# ライブラリ
```
ArduinoJson by Benoit Blanchon version 7.4.1
Adafruit SSD 1306 by Adafruit version 2.5.13
Adafruit NeoPixel by Adafruit version 1.12.5
Adafruit MCP23017 Arduino Library by Adafruit version 2.3.2
```


# hexからuf2出力

```
python.exe uf2conv.py nrf52840.ino.hex --family 0xADA52840 --convert --output azk_nrf52840.uf2
```

# ピン番号
渡す番号がどのGPIOか g_ADigitalPinMap[] に定義してある。ボードによって中身が違うぽいんだけど、XIAO nRF52840 の定義どこにしてあるのか分からなかったから中に何が入ってるか調べた結果をメモしておく

```
0 -> 2, (P0.2) GPIO 0
1 -> 3, (P0.3) GPIO 1
2 -> 28, (P0.28) GPIO 2
3 -> 29, (P0.29) GPIO 3
4 -> 4, (P0.4) GPIO 4
5 -> 5, (P0.5) GPIO 5
6 -> 43, (P1.11) GPIO 6
7 -> 44, (P1.12) GPIO 7
8 -> 45, (P1.13) GPIO 8
9 -> 46, (P1.14) GPIO 9
10 -> 47, (P1.15) GPIO 10

11 -> 26, (LED RED)
12 -> 6, (LED BLUE)
13 -> 30, (LED GREEN)

14 -> 14, (P0.14  BAT) これでバッテリー残量を取得?

15 -> 40, (P1.8) 6D_PWR I2C (これをONにするとI2Cをプルアップできる)
16 -> 27, (P0.27) INTERNAL_I2C_SCL 
17 -> 7, (P0.7) INTERNAL_I2C_SDA
18 -> 11, (P0.11) 6D_INT1 I2C SDA? SCL?

19 -> 42, (P1.10) MIC_PWR  MIC?
20 -> 32, (P1.0) PDM_CLK   MIC?
21 -> 16, (P0.16) PDM_DATA   MIC?

22 -> 13, (P0.13) HICHG (BAT)
23 -> 17, (P0.17) CHG (BAT)

24 -> 21, (P0.21) QSPI_SCK
25 -> 25, (P0.25) QSPI_CSN
26 -> 20, (P0.20) QSPI_SIO_0
27 -> 24, (P0.24) QSPI_SIO_1
28 -> 22, (P0.22) QSPI_SIO_2
29 -> 23, (P0.23) QSPI_SIO_3

30 -> 9, (P0.9) NFC1
31 -> 10, (P0.10) NFC2
32 -> 31, (P0.31) AIN7_BAT (これも使ってバッテリー残量取得)

```

