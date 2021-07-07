#include "Spark.h"
#include "SparkComms.h"
#include "BluetoothSerial.h"

// Updated with changes from David Thompson github: https://github.com/happyhappysundays/SparkBoxHeltec - most of the BLE callbacks

//BLE callbacks


// Callbacks for client events
class ClientCallbacks : public NimBLEClientCallbacks {
  void onConnect(NimBLEClient* pClient) {
    isBTConnected = true;
  }

  void onDisconnect(NimBLEClient* pClient) {
    isBTConnected = false;
  }
};

// Callbacks for advertisment events
class AdvertisedDeviceCallbacks: public NimBLEAdvertisedDeviceCallbacks {
  void onResult(NimBLEAdvertisedDevice* advertisedDevice) {
    if(advertisedDevice->isAdvertisingService(NimBLEUUID("ffc0")))
    {
      NimBLEDevice::getScan()->stop();
    }
  }
};

static ClientCallbacks clientCB;

// Callback to process the results of the last scan or restart it 
//void scanEndedCB(NimBLEScanResults results){
//    DEBUG("Scan Ended");
//}


// Callback function to write to ring buffer
void notifyCB(NimBLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify){
  int i;

  for (i = 0; i < length; i++) 
    ble_in.add(pData[i]);

  ble_in.commit();
}




// Non callback functions
void start_ser() {
  uint8_t b;
  
  ser = new HardwareSerial(2); 
  // 5 is rx, 18 is tx
  ser->begin(HW_BAUD, SERIAL_8N1, 5, 18);

  while (ser->available())
    b = ser->read(); 
}

void start_bt(bool isBLE) {
  is_ble = isBLE;
  if (!is_ble) {
    bt = new BluetoothSerial();
  
    if (!bt->begin (MY_NAME, true)) {
      DEBUG("Bluetooth init fail");
      while (true);
    }    
  }
  else {
    NimBLEDevice::init("");
  }
}

void connect_to_spark() {
  uint8_t b;
  int i;
  bool connected;

  connected = false;

  if (!is_ble) {
    while (!connected) {
      connected = bt->connect(SPARK_NAME);
      if (!(connected && bt->hasClient())) {
        connected = false;
        DEBUG("Not connected");
        delay(2000);
      }
    }

    // flush anything read from Spark - just in case
    while (bt->available())
      b = bt->read(); 
  }
  else {
    while (!connected) {
      NimBLEDevice::init("");
  
      NimBLEScan *pScan = NimBLEDevice::getScan();
      
      // Create a callback that gets called when advertisers are found 
      pScan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());
      NimBLEScanResults results = pScan->start(4);

      NimBLEUUID serviceUuid("ffc0");               // service ffc0 for Spark

      for(i = 0; i < results.getCount() && !connected; i++) {
        device = results.getDevice(i);
    
        if (device.isAdvertisingService(serviceUuid)) {
          pClient = NimBLEDevice::createClient();
          pClient->setClientCallbacks(&clientCB, false);
        
          if(pClient->connect(&device)) {
            connected = true;
            DEBUG("BLE Connected");
          }
        }
      }

      // Get the services
  
      if (connected) {
        pService = pClient->getService(serviceUuid);
                
        if (pService != nullptr) {
          pSender   = pService->getCharacteristic("ffc1");
          pReceiver = pService->getCharacteristic("ffc2");
          if (pReceiver && pReceiver->canNotify()) {
            if (!pReceiver->subscribe(true, notifyCB, true)) {
              connected = false;
              pClient->disconnect();
              DEBUG("BLE not connected - could not subscribe to callback");
            }
          }
        }
      }
      
      if (!connected) {
        DEBUG ("Not connected - trying again");
        delay(200); // Idea from https://github.com/espressif/esp-idf/issues/5105
      }
    }
  }
}



bool ser_available() {
  return ser->available();
}

bool bt_available() {
  if (!is_ble) {
    return bt->available();
  }
  else {
    return (!(ble_in.is_empty()));
  }
}

uint8_t ser_read() {
  return ser->read();
}

uint8_t bt_read() {
  if (!is_ble) {
    return bt->read();
  }
  else {
    uint8_t b;
    ble_in.get(&b);
    return b;
  }
}

void ser_write(byte *buf, int len) {
  ser->write(buf, len);
}

void bt_write(byte *buf, int len) {
  if (!is_ble) 
    bt->write(buf, len);
  else 
    pSender->writeValue(buf, len, false);
}

int ble_getRSSI() {
  return pClient->getRssi();
}
