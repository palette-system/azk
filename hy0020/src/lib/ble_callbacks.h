#ifndef BleCallbacks_h
#define BleCallbacks_h

#include "hid_common.h"
#include "az_common.h"

#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>

using namespace Adafruit_LittleFS_Namespace;

#define INPUT_REPORT_RAW_MAX_LEN 20
#define OUTPUT_REPORT_RAW_MAX_LEN 20

// HidrawCallback
void HidrawCallbackExec(int data_length);

#endif
