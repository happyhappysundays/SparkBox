# SparkBox V0.71
SparkBox is a BT pedal for the Positive Grid Spark 40.  I only needed the functionality of the simpler BT pedals. However many of them use captured hex chunks to communicate with the Spark, or were Python based. Instead I wanted to use Paul Hamshere's amazing code to create and process real messages. Also I wanted to extend the functionality a bit and make an attractive UI.

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
- BLE RSSI indicator
- Now with remote guitar tuner display! Select from Spark, or hold all four buttons down to enter/exit.
- Inter-operable with both conventional and Heltec ESP32 modules.
- Sleep modes added to reduce power when disconnected - only for supported hardware

# Arduino libraries and board versions
Under Files->Preferences->Additional Boards Manager URLs, enter the following:
- https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
- https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series/releases/download/0.0.5/package_heltec_esp32_index.json

Under Tools->Board->Board manager ensure that you have the following version:
- Heltec ESP32 Dev-Boards 0.0.5 (Heltec - WiFi Kit 32) **OR**
- ESP32 by Espressif 2.0.2 (ESP32)

Under Tools->Manage Libraries ensure that you have the following libraries and versions:
- ThingPulse SSD1306 driver 4.2.1 (ESP32)
- NimBLE-Arduino 1.3.6

# Compile options

**define CLASSIC**

Uncomment this to use with Android devices that are happier with classic BT code.

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

**define CLASSIC**
Uncomment for better Bluetooth compatibility with Android devices

**define HELTEC_WIFI**
Uncomment when using a Heltec module as their implementation doesn't support setMTU()

**define TWOCOLOUR**
Uncomment if two-colour OLED screens are used. Offsets some text and shows an alternate tuner.

**define NOSLEEP**
Uncomment if you'd prefer not to use the power-saving sleep modes

# Heltec module version
![alt text](https://github.com/happyhappysundays/SparkBox/blob/main/Pictures/Dev_board.jpg?raw=true)
![alt text](https://github.com/happyhappysundays/SparkBox/blob/main/Pictures/Charge_detect.jpg?raw=true)
![alt text](https://github.com/happyhappysundays/SparkBox/blob/main/Pictures/SparkBox_Heltec_Exp_2.png?raw=true)

# ESP32 version
![alt text](https://github.com/happyhappysundays/SparkBox_old/blob/main/Pictures/thumbnail_IMG_6791.jpg?raw=true)
![alt text](https://github.com/happyhappysundays/SparkBox_old/blob/main/Pictures/SparkBox.jpg?raw=true)
![alt text](https://github.com/happyhappysundays/SparkBox_old/blob/main/Pictures/thumbnail_IMG_6785.jpg?raw=true)
![alt text](https://github.com/happyhappysundays/SparkBox_old/blob/main/Pictures/thumbnail_IMG_6786.jpg?raw=true)
![alt text](https://github.com/happyhappysundays/SparkBox_old/blob/main/Pictures/thumbnail_IMG_6994.jpg?raw=true)
![alt text](https://github.com/happyhappysundays/SparkBox/blob/main/Pictures/meter_during.jpg?raw=true)
![alt text](https://github.com/happyhappysundays/SparkBox/blob/main/Pictures/thumbnail_IMG_7475.jpg?raw=true)
![alt text](https://github.com/happyhappysundays/SparkBox_old/blob/main/Pictures/V0_4.jpg?raw=true)
![alt text](https://github.com/happyhappysundays/SparkBox/blob/main/Pictures/SparkBox_Heltec_Exp.png?raw=true)
![alt text](https://github.com/happyhappysundays/SparkBox_old/blob/main/Pictures/SparkBox_Battery.png?raw=true)

# ESP32 pedal parts list

| Item | Description           | Link               |
| -----| ----------------------|--------------------|
|   1  | Box                   |https://www.aliexpress.com/item/32693268669.html?spm=a2g0s.9042311.0.0.27424c4dlzGiUH
|   2  | Stomp switch          |https://www.aliexpress.com/item/32918205335.html?spm=a2g0s.9042311.0.0.27424c4dszp4Ie
|   3  | ESP-WROOM-32U module  |https://www.aliexpress.com/item/32864722159.html?spm=a2g0s.9042311.0.0.27424c4dlzGiUH
|   4  | LCD screen            |https://www.ebay.com.au/itm/333085424031
|   5  | BT antenna            |https://www.aliexpress.com/item/4001054693109.html?spm=a2g0s.9042311.0.0.27424c4dlzGiUH and https://www.ebay.com.au/itm/233962468558
|   6  | USB extension         |https://www.aliexpress.com/item/32808991941.html?spm=a2g0s.9042311.0.0.27424c4dlzGiUH
|   7  | Power switch          |https://www.jaycar.com.au/dpdt-miniature-toggle-switch-solder-tag/p/ST0355
|   8  | DC input jack         |https://www.jaycar.com.au/2-5mm-bulkhead-male-dc-power-connector/p/PS0524
|   9  | Pedal jack            |https://www.jaycar.com.au/6-5mm-stereo-enclosed-insulated-switched-socket/p/PS0184
|  10  | LiPo battery          |https://www.ebay.com.au/itm/133708965813
|  11  | LiPo charger          |https://www.ebay.com.au/itm/161821599467
|  12  | LiPo booster          |https://www.jaycar.com.au/arduino-compatible-5v-dc-to-dc-converter-module/p/XC4512
|  13  | 9V to 5V converter    |https://www.ebay.com.au/itm/303839459634
|  14  | Glass window (opt)    |https://www.aliexpress.com/item/4000377316108.html?spm=a2g0s.12269583.0.0.1a1e62440DlgU2
