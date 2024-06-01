#include "DHT.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define DHT_PIN   16
#define DHT_TYPE  DHT22

DHT dht (DHT_PIN, DHT_TYPE);

TaskHandle_t rgb_h;
TaskHandle_t dht_h;
TaskHandle_t bt_h;
TaskHandle_t state_h;

void setup()
{
  Serial.begin(115200);

  xTaskCreatePinnedToCore(pvTask_Rgb,          "rgb_h",    2048, NULL, 1, &rgb_h,    APP_CPU_NUM);
  xTaskCreatePinnedToCore(pvTask_Dht22,        "dht_h",    2048, NULL, 1, &dht_h,    APP_CPU_NUM);
  xTaskCreatePinnedToCore(pvTask_Button,       "bt_h",     2048, NULL, 1, &bt_h,     PRO_CPU_NUM);
  xTaskCreatePinnedToCore(pvTask_StateReport,  "state_h",  2048, NULL, 1, &state_h,  PRO_CPU_NUM);

  dht.begin();
}

void loop()
{
    Serial.print(__func__);
    Serial.print(" : ");
    Serial.print(xTaskGetTickCount());
    Serial.print(" : ");
    Serial.print("Arduino loop is running on core:");
    Serial.println(xPortGetCoreID());
    Serial.println();
}

void pvTask_Rgb(void *arg) {
    while(1) {

        Serial.print(__func__);
        Serial.print(" : ");
        Serial.print(xTaskGetTickCount());
        Serial.print(" : ");
        Serial.print("This loop runs on APP_CPU which id is:");
        Serial.println(xPortGetCoreID());
        Serial.println();

        vTaskDelay(100);
    }
}

void pvTask_Dht22(void *arg)
{
  while(1)
  {
    TempAndHum();

    Serial.print(__func__);
    Serial.print(" : ");
    Serial.print(xTaskGetTickCount());
    Serial.print(" : ");
    Serial.print("This loop runs on APP_CPU which id is:");
    Serial.println(xPortGetCoreID());
    Serial.println();

    vTaskDelay(100);
  }
}

void pvTask_Button(void *arg) {
    while(1) {

        Serial.print(__func__);
        Serial.print(" : ");
        Serial.print(xTaskGetTickCount());
        Serial.print(" : ");
        Serial.print("This loop runs on PRO_CPU which id is:");
        Serial.println(xPortGetCoreID());
        Serial.println();

        vTaskDelay(100);
    }
}

void pvTask_StateReport(void *arg) {
    while(1) {

        Serial.print(__func__);
        Serial.print(" : ");
        Serial.print(xTaskGetTickCount());
        Serial.print(" : ");
        Serial.print("This loop runs on PRO_CPU which id is:");
        Serial.println(xPortGetCoreID());
        Serial.println();

        vTaskDelay(100);
    }
}

void TempAndHum()
{
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature))
  {
    Serial.println("DHT-Sensors not ready!");
  }
  else
  {
    Serial.print("Luftfeuchtigkeit: ");
    Serial.print(humidity);
    Serial.print(" %\t");
    Serial.print("Temperatur: ");
    Serial.print(temperature);
    Serial.println(" *C");
  }
}

