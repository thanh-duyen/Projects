#include <Arduino.h>
#include "RTClib.h"
#include "pitches.h"
#include "Adafruit_GFX.h"
#include <SPI.h>
#include "time.h"
#include <EEPROM.h>
#include <map>

#define SDA_PIN 8
#define SCL_PIN 7
#define LATCH_PIN 2
#define SCK_PIN SCK
#define MOSI_PIN MOSI
#define BUTTON_L_PIN 0
#define BUTTON_R_PIN 1
#define BUZZER_PIN 3
#define RX_BUFFER 32

#define MAX_WIDTH_BLOCK 10
#define MAX_HEIGHT_BLOCK 5
#define MAX_SCROLL_BUFFER 254

#define EEPROM_WIDTH 0
#define EEPROM_HEIGHT 1
#define EEPROM_BUFFER 2
#define EEPROM_MAIN_BOARD_POSITION 3

#define MAX_U32 4294967295
RTC_DS3231 rtc;

const uint8_t BUTTON_PINS[2] = {BUTTON_L_PIN, BUTTON_R_PIN};
char daysOfTheWeek[7][4] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
char monthOfTheYear[12][4] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
bool is_RTC_ok = true;

TaskHandle_t Task1;

typedef enum{
  NO_COLOR = 0,
  RED_COLOR = 1,
  GREEN_COLOR = 2,
  BOTH_COLOR = 3
}Color;

typedef enum{
  SUB_BOARD = 0,
  MAIN_BOARD
}Board;

typedef enum{
  CLOCK_DISPLAY,
  SETTING_CLOCK,
  UART_DISPLAY
}Display_Mode;

uint16_t width_block = 3;
uint16_t height_block = 1;
uint16_t width_pixel = width_block*16;
uint16_t scroll_buffer = 200 - width_pixel;
uint16_t height_pixel = height_block*8;
uint16_t ic_num = (width_block*2 + height_block*2);
uint8_t *board_defination;
uint8_t *ic_mapping_bottom;
uint8_t *ic_mapping_left;
uint8_t *ic_mapping_right;

uint8_t *buffer;

const std::map<String, uint16_t> function_arg = {
  {"drawPixel", 1 << 3},
  {"drawFastVLine", 1 << 4},
  {"drawFastHLine", 1 << 4},
  {"fillRect", 1 << 5},
  {"fillScreen", 1 << 1},
  {"drawLine", 1 << 5},
  {"drawRect", 1 << 5},
  {"drawCircle", 1 << 4},
  {"drawCircleHelper", 1 << 5},
  {"fillCircle", 1 << 4},
  {"fillCircleHelper", 1 << 5},
  {"drawTriangle", 1 << 7},
  {"fillTriangle", 1 << 7},
  {"drawRoundRect", 1 << 6},
  {"fillRoundRect", 1 << 6},
  {"setCursor", 1 << 2 | 1 << 1 | 1 << 4},
  {"setTextColor", 1 << 1 | 1 << 2},
  {"drawChar", 1 << 6 | 1 << 7},
  {"setTextSize", 1 << 1 | 1 << 2},
  {"time", 1 << 6 | 1 << 0},
  {"clockDisplay", 1 << 0 | 1 << 1},
  {"setScroll", 1 << 1 | 1 << 4 | 1 << 2}};
const std::map<String, String> function_help = {
  {"print", "message"}, 
  {"drawPixel", "x y color"}, 
  {"drawFastVLine", "x y h color"}, 
  {"drawFastHLine", "x y w color"}, 
  {"fillRect", "x y w h color"}, 
  {"fillScreen", "color"}, 
  {"drawLine", "x0 y0 x1 y1 color"}, 
  {"drawRect", "x y w h color"}, 
  {"drawCircle", "x y r color"}, 
  {"drawCircleHelper", "x y r cornername color"}, 
  {"fillCircle", "x y r color"}, 
  {"fillCircleHelper", "x y r cornername color"}, 
  {"drawTriangle", "x0 y0 x1 y1 x2 y2 color"}, 
  {"fillTriangle", "x0 y0 x1 y1 x2 y2 color"}, 
  {"drawRoundRect", "x y w h r color"}, 
  {"fillRoundRect", "x y w h r color"}, 
  {"setCursor", "\n  x y (Normal set) \n  0 (Off cursor/blink)\n  mode x y color (On cursor(1)/blink(2))"}, 
  {"setTextColor", "\n  color \n  color bg_color"}, 
  {"drawChar", "\n  x y char color bg size \n  x y char color bg x_size y_size"}, 
  {"setTextSize", "\n  size \n  x_size y_size"}, 
  {"time", "\n  yyyy mm dd hh mm ss (Set time) \n  No parameter (Get time: hh:mm:ss wday mmm dd.yyyy)"}, 
  {"clockDisplay", "\n  No parameter (change to clock display)\n  color (change color for clock)"}, 
  {"setDisplay", "width_block height_block buffer board_defination (0: sub, 1: main)"}, 
  {"setScroll", "\n  0 (Off scroll)\n  0 index (change index display, default is 0)\n  1 max_index delay notify (On scroll)"}};
  
class Led_Matrix : public Adafruit_GFX{
  public:
    Led_Matrix(uint16_t width, uint16_t height):Adafruit_GFX(width, height){}
    void drawPixel(int16_t x, int16_t y, uint16_t color){
      if((x >= 0) && (x < width_pixel + scroll_buffer) && (y >= 0) && (y < height_pixel) ) {
        buffer[x + y*(width_pixel + scroll_buffer)] = color & 0x3;
      }
    }
};
Led_Matrix *led_matrix;

int melody[] = {NOTE_C4, NOTE_G3,NOTE_G3, NOTE_A3, NOTE_G3,0, NOTE_B3, NOTE_C4};
int noteDurations[] = {4,8,8,4,4,4,4,4};

/** 0 0 0 0 0 0 0 0  0 0 0 0 0 0 0 0  0 0 1 0 0 0 0 0  0 0 0 0 0 0 0 0  0 0 0 0 0 0 0 0  0 0 0 0 0 0 0 0 
*** 0 0 0 0 0 0 0 0  0 0 0 0 0 0 0 0  0 0 1 0 0 0 0 0  0 0 0 0 0 0 0 0  0 0 0 0 0 0 0 0  0 0 0 0 0 0 0 0 
*** 0 0 0 0 0 0 0 0  0 0 0 0 0 0 0 0  0 0 0 1 0 0 0 0  0 0 0 0 0 0 0 0  0 0 0 0 0 0 0 0  0 0 0 0 0 0 0 0 
*** 0 0 0 0 0 0 0 0  0 0 0 0 0 0 0 0  0 0 0 1 0 0 0 0  0 0 0 0 0 0 0 0  0 0 0 0 0 0 0 0  0 0 0 0 0 0 0 0 
ic5 0 0 0 0 0 0 0 0  0 0 0 0 0 0 0 0  0 0 0 0 0 0 0 0  0 0 0 0 0 0 0 0  0 0 0 0 0 0 0 0  0 0 0 0 0 0 0 0 ic6
*** 0 0 0 0 0 0 0 0  0 0 0 0 0 0 0 0  0 0 0 0 0 1 0 0  0 0 0 0 0 0 0 0  0 0 0 0 0 0 0 0  0 0 0 0 0 0 0 0 
*** 0 0 0 0 0 0 0 0  0 0 0 0 0 0 0 0  0 0 0 0 0 1 2 0  0 0 0 0 0 0 0 0  0 0 0 0 0 0 0 0  0 0 0 0 0 0 0 0 
*** 0 0 0 0 0 0 0 0  0 0 0 0 0 0 0 0  0 0 0 0 0 0 0 0  0 0 0 0 0 0 0 0  0 0 0 0 0 0 0 0  0 0 0 0 0 0 0 0 */
//        ic4              ic3              ic2              ic1              ic8              ic7
//              sub board                        main board                         sub board

/** 0 0 0 0 0 0 0 0  0 0 0 0 0 0 0 0  0 0 1 0 0 0 0 0  0 0 0 0 0 0 0 0  0 0 0 0 0 0 0 0  0 0 0 0 0 0 0 0 
*** 0 0 0 0 0 0 0 0  0 0 0 0 0 0 0 0  0 0 1 0 0 0 0 0  0 0 0 0 0 0 0 0  0 0 0 0 0 0 0 0  0 0 0 0 0 0 0 0 
*** 0 0 0 0 0 0 0 0  0 0 0 0 0 0 0 0  0 0 0 1 0 0 0 0  0 0 0 0 0 0 0 0  0 0 0 0 0 0 0 0  0 0 0 0 0 0 0 0 
*** 0 0 0 0 0 0 0 0  0 0 0 0 0 0 0 0  0 0 0 1 0 0 0 0  0 0 0 0 0 0 0 0  0 0 0 0 0 0 0 0  0 0 0 0 0 0 0 0 
da3 0 0 0 0 0 0 0 0  0 0 0 0 0 0 0 0  0 0 0 0 0 0 0 0  0 0 0 0 0 0 0 0  0 0 0 0 0 0 0 0  0 0 0 0 0 0 0 0 da2
*** 0 0 0 0 0 0 0 0  0 0 0 0 0 0 0 0  0 0 0 0 0 1 0 0  0 0 0 0 0 0 0 0  0 0 0 0 0 0 0 0  0 0 0 0 0 0 0 0 
*** 0 0 0 0 0 0 0 0  0 0 0 0 0 0 0 0  0 0 0 0 0 1 2 0  0 0 0 0 0 0 0 0  0 0 0 0 0 0 0 0  0 0 0 0 0 0 0 0 
*** 0 0 0 0 0 0 0 0  0 0 0 0 0 0 0 0  0 0 0 0 0 0 0 0  0 0 0 0 0 0 0 0  0 0 0 0 0 0 0 0  0 0 0 0 0 0 0 0 */
//        da4              da5              da6              da7              da0              da1
//              sub board                        main board                         sub board

uint32_t time_read_rtc = 0;
struct tm last_time;
int8_t clock_color = GREEN_COLOR;
bool change_color = false;

char data_rx[RX_BUFFER];

Display_Mode mode = CLOCK_DISPLAY;

struct Scroll{
  bool enable;
  int16_t index_scroll;
  uint16_t max_index_scroll;
  uint16_t delay_scroll;
  uint16_t delay_complete;
  uint32_t time_scroll;
  bool notify;
};
Scroll scroll{0, 0, 0, 100, 0, 0, 0};

enum Cursor_Mode{
  NONE = 0,
  CURSOR,
  BLINK
};

uint8_t cursor_mode = NONE;
bool blink_status = false;
int16_t cursor_x = 0, cursor_y = 7;
uint8_t cursor_color = RED_COLOR;
uint32_t time_blink = 0;

bool is_enable_display = true;

bool button_status[2] = {0, 0};
uint32_t hold_event[2] = {MAX_U32, MAX_U32};
bool handle_release[2] =  {1, 1};

void parse_ic_mapping(){
  int16_t i, j, k, h, m;
  for(i = 0; i < width_block && board_defination[i] != MAIN_BOARD; i++){}
  for(m = 0; m < height_block; m++){
    ic_mapping_left[m] = ic_num - (i+1)*2 - m - 1;
    ic_mapping_right[m] = ic_num - height_block - (i+1)*2 - m - 1;
  }
  for(j = i, k = 0; j >= 0; j--, k+=2){
    ic_mapping_bottom[k] = ic_num - height_block*2 - i*2 + k;
    ic_mapping_bottom[k+1] = ic_num - height_block*2 - i*2 + k + 1;
  }
  for(j = width_block - 1, h = 0; j > i; j--, h+=2){
    ic_mapping_bottom[k] = h;
    ic_mapping_bottom[k+1] = h + 1;
  }
  // Serial.print("mapping: ");
  // for(i = 0; i < width_block*2; i++){
  //   Serial.printf("%d ", ic_mapping_bottom[i]);
  // }
  // for(i = 0; i < height_block; i++){
  //   Serial.printf("%d ", ic_mapping_left[i]);
  // }
  // for(i = 0; i < height_block; i++){
  //   Serial.printf("%d ", ic_mapping_right[i]);
  // }
  // Serial.println();
}

uint8_t read8_buffer(uint16_t x, uint16_t y, Color color){
  uint8_t ret = 0;
  for(uint8_t i = 0; i < 8; i++){
    if(x >= (width_pixel + scroll_buffer) || (y + i) >= height_pixel) continue;
    if(buffer[x + (y + i) * (width_pixel + scroll_buffer)] & color){
      ret |= 1 << i;
    }
  }
  return ret;
}

void display_handle( void * pvParameters ){
  uint8_t *data;
  uint16_t step = 0;
  data = new uint8_t(ic_num);
  if(!data) return;
  for(uint16_t i = 0; i < width_block*2; i++)
    data[ic_mapping_bottom[i]] = 255;
  for(;;){
    if(!is_enable_display) continue;
    data[ic_mapping_bottom[step/8]] = 255;
    if(++step >= width_pixel) step = 0;
    data[ic_mapping_bottom[step/8]] = ~(1 << (step%8));
    for(uint8_t i = 0; i < height_block; i++){
      if((step + scroll.index_scroll) < (scroll.max_index_scroll) && (step + scroll.index_scroll) >= 0){
        if(cursor_mode != BLINK || blink_status == 0){
          data[ic_mapping_left[i]] = read8_buffer(step+scroll.index_scroll,height_pixel - (i + 1)*8,GREEN_COLOR);
          data[ic_mapping_right[i]] = read8_buffer(step+scroll.index_scroll,i*8, RED_COLOR);
        }
        else if(cursor_x <= step + scroll.index_scroll && cursor_x + 5 > step + scroll.index_scroll){
          data[ic_mapping_left[i]] = 0;
          data[ic_mapping_right[i]] = 0;
          if(cursor_color == GREEN_COLOR || cursor_color == BOTH_COLOR)
            data[ic_mapping_left[i]] = 127;
          if(cursor_color == RED_COLOR || cursor_color == BOTH_COLOR)
            data[ic_mapping_right[i]] = 127;
        }
        else{
          data[ic_mapping_left[i]] = read8_buffer(step+scroll.index_scroll,height_pixel - (i + 1)*8,GREEN_COLOR);
          data[ic_mapping_right[i]] = read8_buffer(step+scroll.index_scroll,i*8, RED_COLOR);
        }
        if(cursor_mode == CURSOR && cursor_x <= step + scroll.index_scroll && cursor_x + 5 > step + scroll.index_scroll){
          if(cursor_color == GREEN_COLOR || cursor_color == BOTH_COLOR)
            data[ic_mapping_left[i]] = data[ic_mapping_left[i]] | (1 << 7);
          if(cursor_color == RED_COLOR || cursor_color == BOTH_COLOR)
            data[ic_mapping_right[i]] = data[ic_mapping_right[i]] | (1 << 7);
        }
      }
      else{
        data[ic_mapping_left[i]] = 0;
        data[ic_mapping_right[i]] = 0;
      }
    }
    for(uint8_t i = 0; i < ic_num; i++){
      shiftOut(MOSI_PIN, SCK_PIN, MSBFIRST, data[i]);
    }
    digitalWrite(LATCH_PIN,HIGH);
    digitalWrite(LATCH_PIN,LOW);
  }
}

void check_read_eeprom(){
  if(EEPROM.read(EEPROM_WIDTH) >= MAX_WIDTH_BLOCK)
    EEPROM.write(EEPROM_WIDTH, width_block);
  else
    width_block = EEPROM.read(EEPROM_WIDTH);
  if(EEPROM.read(EEPROM_HEIGHT) >= MAX_HEIGHT_BLOCK)
    EEPROM.write(EEPROM_HEIGHT, height_block);
  else
    height_block = EEPROM.read(EEPROM_HEIGHT);
  if(EEPROM.read(EEPROM_BUFFER) >= MAX_SCROLL_BUFFER)
    EEPROM.write(EEPROM_BUFFER, scroll_buffer);
  else
    scroll_buffer = EEPROM.read(EEPROM_BUFFER);

  if(EEPROM.read(EEPROM_MAIN_BOARD_POSITION) >= width_block)
    EEPROM.write(EEPROM_MAIN_BOARD_POSITION, (uint8_t)width_block/2);
  EEPROM.commit();
}

void read_button(){
  for(uint8_t i = 0; i < 2; i++){
    uint8_t j = 0;
    while(digitalRead(BUTTON_PINS[i]) != button_status[i] && j < 20){
      delay(1);
      j++;
    }
    if(j >= 20){
      button_status[i] = !button_status[i];
      if(button_status[i] == 1){ // pressed
        hold_event[i] = millis() + 500;
        handle_release[i] = 1;
      }
      else{ // released
        if(handle_release[i]){
          hold_event[i] = MAX_U32;
          if(mode == CLOCK_DISPLAY){
            (i == 0) ? clock_color--:clock_color++;
            if(clock_color < 1) clock_color = BOTH_COLOR;
            else if(clock_color > 3) clock_color = RED_COLOR;
            change_color = true;
          }
          else if(mode == SETTING_CLOCK){
            if(cursor_mode == CURSOR){
              (i == 0) ? cursor_x-=6:cursor_x+=6;
              if(cursor_x < 0){
                cursor_x = 138;
                scroll.index_scroll = 96;
              }
              else if(cursor_x > 138){
                cursor_x = 0;
                scroll.index_scroll = 0;
              }
              else if(cursor_x >= scroll.index_scroll + width_pixel){
                scroll.index_scroll += 6;
              }
              else if(cursor_x < scroll.index_scroll && scroll.index_scroll > 0){
                scroll.index_scroll -= 6;
              }
            }
            else{
              int8_t value = (i == 0) ? -1:1;
              if(cursor_x == 0*6) last_time.tm_hour += 10*value;
              else if(cursor_x == 1*6) last_time.tm_hour += 1*value;
              else if(cursor_x == 3*6) last_time.tm_min += 10*value;
              else if(cursor_x == 4*6) last_time.tm_min += 1*value;
              else if(cursor_x == 6*6) last_time.tm_sec += 10*value;
              else if(cursor_x == 7*6) last_time.tm_sec += 1*value;
              else if(cursor_x >= 9 * 6 && cursor_x < 12 * 6) last_time.tm_mon += 1*value;
              else if(cursor_x == 13*6) last_time.tm_mday += 10*value;
              else if(cursor_x == 14*6) last_time.tm_mday += 1*value;
              else if(cursor_x == 16*6) last_time.tm_year += 1000*value;
              else if(cursor_x == 17*6) last_time.tm_year += 100*value;
              else if(cursor_x == 18*6) last_time.tm_year += 10*value;
              else if(cursor_x == 19*6) last_time.tm_year += 1*value;
              if(last_time.tm_hour >= 24) last_time.tm_hour = 24;
              else if(last_time.tm_hour < 0) last_time.tm_hour = 0;
              if(last_time.tm_min >= 60) last_time.tm_min = 60;
              else if(last_time.tm_min < 0) last_time.tm_min = 0;
              if(last_time.tm_sec >= 60) last_time.tm_sec = 60;
              else if(last_time.tm_sec < 0) last_time.tm_sec = 0;
              if(last_time.tm_mon > 12) last_time.tm_mon = 12;
              else if(last_time.tm_mon < 0) last_time.tm_mon = 0;
              if(last_time.tm_mday > 31) last_time.tm_mday = 31;
              else if(last_time.tm_mday < 0) last_time.tm_mday = 0;
              if(last_time.tm_year > 9999) last_time.tm_year = 9999;
              else if(last_time.tm_year < 0) last_time.tm_year = 0;
              led_matrix->setTextColor(GREEN_COLOR, NO_COLOR);
              led_matrix->setCursor(0,0);
              led_matrix->printf("%02d:%02d:%02d %s %02d.%04d O X", last_time.tm_hour, last_time.tm_min, last_time.tm_sec,
                                              monthOfTheYear[last_time.tm_mon - 1], last_time.tm_mday, last_time.tm_year);
            }
          }
        }
      }
    }
    if(button_status[i] == 1 && millis() >= hold_event[i]){ // holded for 0.5s
      hold_event[i] = MAX_U32;
      handle_release[i] = 0;
      if(mode == CLOCK_DISPLAY){
        mode = SETTING_CLOCK;
        memset(buffer, 0, (width_pixel + scroll_buffer)*height_pixel);
        led_matrix->setTextColor(GREEN_COLOR);
        led_matrix->setCursor(0,0);
        led_matrix->printf("%02d:%02d:%02d %s %02d.%04d O X", last_time.tm_hour, last_time.tm_min, last_time.tm_sec,
                                         monthOfTheYear[last_time.tm_mon - 1], last_time.tm_mday, last_time.tm_year);
        scroll.enable = 0;
        scroll.index_scroll = 0;
        scroll.notify = 0;
        cursor_mode = CURSOR;
        cursor_color = RED_COLOR;
        cursor_x = 0; cursor_y = 7;
      }
      else if(mode == SETTING_CLOCK){
        if(cursor_x == 126 || cursor_x == 138){ // O || X
          if(cursor_x == 126){
            rtc.adjust(DateTime(last_time.tm_year, last_time.tm_mon, last_time.tm_mday, 
                                last_time.tm_hour, last_time.tm_min, last_time.tm_sec));
          }
          mode = CLOCK_DISPLAY;
          cursor_mode = NONE;
          memset(buffer, 0, (width_pixel + scroll_buffer)*height_pixel);
          memset(&last_time, -1, sizeof(last_time));
          scroll.enable = 0;
          scroll.index_scroll = 0;
          scroll.notify = 0;
        }
        else{
          cursor_mode = (cursor_mode == BLINK) ? CURSOR:BLINK;
          blink_status = 0;
        }
      }
    }
  }
}

void setup () {
  Serial.begin(115200);
  EEPROM.begin(10);
  delay(1000);
  check_read_eeprom();
  pinMode(LATCH_PIN, OUTPUT);
  pinMode(SCK_PIN, OUTPUT);
  pinMode(MOSI_PIN, OUTPUT);

  pinMode(BUTTON_L_PIN, INPUT);
  pinMode(BUTTON_R_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  memset(data_rx, 0, RX_BUFFER);
  led_matrix = new Led_Matrix(width_pixel + scroll_buffer, height_pixel);
  buffer = new uint8_t[(width_pixel + scroll_buffer)*height_pixel];
  if(!buffer){
    Serial.println("buffer allocate error");
    while(1);
  }
  memset(buffer, 0, (width_pixel + scroll_buffer)*height_pixel);
  board_defination = new uint8_t[width_block];
  memset(board_defination, SUB_BOARD, width_block);
  ic_mapping_bottom = new uint8_t[width_block*2];
  ic_mapping_left = new uint8_t[height_block];
  ic_mapping_right = new uint8_t[height_block];
  board_defination[EEPROM.read(EEPROM_MAIN_BOARD_POSITION)] = MAIN_BOARD;
  parse_ic_mapping();
  // for (int thisNote = 0; thisNote < 8; thisNote++) {
  //   int noteDuration = 1000/noteDurations[thisNote];
  //   tone(BUZZER_PIN, melody[thisNote], noteDuration);
  //   int pauseBetweenNotes = noteDuration * 1.30;
  //   delay(pauseBetweenNotes);
  //   noTone(BUZZER_PIN);
  // }
  memset(&last_time, -1, sizeof(last_time));
  if (! rtc.begin(SDA_PIN, SCL_PIN)) {
    Serial.println("Couldn't find RTC");
    time_read_rtc = -1;
  }

  xTaskCreatePinnedToCore(display_handle, "Task1", 10240, NULL, 1, &Task1, 0); delay(500);
}

void loop () {
  if(mode != SETTING_CLOCK && millis() >= time_read_rtc){
    DateTime now = rtc.now();
    if(change_color){
      change_color = false;
      memset(&last_time, -1, sizeof(last_time));
    }
    if(mode == CLOCK_DISPLAY)
      led_matrix->setTextColor(clock_color, NO_COLOR);
    if(now.second() != last_time.tm_sec){
      last_time.tm_sec = now.second();
      if(mode == CLOCK_DISPLAY){
        led_matrix->setCursor(36,0);
        led_matrix->printf("%02d", last_time.tm_sec);
      }
    }
    if(now.hour() != last_time.tm_hour){
      last_time.tm_hour = now.hour();
      if(mode == CLOCK_DISPLAY){
        led_matrix->setCursor(0,0);
        led_matrix->printf("%02d:", last_time.tm_hour);
      }
    }
    if(now.minute() != last_time.tm_min){
      if(last_time.tm_min != -1){
        clock_color = (Color)random(1,4);
        change_color = true;
      }
      last_time.tm_min = now.minute();
      if(mode == CLOCK_DISPLAY){
        led_matrix->setCursor(18,0);
        led_matrix->printf("%02d:", last_time.tm_min);
      }
    }
    if(now.dayOfTheWeek() != last_time.tm_wday){
      last_time.tm_wday = now.dayOfTheWeek();
      if(mode == CLOCK_DISPLAY){
        led_matrix->setCursor(54,0);
        led_matrix->print(daysOfTheWeek[last_time.tm_wday]);
      }
    }
    if(now.month() != last_time.tm_mon){
      last_time.tm_mon = now.month();
      if(mode == CLOCK_DISPLAY){
        led_matrix->setCursor(78,0);
        led_matrix->printf("%s ", monthOfTheYear[last_time.tm_mon - 1]);
      }
    }
    if(now.day() != last_time.tm_mday){
      last_time.tm_mday = now.day();
      if(mode == CLOCK_DISPLAY){
        led_matrix->setCursor(102,0);
        led_matrix->printf("%02d.", last_time.tm_mday);
      }
    }
    if(now.year() != last_time.tm_year){
      last_time.tm_year = now.year();
      if(mode == CLOCK_DISPLAY){
        led_matrix->setCursor(120,0);
        led_matrix->printf("%04d", last_time.tm_year);
      }
    }

    if(!scroll.enable && mode == CLOCK_DISPLAY){
      scroll.enable = 1;
      scroll.delay_complete = 5000;
      scroll.delay_scroll = 100;
      scroll.index_scroll = 0;
      scroll.max_index_scroll = 120 + 24;
    }
    time_read_rtc = millis() + 100;
  }
  while(Serial.available()){
    data_rx[strlen(data_rx)] = Serial.read();
    if(data_rx[strlen(data_rx) - 2] == '\r' && data_rx[strlen(data_rx) - 1] == '\n'){
      int16_t i = 0, j = 0, arg[20], index_mes;
      String str = "", command = "";
      while(data_rx[i] != ' ' && i < strlen(data_rx)){command += data_rx[i]; i++;} i++; index_mes = i;
      while(i < strlen(data_rx) && j < 20){
        if(data_rx[i] != ' ' && data_rx[i] != '\r'){
          str += data_rx[i];
        }
        else{
          if(((int)str.charAt(0) - 48) >= 0 && ((int)str.charAt(0) - 48) < 10){
            arg[j] = str.toInt();
            j++;
          }
          str = "";
        }
        i++;
      }
      
      bool valid_args = true;
      String mes_respond = "OK\r\n";
      command.replace("\r",""); command.replace("\n","");
      if (function_arg.find(command) != function_arg.end()) {
        if(!((1 << j) & function_arg.at(command))){
          valid_args = false;
          mes_respond = String(command) + " " + function_help.at(command) + "\r\n";
        }
      }
      if(valid_args){
        if(mode != UART_DISPLAY){
          const char functions_name[19][20] = {"print","drawPixel","drawFastVLine","drawFastHLine",
                                          "fillRect","fillScreen","drawLine","drawRect","drawCircle","drawCircleHelper",
                                          "fillCircle","fillCircleHelper","drawTriangle","fillTriangle","drawRoundRect",
                                          "fillRoundRect","setTextColor","drawChar","setTextSize"};
          for(uint8_t i = 0; i < 19; i++){
            if(!strncmp(data_rx,functions_name[i], strlen(functions_name[i]))){
              scroll.enable = 0;
              scroll.index_scroll = 0;
              memset(buffer, 0, (width_pixel + scroll_buffer)*height_pixel);
              mode = UART_DISPLAY;
              break;
            }
          }
        }
        if(!strncmp(data_rx,"help",4)){
          Serial.print("------ The input parameters split by space character \' \' ------\r\n");
          for (const auto& pair : function_help) {
            mes_respond = pair.first + " " + pair.second + "\r\n";
            Serial.print(mes_respond);
          }
          mes_respond = "";
        }
        else if(!strncmp(data_rx,"print",5)){
          data_rx[strlen(data_rx) - 2] = '\0';
          led_matrix->print(&data_rx[index_mes]);
        }
        else if(!strncmp(data_rx,"drawPixel",9))
          led_matrix->drawPixel(arg[0], arg[1], arg[2]);
        else if(!strncmp(data_rx,"drawFastVLine",13))
          led_matrix->drawFastVLine(arg[0], arg[1], arg[2], arg[3]);
        else if(!strncmp(data_rx,"drawFastHLine",13))
          led_matrix->drawFastHLine(arg[0], arg[1], arg[2], arg[3]);
        else if(!strncmp(data_rx,"fillRect",8))
          led_matrix->fillRect(arg[0], arg[1], arg[2], arg[3], arg[4]);
        else if(!strncmp(data_rx,"fillScreen",10))
          led_matrix->fillScreen(arg[0]);
        else if(!strncmp(data_rx,"drawLine",8))
          led_matrix->drawLine(arg[0], arg[1], arg[2], arg[3], arg[4]);
        else if(!strncmp(data_rx,"drawRect",8))
          led_matrix->drawRect(arg[0], arg[1], arg[2], arg[3], arg[4]);
        else if(!strncmp(data_rx,"drawCircle",10))
          led_matrix->drawCircle(arg[0], arg[1], arg[2], arg[3]);
        else if(!strncmp(data_rx,"drawCircleHelper",16))
          led_matrix->drawCircleHelper(arg[0], arg[1], arg[2], arg[3], arg[4]);
        else if(!strncmp(data_rx,"fillCircle",10))
          led_matrix->fillCircle(arg[0], arg[1], arg[2], arg[3]);
        else if(!strncmp(data_rx,"fillCircleHelper",16))
          led_matrix->fillCircleHelper(arg[0], arg[1], arg[2], arg[3], arg[4], arg[5]);
        else if(!strncmp(data_rx,"drawTriangle",12))
          led_matrix->drawTriangle(arg[0], arg[1], arg[2], arg[3], arg[4], arg[5], arg[6]);
        else if(!strncmp(data_rx,"fillTriangle",12))
          led_matrix->fillTriangle(arg[0], arg[1], arg[2], arg[3], arg[4], arg[5], arg[6]);
        else if(!strncmp(data_rx,"drawRoundRect",13))
          led_matrix->drawRoundRect(arg[0], arg[1], arg[2], arg[3], arg[4], arg[5]);
        else if(!strncmp(data_rx,"fillRoundRect",13))
          led_matrix->fillRoundRect(arg[0], arg[1], arg[2], arg[3], arg[4], arg[5]);
        else if(!strncmp(data_rx,"setCursor",9)){
          if(j == 1){ // set cursor mode
            if(!arg[0]) cursor_mode = arg[0];
            else        mes_respond = String(command) + " " + function_help.at(command) + "\r\n";
          }
          else if(j == 2){
            if(mode == CLOCK_DISPLAY){
              scroll.enable = 0;
              scroll.index_scroll = 0;
              memset(buffer, 0, (width_pixel + scroll_buffer)*height_pixel);
              mode = UART_DISPLAY;
            }
            led_matrix->setCursor(arg[0], arg[1]);
            cursor_x = arg[0];
            cursor_y = arg[1];
          }
          else{
            cursor_mode = arg[0];
            cursor_x = arg[1];
            cursor_y = arg[2];
            cursor_color = arg[3];
            time_blink = 0;
          }
        }
        else if(!strncmp(data_rx,"setTextColor",12)){
          if(j == 1)      led_matrix->setTextColor(arg[0]);
          else            led_matrix->setTextColor(arg[0], arg[1]);
        }
        else if(!strncmp(data_rx,"drawChar",8)){
          if(j == 6)  led_matrix->drawChar(arg[0], arg[1], (unsigned char)arg[2], arg[3], arg[4], arg[5]);
          else        led_matrix->drawChar(arg[0], arg[1], (unsigned char)arg[2], arg[3], arg[4], arg[5], arg[6]);
        }
        else if(!strncmp(data_rx,"setTextSize",11)){
          if(j == 1)  led_matrix->setTextSize(arg[0]);
          else        led_matrix->setTextSize(arg[0], arg[1]);
        }
        else if(!strncmp(data_rx,"time",4)){ // set/get time
          mes_respond = "";
          if(j == 0){
            char str[30];
            sprintf(str, "%d:%d:%d %d %d %d.%d\r\n", last_time.tm_hour, last_time.tm_sec, last_time.tm_min,
                                  last_time.tm_wday, last_time.tm_mon, last_time.tm_mday, last_time.tm_year);
            Serial.write(str);
          }
          else{
            rtc.adjust(DateTime(arg[0], arg[1], arg[2], arg[3], arg[4], arg[5]));
          }
        }
        else if(!strncmp(data_rx,"clockDisplay",12)){ // set/get time
          if(mode != CLOCK_DISPLAY){
            mode = CLOCK_DISPLAY;
            memset(buffer, 0, (width_pixel + scroll_buffer)*height_pixel);
            memset(&last_time, -1, sizeof(last_time));
            scroll.enable = 0;
            scroll.index_scroll = 0;
            scroll.notify = 0;
          }
          if((int)data_rx[13]-48 >= 1 && (int)data_rx[13]-48 < 4){
            clock_color = (int)data_rx[13]-48;
            change_color = true;
          }
        }
        else if(!strncmp(data_rx,"setDisplay",10)){ // width height buffer board_defination
          // check arg available
          bool arg_available = true;
          if(j < arg[0] + 3) arg_available = false;
          for(uint8_t k = 0, found = 0; k < arg[0]; k++){
            if((arg[3+k] != 0 && arg[3+k] != 1) || (arg[3+k] == MAIN_BOARD && found)){
              arg_available = false;
              break;
            }
            else{
              found = 1;
            }
          }
          if(arg_available){
            is_enable_display = false;
            width_block = arg[0];
            height_block = arg[1];
            scroll_buffer = arg[2];
            if(scroll_buffer + width_block*16 < 200) 
              scroll_buffer = 200 - width_block*16;
            width_pixel = width_block*16;
            height_pixel = height_block*8;
            ic_num = (width_block*2 + height_block*2);
            delete[] board_defination;
            delete[] ic_mapping_bottom;
            delete[] ic_mapping_left;
            delete[] ic_mapping_right;
            delete[] buffer;
            delete led_matrix;

            led_matrix = new Led_Matrix(width_pixel + scroll_buffer, height_pixel);
            buffer = new uint8_t[(width_pixel + scroll_buffer)*height_pixel];
            if(!buffer){
              Serial.println("buffer allocate error");
              while(1);
            }
            memset(buffer, 0, (width_pixel + scroll_buffer)*height_pixel);
            board_defination = new uint8_t[width_block];
            ic_mapping_bottom = new uint8_t[width_block*2];
            ic_mapping_left = new uint8_t[height_block];
            ic_mapping_right = new uint8_t[height_block];
            
            for(uint8_t k = 0; k < width_block; k++){
              board_defination[k] = arg[3+k];
            }
            parse_ic_mapping();
            EEPROM.write(EEPROM_WIDTH, width_block);
            EEPROM.write(EEPROM_HEIGHT, height_block);
            EEPROM.write(EEPROM_BUFFER, scroll_buffer);
            for(uint8_t k = 0; k < width_block; k++){
              if(board_defination[k] == MAIN_BOARD){
                EEPROM.write(EEPROM_MAIN_BOARD_POSITION, k);
              }
            }

            scroll.index_scroll = 0;
            memset(&last_time, -1, sizeof(last_time));
            is_enable_display = true;
          }
          else{
            mes_respond = "setDisplay width_block height_block buffer board_defination\r\n";
          }
        }
        else if(!strncmp(data_rx,"setScroll",9)){
          if(arg[0] && j == 4){
            scroll.enable = 1;
            scroll.delay_complete = arg[2];
            scroll.delay_scroll = arg[2];
            scroll.index_scroll = 0;
            scroll.max_index_scroll = arg[1];
            scroll.notify = arg[3];
          }
          else{
            if(arg[0]){
              mes_respond = String(command) + " " + function_help.at(command) + "\r\n";
            }
            else{
              scroll.enable = 0;
              scroll.index_scroll = 0;
              scroll.notify = 0;
                
              if(j == 2){
                scroll.index_scroll = arg[1];
              }
            }
          }
        }
        else{
          mes_respond = "Unknow command \"" + String(command) + "\"\r\n";
        }
      }

      if(mes_respond != "") Serial.print(mes_respond);
      memset(data_rx, 0, RX_BUFFER);
    }
    if(strlen(data_rx) >= RX_BUFFER){
      memset(data_rx, 0, RX_BUFFER);
      Serial.println("Buffer overflow");
    }
  }
  if(scroll.enable && millis() >= scroll.time_scroll){
    if(scroll.index_scroll >= scroll.max_index_scroll){
      scroll.index_scroll = -width_pixel + 1;
    }
    else{
      scroll.index_scroll++;
    }
    if(scroll.index_scroll == 0){
      scroll.time_scroll = millis() + scroll.delay_complete;
      if(scroll.notify){
        Serial.print("ScrollNotify\r\n");
      }
    }
    else
      scroll.time_scroll = millis() + scroll.delay_scroll;
  }
  read_button();
  if(cursor_mode == BLINK && millis() >= time_blink){
    time_blink = millis() + 300 + blink_status*500;
    blink_status = !blink_status;
  }
  delay(1);
}

