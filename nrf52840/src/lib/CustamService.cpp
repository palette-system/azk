
#include "bluefruit.h"
#include "CustamService.h"

int write_index;

BLECharacteristic *_characteristic_input;
BLECharacteristic *_characteristic_output;

// ブラウザからデータを受け取った
void BLECustam::onCommandWritten(uint16_t conn_hdl, BLECharacteristic* characteristic, uint8_t* data, uint16_t data_length) {
	int i;
	memcpy(remap_buf, data, data_length);

    // 省電力モードの場合解除
    if (hid_power_saving_mode == 1 && hid_power_saving_state == 1) { // 省電力モードON で、現在の動作モードが省電力
        hid_power_saving_state = 2;
    }

    if (remap_buf[0] == id_get_file_data) {
		// 0x31 ファイルデータ要求
		int s, p, h, l, m, j;
		// 情報を取得
		s = remap_buf[1]; // ステップ数
		p = (remap_buf[2] << 16) + (remap_buf[3] << 8) + remap_buf[4]; // 読み込み開始位置
		h = (remap_buf[5] << 24) + (remap_buf[6] << 16) + (remap_buf[7] << 8) + remap_buf[8]; // ハッシュ値
		if (h != 0) {
			l = s * (data_length - 4); // ステップ数 x 1コマンドで送るデータ数
			m = azcrc32(&save_file_data[p - l], l); // 前回送った所のハッシュを計算
			if (h != m) { // ハッシュ値が違えば前に送った所をもう一回送る
				// Serial.printf("NG : [%d %d] [ %d -> %d ]\n", h, m, p, (p - l));
				p = p - l;
			}
		}
		j = 0;
		for (j=0; j<s; j++) {
			send_buf[0] = id_get_file_data;
			send_buf[1] = ((p >> 16) & 0xff);
			send_buf[2] = ((p >> 8) & 0xff);
			send_buf[3] = (p & 0xff);
			i = 4;
			while (p < save_file_length) {
				send_buf[i] = save_file_data[p];
				i++;
				p++;
				if (i >= 32) break;
			}
			while (i<32) {
				send_buf[i] = 0x00;
				i++;
			}
            _characteristic_input->notify(send_buf, 32);
			if (p >= save_file_length) break;

		}
		if (p >= save_file_length) {
			// Serial.printf("free load: %d %d\n", save_file_length, heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
			free(save_file_data);
		}

	} else {
		// それ以外は共通処理
		HidrawCallbackExec(data_length);
		// 返信データ送信
		if (send_buf[0]) {
            // ble_gatt.h の BLE_GATT_ATT_MTU_DEFAULT がデフォルト 23 を 35 にしないと 送信する時 20 で通知が行ってしまう
            _characteristic_input->notify(send_buf, 32);
		}
	}
}

  
BLECustam::BLECustam(void) :
  BLEService(CUSTAM_UUID_SERVICE)
{
  
}

err_t BLECustam::begin(void)
{
    _characteristic_input = new BLECharacteristic(CUSTAM_UUID_INPUT, BLERead | BLENotify, 32, true); // UUID, パーミッション, データサイズ, データサイズ固定かどうか
    _characteristic_output = new BLECharacteristic(CUSTAM_UUID_OUTPUT, BLEWrite, 32, true); // UUID, パーミッション, データサイズ, データサイズ固定かどうか
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
