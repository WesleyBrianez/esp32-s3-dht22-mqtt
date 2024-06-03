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

#include "DHT.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "api_structs.h"
#include <PubSubClient.h>
#include <WiFi.h>
#include <FastLED.h>

#define NUM_LEDS      1
#define LED_PIN       1
#define DHT_PIN       16
#define DHT_TYPE      DHT22
#define BT_PIN        GPIO_NUM_14
#define IS_BT_PRESSED !gpio_get_level(BT_PIN)

/* MQTT config */
#define TOPIC_SUBSCRIBE_LED       "topic_on_off_led"
#define TOPIC_PUBLISH_TEMPERATURE "topic_sensor_temperature"
#define TOPIC_PUBLISH_HUMIDITY    "topic_sensor_humidity"
#define ID_MQTT "esp32_mqtt" //session mqtt id

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

static char strTemperature[10] = {0};
static char strHumidity[10] = {0};

volatile TickType_t Mqtt_tickCount;
volatile TickType_t Dht_tickCount;
float temperature;
float humidity;

WiFiClient espClient;
PubSubClient MQTT(espClient);

CRGB leds[NUM_LEDS];

void setup()
{
  setCpuFrequencyMhz(240); //Set CPU frequency to 240 MHz

  pinMode(BT_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  button.state = UNPRESSED;
  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUM_LEDS);  // GRB ordering is assumed

  xTaskCreatePinnedToCore(pvTask_Rgb,          "rgb_h",    2048, NULL, 1, &rgb_h,    APP_CPU_NUM);
  xTaskCreatePinnedToCore(pvTask_Dht22,        "dht_h",    2048, NULL, 1, &dht_h,    APP_CPU_NUM);
  xTaskCreatePinnedToCore(pvTask_Button,       "bt_h",     2048, NULL, 1, &bt_h,     PRO_CPU_NUM);
  xTaskCreatePinnedToCore(pvTask_StateReport,  "state_h",  4096, NULL, 1, &state_h,  PRO_CPU_NUM);

  Serial.begin(115200);
  dht.begin();

  initWiFi();
  initMQTT();
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
    if(millis()-Mqtt_tickCount >= 3000) //3s period
    {
      Mqtt_tickCount = millis();
      
      checkWiFIAndMQTT();

      // Formata as strings a serem enviadas para o dashboard (campos texto)
      sprintf(strTemperature, "%.2fC", temperature);
      sprintf(strHumidity, "%.2f", humidity);

      // Envia as strings ao dashboard MQTT
      MQTT.publish(TOPIC_PUBLISH_TEMPERATURE, strTemperature);
      MQTT.publish(TOPIC_PUBLISH_HUMIDITY, strHumidity);

      // Keep-alive da comunicação com broker MQTT
      MQTT.loop();
    }
  }
}

void initMQTT(void)
{
  MQTT.setServer(BROKER_MQTT, BROKER_PORT); // Informa qual broker e porta deve ser conectado
  MQTT.setCallback(callbackMQTT);           // Atribui função de callback (função chamada quando qualquer informação de um dos tópicos subescritos chega)
}

void callbackMQTT(char *topic, byte *payload, unsigned int length)
{
  String msg;

  // Obtem a string do payload recebido
  for (int i = 0; i < length; i++) {
    char c = (char)payload[i];
    msg += c;
  }

  Serial.printf("Chegou a seguinte string via MQTT: %s do topico: %s\n", msg, topic);

  /* Toma ação dependendo da string recebida */
  if (msg.equals("1")) {
    digitalWrite(LED_PIN, HIGH);
    Serial.print("LED ligado por comando MQTT");
  }

  if (msg.equals("0")) {
    digitalWrite(LED_PIN, LOW);
    Serial.print("LED desligado por comando MQTT");
  }
}

void reconnectWiFi(void)
{
  // se já está conectado a rede WI-FI, nada é feito.
  // Caso contrário, são efetuadas tentativas de conexão
  if (WiFi.status() == WL_CONNECTED)
    return;

  WiFi.begin(SSID, PASSWORD); // Conecta na rede WI-FI

  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("Conectado com sucesso na rede ");
  Serial.print(SSID);
  Serial.println("IP obtido: ");
  Serial.println(WiFi.localIP());
}

void checkWiFIAndMQTT(void)
{
  if (!MQTT.connected())
    reconnectMQTT(); // se não há conexão com o Broker, a conexão é refeita

  reconnectWiFi(); // se não há conexão com o WiFI, a conexão é refeita
}

void reconnectMQTT(void)
{
  while (!MQTT.connected()) {
    Serial.print("* Tentando se conectar ao Broker MQTT: ");
    Serial.println(BROKER_MQTT);
    if (MQTT.connect(ID_MQTT)) {
      Serial.println("Conectado com sucesso ao broker MQTT!");
      MQTT.subscribe(TOPIC_SUBSCRIBE_LED);
    } else {
      Serial.println("Falha ao reconectar no broker.");
      Serial.println("Nova tentativa de conexao em 2 segundos.");
      delay(2000);
    }
  }
}

void initWiFi(void)
{
  delay(10);
  Serial.println("------Conexao WI-FI------");
  Serial.print("Conectando-se na rede: ");
  Serial.println(SSID);
  Serial.println("Aguarde");

  reconnectWiFi();
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

/*
    leds[0] = CRGB::Red;
    FastLED.show();
    delay(500);
    // Now turn the LED off, then pause
    leds[0] = CRGB::Black;
    FastLED.show();
    delay(500);
*/

