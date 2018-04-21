# Aquarium Controller

The propose of this project was develop a aquarium controller capable to control:

* Lights
* Coolers
* Heater
* Temperature
* Cover Status
* Cover Alarm
* Time of Feeding
* Time of Filter Maintense

## Hardware

The MCU used was AtMega2560 presents on Arduino Mega. Also as additional hardware modules and sensors, was used this follows:

* Display Touch Screen LCD TFT 2.4"
* RTC (Real Time Control)
* 4 Rele
* Temperature Sensor DS18B20
* LED Lights
* 2 Coolers 12V
* Aquarium Heater

Each item is describe bellow.

## Display Touch Screen LCD TFT 2.4"

It was used to user do all of interactions with the system.In there was implemented a GUI that make easy any type of command that the user want to do.

## RTC (Real Time Control)

It was used to syncronize all of time functions of the system. Such as time of lights on, time of feeding, periodics maintences and others functions that used any type of syncronism with time.

## 4 Rele Module

Two of four rele was used to coolers, another one was used to LED lights and the last one was used for heater.

