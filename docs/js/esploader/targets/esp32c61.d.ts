import { ESPLoader } from "../esploader";
import { ESP32C6ROM } from "./esp32c6";
export declare class ESP32C61ROM extends ESP32C6ROM {
    CHIP_NAME: string;
    IMAGE_CHIP_ID: number;
    CHIP_DETECT_MAGIC_VALUE: number[];
    UART_DATE_REG_ADDR: number;
    EFUSE_BASE: number;
    EFUSE_BLOCK1_ADDR: number;
    MAC_EFUSE_REG: number;
    EFUSE_RD_REG_BASE: number;
    EFUSE_PURPOSE_KEY0_REG: number;
    EFUSE_PURPOSE_KEY0_SHIFT: number;
    EFUSE_PURPOSE_KEY1_REG: number;
    EFUSE_PURPOSE_KEY1_SHIFT: number;
    EFUSE_PURPOSE_KEY2_REG: number;
    EFUSE_PURPOSE_KEY2_SHIFT: number;
    EFUSE_PURPOSE_KEY3_REG: number;
    EFUSE_PURPOSE_KEY3_SHIFT: number;
    EFUSE_PURPOSE_KEY4_REG: number;
    EFUSE_PURPOSE_KEY4_SHIFT: number;
    EFUSE_PURPOSE_KEY5_REG: number;
    EFUSE_PURPOSE_KEY5_SHIFT: number;
    EFUSE_DIS_DOWNLOAD_MANUAL_ENCRYPT_REG: number;
    EFUSE_DIS_DOWNLOAD_MANUAL_ENCRYPT: number;
    EFUSE_SPI_BOOT_CRYPT_CNT_REG: number;
    EFUSE_SPI_BOOT_CRYPT_CNT_MASK: number;
    EFUSE_SECURE_BOOT_EN_REG: number;
    EFUSE_SECURE_BOOT_EN_MASK: number;
    FLASH_FREQUENCY: {
        "80m": number;
        "40m": number;
        "20m": number;
    };
    MEMORY_MAP: (string | number)[][];
    UF2_FAMILY_ID: number;
    EFUSE_MAX_KEY: number;
    KEY_PURPOSES: {
        0: string;
        1: string;
        2: string;
        3: string;
        4: string;
        5: string;
        6: string;
        7: string;
        8: string;
        9: string;
        10: string;
        11: string;
        12: string;
        13: string;
        14: string;
        15: string;
    };
    getPkgVersion(loader: ESPLoader): Promise<number>;
    getMinorChipVersion(loader: ESPLoader): Promise<number>;
    getMajorChipVersion(loader: ESPLoader): Promise<number>;
    getChipDescription(loader: ESPLoader): Promise<string>;
    getChipFeatures(loader: ESPLoader): Promise<string[]>;
    readMac(loader: ESPLoader): Promise<string>;
}
