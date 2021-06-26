#include "Spark.h"
#include "SparkComms.h"
#include "BluetoothSerial.h"

SparkComms::SparkComms() {
}

SparkComms::~SparkComms() {
}

void SparkComms::start_bt() {
  bt = new BluetoothSerial();
  
  if (!bt->begin (MY_NAME, true)) {
    DEBUG("Bluetooth init fail");
    while (true);
  }    
}

void SparkComms::connect_to_spark() {
  uint8_t b;
  bool connected;

  connected = false;
  while (!connected) {
    connected = bt->connect(SPARK_NAME);
    if (!(connected && bt->hasClient())) {
      connected = false;
      delay(2000);
    }
  }

  // flush anything read from Spark - just in case
  while (bt->available())
    b = bt->read(); 
}
