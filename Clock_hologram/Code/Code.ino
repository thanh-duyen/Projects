// Copyright (C) 2024 Duyen Nguyen

#include "RTClib.h"
#include <TimerOne.h>
#include "glcdfont.c"

RTC_DS3231 rtc;

#define G_PIN 13
#define B_PIN 12
#define R_PIN 11
#define HALL_PIN 2
#define BUTTON_PIN A6
#define MS 1000
#define NUMBER_OF_LED 13
const uint8_t LED_PIN[NUMBER_OF_LED] = {6,10,9,8,5,7,4,3,A3,A2,A1,A0,1};
bool is_serial_enable = true;
struct stime{
  uint8_t hour;
  uint8_t min;
  uint8_t sec;
};
stime rtc_time;

uint32_t period = 1000;
volatile bool hall_event = false;
volatile bool timer_event = false;

typedef enum eColors{
  emNone = 0b000,
  emBlue = 0b001,
  emGreen = 0b010,
  emCyan = 0b011,
  emRed = 0b100,
  emMagenta = 0b101,
  emYellow = 0b110,
  emWhite = 0b111
};

String text = "";
byte text_data[60] = {0};
bool pos_print[60] = {false};

String messages[9] = {"Hello!","Thank","For","Watching","Analog","Clock","Hologram","V1",""};
uint8_t mes_index = 0;
uint32_t time_change_mes = 0;
void set_text(String text, uint8_t start_pos){
  for(uint8_t i = 0; i < 60; i++){
    text_data[i] = 0;
    pos_print[i] = 0;
  }
  for(uint8_t c = 0; c < text.length() && c < 10; c++){
    for(uint8_t i = 0; i < 5; i++){
      text_data[start_pos] = pgm_read_byte(&font[int(text.charAt(c)) * 5 + i]);
      pos_print[start_pos] = true;
      if(++start_pos >= 60) start_pos = 0;
    }
    if(++start_pos >= 60) start_pos = 0;
  }
}
void led_set(eColors color, uint8_t num_led, ...){
  digitalWrite(R_PIN,(color >> 2) & 0x1);
  digitalWrite(G_PIN,(color >> 1) & 0x1);
  digitalWrite(B_PIN,(color >> 0) & 0x1);
  uint8_t led;
  va_list args;
  va_start(args, num_led);
  for(int i = 0; i < num_led; i++){
    led = va_arg(args, int);
    if(led >= NUMBER_OF_LED) continue;
    digitalWrite(LED_PIN[led],HIGH);
  }
  va_end(args);
}
void led_set_array(eColors color, uint8_t num_led, uint8_t *leds){
  digitalWrite(R_PIN,(color >> 2) & 0x1);
  digitalWrite(G_PIN,(color >> 1) & 0x1);
  digitalWrite(B_PIN,(color >> 0) & 0x1);
  for(int i = 0; i < num_led; i++){
    if(leds[i] >= NUMBER_OF_LED) continue;
    digitalWrite(LED_PIN[leds[i]],HIGH);
  }
}
void led_off(uint8_t num_led, ...){
  uint8_t led;
  va_list args;
  va_start(args, num_led);
  for(int i = 0; i < num_led; i++){
    led = va_arg(args, int);
    if(led >= NUMBER_OF_LED) continue;
    digitalWrite(LED_PIN[led],LOW);
  }
  va_end(args);
}
void led_off_array(uint8_t num_led, uint8_t *leds){
  for(int i = 0; i < num_led; i++){
    if(leds[i] >= NUMBER_OF_LED) continue;
    digitalWrite(LED_PIN[leds[i]],LOW);
  }
}
void serial_set(bool is_enable){
  is_serial_enable = is_enable;
  if(is_serial_enable){
    Serial.begin(115200);
    delay(10);
    Serial.println("Serial enable");
  }
  else{
    Serial.println("Serial disable");
    Serial.flush();
    Serial.end();
  }
}
void read_button(){
  static uint32_t time = 0;
  static bool button = true;
  if(millis() >= time){
    if((bool)analogRead(BUTTON_PIN) != button){
      button = !button;
      if(button == 0){
        is_serial_enable = !is_serial_enable;
        serial_set(is_serial_enable);
      }
    }
    time = millis() + 1000;
  }
}
void read_time(){
  static uint32_t time = 0;
  if(millis() >= time){
    time = millis() + 1000;
    DateTime now = rtc.now();
    rtc_time.hour = now.hour();
    if(rtc_time.hour >= 12) rtc_time.hour -= 12;
    rtc_time.min = now.minute();
    rtc_time.sec = now.second();
  }
}
void timer_handler(){
  timer_event = true;
}
void hall_handler(){
  static uint32_t last_time;
  period = (micros()-last_time)/60+1;
  last_time = micros();
  Timer1.setPeriod(period);
  Timer1.restart();
  hall_event = true;
  timer_event = true;
}
void setup() {
  serial_set(true);
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
  }
  serial_set(false);
  for(uint8_t i = 0; i < NUMBER_OF_LED; i++){
    pinMode(LED_PIN[i],OUTPUT);
  }
  pinMode(G_PIN, OUTPUT);
  pinMode(B_PIN, OUTPUT);
  pinMode(R_PIN, OUTPUT);
  pinMode(HALL_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(HALL_PIN), hall_handler, FALLING);
  Timer1.attachInterrupt(timer_handler);
  Timer1.initialize(10000);
}
void loop() {
label_1:
  read_button();
  read_time();
  if(is_serial_enable){
    if(Serial.available()){
      delay(5);
      String str = "";
      while(Serial.available()){
        str += (char)Serial.read();
      }
      if(str.indexOf("time ") >= 0){
        uint8_t i = 5, j = 0;
        String buff = "";
        uint8_t time_get[3]; //hour, min, sec
        while(i < str.length()){
          if((int)str.charAt(i)-48 >= 0 && (int)str.charAt(i)-48 <= 9){
            buff += str.charAt(i);
          }
          else{
            time_get[j] = buff.toInt();
            buff = "";
            j++;
          }
          i++;
        }
        rtc.adjust(DateTime(2014, 1, 21, time_get[0], time_get[1], time_get[2]));
        char temp[50];
        sprintf(temp, "Setup time to %02d:%02d:%02d succeeded", time_get[0], time_get[1], time_get[2]);
        Serial.println(temp);
        serial_set(false);
      }
      else{
        Serial.println("Usage:");
        Serial.println("  Setup time: time hh mm ss");
      }
    }
  }
  if(mes_index < 9 && millis() >= time_change_mes){
    uint8_t pos = 60 - (messages[mes_index].length()*6)/2 + 2;
    set_text(messages[mes_index],pos);
    time_change_mes = millis() + 3500 - ((bool)mes_index)*2000;
    mes_index++;
  }
  while(!hall_event){
    if(is_serial_enable) goto label_1;
    read_button();
  }
  hall_event = false;
  for(int8_t j = 0, i = 51; j < 60 && !hall_event; j++){
    while(!timer_event){}
    timer_event = false;
    /*for time display*/
    led_set(i%5 ? emGreen:emRed, 1, 0);
    delayMicroseconds(30);
    led_off(1, 0);

    /*for text display*/
    if(pos_print[i] == true){
      uint8_t leds_on[8];
      uint8_t num_led = 0;
      for(int8_t k = 0; k < 7; k++){
        if((text_data[i]>>k)&0x1){
          leds_on[num_led] = k + 2;
          num_led++;
        }
      }
      led_set_array(emCyan, num_led, leds_on);
      delayMicroseconds(50);
      led_off_array(num_led, leds_on);
    }

    /*for time display*/
    if(rtc_time.sec == i) led_set(emGreen, 11, 2,3,4,5,6,7,8,9,10,11,12);
    else if(rtc_time.min == i) led_set(emBlue, 9, 4,5,6,7,8,9,10,11,12);
    else if(rtc_time.hour*5 == i) led_set(emRed, 7, 6,7,8,9,10,11,12);
    delayMicroseconds(50);
    if(rtc_time.sec == i) led_off(11, 2,3,4,5,6,7,8,9,10,11,12);
    else if(rtc_time.min == i) led_off(9, 4,5,6,7,8,9,10,11,12);
    else if(rtc_time.hour*5 == i) led_off(7, 6,7,8,9,10,11,12);
    else led_off(1, 0);
    if(++i >= 60) i = 0;
  }
}
