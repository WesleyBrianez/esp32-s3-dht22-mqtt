#include "DHT.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "api_structs.h"

#define DHT_PIN       16
#define DHT_TYPE      DHT22
#define BT_PIN        GPIO_NUM_14
#define IS_BT_PRESSED !gpio_get_level(BT_PIN)

DHT dht (DHT_PIN, DHT_TYPE);

sButton button;
uint8_t led_collor; //eLedCollor
uint8_t led_state; //eLedState

TaskHandle_t rgb_h;
TaskHandle_t dht_h;
TaskHandle_t bt_h;
TaskHandle_t state_h;

void setup()
{
  setCpuFrequencyMhz(160); //Set CPU frequency to 160 MHz

  pinMode(BT_PIN, INPUT_PULLUP);
  button.state = UNPRESSED;

  xTaskCreatePinnedToCore(pvTask_Rgb,          "rgb_h",    2048, NULL, 1, &rgb_h,    APP_CPU_NUM);
  xTaskCreatePinnedToCore(pvTask_Dht22,        "dht_h",    2048, NULL, 1, &dht_h,    APP_CPU_NUM);
  xTaskCreatePinnedToCore(pvTask_Button,       "bt_h",     2048, NULL, 1, &bt_h,     PRO_CPU_NUM);
  xTaskCreatePinnedToCore(pvTask_StateReport,  "state_h",  2048, NULL, 1, &state_h,  PRO_CPU_NUM);

  Serial.begin(115200);
  dht.begin();
}

void loop()
{
}

void pvTask_Rgb(void *arg)
{
  while(1)
  {
  }
}

void pvTask_Dht22(void *arg)
{
  while(1)
  {
  }
}

void pvTask_Button(void *arg)
{
  bool bt = 0;

  while(1)
  {
    if(millis()-button.tickCount >= 100) //bounce
    {
      bt = IS_BT_PRESSED;

      switch(button.state)
      {
        case UNPRESSED:
        if(bt)
        {
          button.state = PRESSED;
          button.tickCount = millis(); //xTaskGetTickCount()
          Serial.println("bt PRESSED");
        }
        break;

        case PRESSED:
        if(!bt)
        {
          Serial.println(millis()-button.tickCount);
          
          //pressed above 3s, change led_collor
          if(millis()-button.tickCount >= 3000)
          {
            Serial.print("bt 3s - led_collor:");
            if(++led_collor >= LED_COLLOR_MAX_INDEX)
              led_collor = GREEN;

            Serial.println(led_collor);
          }
          //pressed below than 3s, change led_state
          else
          {
            Serial.print("bt 1s - led_state:");
            if(++led_state >= LED_STATE_MAX_INDEX)
              led_state = STATE_OFF;

            Serial.println(led_state);
          }

          button.state = UNPRESSED;
          button.tickCount = millis(); //xTaskGetTickCount()
        }
        break;
      }
    }
  }
}

void pvTask_StateReport(void *arg)
{
  while(1)
  {
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

/*
    Serial.print(__func__);
    Serial.print(" : ");
    Serial.print(xTaskGetTickCount());
    Serial.print(" : ");
    Serial.print("This loop runs on PRO_CPU which id is:");
    Serial.println(xPortGetCoreID());
    Serial.println();

    vTaskDelay(100);
*/

