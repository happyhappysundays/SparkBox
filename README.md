# SparkBoxHeltec V0.56
This is a Heltec WIFI Kit 32 version of SparkBox. SparkBox is another BT pedal for the Positive Grid Spark 40.  I only needed the functionality of the simpler BT pedals. However many of them use captured hex chunks to communicate with the Spark, or were Python based. Instead I wanted to use Paul Hamshere's amazing code to create and process real messages. Also I wanted to extend the functionality a bit and make a pretty UI.

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



![alt text](https://github.com/happyhappysundays/SparkBoxHeltec/blob/main/Pictures/Dev_board.jpg?raw=true)
![alt text](https://github.com/happyhappysundays/SparkBoxHeltec/blob/main/Pictures/Charge_detect.jpg?raw=true)
![alt text](https://github.com/happyhappysundays/SparkBoxHeltec/blob/main/Pictures/SparkBox_Heltec_Exp_2.png?raw=true)

