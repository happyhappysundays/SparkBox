#ifndef SparkComms_h
#define SparkComms_h

#include "BluetoothSerial.h"

// Bluetooth vars
#define  SPARK_NAME  "Spark 40 Audio"
#define  MY_NAME     "SparkBox"

class SparkComms {
  public:
    SparkComms();
    ~SparkComms();

    void start_bt();
    void connect_to_spark();

    // bluetooth communications
    BluetoothSerial *bt;
};

#endif
