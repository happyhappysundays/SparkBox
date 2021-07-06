#include "Spark.h"
#include "SparkComms.h"
#include "BluetoothSerial.h"

static NimBLEAdvertisedDevice* advDevice;
static int iRSSI = 0;
static bool doConnect = false;
static uint32_t scanTime = 0; /** 0 = scan forever */

// Callbacks for client events
class ClientCallbacks : public NimBLEClientCallbacks {
  void onConnect(NimBLEClient* pClient) {
    //Serial.println("NimBLE connected");
    isBTConnected = true;
  };

  void onDisconnect(NimBLEClient* pClient) {
    //Serial.print(pClient->getPeerAddress().toString().c_str());
    //Serial.println("NimBLE disconnected");
    isBTConnected = false;
  };
};

// Callbacks for advertisment events
class AdvertisedDeviceCallbacks: public NimBLEAdvertisedDeviceCallbacks {
    void onResult(NimBLEAdvertisedDevice* advertisedDevice) {
        //Serial.print("Advertised Device found: ");
        //Serial.println(advertisedDevice->toString().c_str());
        if(advertisedDevice->isAdvertisingService(NimBLEUUID("ffc0")))
        {
            /** stop scan before connecting */
            NimBLEDevice::getScan()->stop();
            /** Save the device reference in a global for the client to use*/
            advDevice = advertisedDevice;
            /** Ready to connect now */
            doConnect = true;
        }
    };
};

/** Callback to process the results of the last scan or restart it */
void scanEndedCB(NimBLEScanResults results){
    Serial.println("Scan Ended");
}
/** Create a single global instance of the callback class to be used by all clients */
static ClientCallbacks clientCB;

// BLE callback function to write to ring buffer
void notifyCB(NimBLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify){
  int i;

  for (i = 0; i < length; i++) 
    ble_in.add(pData[i]);

  ble_in.commit();
}


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
    //NimBLEDevice::setPower(ESP_PWR_LVL_P9); /** +9db */ // debug
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
    while (!connected) {   /// <<<<<<
      NimBLEDevice::init("");
      NimBLEScan *pScan = NimBLEDevice::getScan();
      /** create a callback that gets called when advertisers are found */
      pScan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks()); //debug
  
      NimBLEScanResults results = pScan->start(4);
  
      NimBLEUUID serviceUuid("ffc0");               // service ffc0 for Spark
  
      for(i = 0; i < results.getCount() && !connected; i++) {
        device = results.getDevice(i);
      
        if (device.isAdvertisingService(serviceUuid)) {
          pClient = NimBLEDevice::createClient();
          pClient->setClientCallbacks(&clientCB, false);
 
          // Set initial connection parameters: These settings are 15ms interval, 0 latency, 120ms timout.
          //  These settings are safe for 3 clients to connect reliably, can go faster if you have less
          //  connections. Timeout should be a multiple of the interval, minimum is 100ms.
          //  Min interval: 12 * 1.25ms = 15, Max interval: 12 * 1.25ms = 15, 0 latency, 51 * 10ms = 510ms timeout
/*          pClient->setConnectionParams(12,12,0,51);
          // Set how long we are willing to wait for the connection to complete (seconds), default is 30. 
          pClient->setConnectTimeout(5);
  */        
          if(pClient->connect(&device)) {
            connected = true;
            //DEBUG("BLE Connected");
          }
          else
          {
            //DEBUG("BLE failed to connect");
          }
        }
        else
        {
          //DEBUG("AdvertisingService not found on this device");
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
              //DEBUG("BLE not connected - could not subscribe to callback");
            }
          }
        }
      } // connected
      
      if (!connected) {
        DEBUG("Connect failed");
        delay(200); // Idea from https://github.com/espressif/esp-idf/issues/5105
      }   
    }
  } // while (!connected)
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
