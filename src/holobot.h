#ifndef __HOLOBOT_H__
#define __HOLOBOT_H__

#include <inttypes.h>
#include <Arduino.h>

#define LED_COUNT       144
#define DEFAULT_SPEED   25
#define STRIPE_PIN      4
#define BATT_EN_PIN     12
#define BATT_V_PIN      35
#define STATUS_PIN      15

#define BATT_DIVISION   3.2
#define BATT_CORRECTION 1.015
#define BATT_EMPTY      6.4
#define BATT_CHECK_TIME 5000

#define MAX_MILLIAMPS   1000

#define MDNS_DOMAIN     "holobot-leds"

typedef enum {
  IDLE,
  LED_TEST,
  STRIPES,
  RAINBOW,
  PICTURE,
  LOW_BATT,
  BLINK,
} holo_state_t;

typedef enum {
  NO_ERROR,
  FILE_ERROR,
  READ_ERROR,
  SIZE_ERROR, 
  FORMAT_ERROR,
  PARAMS_ERROR,
  SPIFFS_ERROR,
  LOW_BATT_ERROR,
} holo_error_t;

typedef struct {
  holo_error_t code;
  String desc;
} holo_last_error_t;

typedef struct {
    int time;
    int r, g, b;
    int width;
    int steps;
    int step_delay;
} holo_anim_params_t;

typedef struct {
  uint32_t anim_delay;
  String filename;
} holo_picture_params_t;

#endif

