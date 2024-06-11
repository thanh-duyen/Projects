#include <TimerOne.h>
#include <SoftwareSerial.h>

SoftwareSerial mySerial(2, 3); // RX, TX

#define BUZZ_PIN A4
#define RELAY_PIN A2
const uint8_t COL_PIN[2] = {11,12};
const uint8_t ROW_PIN[2] = {9,10};
bool button_status[2][2] = {{1,1},{1,1}};

#define BUZZ_TIME_HIGH 50
#define BUZZ_TIME_LOW 500
struct buzz_info{
  uint32_t time_blink;
  uint8_t times;
};
buzz_info buzz{0,0};

#define STEP_PER_ROUND 2048
#define START_SPEED 5 //rpm
#define START_TICK 60L*1000L*1000L/STEP_PER_ROUND/START_SPEED
#define TIME_PER_TICK 4 //us
const uint8_t MOTOR_PIN[4] = {5,6,7,8};
const uint8_t MOTOR_STATE[4] = {0b0101,0b0110,0b1010,0b1001};

typedef enum eRunState{
  emStop = 0,
  emAccel,  // emAccelerate
  emDecel,  // emDecelerate
  emSlowDown, 
  emRun     // max speed
};
struct motor_control{
  int32_t cur_step;
  int32_t mov_step;
  int8_t state;
  int8_t dir;
  uint32_t cur_ticks;
  uint32_t ticks;
  float speed_rpm;
};
motor_control motor{0,0,0,0,START_TICK,START_TICK,START_SPEED};

char data_Serial[32] = {0};
char data_mySerial[32] = {0};
bool is_dumpSpeed = false;

void move(int32_t target_step){
  if(target_step == motor.cur_step) return;
  motor.mov_step = target_step;
  if(motor.mov_step > motor.cur_step) motor.dir = 1;
  else motor.dir = -1;
  Timer1.initialize(motor.cur_ticks);
  Timer1.attachInterrupt(interrupt_handle);
}
void stop(){
  Timer1.detachInterrupt();
  motor.dir = 0;
}
void set_speed(float speed){
  if(speed < 0) return;
  motor.ticks = 60L * 1000L * 1000L / STEP_PER_ROUND / speed;
  motor.speed_rpm = speed;
}
void interrupt_handle(){
  if(motor.cur_ticks != motor.ticks){
    motor.cur_ticks = motor.ticks;
    Timer1.setPeriod(motor.ticks);
  }
  for(uint8_t i = 0; i < 4; i++)
    digitalWrite(MOTOR_PIN[i], (MOTOR_STATE[motor.state%4]>>i)&0x1);
  motor.state += motor.dir;
  if(motor.state > 3) motor.state = 0;
  else if(motor.state < 0) motor.state = 3;
  motor.cur_step += motor.dir;
  if(motor.cur_step == motor.mov_step) stop();
}
void read_keyPad(){
  static uint32_t time_run = 0;
  static bool index = 0;
  static uint32_t start_press = -1;
  static int8_t pressed = -1;
  static bool handled = false;
  if(time_run > millis()) return;
  time_run = millis()+10;
  for(uint8_t i = 0; i < 2; i++){
    if(pressed == i*2+index || pressed < 0){
      if(millis() - start_press >= 1000 && pressed == i*2+index){ // holded
        if(i*2+index == 0){ //UP
          set_speed(motor.ticks - 5);
        }
        else if(i*2+index == 2){ //DOWN
          set_speed(motor.ticks + 5);
        }
        else if(handled == false){
          handled = true;
          digitalWrite(RELAY_PIN, !digitalRead(RELAY_PIN));
          Serial.println(digitalRead(RELAY_PIN) ? "Power on":"Power off");
        }
      }
      if(digitalRead(ROW_PIN[i]) != button_status[i][index]){
        button_status[i][index] = !button_status[i][index];
        if(button_status[i][index] == 0){
          //buzz.times = 1;
          pressed = i*2+index;
          start_press = millis();
          if(i*2+index == 0){ //UP
            set_speed(motor.speed_rpm - 1);
          }
          else if(i*2+index == 2){ //DOWN
            set_speed(motor.speed_rpm + 1);
          }
        }
        else if(button_status[i][index] == 1){
          if(i*2+index == 0 || i*2+index == 2){
            is_dumpSpeed = true;
          }
          else if(handled == false){
            if(i*2+index == 1){ //CW
              if(motor.dir == 0) move(motor.cur_step + 1000*STEP_PER_ROUND);
              else stop();
            }
            else if(i*2+index == 3){ //CCW
              if(motor.dir == 0) move(motor.cur_step - 1000*STEP_PER_ROUND);
              else stop();
            }
          }
          handled = false;
          pressed = -1;
          start_press = -1;
        }
      }
    }
  }
  if(pressed >= 0) return;
  index = !index;
  digitalWrite(COL_PIN[index],0);
  digitalWrite(COL_PIN[!index],1);
}
void setup() {
  Serial.begin(115200);
  mySerial.begin(115200);
  for(uint8_t i = 0; i < 2; i++){
    pinMode(COL_PIN[i],OUTPUT);
    digitalWrite(COL_PIN[i],HIGH);
    pinMode(ROW_PIN[i],INPUT);
  }
  for(uint8_t i = 0; i < 4; i++){
    pinMode(MOTOR_PIN[i],OUTPUT);
  }
  pinMode(BUZZ_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  delay(1000);
  digitalWrite(RELAY_PIN, HIGH);
  Serial.println("Ready!");
}
void loop() {
  if (Serial.available()){
    while (Serial.available())
      data_Serial[strlen(data_Serial)] = (char)Serial.read();
    if(data_Serial[strlen(data_Serial)-1] == '\n'){
      Serial.print(data_Serial);
      String str = String(data_Serial);
      if(str.indexOf("$CCW ") >= 0){
        str.replace("$CCW ","");
        move(motor.cur_step - str.toFloat()*STEP_PER_ROUND);
      }
      else if(str.indexOf("$CW ") >= 0){
        str.replace("$CW ","");
        move(motor.cur_step - str.toFloat()*STEP_PER_ROUND);
      }
      else if(str.indexOf("$STOP") >= 0){
        stop();
      }
      else if(str.indexOf("$SPD ") >= 0){
        str.replace("$SPD ","");
        set_speed(str.toFloat());
      }
      else{
        mySerial.print(data_Serial);
      }
      memset(data_Serial, 0, 32*sizeof(char));
    }
  }
  if (mySerial.available()){
    while (mySerial.available())
      data_mySerial[strlen(data_mySerial)] = (char)mySerial.read();
    if(data_mySerial[strlen(data_mySerial)-1] == '\n'){
      Serial.print(data_mySerial);
      memset(data_mySerial, 0, 32*sizeof(char));
    }
  }
  read_keyPad();
  if(buzz.times > 0 && millis() >= buzz.time_blink){
    bool cur_val = digitalRead(BUZZ_PIN);
    if(cur_val){
      buzz.time_blink = millis() + BUZZ_TIME_LOW;
      buzz.times--;
    }
    else
      buzz.time_blink = millis() + BUZZ_TIME_HIGH;
    digitalWrite(BUZZ_PIN, !cur_val);
  }
  if(is_dumpSpeed){
    Serial.println(motor.speed_rpm);
    is_dumpSpeed = false;
  }
}
