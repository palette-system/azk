
#include "hid_common.h"
#include "cnc_table.h"

// holdの設定
uint8_t hold_type;
uint8_t hold_time;

// 押している最中のキーデータ
press_key_data press_key_list[PRESS_KEY_MAX];

// 押している最中のマウス移動
press_mouse_data press_mouse_list[PRESS_MOUSE_MAX];

// マウスのスクロールボタンが押されているか
bool mouse_scroll_flag;

// aztoolで設定中かどうか
uint8_t aztool_mode_flag;

// オールクリア送信フラグ
int press_key_all_clear;

// remapへ返事を返す用のバッファ
uint8_t remap_buf[36];

// ファイル送受信用バッファ
uint8_t send_buf[36];
char target_file_path[36];
char second_file_path[36];

// ファイル保存用バッファ
uint8_t *save_file_data;
int save_file_length;
uint8_t save_file_step;
uint8_t save_file_index;
bool save_step_flag[8];

// remapで設定変更があったかどうかのフラグ
uint8_t remap_change_flag;

// crc32のハッシュ値を計算
int azcrc32(uint8_t* d, int len) {
	int i;
    uint32_t r = 0 ^ (-1);
    for (i=0; i<len; i++) {
        r = (r >> 8) ^ crc_table_crc32[(r ^ d[i]) & 0xFF];
    }
    return (r ^ (-1));
};
