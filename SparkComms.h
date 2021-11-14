#ifndef SparkComms_h
#define SparkComms_h

#include "BluetoothSerial.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

//#define BT_CONTROLLER
#include "RingBuffer.h"

//#define HW_BAUD 1000000
#define BLE_BUFSIZE 5000

#define C_SERVICE "ffc0"
#define C_CHAR1   "ffc1"
#define C_CHAR2   "ffc2"

#define S_SERVICE "ffc0"
#define S_CHAR1   "ffc1"
#define S_CHAR2   "ffc2"

#define PEDAL_SERVICE    "03b80e5a-ede8-4b33-a751-6ce34ec4c700"
#define PEDAL_CHAR       "7772e5db-3868-4112-a1a9-f2669d106bf3"

#define  SPARK_BT_NAME  "Spark 40 Audio"

void connect_to_all();
void connect_spark();
#ifdef BT_CONTROLLER
void connect_pedal();
#endif

bool sp_connected();  // DT
bool app_connected(); // DT
bool sp_available();
bool app_available();
uint8_t sp_read();
uint8_t app_read();
void sp_write(byte *buf, int len);
void app_write(byte *buf, int len);
int ble_getRSSI();

BluetoothSerial *bt;
bool is_ble;

#ifdef BT_CONTROLLER
bool connected_pedal
bool found_pedal;
#endif

bool connected_sp;
bool connected_app;
bool found_sp;


BLEServer *pServer;
BLEService *pService;
BLECharacteristic *pCharacteristic_receive;
BLECharacteristic *pCharacteristic_send;
BLEAdvertising *pAdvertising;

BLEScan *pScan;
BLEScanResults pResults;
BLEAdvertisedDevice device;

BLEClient *pClient_sp;
BLERemoteService *pService_sp;
BLERemoteCharacteristic *pReceiver_sp;
BLERemoteCharacteristic *pSender_sp;
BLERemoteDescriptor* p2902_sp;
BLEAddress *sp_address;

#ifdef BT_CONTROLLER
BLEClient *pClient_pedal;
BLERemoteService *pService_pedal;
BLERemoteCharacteristic *pReceiver_pedal;
BLERemoteCharacteristic *pSender_pedal;
BLERemoteDescriptor* p2902_pedal;
BLEAddress *pedal_address;
#endif

RingBuffer ble_in;
RingBuffer ble_app_in;

#endif
