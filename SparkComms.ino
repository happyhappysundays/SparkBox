#include "Spark.h"
#include "SparkComms.h"

const uint8_t notifyOn[] = {0x1, 0x0};

// client callback for connection to Spark

class MyClientCallback : public BLEClientCallbacks
{
  void onConnect(BLEClient *pclient)
  {
    DEBUG("callback: Spark connected");
    set_conn_status_connected(SPK);
  }

  void onDisconnect(BLEClient *pclient)
  {
//    if (pclient->isConnected()) {

    connected_sp = false;         
    DEBUG("callback: Spark disconnected");   
    set_conn_status_disconnected(SPK);   
  }
};

// server callback for connection to BLE app

class MyServerCallback : public BLEServerCallbacks
{
  void onConnect(BLEServer *pserver)
  {
    set_conn_status_connected(APP);
    DEBUG("callback: BLE app connected");
  }

  void onDisconnect(BLEServer *pserver)
  {

//    if (pserver->getConnectedCount() == 1) {
    DEBUG("callback: BLE app disconnected");
    set_conn_status_disconnected(APP);
  }
};

// BLE MIDI
#ifdef BLE_APP_MIDI
class MyMIDIServerCallback : public BLEServerCallbacks
{
  void onConnect(BLEServer *pserver)
  {
    //set_conn_status_connected(APP);
    DEBUG("callback: BLE MIDI connected");
  }

  void onDisconnect(BLEServer *pserver)
  {

//    if (pserver->getConnectedCount() == 1) {
    DEBUG("callback: BLE MIDI disconnected");
    //set_conn_status_disconnected(APP);
  }
};
#endif

#ifdef CLASSIC
// server callback for connection to BT classic app

void bt_callback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param){
  if(event == ESP_SPP_SRV_OPEN_EVT){
    DEBUG("callback: Classic BT app connected");
    //set_conn_status_connected(APP);
  }
 
  if(event == ESP_SPP_CLOSE_EVT ){
    DEBUG("callback: Classic BT app disconnected");
    set_conn_status_disconnected(APP);
  }
}
#endif

void notifyCB_sp(BLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {

  int i;
  byte b;
  
  for (i = 0; i < length; i++) {
    b = pData[i];
    ble_in.add(b);
  }
  ble_in.commit();
}

#ifdef BLE_CONTROLLER
// This works with IK Multimedia iRig Blueboard and the Akai LPD8 wireless - interestingly they have the same UUIDs
void notifyCB_pedal(BLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify){

  int i;
  byte b;

  for (i = 0; i < length; i++) {
    b = pData[i];
    midi_in.add(b);
  }
  midi_in.commit();

  set_conn_received(BLE_MIDI);  
}
#endif

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


// BLE apP MIDI
#ifdef BLE_APP_MIDI
class MIDICharacteristicCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) {
        int j, l;
        const char *p;
        byte b;
        l = pCharacteristic->getValue().length();
        p = pCharacteristic->getValue().c_str();
        for (j=0; j < l; j++) {
          b = p[j];
          ble_midi_in.add(b);
        }
        ble_midi_in.commit();
  };
};

static MIDICharacteristicCallbacks chrCallbacksMIDI;
#endif

BLEUUID SpServiceUuid(C_SERVICE);
#ifdef BLE_CONTROLLER  
BLEUUID PedalServiceUuid(PEDAL_SERVICE);
#endif


void connect_spark() {
  if (found_sp && !connected_sp) {
    if (pClient_sp != nullptr && pClient_sp->isConnected())
       DEBUG("HMMMM - connect_spark() SAYS I WAS CONNECTED ANYWAY");
    
    if (pClient_sp->connect(sp_device)) {
#ifdef CLASSIC  
      pClient_sp->setMTU(517);  
#endif
      connected_sp = true;
      pService_sp = pClient_sp->getService(SpServiceUuid);
      if (pService_sp != nullptr) {
        pSender_sp   = pService_sp->getCharacteristic(C_CHAR1);
        pReceiver_sp = pService_sp->getCharacteristic(C_CHAR2);
        if (pReceiver_sp && pReceiver_sp->canNotify()) {
          pReceiver_sp->registerForNotify(notifyCB_sp);
#ifdef CLASSIC
          p2902_sp = pReceiver_sp->getDescriptor(BLEUUID((uint16_t)0x2902));
          if (p2902_sp != nullptr)
             p2902_sp->writeValue((uint8_t*)notifyOn, 2, true);
#else
          if (!pReceiver_sp->subscribe(true, notifyCB_sp, true)) {
            connected_sp = false;
            DEBUG("Spark disconnected");
            NimBLEDevice::deleteClient(pClient_sp);
          }   
#endif
        } 
      }
      DEBUG("connect_spark(): Spark connected");
    }
  }
}


#ifdef BLE_CONTROLLER
void connect_pedal() {
  if (found_pedal && !connected_pedal) {
    if (pClient_pedal->connect(pedal_device)) {  
#ifdef CLASSIC
      pClient_sp->setMTU(517);
#endif
      connected_pedal = true;
      pService_pedal = pClient_pedal->getService(PedalServiceUuid);
      if (pService_pedal != nullptr) {
        pReceiver_pedal = pService_pedal->getCharacteristic(PEDAL_CHAR);

        if (pReceiver_pedal && pReceiver_pedal->canNotify()) {
#ifdef CLASSIC
          pReceiver_pedal->registerForNotify(notifyCB_pedal);
          p2902_pedal = pReceiver_pedal->getDescriptor(BLEUUID((uint16_t)0x2902));
          if(p2902_pedal != nullptr)
            p2902_pedal->writeValue((uint8_t*)notifyOn, 2, true);
#else
          if (!pReceiver_pedal->subscribe(true, notifyCB_pedal, true)) {
            connected_pedal = false;
            DEBUG("Pedal disconnected");
            NimBLEDevice::deleteClient(pClient_pedal);
          } 
#endif
        }
      }
      DEBUG("connect_pedal(): pedal connected");
      set_conn_status_connected(BLE_MIDI);
    }
  }
}
#endif

void connect_to_all() {
  int i, j;
  uint8_t b;
  unsigned long t;

  // set up connection status tracking array
  t = millis();
  for (i = 0; i < NUM_CONNS; i++) {
    conn_status[i] = false;
    for (j = 0; j < 3; j++)
      conn_last_changed[j][i] = t;
  }

  is_ble = true;

  BLEDevice::init("Spark 40 MIDI BLE");
  pClient_sp = BLEDevice::createClient();
  pClient_sp->setClientCallbacks(new MyClientCallback());
  
#ifdef BLE_CONTROLLER  
  pClient_pedal = BLEDevice::createClient();
#endif
  pScan = BLEDevice::getScan();
  
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallback());  
  pService = pServer->createService(S_SERVICE);

#ifdef CLASSIC  
  pCharacteristic_receive = pService->createCharacteristic(S_CHAR1, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
  pCharacteristic_send = pService->createCharacteristic(S_CHAR2, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
#else
  pCharacteristic_receive = pService->createCharacteristic(S_CHAR1, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
  pCharacteristic_send = pService->createCharacteristic(S_CHAR2, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY); 
#endif

  pCharacteristic_receive->setCallbacks(&chrCallbacks_r);
  pCharacteristic_send->setCallbacks(&chrCallbacks_s);
#ifdef CLASSIC
  pCharacteristic_send->addDescriptor(new BLE2902());
#endif


#ifdef BLE_APP_MIDI
//  pServerMIDI = BLEDevice::createServer();
//  pServerMIDI->setCallbacks(new MyMIDIServerCallback());  
  pServiceMIDI = pServer->createService(MIDI_SERVICE);

#ifdef CLASSIC  
  pCharacteristicMIDI = pServiceMIDI->createCharacteristic(MIDI_CHAR, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR | BLECharacteristic::PROPERTY_NOTIFY);
#else
  pCharacteristicMIDI = pServiceMIDI->createCharacteristic(MIDI_CHAR, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR | NIMBLE_PROPERTY::NOTIFY);
#endif

  pCharacteristicMIDI->setCallbacks(&chrCallbacksMIDI);
#ifdef CLASSIC
  pCharacteristicMIDI->addDescriptor(new BLE2902()); 
#endif
#endif


  pService->start();
#ifdef BLE_APP_MIDI
  pServiceMIDI->start();
#endif

#ifndef CLASSIC
  pServer->start(); 
#endif

  pAdvertising = BLEDevice::getAdvertising(); // create advertising instance
  pAdvertising->addServiceUUID(pService->getUUID()); // tell advertising the UUID of our service
#ifdef BLE_APP_MIDI
  pAdvertising->addServiceUUID(pServiceMIDI->getUUID()); // tell advertising the UUID of our service
#endif
  pAdvertising->setScanResponse(true);  

  // Connect to Spark
  connected_sp = false;
  found_sp = false;
 
#ifdef BLE_CONTROLLER
  connected_pedal = false;
  found_pedal = false;
#endif

DEBUG("Scanning...");

  while (!found_sp) {   // assume we only use a pedal if on already and hopefully found at same time as Spark, don't wait for it
    pResults = pScan->start(4);
    
    for(i = 0; i < pResults.getCount()  && !found_sp; i++) {
      device = pResults.getDevice(i);

      if (device.isAdvertisingService(SpServiceUuid)) {
        DEBUG("Found Spark");
        found_sp = true;
        connected_sp = false;
        sp_device = new BLEAdvertisedDevice(device);
      }
      
#ifdef BLE_CONTROLLER
      if (device.isAdvertisingService(PedalServiceUuid) || strcmp(device.getName().c_str(),"iRig BlueBoard") == 0) {
        DEBUG("Found pedal");
        found_pedal = true;
        connected_pedal = false;
        pedal_device = new BLEAdvertisedDevice(device);
      }
#endif
    }
  }

    // Set up client
  connect_spark();
#ifdef BLE_CONTROLLER
  connect_pedal();
#endif    

#ifdef CLASSIC
  DEBUG("Starting classic bluetooth");
  // now advertise Serial Bluetooth
  bt = new BluetoothSerial();
  bt->register_callback(bt_callback);
  if (!bt->begin (SPARK_BT_NAME)) {
    DEBUG("Classic bluetooth init fail");
    while (true);
  }

  // flush anything read from App - just in case
  while (bt->available())
    b = bt->read(); 
  DEBUG("Spark 40 Audio set up");
#endif

  DEBUG("Available for app to connect...");  
  pAdvertising->start(); 
}


// app_available both returns whether any data is available but also selects which type of bluetooth to use based
// on whether there is any input there
bool app_available() {
  if (!ble_app_in.is_empty()) {
    is_ble = true;
    return true;
  }
#ifdef CLASSIC    
  if (bt->available()) {
    is_ble = false;
    return true;
  }
#endif
  // if neither have input, then there definitely is no input
  return false;
}

uint8_t app_read() {
  set_conn_received(APP);
  if (is_ble) {
     uint8_t b;
     ble_app_in.get(&b);
     return b;
  }
#ifdef CLASSIC  
  else
    return bt->read();
#endif
}

void app_write(byte *buf, int len) {
  set_conn_sent(APP);
  if (is_ble) {
    pCharacteristic_send->setValue(buf, len);
    pCharacteristic_send->notify(true);
  }
#ifdef CLASSIC 
  else {
    bt->write(buf, len);
  }
#endif
}


void app_write_timed(byte *buf, int len) {               // same as app_write but with a slight delay for classic bluetooth - it seems to need it
  set_conn_sent(APP);
  if (is_ble) {
    pCharacteristic_send->setValue(buf, len);
    pCharacteristic_send->notify(true);
  }
#ifdef CLASSIC 
  else {
    bt->write(buf, len);
    delay(50);                // this helps the timing of a 'fake' store hardware preset
  }
#endif
}

bool sp_available() {
  return !ble_in.is_empty();
}

uint8_t sp_read() {
  uint8_t b;
  
  set_conn_received(SPK);
  ble_in.get(&b);
  return b;
}

void sp_write(byte *buf, int len) {
  set_conn_sent(SPK);  
  pSender_sp->writeValue(buf, len, false);
}

// for some reason getRssi() crashes with two clients!
int ble_getRSSI() { 
#ifdef BLE_CONTROLLER  
  return 0;
#else
  return pClient_sp->getRssi();
#endif
}


// Code to enable UI changes


void set_conn_received(int connection) {
  conn_last_changed[FROM][connection] = millis();
}

void set_conn_sent(int connection) {
  conn_last_changed[TO][connection] = millis();
}

void set_conn_status_connected(int connection) {
  if (conn_status[connection] == false) {
    conn_status[connection] = true;
    conn_last_changed[STATUS][connection] = millis();
  }
}

void set_conn_status_disconnected(int connection) {
  if (conn_status[connection] == true) {
    conn_status[connection] = false;
    conn_last_changed[STATUS][connection] = millis();
  }
}
