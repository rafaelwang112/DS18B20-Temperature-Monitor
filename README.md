# Embedded temperature monitoring system that uses a DS18B20 temperature sensor. 
## Features include:
  - determining temperature of the room
  - LCD display that shows current temperature as well as high/low temperature threshold settings that can be adjusted with a rotary encoder
  - RGB LED to determine if the temperature is in range, too hot, or too cold with the green LED changing in intensity depending on temperature relative to cold/hot limits
  - servo motor that serves as a 10-second countdown when temperature is out of range of threshold settings
  - RS-232 serial interface that allows local system to transmit its high/low threshold settings to remote system and vice versa (local system can choose which settings to use as threshold limits)
