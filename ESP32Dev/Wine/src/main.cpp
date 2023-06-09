#include <WiFi.h>
#include <EloquentTinyML.h>
#include "wine_model.h"
#include "esp_timer.h"

#define NUMBER_OF_INPUTS 13
#define NUMBER_OF_OUTPUTS 3
#define TENSOR_ARENA_SIZE 16 * 1024

Eloquent::TinyML::TfLite<NUMBER_OF_INPUTS, NUMBER_OF_OUTPUTS, TENSOR_ARENA_SIZE> tf;

// FreeRTOS timers

extern "C"{
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
}
#include <AsyncMqttClient.h>

// WiFi SSID and password

#define WIFI_SSID "Berto's iPhone"
#define WIFI_PASSWORD "fogfogfog"

// MQTT Broker configuration and port

#define MQTT_HOST IPAddress(172, 20, 10, 5)
#define MQTT_PORT 1883

// Global variables for MQTT and timer for handling the reconnections

AsyncMqttClient mqttClient;
TimerHandle_t mqttReconnectTimer;
TimerHandle_t wifiReconnectTimer;

// Variable to keep track of the iteration number

int currentIteration = 0;

// Elements for evaluation

float X_test[20][13] = {
    {1.340e+01, 4.600e+00, 2.860e+00, 2.500e+01, 1.120e+02, 1.980e+00,
     9.600e-01, 2.700e-01, 1.110e+00, 8.500e+00, 6.700e-01, 1.920e+00, 6.300e+02},
    {1.285e+01, 3.270e+00, 2.580e+00, 2.200e+01, 1.060e+02, 1.650e+00,
     6.000e-01, 6.000e-01, 9.600e-01, 5.580e+00, 8.700e-01, 2.110e+00, 5.700e+02},
    {1.334e+01, 9.400e-01, 2.360e+00, 1.700e+01, 1.100e+02, 2.530e+00,
     1.300e+00, 5.500e-01, 4.200e-01, 3.170e+00, 1.020e+00, 1.930e+00, 7.500e+02},
    {1.423e+01, 1.710e+00, 2.430e+00, 1.560e+01, 1.270e+02, 2.800e+00,
     3.060e+00, 2.800e-01, 2.290e+00, 5.640e+00, 1.040e+00, 3.920e+00, 1.065e+03},
    {1.483e+01, 1.640e+00, 2.170e+00, 1.400e+01, 9.700e+01, 2.800e+00,
     2.980e+00, 2.900e-01, 1.980e+00, 5.200e+00, 1.080e+00, 2.850e+00, 1.045e+03},
    {1.245e+01, 3.030e+00, 2.640e+00, 2.700e+01, 9.700e+01, 1.900e+00,
     5.800e-01, 6.300e-01, 1.140e+00, 7.500e+00, 6.700e-01, 1.730e+00, 8.800e+02},
    {1.430e+01, 1.920e+00, 2.720e+00, 2.000e+01, 1.200e+02, 2.800e+00,
     3.140e+00, 3.300e-01, 1.970e+00, 6.200e+00, 1.070e+00, 2.650e+00, 1.280e+03},
    {1.390e+01, 1.680e+00, 2.120e+00, 1.600e+01, 1.010e+02, 3.100e+00,
     3.390e+00, 2.100e-01, 2.140e+00, 6.100e+00, 9.100e-01, 3.330e+00, 9.850e+02},
    {1.165e+01, 1.670e+00, 2.620e+00, 2.600e+01, 8.800e+01, 1.920e+00,
     1.610e+00, 4.000e-01, 1.340e+00, 2.600e+00, 1.360e+00, 3.210e+00, 5.620e+02},
    {1.386e+01, 1.510e+00, 2.670e+00, 2.500e+01, 8.600e+01, 2.950e+00,
     2.860e+00, 2.100e-01, 1.870e+00, 3.380e+00, 1.360e+00, 3.160e+00, 4.100e+02},
    {1.377e+01, 1.900e+00, 2.680e+00, 1.710e+01, 1.150e+02, 3.000e+00,
     2.790e+00, 3.900e-01, 1.680e+00, 6.300e+00, 1.130e+00, 2.930e+00, 1.375e+03},
    {1.296e+01, 3.450e+00, 2.350e+00, 1.850e+01, 1.060e+02, 1.390e+00,
     7.000e-01, 4.000e-01, 9.400e-01, 5.280e+00, 6.800e-01, 1.750e+00, 6.750e+02},
    {1.305e+01, 5.800e+00, 2.130e+00, 2.150e+01, 8.600e+01, 2.620e+00,
     2.650e+00, 3.000e-01, 2.010e+00, 2.600e+00, 7.300e-01, 3.100e+00, 3.800e+02},
    {1.182e+01, 1.470e+00, 1.990e+00, 2.080e+01, 8.600e+01, 1.980e+00,
     1.600e+00, 3.000e-01, 1.530e+00, 1.950e+00, 9.500e-01, 3.330e+00, 4.950e+02},
    {1.164e+01, 2.060e+00, 2.460e+00, 2.160e+01, 8.400e+01, 1.950e+00,
     1.690e+00, 4.800e-01, 1.350e+00, 2.800e+00, 1.000e+00, 2.750e+00, 6.800e+02},
    {1.303e+01, 9.000e-01, 1.710e+00, 1.600e+01, 8.600e+01, 1.950e+00,
     2.030e+00, 2.400e-01, 1.460e+00, 4.600e+00, 1.190e+00, 2.480e+00, 3.920e+02},
    {1.176e+01, 2.680e+00, 2.920e+00, 2.000e+01, 1.030e+02, 1.750e+00,
     2.030e+00, 6.000e-01, 1.050e+00, 3.800e+00, 1.230e+00, 2.500e+00, 6.070e+02},
    {1.439e+01, 1.870e+00, 2.450e+00, 1.460e+01, 9.600e+01, 2.500e+00,
     2.520e+00, 3.000e-01, 1.980e+00, 5.250e+00, 1.020e+00, 3.580e+00, 1.290e+03},
    {1.420e+01, 1.760e+00, 2.450e+00, 1.520e+01, 1.120e+02, 3.270e+00,
     3.390e+00, 3.400e-01, 1.970e+00, 6.750e+00, 1.050e+00, 2.850e+00, 1.450e+03},
    {1.368e+01, 1.830e+00, 2.360e+00, 1.720e+01, 1.040e+02, 2.420e+00,
     2.690e+00, 4.200e-01, 1.970e+00, 3.840e+00, 1.230e+00, 2.870e+00, 9.900e+02}};

uint8_t y_test[20] = {2, 2, 1, 0, 0, 2, 0, 0, 1, 1, 0, 2, 1, 1, 1, 1, 1, 0, 0, 0};

// Method to connect to WiFi

void connectToWifi()
{
  WiFi.disconnect();
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

// Method to connect to Mqtt

void connectToMqtt()
{
  Serial.println("Connecting to MQTT...");
  mqttClient.setCredentials("federico", "iotexamdemo");
  mqttClient.connect();
}

// Callback for a WiFi event

void WiFiEvent(WiFiEvent_t event) {
    //Serial.printf("[WiFi-event] event: %d\n", event);
    switch(event) {
    case SYSTEM_EVENT_STA_GOT_IP:
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
        connectToMqtt();
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        Serial.println("WiFi lost connection");
        xTimerStop(mqttReconnectTimer, 0); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
        xTimerStart(wifiReconnectTimer, 0);
        break;
    }
}

// Callback for mqtt connection

void onMqttConnect(bool sessionPresent)
{
  Serial.println("Connected to MQTT.");
  Serial.print("Session present: ");
  Serial.println(sessionPresent);
}

// Callback for mqtt disconnection

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{
  Serial.println("Disconnected from MQTT.");

  if (WiFi.isConnected())
  {
    xTimerStart(mqttReconnectTimer, 0);
  }
}

// Setup method

void setup()
{
  Serial.begin(9600);
  Serial.println();

  mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(2000), pdFALSE, (void *)0, reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt));
  wifiReconnectTimer = xTimerCreate("wifiTimer", pdMS_TO_TICKS(2000), pdFALSE, (void *)0, reinterpret_cast<TimerCallbackFunction_t>(connectToWifi));

  WiFi.onEvent(WiFiEvent);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);

  connectToWifi();

   // TensorFlow initialization

  tf.begin(wine_model);

  // check if model loaded fine
  if (!tf.initialized())
  {
    Serial.print("Error initializing TensorFlow");
    while (true)
      delay(1000);
  }
}

// Loop method

void loop()
{
  if (WiFi.isConnected() && mqttClient.connected())
  {
    // Runs a loop of 10 iterations, at every one it sends the result (if enabled)
    // Waits 5 seconds, then restarts

    for (uint8_t i = 0; i < 10; i++)
    {

      currentIteration += 1; // To keep track of iterations between loops

      int start = esp_timer_get_time(); // Evaluation start time
      uint8_t resultClass = tf.predictClass(X_test[i]);
      int end = esp_timer_get_time() - start; // Evaluation end time

      Serial.print("Sample #");
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print("predicted ");
      Serial.print(resultClass);
      Serial.print(" vs ");
      Serial.print(y_test[i]);
      Serial.println(" actual");
      Serial.println("Evaluation time: " + String(end) + " microseconds");

      // DTO to be created
      //{"board": "esp32dev", "model": "wine", "result": 1, "iteration": 1, "microseconds": 120}

      String startPar = "{";
      String board = "\"board\":\"esp32dev\",";
      String model = "\"model\":\"wine\",";
      String result = "\"result\":" + String(resultClass) + ",";
      String iteration = "\"iteration\":" + String(int(currentIteration)) + ",";
      String time = "\"microseconds\":" + String(int(end));
      String endPar = "}";

      String resultString = startPar + board + model + result + iteration + time + endPar;

      uint16_t packetId = mqttClient.publish("iotdemo.esp32dev", 1, true, (char *)resultString.c_str());
      Serial.print("Message sent with packetId: ");
      Serial.println(packetId);
      delay(100);
    }
  }
  else
  {
    Serial.println("WiFi or MQTT not connected, waiting before running evaluation.");
  }

  delay(5000);
}