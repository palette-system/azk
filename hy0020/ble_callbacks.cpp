#include "Arduino.h"
#include "ble_callbacks.h"

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
		default: {
			send_buf[0] = 0xFF;
			for (i=1; i<32; i++) send_buf[i] = 0x00;
            return;

        }
    }
}
