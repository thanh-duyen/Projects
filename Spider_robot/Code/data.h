#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include "printf.h"
#include <Servo.h>
#include <EEPROM.h>
#include <Ultrasonic.h>
#include <WiFi.h>
#include <WebSocketsServer.h>
#include <WebServer.h>
#include <Arduino.h>

#define BUZZER_PWM_CHANNEL 12
#define BUZZER_PIN 25
#define TRIGGER_PIN  32
#define ECHO_PIN     35

#define CE_PIN 17
#define CSN_PIN 16

#define TIMEOUT_DEFAULT 5000

#define BUTTON_NUM 12
struct sData{
  sData(){
    joys_leftX = joys_leftY = joys_rightX = joys_rightY = 0;
    mode_sel1 = mode_sel2 = 0;
    time_lastRecive = 0;
    for(uint8_t i = 0; i < BUTTON_NUM; i++){
      button[i] = 0;
    }
  }
  int16_t joys_leftX, joys_leftY, joys_rightX, joys_rightY;
  bool button[BUTTON_NUM];
  int8_t encoder_postion; // 0 to 19 (encoder 20 PPR)
  bool mode_sel1, mode_sel2;
  uint32_t time_lastRecive;
};
sData rfdata;
bool is_nRF_ok;

float distance_mm;
uint32_t time_read = 0;

// Femur,Tibia,Coxa
const uint8_t SERVO_PINS[4][3] = { {22, 21, 13}, {4, 5, 26}, {33, 14, 12},{2, 15, 27}};
/* Size of the robot ---------------------------------------------------------*/
const float length_a = 47;
const float length_b = 72;
const float length_c = 27.5;
const float length_side = 50;
const float z_absolute = -5;
/* Constants for movement ----------------------------------------------------*/
const float z_default = -50, z_up = -30, z_boot = z_absolute;
const float x_default = 62, x_offset = 0;
const float y_start = 0, y_step = 40;
const float y_default = x_default;
/* variables for movement ----------------------------------------------------*/
volatile float site_now[4][3];    //real-time coordinates of the end of each leg
volatile float site_expect[4][3]; //expected coordinates of the end of each leg
float temp_speed[4][3];   //each axis' speed, needs to be recalculated before each movement
float move_speed;     //movement speed
float speed_multiple = 1; //movement speed multiple
const float spot_turn_speed = 4;
float leg_move_speed = 6;
float body_move_speed = 3;
const float stand_seat_speed = 1;
volatile int rest_counter;      //+1/0.02s, for automatic rest
//functions' parameter
const float KEEP = 255;
//define PI for calculation
const float pi = 3.1415926;
/* Constants for turn --------------------------------------------------------*/
//temp length
const float temp_a = sqrt(pow(2 * x_default + length_side, 2) + pow(y_step, 2));
const float temp_b = 2 * (y_start + y_step) + length_side;
const float temp_c = sqrt(pow(2 * x_default + length_side, 2) + pow(2 * y_start + y_step + length_side, 2));
const float temp_alpha = acos((pow(temp_a, 2) + pow(temp_b, 2) - pow(temp_c, 2)) / 2 / temp_a / temp_b);
//site for turn
const float turn_x1 = (temp_a - length_side) / 2;
const float turn_y1 = y_start + y_step / 2;
const float turn_x0 = turn_x1 - temp_b * cos(temp_alpha);
const float turn_y0 = temp_b * sin(temp_alpha) - turn_y1 - length_side;
/* ---------------------------------------------------------------------------*/

uint32_t execution_time = 0;

float cur_alpha[4], cur_beta[4], cur_gamma[4];
int16_t offset_alpha[4] = {0,0,0,0}, offset_beta[4] = {0,0,0,0}, offset_gamma[4] = {0,0,0,0};

typedef enum ecommand{
  emNone = 0,
  emStand,
  emForward,
  emSit,
  emBack,
  emRight,
  emLeft,
  emWave,
  emShake,
  emStomp
};

bool exit_moving = false;
ecommand cur_command = emNone;
ecommand next_command = emStand;
int16_t step_moving = -1;

bool web_enable = false;
bool rf_enable = true;

bool connected = false;

uint32_t reconnect_wifi_time;
typedef enum eWifiState{
  emNotConnect,
  emSetup,
  emConnecting,
  emConnected,
  emAccessPointSetup,
  emAccessPoint
};
eWifiState wifi_state = emNotConnect;
char ssid[50] = "SpiderRobot";
char password[50] = "00000000";

uint32_t beep_run_time = 0;
int16_t beep_count = 4;
