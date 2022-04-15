/*
 * SMART WATER METER
 * Author: Nguyen Phuoc nguyen
 * Description: This is project get data water everydate and update it on azure IoT
 * References: Example DHT library, Example library Esp32 Azure IoT Arduino 
 * Hardware: Esp32 Module
 * 
*/
#include <WiFi.h>
#include "AzureIotHub.h"
#include "Esp32MQTTClient.h"
#define INTERVAL 3000
#define DEVICE_ID "Device0001"
#define USER_ID "User0001"
#define MESSAGE_MAX_LEN 256

const char* ssid     = "Binary";
const char* password = "123456778";
/*String containing Hostname, Device Id & Device Key in the format:*/  
static const char* connectionString =  "HostName=WaterMeterSystemIoTHub.azure-devices.net;DeviceId=waterMeterDevice1;SharedAccessKey=l8DAa/iZEdHnCmNC2ZzOIMJ6wxK1171GMmiU9ytUgsw=" ; 
const char *messageData = "{\"deviceId\":\"%s\", \"userId\":\"%s\",\"messageId\":%d,  \"waterIndex\":%.2f, \"valveStatus\":%d, \"batteryPower\":%.2f}";
int messageCount = 1;
static bool hasWifi = false;
static bool messageSending = true;
static uint64_t send_interval_ms;

float waterIndex = 99.66;
float batteryPower =69.96;
int valveStatus = 0; /*TODO: use boolean*/

static void InitWifi()
{
  Serial.println("Connecting...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  hasWifi = true;
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

static void SendConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result)
{
  if (result == IOTHUB_CLIENT_CONFIRMATION_OK)
  {
    Serial.println("Send Confirmation Callback finished.");
  }
}

static void MessageCallback(const char* payLoad, int size)
{
  Serial.println("Message callback:");
  Serial.println(payLoad);
}

static void DeviceTwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char *payLoad, int size)
{
  char *temp = (char *)malloc(size + 1);
  if (temp == NULL)
  {
    return;
  }
  memcpy(temp, payLoad, size);
  temp[size] = '\0';
  // Display Twin message.
  Serial.println(temp);
  free(temp);
}

static int  DeviceMethodCallback(const char *methodName, const unsigned char *payload, int size, unsigned char **response, int *response_size)
{
  LogInfo("Try to invoke method %s", methodName);
  const char *responseMessage = "\"Successfully invoke device method\"";
  int result = 200;

  if (strcmp(methodName, "start") == 0)
  {
    LogInfo("Start sending temperature and humidity data");
    messageSending = true;
  }
  else if (strcmp(methodName, "stop") == 0)
  {
    LogInfo("Stop sending temperature and humidity data");
    messageSending = false;
  }
  else
  {
    LogInfo("No method %s found", methodName);
    responseMessage = "\"No method found\"";
    result = 404;
  }

  *response_size = strlen(responseMessage) + 1;
  *response = (unsigned char *)strdup(responseMessage);

  return result;
}

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 Device");
  Serial.println("Initializing...");
  Serial.println(" > WiFi");
  hasWifi = false;
  InitWifi();
  if (!hasWifi)
  {
    return;
  }
  Serial.println(" > IoT Hub");
  Esp32MQTTClient_SetOption(OPTION_MINI_SOLUTION_NAME, "GetStarted");
  Esp32MQTTClient_Init((const uint8_t*)connectionString, true);

  Esp32MQTTClient_SetSendConfirmationCallback(SendConfirmationCallback);
  Esp32MQTTClient_SetMessageCallback(MessageCallback);
  Esp32MQTTClient_SetDeviceTwinCallback(DeviceTwinCallback);
  Esp32MQTTClient_SetDeviceMethodCallback(DeviceMethodCallback);

  send_interval_ms = millis();
}

void loop() {
  if (hasWifi)
  {
    if (messageSending && 
        (int)(millis() - send_interval_ms) >= INTERVAL)
    {
      // Send teperature data
      char messagePayload[MESSAGE_MAX_LEN];
      // "{\"deviceId\":\"%s\", \"userId\":\"%s\",\"messageId\":%d,  \"waterIndex\":%f, \"valveStatus\":%d, \"batteryPower\":%f}";
      snprintf(messagePayload,MESSAGE_MAX_LEN, messageData, DEVICE_ID, USER_ID, messageCount++, waterIndex, valveStatus, batteryPower);
      Serial.println(messagePayload);
      EVENT_INSTANCE* message = Esp32MQTTClient_Event_Generate(messagePayload, MESSAGE);
      Esp32MQTTClient_Event_AddProp(message, "temperatureAlert", "true");
      Esp32MQTTClient_SendEventInstance(message);
      
      send_interval_ms = millis();
    }
    else
    {
      Esp32MQTTClient_Check();
    }
  }
  delay(10);
}