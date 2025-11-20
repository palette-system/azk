#ifndef wirelib_h
#define wirelib_h


#include <Wire.h>


// 1U トラックボール PIM447 から取得したデータ
struct tracktall_pim447_data {
    uint8_t left;  // 左移動
    uint8_t right; // 右移動
    uint8_t up;    // 上移動
    uint8_t down;  // 下移動
    uint8_t click; // スイッチ
};

// AZエクスパンダのキー読み込み情報
struct azxp_key_info {
    uint8_t key_count; // 読み込むキー数
    uint8_t key_byte; // 受けとるバイト数
};

// AZエクスパンダの入力データ
struct azxp_key_data {
    uint8_t key_input[16];
};

// トラックパッド CST816用レスポンスデータ
struct trackpad_cst816_data {
    uint8_t gesture_id;  // ジェスチャーID (0=none, 1=up, 2=down, 3=left, 4=right, 5=single_click, 6=double_click, 12=long_press)
    uint8_t points; // タッチ数 (1～2まで)
    uint8_t event;    // イベント (0=Down, 1=Up, 2=Contact)
    uint16_t x;  // x座標
    uint16_t y; // y座標
};


// クラスの定義
class Wirelib
{
	public:
		Wirelib();   // コンストラクタ
        int write(int addr, uint8_t *send_data, int send_len); // I2Cへデータ送信
        int read(int addr, uint8_t *read_data, int read_len); // I2Cからデータ読み込み
		uint8_t read_rotary(int addr); // ロータリエンコーダの入力取得
        void set_az1uball_read_type(int addr, int set_mode); // AZ1UBALLのデータ取得タイプを設定
		tracktall_pim447_data read_trackball_pim447(int addr); // 1U トラックボール PIM447 の入力取得
        void send_azxp_setting(int addr, uint8_t *setting); // AZエクスパンダ コンフィグ送信
        azxp_key_info read_key_info(int addr); // AZエクスパンダ キー数取得
        azxp_key_data read_azxp_key(int addr, azxp_key_info kinfo); // AZエクスパンダのキー入力状態を取得
        trackpad_cst816_data read_cst816(int addr);
		
};

#endif
