/**
  ******************************************************************************
  * @file    ds18b20.h
  * @author  Nguyen Phuoc Nguyen
  * @date    08-11-2020
  * @brief   This file contains definitions for 
	
  ******************************************************************************
  */
#include "flow_sensor.h"
void printHello(){
    Serial.println(" wtf mannnnnnnnnnnnnnnnnnnnnn");
}

// struct flowSensor
// {
//     const uint8_t PIN;
//     unsigned long oldTime;
//     float calibrationFactor = 4.5;
//     volatile byte pulseCount;
//     float flowRate;
//     unsigned int flowMilliLitres;
//     unsigned long totalMilliLitres;
// };

byte sensorPin = 18;
float calibrationFactor = 4.5;
volatile byte pulseCount;
float flowRate;
unsigned int flowMilliLitres;
unsigned long totalMilliLitres;
// unsigned long oldTime;

/*
Insterrupt Service Routine
 */
void IRAM_ATTR pulseCounter()
{
    // Increment the pulse counter
    pulseCount++;

}
void initFlowSensor(){
    pinMode(sensorPin, INPUT_PULLUP);
    //digitalWrite(sensorPin, HIGH);
    pulseCount = 0;
    flowRate = 0.0;
    flowMilliLitres = 0;
    totalMilliLitres = 0;
    attachInterrupt(sensorPin, pulseCounter, FALLING);
}
unsigned long readFlowSensor(  unsigned long oldTimeSensor ){
    detachInterrupt(sensorPin);
    flowRate = ((1000.0 / (millis() - oldTimeSensor)) * pulseCount) / calibrationFactor;
    oldTimeSensor = millis();
    flowMilliLitres = (flowRate / 60) * 1000;
    totalMilliLitres += flowMilliLitres;
    // unsigned int frac;
    // //Print the flow rate for this second in litres / minute
    // Serial.print("Flow rate: ");
    // Serial.print(int(flowRate)); // Print the integer part of the variable
    // Serial.print("L/min");
    // Serial.print("\t"); // Print tab space
    //                     // Print the cumulative total of litres flowed since starting
    // Serial.print("Output Liquid Quantity: ");
    // Serial.print(totalMilliLitres);
    // Serial.println("mL");
    // Serial.print("\t"); // Print tab space
    // Serial.print(totalMilliLitres / 1000);
    // Serial.print("L");
    // Reset the pulse counter so we can start incrementing again
    pulseCount = 0;
    // Enable the interrupt again now that we've finished sending output
    attachInterrupt(sensorPin, pulseCounter, FALLING);
    return (totalMilliLitres) ;
}
