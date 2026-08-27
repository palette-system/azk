

#include "ble_keyboard_jis.h"


// コンストラクタ
BleKeyboardJIS::BleKeyboardJIS(void)
{
};


// BLEキーボードとして処理開始
void BleKeyboardJIS::begin(char *deviceName)
{
    uint8_t  child_file_data[16]; // 前に接続した子端末のアドレスデータ
    char addr_path[16];
    short r;
    sprintf(addr_path, "/child");

    if (ble_type == 1) { // 分割：親
      Bluefruit.begin(1, 1); // HID (1 接続) , BLE クライアント(1 接続)
    } else {
      Bluefruit.begin(1); // HID (1 接続)
    }
    Bluefruit.setTxPower(4);    // Check bluefruit.h for supported values
    Bluefruit.setName(deviceName);
    Bluefruit.getAddr(my_addr); // 自分のアドレス取得

    r = common_cls.read_file(addr_path, child_file_data);
    if (r > 0) {
      if (addr_check(child_file_data, my_addr)) {
        // 頭6バイトが自分のアドレスなら子端末のアドレスを取得
        child_addr_flag = true; // 子供アドレス取得したフラグ
        addr_copy(child_addr, &child_file_data[6]); // 子供アドレス保持
      } else {
        // 頭6バイトが自分のアドレスじゃない場合、別の端末の設定をインポートしたデータなので削除
        common_cls.remove_file(addr_path);
      }
    }

    if (ble_type == 1) { // 分割：親
      // Init BLE Central Uart Serivce
      clientUart.begin();
      clientUart.setRxCallback(bleuart_rx_callback);

      // Increase Blink rate to different from PrPh advertising mode
      // Bluefruit.setConnLedInterval(250);

      // BLE クライアント コールバック設定
      Bluefruit.Central.setConnectCallback(client_connect_callback);
      Bluefruit.Central.setDisconnectCallback(client_disconnect_callback);

      // BLE クライアント 設定
      Bluefruit.Scanner.setInterval(160, 80);       // in units of 0.625 ms
      Bluefruit.Scanner.setRxCallback(scan_callback); // アドバタイズ端末を見つけた時に実行される関数を設定
      Bluefruit.Scanner.filterUuid(BLEUART_UUID_SERVICE); // 指定したサービスがある端末だけスキャンする
      Bluefruit.Scanner.restartOnDisconnect(true); // 接続が切断されたらスキャン再開
      Bluefruit.Scanner.useActiveScan(false);       // アドバタイズ端末を見つけた時に追加情報要求を送るか

      // 子供アドレスが設定されていればスキャン開始
      if (child_addr_flag || strlen(child_name)) {
        Bluefruit.Scanner.start(0); // 指定したミリ秒間だけスキャンをする 0=ずっとスキャン
      }
    }

    // Configure and Start Device Information Service
    bledis.setManufacturer("AZKEYBOARD");
    bledis.setModel("AZK");
    uint8_t sig = 0x02;
    uint16_t hid_vid = 0xe502;
    uint16_t hid_pid = 0x0200;
    uint16_t version = 0x0001;
    uint8_t pnp[] = {
      sig,
      (uint8_t) (hid_vid >> 8), (uint8_t) hid_vid,
      (uint8_t) (hid_pid >> 8), (uint8_t) hid_pid,
      (uint8_t) (version >> 8), (uint8_t) version
    };
    bledis.setPNPID((const char*)pnp, sizeof(pnp));
    // Bluefruit.configAttrTableSize(64);
    // Bluefruit.configPrphBandwidth(BANDWIDTH_NORMAL);
    // Bluefruit.Security.setIOCaps(false, false, false);
    // Bluefruit.Security.setMITM(true);
    bledis.begin();
  
    /* Start BLE HID
     * Note: Apple requires BLE device must have min connection interval >= 20m
     * ( The smaller the connection interval the faster we could send data).
     * However for HID and MIDI device, Apple could accept min connection interval 
     * up to 11.25 ms. Therefore BLEHidAdafruit::begin() will try to set the min and max
     * connection interval to 11.25  ms and 15 ms respectively for best performance.
     */
    if (ble_type == 0 || ble_type == 1) { // シングル / 分割：親
      blehid.begin();
  
      // Set callback for set LED from central
      blehid.setKeyboardLedCallback(set_keyboard_led);
    }
  
    if (ble_type == 2) { // 分割：小
      // Configure and Start BLE Uart Service
      bleuart.begin();
      bleuart.setRxCallback(prph_bleuart_rx_callback);
    }

    if (ble_type == 0 || ble_type == 1) { // シングル / 分割：親
      blecus.begin();
    }

  
    /* Set connection interval (min, max) to your perferred value.
     * Note: It is already set by BLEHidAdafruit::begin() to 11.25ms - 15ms
     * min = 9*1.25=11.25 ms, max = 12*1.25= 15 ms 
     */
    /* Bluefruit.Periph.setConnInterval(9, 12); */
  
    // Set up and start advertising
    startAdv();
  
};


void BleKeyboardJIS::startAdv(void)
{  
  // Advertising packet
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  if (ble_type == 0 || ble_type == 1) { // シングル / 分割：親
    Bluefruit.Advertising.addAppearance(BLE_APPEARANCE_HID_KEYBOARD);
  }
  
  // Include BLE HID service
  if (ble_type == 0 || ble_type == 1) { // シングル / 分割：親
    Bluefruit.Advertising.addService(blehid); // HID
  }
  if (ble_type == 2) { // 分割：小
    Bluefruit.Advertising.addService(bleuart); // Uart
  }
  if (ble_type == 0 || ble_type == 1) { // シングル / 分割：親
    Bluefruit.Advertising.addService(blecus); // Custam
  }

  // There is enough room for the dev name in the advertising packet
  Bluefruit.Advertising.addName();
  
  /* Start Advertising
   * - Enable auto advertising if disconnected
   * - Interval:  fast mode = 20 ms, slow mode = 152.5 ms
   * - Timeout for fast mode is 30 seconds
   * - Start(timeout) with timeout = 0 will advertise forever (until connected)
   * 
   * For recommended advertising interval
   * https://developer.apple.com/library/content/qa/qa1931/_index.html   
   */
  Bluefruit.Advertising.restartOnDisconnect(true); // 接続が切れたら再度アドバタイズ開始する
  Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
  Bluefruit.Advertising.start(0);                // 起動してから何秒間接続先を探すか 0はずっと探す
}

/**
 * Callback invoked when received Set LED from central.
 * Must be set previously with setKeyboardLedCallback()
 *
 * The LED bit map is as follows: (also defined by KEYBOARD_LED_* )
 *    Kana (4) | Compose (3) | ScrollLock (2) | CapsLock (1) | Numlock (0)
 */
void BleKeyboardJIS::set_keyboard_led(uint16_t conn_handle, uint8_t led_bitmap)
{
  led_map = led_bitmap;
}


// 接続中かどうかを返す
bool BleKeyboardJIS::isConnected(void)
{
    return (Bluefruit.connected() > 0);
};


unsigned short BleKeyboardJIS::modifiers_press(unsigned short k) {
  this->setConnInterval(0); // 消費電力モード解除
  if (k & JIS_SHIFT) { // shift
    this->_keyReport.modifier |= 0x02; // the left shift modifier
    k &= 0xFF;
  }
  if (k == 224) { k = 0; this->_keyReport.modifier |= 0x01; } // LEFT Ctrl
  if (k == 228) { k = 0; this->_keyReport.modifier |= 0x10; } // RIGHT Ctrl
  if (k == 225) { k = 0; this->_keyReport.modifier |= 0x02; } // LEFT Shift
  if (k == 229) { k = 0; this->_keyReport.modifier |= 0x20; } // RIGHT Shift
  if (k == 226) { k = 0; this->_keyReport.modifier |= 0x04; } // LEFT Alt
  if (k == 230) { k = 0; this->_keyReport.modifier |= 0x40; } // RIGHT Alt
  if (k == 227) { k = 0; this->_keyReport.modifier |= 0x08; } // LEFT GUI
  if (k == 231) { k = 0; this->_keyReport.modifier |= 0x80; } // RIGHT GUI
  return k;
};


unsigned short BleKeyboardJIS::modifiers_release(unsigned short k) {
  if (k & JIS_SHIFT) { // shift
    this->_keyReport.modifier &= ~(0x02);  // the left shift modifier
    k &= 0xFF;
  }
  if (k == 224) { k = 0; this->_keyReport.modifier &= ~(0x01); } // LEFT Ctrl
  if (k == 228) { k = 0; this->_keyReport.modifier &= ~(0x10); } // RIGHT Ctrl
  if (k == 225) { k = 0; this->_keyReport.modifier &= ~(0x02); } // LEFT Shift
  if (k == 229) { k = 0; this->_keyReport.modifier &= ~(0x20); } // RIGHT Shift
  if (k == 226) { k = 0; this->_keyReport.modifier &= ~(0x04); } // LEFT Alt
  if (k == 230) { k = 0; this->_keyReport.modifier &= ~(0x40); } // RIGHT Alt
  if (k == 227) { k = 0; this->_keyReport.modifier &= ~(0x08); } // LEFT GUI
  if (k == 231) { k = 0; this->_keyReport.modifier &= ~(0x80); } // RIGHT GUI
  return k;
};

// Shiftを離す
void BleKeyboardJIS::shift_release() {
  int i;
  this->_keyReport.modifier &= ~(0x22);
  for (i=0; i<6; i++) {
    if (this->_keyReport.keycode[i] == 225 || this->_keyReport.keycode[i] == 229) {
      this->_keyReport.keycode[i] = 0;
    }
  }
}

unsigned short BleKeyboardJIS::modifiers_media_press(unsigned short k) {
  this->setConnInterval(0); // 消費電力モード解除
  if (k == 8193) { // Eject
    this->_mediaKeyReport[0] |= 0x01;
    this->sendReport(&this->_mediaKeyReport);
    return 1;
  } else if (k == 8194) { // Media Next
    this->_mediaKeyReport[0] |= 0x02;
    this->sendReport(&this->_mediaKeyReport);
    return 1;
  } else if (k == 8195) { // Media Previous
    this->_mediaKeyReport[0] |= 0x04;
    this->sendReport(&this->_mediaKeyReport);
    return 1;
  } else if (k == 8196) { // Media Stop
    this->_mediaKeyReport[0] |= 0x08;
    this->sendReport(&this->_mediaKeyReport);
    return 1;
  } else if (k == 8197) { // Media play / pause
    this->_mediaKeyReport[0] |= 0x10;
    this->sendReport(&this->_mediaKeyReport);
    return 1;
  } else if (k == 8198) { // Media Mute
    this->_mediaKeyReport[0] |= 0x20;
    this->sendReport(&this->_mediaKeyReport);
    return 1;
  } else if (k == 8199) { // Media volume +
    this->_mediaKeyReport[0] |= 0x40;
    this->sendReport(&this->_mediaKeyReport);
    return 1;
  } else if (k == 8200) { // Media volume -
    this->_mediaKeyReport[0] |= 0x80;
    this->sendReport(&this->_mediaKeyReport);
    return 1;
  }
  return 0;
};

unsigned short BleKeyboardJIS::modifiers_media_release(unsigned short k) {
  if (k == 8193) { // Eject
    this->_mediaKeyReport[0] &= ~(0x01);
    this->sendReport(&this->_mediaKeyReport);
    return 1;
  } else if (k == 8194) { // Media Next
    this->_mediaKeyReport[0] &= ~(0x02);
    this->sendReport(&this->_mediaKeyReport);
    return 1;
  } else if (k == 8195) { // Media Previous
    this->_mediaKeyReport[0] &= ~(0x04);
    this->sendReport(&this->_mediaKeyReport);
    return 1;
  } else if (k == 8196) { // Media Stop
    this->_mediaKeyReport[0] &= ~(0x08);
    this->sendReport(&this->_mediaKeyReport);
    return 1;
  } else if (k == 8197) { // Media play / pause
    this->_mediaKeyReport[0] &= ~(0x10);
    this->sendReport(&this->_mediaKeyReport);
    return 1;
  } else if (k == 8198) { // Media Mute
    this->_mediaKeyReport[0] &= ~(0x20);
    this->sendReport(&this->_mediaKeyReport);
    return 1;
  } else if (k == 8199) { // Media volume +
    this->_mediaKeyReport[0] &= ~(0x40);
    this->sendReport(&this->_mediaKeyReport);
    return 1;
  } else if (k == 8200) { // Media volume -
    this->_mediaKeyReport[0] &= ~(0x80);
    this->sendReport(&this->_mediaKeyReport);
    return 1;
  }
  return 0;
};

void BleKeyboardJIS::sendReport(hid_keyboard_report_t* keys)
{
    this->setConnInterval(0); // 消費電力モード解除
    blehid.keyboardReport(keys);
    // this->pInputCharacteristic->setValue((uint8_t*)keys, sizeof(KeyReport));
    // this->pInputCharacteristic->notify();
};

void BleKeyboardJIS::sendReport(MediaKeyReport* keys)
{
  this->setConnInterval(0); // 消費電力モード解除
  if (this->isConnected())
  {
    // this->pInputCharacteristic2->setValue((uint8_t*)keys, sizeof(MediaKeyReport));
    // this->pInputCharacteristic2->notify();
    // uint16_t s;
    // s = keys[1];
    // blehid.consumerReport(keys[0]);
  }
};

void BleKeyboardJIS::mouse_click(uint8_t b)
{
    this->_MouseButtons = b;
    this->mouse_move(0,0,0,0);
    delay(10);
    this->_MouseButtons = 0x00;
    this->mouse_move(0,0,0,0);
};


void BleKeyboardJIS::mouse_press(uint8_t b)
{
    this->_MouseButtons |= b;
    this->mouse_move(0,0,0,0);
};

void BleKeyboardJIS::mouse_release(uint8_t b)
{
    this->_MouseButtons &= ~(b);
    this->mouse_move(0,0,0,0);
};

void BleKeyboardJIS::mouse_move(signed char x, signed char y, signed char wheel, signed char hWheel)
{
  if (this->isConnected()) {
        if (x != 0 || y != 0 || wheel != 0 || hWheel != 0 || this->_MouseButtons != 0) {
            this->setConnInterval(0); // 消費電力モード解除
        }
        /*
        uint8_t m[5];
        m[0] = this->_MouseButtons;
        m[1] = x;
        m[2] = y;
        m[3] = wheel;
        m[4] = hWheel;
        */
        blehid.mouseReport(this->_MouseButtons, x, y, wheel, hWheel);
        // this->pInputCharacteristic3->setValue(m, 5);
        // this->pInputCharacteristic3->notify();
    }
};

size_t BleKeyboardJIS::press_raw(unsigned short k)
{
  uint8_t i;
  unsigned short kk;
  // メディアキー
  if (modifiers_media_press(k)) return 1;
  kk = this->modifiers_press(k);
  if (this->_keyReport.keycode[0] != kk && this->_keyReport.keycode[1] != kk &&
    this->_keyReport.keycode[2] != kk && this->_keyReport.keycode[3] != kk &&
    this->_keyReport.keycode[4] != kk && this->_keyReport.keycode[5] != kk) {

    for (i=0; i<6; i++) {
      if (this->_keyReport.keycode[i] == 0x00) {
        this->_keyReport.keycode[i] = kk;
        break;
      }
    }
    if (i == 6) {
      return 0;
    }
  }
  this->sendReport(&_keyReport);
  return 1;
};

size_t BleKeyboardJIS::press_set(uint8_t k)
{
  unsigned short kk;
  kk = _asciimap[k];
  if (!kk) {
    return 0;
  }
  this->_keyReport.modifier = 0x00;
  kk = this->modifiers_press(kk);
  this->_keyReport.keycode[0] = kk;
  this->_keyReport.keycode[1] = 0x00;
  this->_keyReport.keycode[2] = 0x00;
  this->_keyReport.keycode[3] = 0x00;
  this->_keyReport.keycode[4] = 0x00;
  this->_keyReport.keycode[5] = 0x00;

  this->sendReport(&this->_keyReport);
  return 1;
};

size_t BleKeyboardJIS::release_raw(unsigned short k)
{
  uint8_t i;
  unsigned short kk;
  // メディアキー
  if (modifiers_media_release(k)) return 1;
  kk = this->modifiers_release(k);

  // Test the key report to see if k is present.  Clear it if it exists.
  // Check all positions in case the key is present more than once (which it shouldn't be)
  for (i=0; i<6; i++) {
    if (0 != kk && this->_keyReport.keycode[i] == kk) {
      this->_keyReport.keycode[i] = 0x00;
    }
  }

  this->sendReport(&this->_keyReport);
  return 1;
};

void BleKeyboardJIS::releaseAll(void)
{
  this->_keyReport.keycode[0] = 0;
  this->_keyReport.keycode[1] = 0;
  this->_keyReport.keycode[2] = 0;
  this->_keyReport.keycode[3] = 0;
  this->_keyReport.keycode[4] = 0;
  this->_keyReport.keycode[5] = 0;
  this->_keyReport.modifier = 0;
  this->_mediaKeyReport[0] = 0;
  this->_mediaKeyReport[1] = 0;
  this->sendReport(&this->_keyReport);
  this->sendReport(&this->_mediaKeyReport);
};

// Shiftが押されている状態かどうか(物理的に)
bool BleKeyboardJIS::onShift()
{
  int i;
  for (i=0; i<PRESS_KEY_MAX; i++) {
    if (press_key_list[i].key_num < 0) continue; // 押されたよデータ無ければ無視
    if (press_key_list[i].unpress_time > 0) continue; // 離したよカウントが始まっていたら押していないので無視
    if (press_key_list[i].key_id == 225 || press_key_list[i].key_id == 229) return true; // ShiftコードならばShiftが押されている
  }
  return false;
}

// コネクションインターバル設定
void BleKeyboardJIS::setConnInterval(int interval_type)
{
  // if (hid_power_saving_mode == 0) return; // 通常モードなら何もしない
  // hid_state_change_time = millis() + hid_saving_time;
  // if (hid_power_saving_state == interval_type) return; // ステータスの変更が無ければ何もしない
  // hid_power_saving_state = interval_type;
  // if (hid_interval_saving == hid_interval_normal) return; // 省電力モードのインターバルと通常モードのインターバルが一緒なら何もしない
  // if (!this->isConnected()) return; // 接続していなければ何もしない
  // if (interval_type == 1) {
    // 省電力中
    // loop_delay = 100;
    // this->pServer->updateConnParams(hid_conn_handle, hid_interval_saving - 2, hid_interval_saving + 2, 0, 200);
  // } else {
    // 通常
    // loop_delay = loop_delay_default;
    // this->pServer->updateConnParams(hid_conn_handle, hid_interval_normal - 2, hid_interval_normal + 2, 0, 200);
  // }
    
}

// 分割：小 の入力キーを親に送る
void BleKeyboardJIS::split_send_key()
{
  short i, p;
  uint8_t send_data[12];
  memset(send_data, 0x00, 12);
  send_data[0] = id_send_child_key;
  p = 0;
  for (i=0; i<key_input_length; i++) {
      if (common_cls.input_key[i]) {
        if (p<6) send_data[p+2] = i;
        p++;
      }
  }
  send_data[1] = p;
  bleuart.write(send_data, p + 2);
}
