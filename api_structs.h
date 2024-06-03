#ifndef api_structs_h
#define api_structs_h

//enums----------------------------
typedef enum
{
  GREEN = 0,
  YELLOW,
  RED,
  BLUE,
  LED_COLLOR_MAX_INDEX
}eLedCollor;

typedef enum
{
  STATE_OFF = 0,
  STATE_ON,
  STATE_BLINK_1000_MS,
  STATE_BLINK_300_MS,
  LED_STATE_MAX_INDEX
}eLedState;

typedef enum
{
  UNPRESSED = 0,
  PRESSED,
  BT_STATE_MAX_INDEX
}eButtonState;

typedef enum
{
  DISCONNECTED = 0,
  CONNECTED,
}eMqttState;

//structs---------------------------
typedef struct
{
  eButtonState state;
  volatile TickType_t tickCount;
}sButton;

#endif