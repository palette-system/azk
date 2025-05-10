#ifndef hid_common_h
#define hid_common_h

#include "Arduino.h"



enum via_command_id {
    // remap用
    id_get_protocol_version                 = 0x01,  // always 0x01
    id_get_keyboard_value                   = 0x02,
    id_set_keyboard_value                   = 0x03,
    id_dynamic_keymap_get_keycode           = 0x04,
    id_dynamic_keymap_set_keycode           = 0x05,
    id_dynamic_keymap_reset                 = 0x06,
    id_lighting_set_value                   = 0x07,
    id_lighting_get_value                   = 0x08,
    id_lighting_save                        = 0x09,
    id_eeprom_reset                         = 0x0A,
    id_bootloader_jump                      = 0x0B,
    id_dynamic_keymap_macro_get_count       = 0x0C,
    id_dynamic_keymap_macro_get_buffer_size = 0x0D,
    id_dynamic_keymap_macro_get_buffer      = 0x0E,
    id_dynamic_keymap_macro_set_buffer      = 0x0F,
    id_dynamic_keymap_macro_reset           = 0x10,
    id_dynamic_keymap_get_layer_count       = 0x11,
    id_dynamic_keymap_get_buffer            = 0x12,
    id_dynamic_keymap_set_buffer            = 0x13,

    // aztool用
    id_get_file_start                       = 0x30,
    id_get_file_data                        = 0x31,
    id_save_file_start                      = 0x32,
    id_save_file_data                       = 0x33,
    id_save_file_complate                   = 0x34,
    id_remove_file                          = 0x35,
    id_remove_all                           = 0x36,
    id_move_file                            = 0x37,
    id_get_file_list                        = 0x38,
    id_get_disk_info                        = 0x39,
    id_restart                              = 0x3A,
    id_get_ioxp_key                         = 0x3B,
    id_set_mode_flag                        = 0x3C,
    id_get_ap_list                          = 0x3D,
    id_read_key                             = 0x3E,
    id_get_rotary_key                       = 0x3F,
    id_get_pim447                           = 0x40,
    id_set_pin_set                          = 0x41,
    id_i2c_read                             = 0x42,
    id_i2c_write                            = 0x43,
    id_get_analog_switch                    = 0x44,
    id_set_analog_switch                    = 0x45,
    id_get_serial_input                     = 0x46,
    id_get_serial_setting                   = 0x47,

    // ステータス取得
    id_get_firmware_status                  = 0x60,

    // システム用
    id_unhandled                            = 0xFF,
};

enum via_keyboard_value_id {
    id_uptime              = 0x01,  //
    id_layout_options      = 0x02,
    id_switch_matrix_state = 0x03
};

// remapへ返事を返す用のバッファ
extern uint8_t remap_buf[36];

// ファイル送受信用バッファ
extern uint8_t send_buf[36];
extern char target_file_path[36];
extern char second_file_path[36];

// ファイル保存用バッファ
extern uint8_t *save_file_data;
extern int save_file_length;
extern uint8_t save_file_step;
extern uint8_t save_file_index;
extern bool save_step_flag[8];

// remapで設定変更があったかどうかのフラグ
extern uint8_t remap_change_flag;


#define JIS_SHIFT 0x1000
#define MOUSE_CODE 0x4000

// マウスボタン
/*
#define MOUSE_BUTTON_LEFT  0x01
#define MOUSE_BUTTON_RIGHT  0x02
#define MOUSE_BUTTON_MIDDLE 0x04
#define MOUSE_BUTTON_BACK   0x08
*/

// 通常キー構造
typedef struct
{
  uint8_t modifiers;
  uint8_t reserved;
  uint8_t keycode[6];
} KeyReport;

// メディアキー構造
typedef uint8_t MediaKeyReport[2];


#define INPUT_REPORT_RAW_MAX_LEN 32
#define OUTPUT_REPORT_RAW_MAX_LEN 32


const unsigned short _asciimap[] PROGMEM =
{
  // 0x00
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x28,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,

  // 0x10
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,

  // 0x20
  0x2c, // ' '
  0x1e | JIS_SHIFT, // !
  0x1f | JIS_SHIFT, // "
  0x20 | JIS_SHIFT, // #
  0x21 | JIS_SHIFT, // $
  0x22 | JIS_SHIFT, // %
  0x23 | JIS_SHIFT, // &
  0x24 | JIS_SHIFT, // '
  0x25 | JIS_SHIFT, // (
  0x26 | JIS_SHIFT, // )
  0x34 | JIS_SHIFT, // *
  0x33 | JIS_SHIFT, // +
  0x36, // ,
  0x2d, // -
  0x37, // .
  0x38, // /

  // 0x30
  0x27, // 0
  0x1e, // 1
  0x1f, // 2
  0x20, // 3
  0x21, // 4
  0x22, // 5
  0x23, // 6
  0x24, // 7
  0x25, // 8
  0x26, // 9
  0x34, // :
  0x33, // ;
  0x36 | JIS_SHIFT, // <
  0x2d | JIS_SHIFT, // =
  0x37 | JIS_SHIFT, // >
  0x38 | JIS_SHIFT, // ?

  // 0x40
  0x2f, // @
  0x04 | JIS_SHIFT, // A
  0x05 | JIS_SHIFT, // B
  0x06 | JIS_SHIFT, // C
  0x07 | JIS_SHIFT, // D
  0x08 | JIS_SHIFT, // E
  0x09 | JIS_SHIFT, // F
  0x0a | JIS_SHIFT, // G
  0x0b | JIS_SHIFT, // H
  0x0c | JIS_SHIFT, // I
  0x0d | JIS_SHIFT, // J
  0x0e | JIS_SHIFT, // K
  0x0f | JIS_SHIFT, // L
  0x10 | JIS_SHIFT, // M
  0x11 | JIS_SHIFT, // N
  0x12 | JIS_SHIFT, // O
  
  // 0x50
  0x13 | JIS_SHIFT, // P
  0x14 | JIS_SHIFT, // Q
  0x15 | JIS_SHIFT, // R
  0x16 | JIS_SHIFT, // S
  0x17 | JIS_SHIFT, // T
  0x18 | JIS_SHIFT, // U
  0x19 | JIS_SHIFT, // V
  0x1a | JIS_SHIFT, // W
  0x1b | JIS_SHIFT, // X
  0x1c | JIS_SHIFT, // Y
  0x1d | JIS_SHIFT, // Z
  0x30, // [
  0x89, // yen
  0x32, // ]
  0x2e, // ^
  0x87 | JIS_SHIFT, // _

  // 0x60
  0x2f | JIS_SHIFT, // `
  0x04, // a
  0x05, // b
  0x06, // c
  0x07, // d
  0x08, // e
  0x09, // f
  0x0a, // g
  0x0b, // h
  0x0c, // i
  0x0d, // j
  0x0e, // k
  0x0f, // l
  0x10, // m
  0x11, // n
  0x12, // o

  // 0x70
  0x13, // p
  0x14, // q
  0x15, // r
  0x16, // s
  0x17, // t
  0x18, // u
  0x19, // v
  0x1a, // w
  0x1b, // x
  0x1c, // y
  0x1d, // z
  0x30 | JIS_SHIFT, // {
  0x89 | JIS_SHIFT, // |
  0x32 | JIS_SHIFT, // }
  0x2e | JIS_SHIFT, // ~
  
  0       // DEL
};


#endif
