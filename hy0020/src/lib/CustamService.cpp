
#include "bluefruit.h"
#include "CustamService.h"

int write_index;

BLECustam blecus; // 送受信用サービス

BLECharacteristic *_characteristic_input;
BLECharacteristic *_characteristic_output;

// ブラウザからデータを受け取った
void prph_bleuart_rx_callback(uint16_t conn_handle)
{
	(void) conn_handle;
	int s, p, h, l, m, i, j;
	uint16_t data_length = INPUT_REPORT_RAW_MAX_LEN;
	memset(remap_buf, 0, INPUT_REPORT_RAW_MAX_LEN);
	p = 0;
	while (bleuart.available() && p < INPUT_REPORT_RAW_MAX_LEN) {
		remap_buf[p] = (uint8_t)bleuart.read();
		p++;
	}
  
	// それ以外は共通処理
	HidrawCallbackExec(data_length);
	// 返信データ送信
	if (send_buf[0]) {
		// ble_gatt.h の BLE_GATT_ATT_MTU_DEFAULT がデフォルト 23 を 35 にしないと 送信する時 20 で通知が行ってしまう
		bleuart.write(send_buf, OUTPUT_REPORT_RAW_MAX_LEN);
	}
}

void BLECustam::onCommandWritten(uint16_t conn_hdl, BLECharacteristic* characteristic, uint8_t* data, uint16_t data_length) {
	int i;
	memcpy(remap_buf, data, data_length);

    // 省電力モードの場合解除
    if (hid_power_saving_mode == 1 && hid_power_saving_state == 1) { // 省電力モードON で、現在の動作モードが省電力
        hid_power_saving_state = 2;
    }

	// それ以外は共通処理
	HidrawCallbackExec(data_length);
	// 返信データ送信
	if (send_buf[0]) {
		// ble_gatt.h の BLE_GATT_ATT_MTU_DEFAULT がデフォルト 23 を 35 にしないと 送信する時 20 で通知が行ってしまう
		_characteristic_input->notify(send_buf, OUTPUT_REPORT_RAW_MAX_LEN);
	}
}


BLECustam::BLECustam(void) :
  BLEService(CUSTAM_UUID_SERVICE)
{
  
}

err_t BLECustam::begin(void)
{
    _characteristic_input = new BLECharacteristic(CUSTAM_UUID_INPUT, BLERead | BLENotify, INPUT_REPORT_RAW_MAX_LEN, true); // UUID, パーミッション, データサイズ, データサイズ固定かどうか
    _characteristic_output = new BLECharacteristic(CUSTAM_UUID_OUTPUT, BLEWrite, OUTPUT_REPORT_RAW_MAX_LEN, true); // UUID, パーミッション, データサイズ, データサイズ固定かどうか
    write_index = 0;
  // Invoke base class begin()
  VERIFY_STATUS( BLEService::begin() );

  // ブラウザからデータ受け取る用のcharacteristic
  _characteristic_output->setWriteCallback(onCommandWritten, true); // データを受け取った時のイベント登録
  VERIFY_STATUS( _characteristic_output->begin() );

  // XIAOからブラウザにデータを送る用のcharacteristic
  VERIFY_STATUS( _characteristic_input->begin() );

  return ERROR_NONE;
}

bool BLECustam::write(const char* str)
{
  return _characteristic_input->write(str) > 0;
}

bool BLECustam::notify(uint8_t level)
{
    int i;
    uint8_t data[4];
    for (i=0; i<4; i++) {
        data[i] = (write_index << 6) + (i << 4) + level;
    }
  
    return _characteristic_input->notify(data, 4);
}

bool BLECustam::notify(uint16_t conn_hdl, uint8_t level)
{
  return _characteristic_input->notify8(conn_hdl, level);
}

