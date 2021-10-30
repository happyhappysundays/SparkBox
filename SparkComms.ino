#include "Spark.h"
#include "SparkComms.h"
#include "BluetoothSerial.h"

const uint8_t notifyOn[] = {0x1, 0x0};

void notifyCB_sp(BLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify){

  int i;
  byte b;

  for (i = 0; i < length; i++) {
    b = pData[i];
    ble_in.add(b);
  }
  ble_in.commit();
}


void notifyCB_pedal(BLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify){

  int i;

  // LPD8 gives 0xab 0xcd 0x90 0xNN 0xef or 0xab 0xcd 0x80 0xNN 0x00 for on and off
  // NN is 1f 21 23 24
  // NN is 18 1a 1c 1d

  // Blueboard does this:
  // In mode B the BB gives 0x80 0x80 0x90 0xNN 0x64 or 0x80 0x80 0x80 0xNN 0x00 for on and off
  // In mode C the BB gives 0x80 0x80 0xB0 0xNN 0x7F or 0x80 0x80 0xB0 0xNN 0x00 for on and off

  if (pData[2] == 0x90 || (pData[2] == 0xB0 && pData[4] == 0x7F)) {
    switch (pData[3]) {
      case 0x3C:
      case 0x14:
      
      case 0x1f:
      case 0x18:
      
      case 0x00:
      case 0x04:
        curr_preset = 0;
        Serial.println("0");
        break;
      case 0x3E:
      case 0x15:

      case 0x21:
      case 0x1a:
      
      case 0x01:
      case 0x05:
        curr_preset = 1;
        Serial.println("1");
        break;
      case 0x40:
      case 0x16:

      case 0x23:
      case 0x1c:
      
      case 0x02:
      case 0x06:
        curr_preset = 2;
        Serial.println("2");
        break;
      case 0x41:   // Blueboard B
      case 0x17:   // Blueboard C

      case 0x24:   // LPD 8 pad
      case 0x1d:   // LPD 8 pad
      
      case 0x03:   // LPD8 dial
      case 0x07:
      
        curr_preset = 3;
        Serial.println("3");
        break;
    }
  triggered_pedal = true;
  }
}


class CharacteristicCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) {
        int j, l;
        const char *p;
        byte b;
        l = pCharacteristic->getValue().length();
        p = pCharacteristic->getValue().c_str();
        for (j=0; j < l; j++) {
          b = p[j];
          ble_app_in.add(b);
        }
        ble_app_in.commit();
  };
};

static CharacteristicCallbacks chrCallbacks_s, chrCallbacks_r;


bool connected_pedal, connected_sp;
bool bt_connected;

void connect_to_all() {
  int i;
  uint8_t b;

  is_ble = true;


  BLEDevice::init("Spark 40 BLE");
  pClient_sp =    BLEDevice::createClient();
//  pClient_pedal = BLEDevice::createClient();
  pScan  =        BLEDevice::getScan();
  
  pServer =       BLEDevice::createServer();
  pService =      pServer->createService(S_SERVICE);
  
  pCharacteristic_receive = pService->createCharacteristic(S_CHAR1, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
  pCharacteristic_send = pService->createCharacteristic(S_CHAR2, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);

  pCharacteristic_receive->setCallbacks(&chrCallbacks_r);
  pCharacteristic_send->setCallbacks(&chrCallbacks_s);

  pCharacteristic_send->addDescriptor(new BLE2902());
  
  pService->start();
  // pServer->start();

  pAdvertising = BLEDevice::getAdvertising(); // create advertising instance
  pAdvertising->addServiceUUID(pService->getUUID()); // tell advertising the UUID of our service
  pAdvertising->setScanResponse(true);  


  DEBUG("Service set up");
  
  // Connect to Spark
  connected_sp = false;
  connected_pedal = false;

  BLEUUID SpServiceUuid(C_SERVICE);
  BLEUUID PedalServiceUuid(PEDAL_SERVICE);

  
//  while (!connected_sp || !connected_pedal ) {
  while (!connected_sp) {   
    pResults = pScan->start(4);
    for(i = 0; i < pResults.getCount()  && (!connected_sp /* || !connected_pedal*/); i++) {
      device = pResults.getDevice(i);

      if (device.isAdvertisingService(SpServiceUuid)) {
        DEBUG("Found Spark - trying to connect....");
        if(pClient_sp->connect(&device)) {
          connected_sp = true;
          DEBUG("Spark connected");
        }
      }

//      if (strcmp(device.getName().c_str(),"iRig BlueBoard") == 0) {
 /*     if (device.isAdvertisingService(PedalServiceUuid)) {
        DEBUG("Found pedal by name - trying to connect....");

        if(pClient_pedal->connect(&device)) {
          connected_pedal = true;
          DEBUG("Pedal connected");
        }
      }*/
    }

    // Set up client
    if (connected_sp) {
      DEBUG("Setting up client");
      pService_sp = pClient_sp->getService(SpServiceUuid);
      if (pService_sp != nullptr) {
        pSender_sp   = pService_sp->getCharacteristic(C_CHAR1);
        pReceiver_sp = pService_sp->getCharacteristic(C_CHAR2);
        if (pReceiver_sp && pReceiver_sp->canNotify()) {
          pReceiver_sp->registerForNotify(notifyCB_sp);
          p2902 = pReceiver_sp->getDescriptor(BLEUUID((uint16_t)0x2902));
          if(p2902 != nullptr)
            p2902->writeValue((uint8_t*)notifyOn, 2, true);
        }
      }
    }

    DEBUG("Client set up");

/*    if (connected_pedal) {
      pService_pedal = pClient_pedal->getService(PedalServiceUuid);
      if (pService_pedal != nullptr) {
        pReceiver_pedal = pService_pedal->getCharacteristic(PEDAL_CHAR);
        if (pReceiver_pedal && pReceiver_pedal->canNotify()) {
          pReceiver_pedal->registerForNotify(notifyCB_pedal);
          p2902 = pReceiver_pedal->getDescriptor(BLEUUID((uint16_t)0x2902));
          if(p2902 != nullptr)
            p2902->writeValue((uint8_t*)notifyOn, 2, true);
        }
      }
    }*/
  }

 
  Serial.println("Starting classic bluetooth");
  // now advertise Serial Bluetooth
  bt = new BluetoothSerial();
  if (!bt->begin (SPARK_BT_NAME)) {
    DEBUG("Classic bluetooth init fail");
    while (true);
  }

  // flush anything read from App - just in case
  while (bt->available())
    b = bt->read(); 

  DEBUG("Spark 40 Audio set up");
  
  Serial.println("Available for app to connect...");  
  pAdvertising->start(); 


}


// app_available both returns whether any data is available but also selects which type of bluetooth to use based
// on whether there is any input there
bool app_available() {
  if (!ble_app_in.is_empty()) {
    is_ble = true;
    return true;
  }
    
  if (bt->available()) {
    is_ble = false;
    return true;
  }

  // if neither have input, then there definitely is no input
  return false;
}

uint8_t app_read() {
   if (is_ble) {
     uint8_t b;
     ble_app_in.get(&b);
     return b;
  }
  else
    return bt->read();
}

void app_write(byte *buf, int len) {
  if (is_ble) {
    pCharacteristic_send->setValue(buf, len);
    pCharacteristic_send->notify(true);
  }
  else  
    bt->write(buf, len);

/*
if (pCharacteristic_send != nullptr) {
    pCharacteristic_send->setValue(buf, len);
    pCharacteristic_send->notify(true);
}
    bt->write(buf, len);
*/
}

bool sp_available() {
  return !ble_in.is_empty();
}

uint8_t sp_read() {
  uint8_t b;
  ble_in.get(&b);
  return b;
}

void sp_write(byte *buf, int len) {
  pSender_sp->writeValue(buf, len, false);
}

int ble_getRSSI() {
  if (pClient_sp != nullptr) 
    return pClient_sp->getRssi();    
  else
    return 0;


}
