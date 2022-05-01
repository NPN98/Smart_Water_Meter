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
#include <HTTPClient.h>
#include "flow_sensor.h"

#define INTERVAL 30000
#define TIME_GET_DATA_SENSOR 1000
#define TIME_CHECK_LOCK_VAN 5000
#define TIME_CONTROL_VAN 15000
#define DEVICE_ID "Device0001"
#define USER_ID "User0001"
#define MESSAGE_MAX_LEN 256
#define PIN_CONTROL_OPEN 2
#define PIN_CONTROL_CLOSE 17
#define VALVE_OPEN 0
#define VALVE_CLOSE 1

unsigned long lastTimeSensor; //

const char *ssid = "Binary";
const char *password = "123456778";
/*String containing Hostname, Device Id & Device Key in the format:*/
static const char *connectionString = "HostName=WaterMeterSystemIoTHub.azure-devices.net;DeviceId=waterMeterDevice1;SharedAccessKey=l8DAa/iZEdHnCmNC2ZzOIMJ6wxK1171GMmiU9ytUgsw=";
const char *messageData = "{\"deviceId\":\"%s\", \"userId\":\"%s\",\"messageId\":%d,  \"waterIndex\":%.d, \"valveStatus\":%d, \"batteryPower\":%.2f}";
int messageCount = 1;
static bool hasWifi = false;
static bool messageSending = true;
static uint64_t send_interval_ms;

unsigned long waterIndex;
float batteryPower = 69.96;
int valveStatus = VALVE_OPEN;
; /*TODO: use boolean*/

String serverName = "https://nodejstriggerfunction.azurewebsites.net/api/deviceId/";
boolean requestLock = false;
boolean closing = false;
boolean opening = false;
unsigned long lastTimeGetRequestLock = 0;
unsigned long timeStartControlVan = 0;
// unsigned long timerDelay = 5000;
//  unsigned long timerDelayControl = 15000;
String payload = "";

static void InitWifi()
{
  Serial.println("Connecting...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
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

static void MessageCallback(const char *payLoad, int size)
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

static int DeviceMethodCallback(const char *methodName, const unsigned char *payload, int size, unsigned char **response, int *response_size)
{
  LogInfo("Try to invoke method %s", methodName);
  const char *responseMessage = "\"Successfully invoke device method\"";
  int result = 200;

  if (strcmp(methodName, "start") == 0)
  {
    LogInfo("Start sending data");
    messageSending = true;
  }
  else if (strcmp(methodName, "stop") == 0)
  {
    LogInfo("Stop sending data");
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
void sendDataAzureIoT()
{
  char messagePayload[MESSAGE_MAX_LEN];
  // "{\"deviceId\":\"%s\", \"userId\":\"%s\",\"messageId\":%d,  \"waterIndex\":%f, \"valveStatus\":%d, \"batteryPower\":%f}";
  snprintf(messagePayload, MESSAGE_MAX_LEN, messageData, DEVICE_ID, USER_ID, messageCount++, waterIndex, valveStatus, batteryPower);
  Serial.println(messagePayload);
  EVENT_INSTANCE *message = Esp32MQTTClient_Event_Generate(messagePayload, MESSAGE);
  Esp32MQTTClient_Event_AddProp(message, "temperatureAlert", "true");
  Esp32MQTTClient_SendEventInstance(message);
  send_interval_ms = millis();
}

void controlVan()
{
  // request lock
  if (requestLock == true && valveStatus == VALVE_OPEN)
  {
    if (closing == false)
    {
      timeStartControlVan = millis();
      Serial.println("Start close Van ");
      digitalWrite(PIN_CONTROL_CLOSE, 1);
      closing = true;
    }

    if (closing == true)
    {
      if ((millis() - timeStartControlVan) > TIME_CONTROL_VAN)
      {
        Serial.println("End close Van ");
        digitalWrite(PIN_CONTROL_CLOSE, 0);
        valveStatus = VALVE_CLOSE;
        closing = false;
      }
    }
  }

  // request open
  if (requestLock == false && valveStatus == VALVE_CLOSE)
  {
    if (opening == false)
    {
      timeStartControlVan = millis();
      Serial.println(" Opening van ");
      digitalWrite(PIN_CONTROL_CLOSE, 1);
      opening = true;
    }

    if (opening == true)
    {
      if ((millis() - timeStartControlVan) > TIME_CONTROL_VAN)
      {
        Serial.println("End Open van");
        digitalWrite(PIN_CONTROL_OPEN, 0);
        valveStatus = VALVE_OPEN;
        opening = false;
      }
    }
  }
}

void checkRequestLockVan()
{
  // Check WiFi connection status
  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;
    String serverPath = serverName + "Device0001";
    // Your Domain name with URL path or IP address with path
    http.begin(serverPath.c_str());
    // Send HTTP GET request
    int httpResponseCode = http.GET();
    if (httpResponseCode > 0)
    {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      payload = "";
      payload = http.getString();
      if (payload == "True")
      {
        requestLock = true;
      }
      else
      {
        requestLock = false;
      }
      Serial.println(payload);
    }
    else
    {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    // Free resources
    http.end();
  }
  else
  {
    Serial.println("WiFi Disconnected");
  }
  lastTimeGetRequestLock = millis();
}
void setup()
{
  pinMode(PIN_CONTROL_OPEN, OUTPUT);
  pinMode(PIN_CONTROL_CLOSE, OUTPUT);
  Serial.begin(115200);
  void initFlowSensor();
  lastTimeSensor = 0;

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
  Esp32MQTTClient_Init((const uint8_t *)connectionString, true);

  Esp32MQTTClient_SetSendConfirmationCallback(SendConfirmationCallback);
  Esp32MQTTClient_SetMessageCallback(MessageCallback);
  Esp32MQTTClient_SetDeviceTwinCallback(DeviceTwinCallback);
  Esp32MQTTClient_SetDeviceMethodCallback(DeviceMethodCallback);

  send_interval_ms = millis();
}

void loop()
{
  if ((millis() - lastTimeSensor) > TIME_GET_DATA_SENSOR) // Only process counters once per second
  {
    // readFlowSensor(oldTime);
    //  Serial.println();
    //  Serial.print("Output Liquid Quantity: ");
    waterIndex = readFlowSensor(lastTimeSensor);
    // Serial.print(waterIndex);
    // Serial.println("mL");
    lastTimeSensor = millis();
  }
  if (hasWifi)
  {
    if (messageSending && (int)(millis() - send_interval_ms) >= INTERVAL)
    {
      // Send data to azure IoT hub
      sendDataAzureIoT();
    }
    else
    {
      Esp32MQTTClient_Check();
    }

    if ((millis() - lastTimeGetRequestLock) > TIME_CHECK_LOCK_VAN)
    {
      checkRequestLockVan();
    }
  }
  // delay(10);
  controlVan();
}