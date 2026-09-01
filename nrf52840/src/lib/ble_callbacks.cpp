
#include "Arduino.h"
#include "ble_callbacks.h"

// HID キーボード
BLEDis bledis; // BLE 情報サービス
BLEHidAdafruit blehid; // キーボードサービス
BLEUart bleuart; // uart over ble

// BLE Uart Dis クライアント
BLEClientDis  clientDis;  // device information client
BLEClientUart clientUart; // bleuart client

BLECharacteristic *_characteristic_input;
BLECharacteristic *_characteristic_output;

uint8_t *check_addr;



/* ====================================================================================================================== */
/** コールバック用共通 関数 */
/* ====================================================================================================================== */

static int _lfs_count(void *p, lfs_block_t block) {
  (void) block;
  *(size_t *)p += 1;
  return 0;
};

// ステップ分受信したか確認
int check_step() {
	int i, r = 0;
	for (i=0; i<8; i++) {
		if (save_step_flag[i]) r++;
	}
	return r;
};

// アドレス設定が無いか確認
bool addr_is_none(uint8_t *addr_a) {
  short i;
  for (i=0; i<BLE_GAP_ADDR_LEN; i++) {
    if (addr_a[i] != 0) return false;
  }
  return true;
}

// アドレス２つ比較
bool addr_check(uint8_t *addr_a, uint8_t *addr_b) {
  short i;
  for (i=0; i<BLE_GAP_ADDR_LEN; i++) {
    if (addr_a[i] != addr_b[i]) return false;
  }
  return true;
}

// アドレスコピー
void addr_copy(uint8_t *addr_a, uint8_t *addr_b) {
  short i;
  for (i=0; i<BLE_GAP_ADDR_LEN; i++) {
    addr_a[i] = addr_b[i];
  }
}

/* ====================================================================================================================== */
/** HID RAW コールバック用 クラス */
/* ====================================================================================================================== */

void HidrawCallbackExec(int data_length) {
	int h, i, j, k, l, m, s, o, p, x;
    uint8_t *command_id   = &(remap_buf[0]);
	tracktall_pim447_data pim447_data_obj;
	trackpad_cst816_data cst816_data_obj;

	// 設定変更がされていて設定変更以外のコマンドが飛んできたら設定を保存
	if (remap_change_flag && *command_id != 0x05) {
		remap_change_flag = 0;
	}
	
	switch (*command_id) {



		case id_get_file_start: { // 0x30 ファイル取得開始
			// ファイル名を取得
			i = 1;
			while (remap_buf[i]) {
				target_file_path[i - 1] = remap_buf[i];
				i++;
				if (i >= OUTPUT_REPORT_RAW_MAX_LEN) break;
			}
			target_file_path[i - 1] = 0x00;

		    // ファイルが無ければ0を返す
			if (!InternalFS.exists(target_file_path)) {
				send_buf[0] = id_get_file_start;
				for (i=1; i<OUTPUT_REPORT_RAW_MAX_LEN; i++) send_buf[i] = 0x00;
				return;
			}
            File fp = InternalFS.open(target_file_path, FILE_O_READ);
			save_file_length = fp.size();
			// Serial.printf("ps_malloc load: %s %d %d\n", target_file_path, save_file_length, heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
			save_file_data = (uint8_t *)malloc(save_file_length);
			fp.read(save_file_data, save_file_length);
			fp.close();
			send_buf[0] = id_get_file_start;
			send_buf[1] = 0x01; // ファイルは存在する
			send_buf[2] = ((save_file_length >> 24) & 0xff);
			send_buf[3] = ((save_file_length >> 16) & 0xff);
			send_buf[4] = ((save_file_length >> 8) & 0xff);
			send_buf[5] = (save_file_length & 0xff);
			for (i=6; i<OUTPUT_REPORT_RAW_MAX_LEN; i++) send_buf[i] = 0x00;
			return;
		    
		}
		case id_get_file_data: { // 0x31 ファイルデータ要求
			// 情報を取得
			s = remap_buf[1]; // ステップ数
			p = (remap_buf[2] << 16) + (remap_buf[3] << 8) + remap_buf[4]; // 読み込み開始位置
			h = (remap_buf[5] << 24) + (remap_buf[6] << 16) + (remap_buf[7] << 8) + remap_buf[8]; // ハッシュ値
			if (h != 0) {
				l = (OUTPUT_REPORT_RAW_MAX_LEN - 4); // ステップ数 x 1コマンドで送るデータ数
				m = azcrc32(&save_file_data[p - l], l); // 前回送った所のハッシュを計算
				if (h != m) { // ハッシュ値が違えば前に送った所をもう一回送る
					p = p - l;
				}
			}
			send_buf[0] = id_get_file_data;
			send_buf[1] = ((p >> 16) & 0xff);
			send_buf[2] = ((p >> 8) & 0xff);
			send_buf[3] = (p & 0xff);
			i = 4;
			while (p < save_file_length) {
				send_buf[i] = save_file_data[p];
				i++;
				p++;
				if (i >= OUTPUT_REPORT_RAW_MAX_LEN) break;
			}
			while (i < OUTPUT_REPORT_RAW_MAX_LEN) {
				send_buf[i] = 0x00;
				i++;
			}

			if (p >= save_file_length) {
				free(save_file_data);
			}
			return;

		}
		case id_save_file_start: { // 0x32 ファイル保存開始
		    // 容量を取得
			save_file_length = (remap_buf[1] << 24) + (remap_buf[2] << 16) + (remap_buf[3] << 8) + remap_buf[4];
			// 保存時のステップ数
			save_file_step = remap_buf[5];
			// ファイル名を取得
			i = 6;
			while (remap_buf[i]) {
				target_file_path[i - 6] = remap_buf[i];
				i++;
				if (i >= OUTPUT_REPORT_RAW_MAX_LEN) break;
			}
			target_file_path[i - 6] = 0x00;
			// データ受け取りバッファクリア
			// for (i=0; i<512; i++) save_file_data[i] = 0x00;
			// 取得したステップのインデックス
			for (i=0; i<8; i++) save_step_flag[i] = false;
			// ファイルオープン
			// Serial.printf("ps_malloc save: %d %d\n", save_file_length, heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
			save_file_data = (uint8_t *)malloc(save_file_length);
			// open_file = SPIFFS.open(target_file_path, "w");
			// データ要求コマンド送信
			send_buf[0] = id_save_file_data;
			send_buf[1] = save_file_step;
			for (i=2; i<OUTPUT_REPORT_RAW_MAX_LEN; i++) send_buf[i] = 0x00;
			return;

		}
		case id_save_file_data: { // 0x33 ファイルデータ受け取り
		    s = remap_buf[1]; // 何ステップ目のデータか
			j = (remap_buf[2] << 16) + (remap_buf[3] << 8) + remap_buf[4]; // 何処開始のデータか
			m = data_length - 5; // データの長さ
			// バッファにデータを貯める
			p = s * m; // バッファの書込み開始位置
			// Serial.printf("save: %d %d %d\n", j, p, m);
			for (i=0; i<m; i++) {
				// save_file_data[p + i] = remap_buf[i + 5];
				if ((j + p + i) >= save_file_length) break;
				save_file_data[j + p + i] = remap_buf[i + 5];
			}
			// ステップのインデックス加算
			save_step_flag[s] = true;
			// 全ステップ取得した
			if (check_step() >= save_file_step) {
				// ステップインデックスをリセット
				for (i=0; i<8; i++) save_step_flag[i] = false;
				// バッファに入ったデータをファイルに書き出し
				l = m * save_file_step; // 書込みを行うサイズ
				k = j + l; // 書込み後のシークポイント
				// 書込みサイズが保存予定のサイズを超えたら超えない数値にする
				if (k > save_file_length) {
					l = save_file_length - j;
					k = j + l;
				}
				// 書き込む
				h = azcrc32(&save_file_data[j], l);
				if (k < save_file_length) {
					// まだデータを全部受け取って無ければ次を要求するコマンドを送信
					send_buf[0] = id_save_file_data;
					send_buf[1] = save_file_step;
					send_buf[2] = (k >> 24) & 0xff; // データの開始位置 1
					send_buf[3] = (k >> 16) & 0xff; // データの開始位置 2
					send_buf[4] = (k >> 8) & 0xff;  // データの開始位置 3
					send_buf[5] = k & 0xff;         // データの開始位置 4
					send_buf[6] = (h >> 24) & 0xff; // データ確認用ハッシュ 1
					send_buf[7] = (h >> 16) & 0xff; // データ確認用ハッシュ 2
					send_buf[8] = (h >> 8) & 0xff;  // データ確認用ハッシュ 3
					send_buf[9] = h & 0xff;         // データ確認用ハッシュ 4
					for (i=10; i<OUTPUT_REPORT_RAW_MAX_LEN; i++) send_buf[i] = 0x00;
					// this->sendRawData(send_buf, 32);
					return;
				} else {
					// データを全部受け取り終わり
					// 完了を送る
					h = azcrc32(save_file_data, save_file_length);
					send_buf[0] = id_save_file_complate; // 保存完了
					send_buf[1] = 0x00; // データ受信完了
					send_buf[2] = (h >> 24) & 0xff; // データ確認用ハッシュ 1
					send_buf[3] = (h >> 16) & 0xff; // データ確認用ハッシュ 2
					send_buf[4] = (h >> 8) & 0xff;  // データ確認用ハッシュ 3
					send_buf[5] = h & 0xff;         // データ確認用ハッシュ 4
					for (i=6; i<OUTPUT_REPORT_RAW_MAX_LEN; i++) send_buf[i] = 0x00;
					// this->sendRawData(send_buf, 32);
					return;
				}


			}
			send_buf[0] = 0;
			return;
		}
		case id_save_file_complate: {
			// 保存完了
            common_cls.write_file(target_file_path, save_file_data, save_file_length);
			free(save_file_data);
			send_buf[0] = id_save_file_complate; // 保存完了
			send_buf[1] = 0x01; // データ保存完了
			for (i=2; i<OUTPUT_REPORT_RAW_MAX_LEN; i++) send_buf[i] = 0x00;
			return;
		}
		case id_remove_file: { // 0x35 ファイル削除要求
			// ファイル名を取得
			i = 1;
			while (remap_buf[i]) {
				target_file_path[i - 1] = remap_buf[i];
				i++;
				if (i >= OUTPUT_REPORT_RAW_MAX_LEN) break;
			}
			target_file_path[i - 1] = 0x00;
			send_buf[0] = id_remove_file;
		    // ファイルがあればファイルを消す
			if (InternalFS.exists(target_file_path)) {
				if (!InternalFS.remove(target_file_path)) {
					// 削除失敗したら2にする
					send_buf[1] = 0x02;
				} else {
					// 成功は0
					send_buf[1] = 0x00;
				}
			} else {
				// ファイルが無ければ1
				send_buf[1] = 0x01;
			}
			// 完了を返す
			for (i=2; i<OUTPUT_REPORT_RAW_MAX_LEN; i++) send_buf[i] = 0x00;
			return;

		}
		case id_remove_all: { // 全てのファイルを削除する
			keyboard_status = 2; // ステータスをAZTOOLの処理中にする
            InternalFS.format();
			keyboard_status = 1; // ステータスを元に戻す

            // 結果を返すコマンドを送信
			send_buf[0] = id_remove_all;
			for (i=1; i<OUTPUT_REPORT_RAW_MAX_LEN; i++) send_buf[i] = 0x00;
			return;

		}
		case id_move_file: { // 0x36 ファイル名変更
			// ファイル名を取得
			i = 1;
			j = 0;
			while (remap_buf[i]) {
				target_file_path[j] = remap_buf[i];
				i++;
				j++;
				if (i >= OUTPUT_REPORT_RAW_MAX_LEN) break;
			}
			target_file_path[j] = 0x00;
			i++;
			// 変更後ファイル名を取得
			j = 0;
			while (remap_buf[i]) {
				second_file_path[j] = remap_buf[i];
				i++;
				j++;
				if (i >= OUTPUT_REPORT_RAW_MAX_LEN) break;
			}
			second_file_path[j] = 0x00;
		    send_buf[0] = id_move_file;
			if (!InternalFS.exists(target_file_path)) {
				// 該当ファイルが無ければ1を返す
				send_buf[1] = 0x01;
			} else if (InternalFS.rename(target_file_path, second_file_path)) {
				// ファイル名変更 成功
				send_buf[1] = 0x00;
			} else {
				// ファイル名変更 失敗
				send_buf[1] = 0x02;
			}
			for (i=2; i<OUTPUT_REPORT_RAW_MAX_LEN; i++) send_buf[i] = 0x00;
			return;

        }
		case id_get_file_list: {
			// 0x38 ファイルリストの取得
            File dirp = InternalFS.open("/", FILE_O_READ);
			File filep = dirp.openNextFile();
			String res = "{\"list\":[";
			i = 0;
			while(filep){
				if (i) res += ",";
				res += "{\"name\":\"" +String(filep.name()) + "\",\"size\":" + String(filep.size()) + "}";
				filep = dirp.openNextFile();
				i++;
			}
			res += "]}";
			save_file_length = res.length();
			m = save_file_length;
			// ファイルリストの結果を送信用バッファに入れる
			save_file_data = (uint8_t *)malloc(m + 1);
			res.toCharArray((char *)save_file_data, m + 1);
			// 結果を返すコマンドを送信
			send_buf[0] = id_get_file_list;
			send_buf[1] = ((save_file_length >> 24) & 0xff);
			send_buf[2] = ((save_file_length >> 16) & 0xff);
			send_buf[3] = ((save_file_length >> 8) & 0xff);
			send_buf[4] = (save_file_length & 0xff);
			for (i=5; i<OUTPUT_REPORT_RAW_MAX_LEN; i++) send_buf[i] = 0x00;
			// this->sendRawData(send_buf, 32);
			return;
		}
		case id_get_disk_info: {
			// 0x39 SPIFFSの容量を返す(ブロック数(128)ごとの数値)
			// ここのコードをマネした： https://github.com/adafruit/Adafruit_nRF52_Arduino/blob/master/libraries/Adafruit_LittleFS/src/Adafruit_LittleFS.cpp#L258-L268
			send_buf[0] = id_get_disk_info; // 結果の返すコマンド
			// spiffs の容量
			lfs_t *f;
			f = InternalFS._getFS();
			m = f->cfg->block_count * f->cfg->block_size;
			send_buf[1] = ((m >> 24) & 0xff);
			send_buf[2] = ((m >> 16) & 0xff);
			send_buf[3] = ((m >> 8) & 0xff);
			send_buf[4] = (m & 0xff);
			// spiffs の使用容量
			InternalFS._lockFS();
			size_t block_used = 0;
			lfs_traverse(f, _lfs_count, &block_used);
			InternalFS._unlockFS();
			m = block_used * f->cfg->block_size;
			send_buf[5] = ((m >> 24) & 0xff);
			send_buf[6] = ((m >> 16) & 0xff);
			send_buf[7] = ((m >> 8) & 0xff);
			send_buf[8] = (m & 0xff);
			// 結果を返すコマンドを送信
			for (i=9; i<OUTPUT_REPORT_RAW_MAX_LEN; i++) send_buf[i] = 0x00;
			return;
		}
		case id_restart: {
			// 0x3a の再起動
			aztool_mode_flag = 3; // キーボードリスタート要求
			send_buf[0] = id_restart; // 結果の返すコマンド
			for (i=1; i<OUTPUT_REPORT_RAW_MAX_LEN; i++) send_buf[i] = 0x00;
			return;

		}
		case id_get_ioxp_key: {
			// IOエキスパンダからキーの読み取り
			uint8_t rows[8]; // rowのピン
			uint16_t out_mask; // rowのピンを立てたマスク
			x = remap_buf[1]; // エキスパンダのアドレス(0～7)
			// 既に使用しているIOエキスパンダなら読み込みステータス0で返す
			if (ioxp_hash[x] == 1) {
				send_buf[0] = id_get_ioxp_key; // IOエキスパンダキー読み込み
				send_buf[1] = 0x01; // 使用中
				for (i=2; i<OUTPUT_REPORT_RAW_MAX_LEN; i++) send_buf[i] = 0x00;
				// this->sendRawData(send_buf, 32);
				return;
			}
			// 初期化がまだであれば初期化
			if (ioxp_status[x] < 0) {
                ioxp_obj[x] = new Adafruit_MCP23X17();
                ioxp_status[x] = 0;
			}
			if (ioxp_status[x] < 1) {
				if (!ioxp_obj[x]->begin_I2C(0x20 + x, &Wire)) {
					// 初期化失敗
					send_buf[0] = id_get_ioxp_key; // IOエキスパンダキー読み込み
					send_buf[1] = 0x02; // 初期化失敗
					for (i=2; i<OUTPUT_REPORT_RAW_MAX_LEN; i++) send_buf[i] = 0x00;
					// this->sendRawData(send_buf, 32);
					return;
				}
                ioxp_status[x] = 1;
			}
			// row の情報を取得
			s = remap_buf[2]; // row の数
			if (s > 8) s = 8;
			for (i=0; i<s; i++) {
				rows[i] = remap_buf[3 + i]; // row の番号取得
			}
			// ピンの初期化
			h = azcrc32(remap_buf, OUTPUT_REPORT_RAW_MAX_LEN); // 受け取ったデータのハッシュを取得 // これで大丈夫？
			if (h != ioxp_hash[x]) { // 最後に設定したピン情報と違えばピンの初期化をする
				for (i=0; i<16; i++) {
					// row のピンかチェックして rowならOUTPUTに指定
					k = false;
					for (j=0; j<s; j++) {
						if (rows[j] == i) {
							ioxp_obj[x]->pinMode(i, OUTPUT);
							k = true;
							break;
						}
					}
					if (k) continue; // row だったなら次のピンへ
					// row以外は全てinput
					ioxp_obj[x]->pinMode(i, INPUT_PULLUP);
				}
				ioxp_hash[x] = h;
			}
			// キーの読み込み
			p = 3;
			send_buf[0] = id_get_ioxp_key; // IOエキスパンダキー読み込み
			send_buf[1] = 0x00; // 読み取り成功
			send_buf[2] = s; // rowの数
			if (s) {
				// row があればマトリックス読み取り
				// マスク作成
				out_mask = 0x00;
				for (i=0; i<s; i++) {
					out_mask |= (0x01 << rows[i]);
				}
				// マトリックス読み込み
				for (i=0; i<s; i++) {
					o = out_mask & ~(0x01 << rows[i]);
					if (out_mask & 0xff00) { // ポートB
						ioxp_obj[x]->writeGPIO((o >> 8) & 0xff, 1); // ポートBに出力
					}
					if (out_mask & 0xff) { // ポートA
						ioxp_obj[x]->writeGPIO(o & 0xff, 0); // ポートAに出力
					}
					h = ~(ioxp_obj[x]->readGPIOAB() | out_mask); // ポートA,B両方のデータを取得(rowのピンは全て1)
					send_buf[p] = (h >> 8) & 0xff;
					p++;
					send_buf[p] = h & 0xff;
					p++;
				}
			} else {
				// row が無ければ全ピンダイレクト
				h = ~(ioxp_obj[x]->readGPIOAB()); // ポートA,B両方のデータを取得
				send_buf[p] = (h >> 8) & 0xff;
				p++;
				send_buf[p] = h & 0xff;
				p++;
			}
			// 結果を送信
			for (i=p; i<OUTPUT_REPORT_RAW_MAX_LEN; i++) send_buf[i] = 0x00; // 残りのデータを0詰め
			// this->sendRawData(send_buf, 32);
			return;
		}
		case id_set_mode_flag: {
			// WEBツール作業中フラグの設定
			aztool_mode_flag = remap_buf[1];
			// 結果を送信
			send_buf[0] = id_set_mode_flag; // フラグの設定
			for (i=1; i<OUTPUT_REPORT_RAW_MAX_LEN; i++) send_buf[i] = 0x00; // 残りのデータを0詰め
			// this->sendRawData(send_buf, 32);
			return;
			
		}
		case id_get_ap_list: {
			// WIFI のアクセスポイントの一覧取得
			send_buf[0] = id_get_ap_list;
			for (i=1; i<OUTPUT_REPORT_RAW_MAX_LEN; i++) send_buf[i] = 0x00;
			return;

		}
		case id_read_key: {
			// キーの入力状態取得
			// 結果コマンドの準備
			send_buf[0] = id_read_key; // キーの入力状態
			send_buf[1] = key_input_length & 0xff; // キーの数
			for (i=2; i<OUTPUT_REPORT_RAW_MAX_LEN; i++) send_buf[i] = 0x00; // 残りのデータを0詰め
			// 結果コマンドに入力データを入れていく
			j = 0;
			s = 2;
			for (i=0; i<key_input_length; i++) {
				if (j == 8) {
					j = 0;
					s++;
				} else {
					send_buf[s] = send_buf[s] << 1;
				}
				if (s > OUTPUT_REPORT_RAW_MAX_LEN - 1) break;
				if (common_cls.input_key[i]) send_buf[s]++;
				j++;
			}
			if (j > 0 && j < 8) send_buf[s] = send_buf[s] << (8 - j);
			// 結果を送信
			// this->sendRawData(send_buf, 32);
			return;

		}
		case id_get_rotary_key: {
			// ロータリーエンコーダの入力状態取得
		    m = remap_buf[1]; // 読み込みに行くアドレス取得
			send_buf[0] = id_get_rotary_key; // キーの入力状態
			send_buf[1] = m; // 読み込みに行くアドレス
            send_buf[2] = wirelib_cls.read_rotary(m); // データ受け取る
			for (i=3; i<OUTPUT_REPORT_RAW_MAX_LEN; i++) send_buf[i] = 0x00;
			// this->sendRawData(send_buf, 32);
			return;

		}
		case id_get_pim447: {
			// 1U トラックボールデータ取得 PIM447
		    m = remap_buf[1]; // 読み込みに行くアドレス取得
			send_buf[0] = id_get_pim447; // キーの入力状態
			send_buf[1] = m; // 読み込みに行くアドレス
            pim447_data_obj = wirelib_cls.read_trackball_pim447(m); // データ受け取る
			send_buf[2] = pim447_data_obj.left;
			send_buf[3] = pim447_data_obj.right;
			send_buf[4] = pim447_data_obj.up;
			send_buf[5] = pim447_data_obj.down;
			send_buf[6] = pim447_data_obj.click;
			for (i=7; i<OUTPUT_REPORT_RAW_MAX_LEN; i++) send_buf[i] = 0x00;
			// this->sendRawData(send_buf, 32);
			return;

		}
		case id_set_pin_set: {
			// ESP32 本体の direct, touch, col, row を変更する(pinModeの初期化もする)
			// 現状でi2c の設定がされている場合は一旦無しにしちゃう
			// (i2c で使ってるピンがioで設定されるとややこしい話になる)
			if (ioxp_sda >= 0 && ioxp_scl >= 0) {
				// i2c 通信終了
				// Wire.end(); // ボード ESP32 の 1.0.6 の Wire に end() が無かった
				// ピンの設定削除
				ioxp_sda = -1;
				ioxp_scl = -1;
			}
			if (i2copt_len > 0) {
				i2copt_len = -1;
				delete[] i2copt;
			}
			// 現状のピン設定を解放
			delete[] direct_list;
			delete[] touch_list;
			delete[] col_list;
			delete[] row_list;
			// 読み込み開始バイト
			m = 1;
			// direct ピン設定
			direct_len = remap_buf[m]; // direct ピンの設定数取得
			m++;
			direct_list = new short[direct_len];
			for (i=0; i<direct_len; i++) {
				direct_list[i] = remap_buf[m];
				m++;
			}
			// touch ピン設定
			touch_len = remap_buf[m]; // touch ピンの設定数取得
			m++;
			touch_list = new short[touch_len];
			for (i=0; i<touch_len; i++) {
				touch_list[i] = remap_buf[m];
				m++;
			}
			// col ピン設定
			col_len = remap_buf[m]; // col ピンの設定数取得
			m++;
			col_list = new short[col_len];
			for (i=0; i<col_len; i++) {
				col_list[i] = remap_buf[m];
				m++;
			}
			// row ピン設定
			row_len = remap_buf[m]; // row ピンの設定数取得
			m++;
			row_list = new short[row_len];
			for (i=0; i<row_len; i++) {
				row_list[i] = remap_buf[m];
				m++;
			}
			// マトリックスタイプ
			read_type = remap_buf[m];
			m++;
			// キー数計算
			common_cls.pin_setup();
			// レスポンスデータ作成
			send_buf[0] = id_set_pin_set; // ピン設定
			send_buf[1] = 0;
			send_buf[2] = 0;
			send_buf[3] = ((key_input_length >> 8) & 0xff);
			send_buf[4] = (key_input_length & 0xff);
			for (i=2; i<OUTPUT_REPORT_RAW_MAX_LEN; i++) send_buf[i] = 0x00;
			return;
		}
		case id_i2c_read: {
			// i2c からデータ読み込み
		    m = remap_buf[1]; // 読み込みに行くアドレス取得
			l = remap_buf[2]; // 読み込む長さ
			if (l > 28) l = 28; // 読み込む長さは28バイトがMAX
			for (i=0; i<OUTPUT_REPORT_RAW_MAX_LEN; i++) send_buf[i] = 0x00;
			send_buf[0] = id_i2c_read; // キーの入力状態
			send_buf[1] = m; // 読み込みに行くアドレス
			send_buf[2] = wirelib_cls.read(m, &send_buf[3], l); // 読み込み
			return;
		}
		case id_i2c_write: {
			// i2c へデータ書込み
		    m = remap_buf[1]; // 書込みに行くアドレス取得
			l = remap_buf[2]; // 書き込む長さ
			if (l > 28) l = 28; // 書き込む長さは28バイトがMAX
			send_buf[0] = id_i2c_write; // キーの入力状態
			send_buf[1] = m; // 読み込みに行くアドレス
			send_buf[2] = wirelib_cls.write(m, &remap_buf[3], l); // 書込み
			for (i=2; i<OUTPUT_REPORT_RAW_MAX_LEN; i++) send_buf[i] = 0x00;
			return;
		}
		case id_get_analog_switch: {
			// アナログスイッチの情報を取得
			send_buf[0] = id_get_analog_switch;
			for (i=1; i<OUTPUT_REPORT_RAW_MAX_LEN; i++) send_buf[i] = 0x00;
			return;
		}
		case id_set_analog_switch: {
			// アナログスイッチの設定を変更
			send_buf[0] = id_set_analog_switch;
			for (i=1; i<OUTPUT_REPORT_RAW_MAX_LEN; i++) send_buf[i] = 0x00;
			return;
		}
		case id_get_serial_input: {
			// シリアル通信(赤外線)のキー入力取得
			send_buf[0] = id_get_serial_input;
			p = 1;
			for (i=0; i<256; i++) {
				s = i / 16;
				x = i % 16;
				if (seri_input[s] & (0x01 << x)) {
					send_buf[p] = i;
					p++;
				}
				if (p >=OUTPUT_REPORT_RAW_MAX_LEN) break;
			}
			for (i=p; i<OUTPUT_REPORT_RAW_MAX_LEN; i++) send_buf[i] = 0x00;
			return;
		}
		case id_get_serial_setting: {
			// シリアル通信(赤外線)のセッティング情報取得
			send_buf[0] = id_get_serial_setting;
			for (i=1; i<12; i++) {
				send_buf[i+1] = seri_setting[i];
			}
			while (i<OUTPUT_REPORT_RAW_MAX_LEN) { send_buf[i] = 0x00; i++; }
			return;
		}
		case id_get_firmware_status: {
			// ファームウェアステータス取得
			memset(send_buf, 0x00, OUTPUT_REPORT_RAW_MAX_LEN);
			sprintf((char *)send_buf, "%c%s-%s", id_get_firmware_status, FIRMWARE_VERSION, EEP_DATA_VERSION);
			// this->sendRawData(send_buf, 32);
			return;

		}
		case id_get_ble_info: {
			// BLE 情報取得
			memset(send_buf, 0x00, OUTPUT_REPORT_RAW_MAX_LEN);
			send_buf[0] = id_get_ble_info; // キー数取得
			for (i=0; i<6; i++) {
				send_buf[1 + i] = my_addr[i]; // 自分の BLE mac アドレス
			}
			send_buf[7] = ble_scan_flag; // スキャン中かどうか
			send_buf[8] = (child_conn_flag)? 0x01: 0x00; // 子端末に接続しているかどうか
			send_buf[9] = (host_input_length >> 8) & 0xFF; // 自分のキー数
			send_buf[10] = (host_input_length & 0xFF); // 自分のキー数
			send_buf[11] = (child_input_length >> 8) & 0xFF; // 子端末のキー数
			send_buf[12] = (child_input_length & 0xFF); // 子端末のキー数
			send_buf[13] = (key_input_length >> 8) & 0xFF; // キー数合計
			send_buf[14] = (key_input_length & 0xFF); // キー数合計
			return;

		}
		case id_get_device_name: {
			// BLE デバイス名返却
			memset(send_buf, 0x00, OUTPUT_REPORT_RAW_MAX_LEN);
		    m = remap_buf[1]; // 取得開始バイト
			send_buf[0] = id_get_device_name; // BLE デバイス名返却
			send_buf[1] = strlen(keyboard_name_str); // デバイス名サイズ
			send_buf[2] = m; // レスポンスに渡したデータが何バイト目からのデータか
			for (i=0; i<(OUTPUT_REPORT_RAW_MAX_LEN - 3); i++) {
				if ((m + i) < send_buf[1]) { // デバイス名文字数をオーバーしていない
					send_buf[3 + i] = keyboard_name_str[m + i];
				} else {
					send_buf[3 + i] = 0x00;
				}
			}
			return;

		}
		case id_get_cst816: {
			// トラックパッド CST816 情報取得
		    m = remap_buf[1]; // 読み込みに行くアドレス取得
			send_buf[0] = id_get_cst816; // キーの入力状態
			send_buf[1] = m; // 読み込みに行くアドレス
            cst816_data_obj = wirelib_cls.read_cst816(m); // データ受け取る
			send_buf[2] = cst816_data_obj.gesture_id;
			send_buf[3] = cst816_data_obj.points;
			send_buf[4] = cst816_data_obj.event;
			send_buf[5] = (cst816_data_obj.x >> 8) & 0xFF;
			send_buf[6] = (cst816_data_obj.x & 0xFF);
			send_buf[7] = (cst816_data_obj.y >> 8) & 0xFF;
			send_buf[8] = (cst816_data_obj.y & 0xFF);
			for (i=9; i<OUTPUT_REPORT_RAW_MAX_LEN; i++) send_buf[i] = 0x00;
			return;

		}
		case id_ble_scan_start: {
			// アドバタイズ端末をスキャン開始
			// BLE Uart のアドバタイズしている端末のリストを返す
			send_buf[0] = id_ble_scan_start; // BLE Uart リスト
			for (i=1; i<OUTPUT_REPORT_RAW_MAX_LEN; i++) send_buf[i] = 0x00;
			// 分割：親 意外はスキャンできない || 子端末名が指定してあれば既にスキャン中なのでスキャンできない
            if (ble_type != 1 || strlen(child_name)) return;
			// 子供アドレスが設定されていたらスキャンできない
			if (child_addr_flag) return;
			// スキャン開始
			ble_scan_flag = 1; // スキャン中フラグON
			// アドバタイズ中の端末をスキャン
            Bluefruit.Scanner.start(0);
			return;

		}
		case id_ble_scan_end: {
			// アドバタイズ端末をスキャン終了
			// 分割：親 意外はスキャンできない / 子端末名が指定してあればスキャンできない
            if (ble_type == 1 && !strlen(child_name)) {
			    Bluefruit.Scanner.stop();
				// 同じアドレスチェックするリストをクリア
				if (check_addr != NULL) {
					free(check_addr);
					check_addr = NULL;
				}
			}
			ble_scan_flag = 0; // スキャン中フラグOFF
			send_buf[0] = id_ble_scan_end; // BLE Uart リスト
			for (i=1; i<OUTPUT_REPORT_RAW_MAX_LEN; i++) send_buf[i] = 0x00;
			return;

		}
		case id_get_child_file: {
			// 分割：小 からファイルを取得する
			// 分割：親 意外は子と接続してない || 子供アドレスが設定されていないと取得できない || 子供と接続していない
            if (ble_scan_flag == 0 && (ble_type != 1 || !child_addr_flag || !child_conn_flag)) {
			    send_buf[0] = id_get_child_file; // BLE Uart リスト
			    for (i=1; i<OUTPUT_REPORT_RAW_MAX_LEN; i++) send_buf[i] = 0x00;
				save_file_data = (uint8_t *)malloc(16); // レスポンスJSONを格納するバッファを確保
				memset(save_file_data, 0x00, 16);
				strcat((char*)save_file_data, "");
				save_file_length = strlen((char*)save_file_data); // 作ったレスポンスJSONのサイズを取得
				return;
			}
			// 子端末にファイル取得要求
			send_buf[0] = id_get_file_start;
			for (i=1; i<OUTPUT_REPORT_RAW_MAX_LEN; i++) {
				send_buf[i] = remap_buf[i];
			}
			// sprintf("%c/kle.json", id_get_file_start);
			clientUart.write(send_buf, OUTPUT_REPORT_RAW_MAX_LEN);
			// 小からファイルを受け取った後に
			// このタイミングではブラウザには何も返さない
			send_buf[0] = 0x00;
			return;

		}
		case id_get_scan_data_end: {
			// スキャン中に接続した端末のデータ抽出終了
			if (ble_scan_flag && child_conn_flag) { // スキャン中 & 子端末に接続中
				Bluefruit.disconnect(ble_scan_handle);
			}
			// 抽出終了を受け取りましたを返す
			send_buf[0] = id_get_scan_data_end; // BLE Uart リスト
			for (i=1; i<OUTPUT_REPORT_RAW_MAX_LEN; i++) send_buf[i] = 0x00;
			return;

		}
		default: {
			send_buf[0] = 0xFF;
			for (i=1; i<OUTPUT_REPORT_RAW_MAX_LEN; i++) send_buf[i] = 0x00;
            return;

        }
	}
}

// BLE クライアント アドバタイズ端末を発見した時の スキャン コールバック
void scan_callback(ble_gap_evt_adv_report_t* report)
{
  short i;

  // 同じアドレスをチェックするメモリが確保されていなければ確保
  if (check_addr == NULL) {
	check_addr = (uint8_t *)malloc(20 * BLE_GAP_ADDR_LEN); // 同じアドレスの端末をスキャンしないためのリスト
	memset(check_addr, 0x00, 20 * BLE_GAP_ADDR_LEN);
  }
  // すでにスキャンした端末かどうかチェック
  for (i=0; i<120; i+=6) {
    if (addr_check(&check_addr[i], report->peer_addr.addr)) {
		Bluefruit.Scanner.resume();
      return;
    }
    if (addr_is_none(&check_addr[i])) break;
  }
  if (i >= 119) {
	Bluefruit.Scanner.resume();
	return;
  }
  // スキャンしたよリストにアドレスを追加
  addr_copy(&check_addr[i], report->peer_addr.addr);

  if (ble_scan_flag) {
    // アドバタイズ端末スキャン中
	// 2端末以降の場合はスキャン結果にカンマを追加
	Bluefruit.Central.connect(report); // 接続要求

  } else if (child_addr_flag) {
	// 子供端末であれば接続要求する
	if (addr_check(child_addr, report->peer_addr.addr)) {
		Bluefruit.Central.connect(report); // 接続要求
	} else {
		Bluefruit.Scanner.resume();
	}

  } else if (strlen(child_name)) {
	// 子端末の名前が設定されていればコネクション
	Bluefruit.Central.connect(report); // 接続要求

  } else {
	Bluefruit.Scanner.resume();
  }
}


// BLE Client コネクションが確立した時のコールバック
void client_connect_callback(uint16_t conn_handle)
{
  // 相手の端末名、アドレスを取得
  char central_name[33] = "";
  char child_path[] = "/child";
  uint8_t save_data[16]; // 子端末のアドレス保存用バッファ
  char model_buf[33] = "";
  char key_len_buf[4] = "";
  short key_len;
  BLEConnection* conn = Bluefruit.Connection(conn_handle); // 接続情報取得
  ble_gap_addr_t addr = conn->getPeerAddr(); // 接続アドレス取得
  conn->getPeerName(central_name, sizeof(central_name)); // 接続端末の名前取得

  if (ble_scan_flag) {
	// アドバタイズ端末スキャン中
	ble_scan_handle = conn_handle; // スキャン中にコネクションしたハンドルを保持
	// Uart サービスが無ければ接続しない
	if (!clientUart.discover(ble_scan_handle)) {
		Bluefruit.disconnect(ble_scan_handle);
		return;
	}
	// Dis サービスがあれば Dis 初期化 + キーボードの型番取得
	if (!clientDis.discover(ble_scan_handle)) {
		// Dis サービスが無ければ接続しない
		Bluefruit.disconnect(ble_scan_handle);
		return;
	}
	// 型番取得
	if (!clientDis.getModel(model_buf, sizeof(model_buf))) {
		// 型番が取得できなければ接続しない
		Bluefruit.disconnect(ble_scan_handle);
		return;
	}
	// シリアル通信開始
	clientUart.enableTXD();
	// 接続したよフラグを立てる
	child_conn_flag = true;
	// 接続できたらスキャン終了
	Bluefruit.Scanner.stop();
	// アドレスと名前を返すデータ作成
	save_file_data = (uint8_t *)malloc(256);
	memset(save_file_data, 0x00, 256); // メモリ内クリア
	sprintf((char *)save_file_data, "{\"addr\":[%d,%d,%d,%d,%d,%d],\"model\":\"%s\",\"name\":\"%s\"}",
			addr.addr[0], addr.addr[1], addr.addr[2], addr.addr[3], addr.addr[4], addr.addr[5], model_buf, central_name);
	// アドレスとデータを返すコマンドをブラウザに送信
	save_file_length = strlen((char *)save_file_data); // 生成したJSONのサイズ
	memset(send_buf, 0x00, OUTPUT_REPORT_RAW_MAX_LEN);
	send_buf[0] = id_get_scan_addr; // 子アドレスを返す
	send_buf[1] = save_file_length & 0xFF; // 送るデータのサイズ
	_characteristic_input->notify(send_buf, OUTPUT_REPORT_RAW_MAX_LEN); // コマンド送信

  } else if (child_addr_flag) {
	// 子端末のアドレスから接続した
	// Dis サービスがあれば Dis 初期化 + キーボードのキー数取得
	if (clientDis.discover(conn_handle)) {
		if (clientDis.getModel(model_buf, sizeof(model_buf))) {
			// 2文字目から数字３桁を取得
			key_len_buf[0] = model_buf[1];
			key_len_buf[1] = model_buf[2];
			key_len_buf[2] = model_buf[3];
			key_len_buf[3] = 0x00;
			key_len = atoi(key_len_buf);
			if (child_input_length != key_len) {
				// ファイルに保存してたキー数と変わっていればファイルを更新して再起動
				addr_copy(&save_data[0], my_addr); // 自分のアドレス
				addr_copy(&save_data[6], addr.addr); // 子端末のアドレス
				save_data[12] = (key_len >> 8) & 0xFF;
				save_data[13] = key_len & 0xFF;
				common_cls.write_file(child_path, save_data, 14); // 子端末のアドレスファイルを保存
				aztool_mode_flag = 3; // キーボードリスタート要求
			}
		}
	}
	// Uart サービスがあれば Uart 初期化
	if (clientUart.discover(conn_handle)) {
		// シリアル通信開始
		clientUart.enableTXD();
		// 接続したよフラグを立てる
		child_conn_flag = true;
		// 接続できたらスキャン終了
		Bluefruit.Scanner.stop();
		// 同じアドレスチェックするリストをクリア
		free(check_addr);
		check_addr = NULL;
	} else {
		// Uart サービスが無ければ接続しない
		Bluefruit.disconnect(conn_handle);
	}

  } else if (strlen(child_name)) {
	// 子端末の名前が設定してあれば、名前が一致する端末だけ接続する
	if (strcmp(central_name, child_name) == 0) { // 子端末の名前と一致する端末
		// Dis サービスがあれば Dis 初期化 + キーボードのキー数取得
		key_len = 0;
		if (clientDis.discover(conn_handle)) {
			if (clientDis.getModel(model_buf, sizeof(model_buf))) {
				// 2文字目から数字３桁を取得
				key_len_buf[0] = model_buf[1];
				key_len_buf[1] = model_buf[2];
				key_len_buf[2] = model_buf[3];
				key_len_buf[3] = 0x00;
				key_len = atoi(key_len_buf); // キー数取得
			} else {
				// Dis から Model が無ければ接続しない
				Bluefruit.disconnect(conn_handle);
			}
		} else {
			// Dis サービスが無ければ接続しない
			Bluefruit.disconnect(conn_handle);
		}
		// Uart サービスがあれば Uart 初期化
		if (clientUart.discover(conn_handle)) {
			addr_copy(&save_data[0], my_addr); // 自分のアドレス
			addr_copy(&save_data[6], addr.addr); // 子端末のアドレス
			save_data[12] = (key_len >> 8) & 0xFF;
			save_data[13] = key_len & 0xFF;
            common_cls.write_file(child_path, save_data, 14); // 子端末のアドレスファイルを作成
			// 接続できたらスキャン終了
			Bluefruit.Scanner.stop();
			// アドレス情報、キー情報が取れたら切断
			Bluefruit.disconnect(conn_handle);
			// キーボードリスタート要求
			aztool_mode_flag = 3;
		} else {
			// Uart サービスが無ければ接続しない
			Bluefruit.disconnect(conn_handle);
		}
	} else {
		// 端末名が一致しない端末は接続しない
		Bluefruit.disconnect(conn_handle);
	}

  }
}


/**
 * BLE Client Callback invoked when a connection is dropped
 * @param conn_handle
 * @param reason is a BLE_HCI_STATUS_CODE which can be found in ble_hci.h
 */
void client_disconnect_callback(uint16_t conn_handle, uint8_t reason)
{
	(void) conn_handle;
	(void) reason;
	// 接続したよフラグを下げる
	child_conn_flag = false;
}


// 分割キーボード小から送られてきたコマンドを親が取得した時のコールバック
// (親側処理：いつもはAZTOOL側がやってる処理)
void bleuart_rx_callback(BLEClientUart& uart_svc)
{
	uint8_t get_data[OUTPUT_REPORT_RAW_MAX_LEN];
	uint8_t send_data[OUTPUT_REPORT_RAW_MAX_LEN];
	short i, read_length;
	int s;

	read_length = 0;
	while (uart_svc.available()) {
		get_data[read_length] = uart_svc.read();
		read_length++;
		if (read_length >= OUTPUT_REPORT_RAW_MAX_LEN) break;
	}
	for (i=read_length; i<OUTPUT_REPORT_RAW_MAX_LEN; i++) get_data[i] = 0x00;

	uint8_t *command_id = &(get_data[0]);
	memset(send_data, 0x00, OUTPUT_REPORT_RAW_MAX_LEN);

	switch (*command_id) {
		case id_get_file_start: { // 0x30 ファイル取得開始を受け取った
			if (!get_data[1]) {
				// ファイルが存在しなかったを返す
				send_data[0] = id_get_child_file; // ファイルデータ取得
				send_data[1] = 0x00; // ファイルは存在しない
				for (i=2; i<OUTPUT_REPORT_RAW_MAX_LEN; i++) send_data[i] = 0x00;
				_characteristic_input->notify(send_data, OUTPUT_REPORT_RAW_MAX_LEN);
				return;
			}
			// ファイルを受け取る準備をしてデータ取得開始する
			save_file_length = (get_data[2] << 24) + (get_data[3] << 16) + (get_data[4] << 8) + get_data[5]; // ファイルのサイズ取得
			save_file_data = (uint8_t *)malloc(save_file_length); // ファイル取得用のメモリ確保
			send_data[0] = id_get_file_data; // ファイルデータ取得
			send_data[1] = 1; // 並列でロードする step 数 (1固定)
			send_data[2] = 0; // ロード開始位置 1
			send_data[3] = 0; // ロード開始位置 2
			send_data[4] = 0; // ロード開始位置 3
			send_data[5] = 0; // ハッシュ値 1 (ハッシュ0はハッシュチェックをスキップ)
			send_data[6] = 0; // ハッシュ値 2
			send_data[7] = 0; // ハッシュ値 3
			send_data[8] = 0; // ハッシュ値 4
			// ファイルデータ取得コマンド送信
			uart_svc.write(send_data, OUTPUT_REPORT_RAW_MAX_LEN);
			return;

		}
		case id_get_file_data: {
			// ファイルデータを受け取る
			s = (get_data[1] << 16) + (get_data[2] << 8) + get_data[3];
			for (i=0; i<16; i++) {
				if (s < save_file_length) {
					save_file_data[s] = get_data[i + 4];
				    s++;
				}
			}
			if (s >= save_file_length) {
				// 全データ受信完了
				// common_cls.write_file("/child", save_file_data, save_file_length);
				// free(save_file_data);
				// データ準備できたよを返す
				send_data[0] = id_get_child_file; // ファイルデータ取得
				send_data[1] = 0x01; // ファイルは存在する
				send_data[2] = ((save_file_length >> 24) & 0xff); // ファイルサイズ 1
				send_data[3] = ((save_file_length >> 16) & 0xff); // ファイルサイズ 2
				send_data[4] = ((save_file_length >> 8) & 0xff);  // ファイルサイズ 3
				send_data[5] = (save_file_length & 0xff);         // ファイルサイズ 4
				for (i=6; i<OUTPUT_REPORT_RAW_MAX_LEN; i++) send_data[i] = 0x00;
				// ブラウザに送信
				_characteristic_input->notify(send_data, OUTPUT_REPORT_RAW_MAX_LEN);


			} else {
				// 次のデータ要求
				send_data[0] = id_get_file_data; // ファイルデータ取得
				send_data[1] = 1; // 並列でロードする step 数 (1固定)
				send_data[2] = (s >> 16) & 0xFF; // ロード開始位置 1
				send_data[3] = (s >> 8) & 0xFF; // ロード開始位置 2
				send_data[4] = s & 0xFF; // ロード開始位置 3
				// azcrc32();
				send_data[5] = 0; // ハッシュ値 1 (ハッシュ0はハッシュチェックをスキップ)
				send_data[6] = 0; // ハッシュ値 2
				send_data[7] = 0; // ハッシュ値 3
				send_data[8] = 0; // ハッシュ値 4
				// ファイルデータ取得コマンド送信
				uart_svc.write(send_data, OUTPUT_REPORT_RAW_MAX_LEN);

			}
			return;

		}
		case id_send_child_key: {
			// 分割：子 からキー入力を受け取った。そのまま子用入力キーバッファに保存
			for (i=0; i<CHILD_INPUT_KEY_MAX; i++) {
				child_input_key[i] = get_data[i + 1];
			}
            return;

        }
		case id_send_child_mouse: {
			// 分割：子からマウス移動情報を受け取った。マウス移動リストに追加
			common_cls.press_mouse_list_push(0x2000, 5, get_data[1], get_data[2], get_data[3], get_data[4], 100);
			return;

		}
	}
}
