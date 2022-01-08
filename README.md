# SparkBoxHeltec V0.58
This is a Heltec WIFI Kit 32 version of SparkBox. SparkBox is another BT pedal for the Positive Grid Spark 40.  I only needed the functionality of the simpler BT pedals. However many of them use captured hex chunks to communicate with the Spark, or were Python based. Instead I wanted to use Paul Hamshere's amazing code to create and process real messages. Also I wanted to extend the functionality a bit and make an attractive UI.

# Functions
- Expression pedal input on D34 for altering the current parameter or on/off switch
- Uses BLE so that it can be used with music function of the Spark app
- Allows connection of the app for full simultaneous control
- Switch presets either on footswitch, app or Spark to update display
- Switch on and off all four major effects dynamically
- Graphically display the effect state on the display
- Supports 4-button pedals
- Hold down any switch for 1s to switch between Effect mode and Preset mode
- Battery level indicator on UI
- BLE RSSI inidcator

# Arduino libraries and board versions
Under Files->Preferences->Additional Boards Manager URLs, enter the following:
- https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
- https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series/releases/download/0.0.5/package_heltec_esp32_index.json

Under Tools->Board->Board manager ensure that you have the following version:
- Heltec ESP32 Dev-Boards 0.0.5 (SparkBoxHeltec - WiFi Kit 32)

Under Tools->Manage Libraries ensure that you have the following libraries and versions:
- Heltec ESP32 Dev-Boards 1.1.0 (SparkBoxHeltec - WiFi Kit 32)
- NimBLE-Arduino 1.3.3

# Compile options

Uncomment ONE battery option to match your hardware.

**define BATT_CHECK_0**

You have no mods to monitor the battery, so it will show empty (default).

**define BATT_CHECK_1**

You are monitoring the battery via a 2:1 10k/10k resistive divider to GPIO23.
You can see an accurate representation of the remaining battery charge and a kinda-sorta
indicator of when the battery is charging. Maybe.

**define BATT_CHECK_2**

You have the battery monitor mod described above AND you have a connection between the 
CHRG pin of the charger chip and GPIO 33. Go you! Now you have a guaranteed charge indicator too.

**define EXPRESSION_PEDAL**

Expression pedal define. Comment this out if you DO NOT have the expression pedal mod.

**define DUMP_ON**
Dump preset define. Comment out if you'd prefer to not see so much text output

![alt text](https://github.com/happyhappysundays/SparkBoxHeltec/blob/main/Pictures/Dev_board.jpg?raw=true)
![alt text](https://github.com/happyhappysundays/SparkBoxHeltec/blob/main/Pictures/Charge_detect.jpg?raw=true)
![alt text](https://github.com/happyhappysundays/SparkBoxHeltec/blob/main/Pictures/SparkBox_Heltec_Exp_2.png?raw=true)

