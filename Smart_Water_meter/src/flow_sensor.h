/**
  ******************************************************************************
  * @file    flow_sensor.h
  * @author  Nguyen Phuoc Nguyen
  * @date    16-04-2022
  * @brief   This file contains definitions for 
 ******************************************************************************
  */
#ifndef __FLOW_SENSOR_H
#define __FLOW_SENSOR_H	
#include "HardwareSerial.h"
#include "Arduino.h"
#include "esp32-hal-gpio.h"
void printHello();
unsigned long readFlowSensor(  unsigned long oldTimeSensor );
void initFlowSensor();
#endif