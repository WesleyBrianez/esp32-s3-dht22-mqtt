/*
  Configure o cliente do navegador HiveMQ para visualizar as
  mensagens MQTT publicadas pelo cliente MQTT.

  1) Vá para a URL abaixo e clique no botão conectar
     http://www.hivemq.com/demos/websocket-client/

  2) Adicione os tópicos de assinatura um para cada tópico que o ESP32 usa:
     topic_on_off_led/#
     topic_sensor_temperature/#
     topic_sensor_humidity/#

  3) Experimento publicar no topico topic_on_off_led com a mensagem 1 e 0
     para ligar e desligar o LED"

  IMPORTANTE: É necessário rodar o Wokwi Gateway e habilitar a opção
  "Enable Private Wokwi IoT Gateway" através da tecla de atalho F1 no editor de código.

  Para baixar o Wokwi IoT Network Gateway acesse o seguinte link:
    https://github.com/wokwi/wokwigw/releases/
*/

#include <ArduinoJson.h>
#include "DHT.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "api_structs.h"
#include <PubSubClient.h>
#include <WiFi.h>

#define RED_PIN       10
#define GREEN_PIN     9
#define BLUE_PIN      46

#define LED_PIN       1
#define DHT_PIN       16
#define DHT_TYPE      DHT22
#define BT_PIN        GPIO_NUM_14
#define IS_BT_PRESSED !gpio_get_level(BT_PIN)

/* MQTT config */
#define TOPIC_SUBSCRIBE_LED "topic_on_off_led"
#define TOPIC_PUBLISH_JSON  "topic_json"
#define ID_MQTT             "esp32_mqtt" //session mqtt id

DHT dht (DHT_PIN, DHT_TYPE);

sButton button;
uint8_t led_collor; //eLedCollor
uint8_t led_state; //eLedState

TaskHandle_t rgb_h;
TaskHandle_t dht_h;
TaskHandle_t bt_h;
TaskHandle_t state_h;

const char *SSID = "Wokwi-GUEST";
const char *PASSWORD = "";
const char *BROKER_MQTT = "broker.hivemq.com";
int BROKER_PORT = 1883;

static char strStatus[40] = {0};
static char strTemp[10] = {0};

static char *led_collor_string[] = {"GREEN","YELLOW","RED","BLUE"};
static char *led_state_string[] = {"OFF","ON","BLINK_1000MS","BLINK_300MS"};

volatile TickType_t Wifi_tickCount;
volatile TickType_t Mqtt_tickCount;
volatile TickType_t Dht_tickCount;

float temperature;
float humidity;

WiFiClient espClient;
PubSubClient MQTT(espClient);

void setup()
{
  setCpuFrequencyMhz(240); //Set CPU frequency to 240 MHz
  gpio_init();

  button.state = UNPRESSED;

  xTaskCreatePinnedToCore(pvTask_Rgb,          "rgb_h",    2048, NULL, 1, &rgb_h,    APP_CPU_NUM);
  xTaskCreatePinnedToCore(pvTask_Dht22,        "dht_h",    2048, NULL, 1, &dht_h,    APP_CPU_NUM);
  xTaskCreatePinnedToCore(pvTask_Button,       "bt_h",     2048, NULL, 1, &bt_h,     PRO_CPU_NUM);
  xTaskCreatePinnedToCore(pvTask_StateReport,  "state_h",  4096, NULL, 1, &state_h,  PRO_CPU_NUM);

  Serial.begin(115200);
}

void loop()
{
}

void pvTask_Rgb(void *arg)
{
  while(1)
  {
    if(led_state != STATE_OFF)
    {
      switch(led_collor)
      {
        case GREEN:
        set_rgb(0,1,0);
        break;
        case YELLOW:
        set_rgb(1,1,0);
        break;
        case RED:
        set_rgb(1,0,0);
        break;
        case BLUE:
        set_rgb(0,0,1);
        break;
      }
    }
    else
    {
      set_rgb(0,0,0);
    }

  }
}

void pvTask_Dht22(void *arg)
{
  dht.begin();

  while(1)
  {
    if(millis()-Dht_tickCount >= 250) //read dht each 250ms
    {
      Dht_tickCount = millis();
      getTemperature();
      getHumidity();
    }
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
            digitalWrite(LED_PIN, !(digitalRead(LED_PIN)));
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
  initMQTT();

  while(1)
  {
    //print each 3s the temp, hum, led_state, led_collor
    if(millis()-Mqtt_tickCount >= 3000)
    {
      Mqtt_tickCount = millis();
      print_status();

      Serial.print(strStatus);

      if (WiFi.status() != WL_CONNECTED)
        ConnectWiFi();

      if (!MQTT.connected())
        ConnectMQTT();

      if (MQTT.connected() && (WiFi.status() == WL_CONNECTED))
      {
        DynamicJsonDocument doc(1024);
        doc["id"] = "sensor1";
        doc["humid"] = humidity;
        doc["temp"] = temperature;

        // Serialize JSON document
        String payload;
        serializeJson(doc, payload);
        MQTT.publish(TOPIC_PUBLISH_JSON, payload.c_str());  

        // Keep-alive broker MQTT
        MQTT.loop();
      }
    }
  }
}

void print_status(void)
{
  memset(strStatus, 0, sizeof(strStatus));
  memset(strTemp, 0, sizeof(strTemp));

  strcat(strStatus, "temp:");
  sprintf(strTemp, "%.2f, ", temperature);
  strcat(strStatus, strTemp);

  strcat(strStatus, "hum:");
  sprintf(strTemp, "%.2f, ", humidity);
  strcat(strStatus, strTemp);

  strcat(strStatus, "led_state:");
  strcat(strStatus, led_state_string[led_state]);
  strcat(strStatus, ", ");

  strcat(strStatus, "led_collor:");
  strcat(strStatus, led_collor_string[led_collor]);
  strcat(strStatus, "\r\n");
}

void initMQTT(void)
{
  MQTT.setServer(BROKER_MQTT, BROKER_PORT);
  MQTT.setCallback(callbackMQTT);
}

void callbackMQTT(char *topic, byte *payload, unsigned int length)
{
  String msg;

  // Obtem a string do payload recebido
  for (int i = 0; i < length; i++) {
    char c = (char)payload[i];
    msg += c;
  }

  Serial.printf("MQTT-rx:%s from topic:%s\n", msg, topic);

  /* Toma ação dependendo da string recebida */
  if (msg.equals("1")) {
    digitalWrite(LED_PIN, HIGH);
  }

  if (msg.equals("0")) {
    digitalWrite(LED_PIN, LOW);
  }
}

void ConnectWiFi(void)
{
  vTaskDelay(pdMS_TO_TICKS(10));
  Serial.print("Connecting on network: ");
  Serial.println(SSID);

  WiFi.begin(SSID, PASSWORD);
  Wifi_tickCount = millis();

  do{
    vTaskDelay(pdMS_TO_TICKS(150));
    Serial.print(".");
  }while((millis()-Wifi_tickCount <= 10000) && (WiFi.status() != WL_CONNECTED));

  if(WiFi.status() != WL_CONNECTED)
  {
    Serial.println("\nTimeout reached");    
  }
  else
  {
    Serial.print("\nConnected SSID: ");
    Serial.println(SSID);
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  }
}

void ConnectMQTT(void)
{
  while (!MQTT.connected()) {
    Serial.print("Attempting MQTT connection: ");
    Serial.println(BROKER_MQTT);
    if (MQTT.connect(ID_MQTT)) {
      Serial.println("Connected");
      // Once connected, publish an announcement...
      MQTT.subscribe(TOPIC_SUBSCRIBE_LED);
    } else {
      Serial.print("failed with state ");
      Serial.print(MQTT.state());
      Serial.println(", try again in 2 seconds");
      vTaskDelay(pdMS_TO_TICKS(2000));
    }
  }
}

void getTemperature(void)
{
  float data = dht.readTemperature();

  if (isnan(data))
    temperature = -99;
  else
    temperature = data;
}

void getHumidity(void)
{
  float data = dht.readHumidity();

  if (isnan(data))
    humidity = -99;
  else
    humidity = data;
}

void gpio_init(void)
{
  pinMode(BT_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  set_rgb(0,0,0);
}

void set_rgb(bool r, bool g, bool b)
{
  digitalWrite(RED_PIN, !r); digitalWrite(GREEN_PIN, !g); digitalWrite(BLUE_PIN, !b);
}
/*
    Serial.print(__func__);
    Serial.print(" : ");
    Serial.print(xTaskGetTickCount());
    Serial.print(" : ");
    Serial.print("This loop runs on PRO_CPU which id is:");
    Serial.println(xPortGetCoreID());
    Serial.println();

    vTaskvTaskDelay(100);
*/

/*
    leds[0] = CRGB::Red;
    FastLED.show();
    vTaskDelay(500);
    // Now turn the LED off, then pause
    leds[0] = CRGB::Black;
    FastLED.show();
    vTaskDelay(500);
*/

