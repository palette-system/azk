#include "Arduino.h"
#include "ble_callbacks.h"


// ステップ分受信したか確認
int check_step() {
	int i, r = 0;
	for (i=0; i<8; i++) {
		if (save_step_flag[i]) r++;
	}
	return r;
};


void HidrawCallbackExec(int data_length) {
	int h, i, j, k, l, m, s, o, p, x;
    uint8_t *command_id   = &(remap_buf[0]);
    uint8_t *command_data = &(remap_buf[1]);
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
			// ファイルが存在すればファイルを削除
			if (InternalFS.exists(target_file_path)) {
				InternalFS.remove(target_file_path);
			}
			// 書込みモードでファイルオープン
			File fp = InternalFS.open(target_file_path, FILE_O_WRITE);
			// 書込み
			fp.write(save_file_data, save_file_length);
			fp.close();
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
			// keyboard_status = 2; // ステータスをAZTOOLの処理中にする
            InternalFS.format();
			// keyboard_status = 1; // ステータスを元に戻す

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
			// 0x39 SPIFFSの容量を返す
			send_buf[0] = id_get_disk_info; // 結果の返すコマンド
			// 結果を返すコマンドを送信
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
