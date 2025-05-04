#include "WedUI.h"
#include "data.h"

WebSocketsServer webSocket = WebSocketsServer(81);
WebServer server(80);

RF24 radio(CE_PIN, CSN_PIN);

//define 12 servos for 4 legs
Servo servo[4][3];

/* 3 bytes per value*/
void write_eeprom(uint16_t address, float value){
  value += 32640;
  EEPROM.write(address + 0, value / 255);
  EEPROM.write(address + 1, (int)value % 255);
  EEPROM.write(address + 2, (int)(value * 100) % 100);
  EEPROM.commit();
}

float read_eeprom(uint16_t address){
  float value;
  value = EEPROM.read(address) * 255;
  value += EEPROM.read(address + 1);
  value += EEPROM.read(address + 2)/100.0;
  value -= 32640;
  return value; 
}

void send2wed(String str){
  if(connected == false)
    return;
  webSocket.sendTXT(0, str.c_str(), str.length()+1);
}

void body_left(int i){
  set_site(0, site_now[0][0] + i, KEEP, KEEP);
  set_site(1, site_now[1][0] + i, KEEP, KEEP);
  set_site(2, site_now[2][0] - i, KEEP, KEEP);
  set_site(3, site_now[3][0] - i, KEEP, KEEP);
}

void body_right(int i){
  set_site(0, site_now[0][0] - i, KEEP, KEEP);
  set_site(1, site_now[1][0] - i, KEEP, KEEP);
  set_site(2, site_now[2][0] + i, KEEP, KEEP);
  set_site(3, site_now[3][0] + i, KEEP, KEEP);
}

void head_up(int i){
  set_site(0, KEEP, KEEP, site_now[0][2] - i);
  set_site(1, KEEP, KEEP, site_now[1][2] + i);
  set_site(2, KEEP, KEEP, site_now[2][2] - i);
  set_site(3, KEEP, KEEP, site_now[3][2] + i);
}

void head_down(int i){
  set_site(0, KEEP, KEEP, site_now[0][2] + i);
  set_site(1, KEEP, KEEP, site_now[1][2] - i);
  set_site(2, KEEP, KEEP, site_now[2][2] + i);
  set_site(3, KEEP, KEEP, site_now[3][2] - i);
}

void stomp(bool next_step){
  static uint32_t run_time = 0;
  if(!next_step) return;
  if(millis() <= run_time){
    if(exit_moving)
      step_moving = 2;
    else
      return;
  }
  step_moving++;
  static float x_tmp, y_tmp, z_tmp;
  static bool saved = false;
  move_speed = body_move_speed;
  if(step_moving == 0){
    body_right(15);
  }
  else if(step_moving == 1){
    if(saved == false){
      x_tmp = site_now[2][0];
      y_tmp = site_now[2][1];
      z_tmp = site_now[2][2];
      saved = true;
    }
    move_speed = leg_move_speed / 2.0;
    set_site(2, KEEP, KEEP, z_default + 20);
  }
  else if(step_moving == 2){
    move_speed = leg_move_speed;
    set_site(2, KEEP, KEEP, z_default);
    if(!exit_moving){
      step_moving = 0; // loop to step 1
    }
    run_time = millis() + 500;
  }
  else if(step_moving == 3){
    move_speed = leg_move_speed;
    set_site(2, x_tmp, y_tmp, z_tmp);
  }
  else if(step_moving == 4){
    step_moving = -1;
    exit_moving = false;
    saved = false;
    body_left(15);
  }
}

void hand_wave(bool next_step){
  if(!next_step) return;
  step_moving++;
  static float x_tmp, y_tmp, z_tmp;
  static bool saved = false;
  move_speed = body_move_speed;
  if(step_moving == 0){
    body_right(15);
  }
  else if(step_moving == 1){
    if(saved == false){
      x_tmp = site_now[2][0];
      y_tmp = site_now[2][1];
      z_tmp = site_now[2][2];
      saved = true;
    }
    move_speed = leg_move_speed / 2.0;
    set_site(2, turn_x1, turn_y1, 50);
  }
  else if(step_moving == 2){
    move_speed = leg_move_speed / 2.0;
    set_site(2, turn_x0, turn_y0, 50);
    if(!exit_moving){
      step_moving = 0; // loop to step 1
    }
  }
  else if(step_moving == 3){
    move_speed = leg_move_speed;
    set_site(2, x_tmp, y_tmp, z_tmp);
  }
  else if(step_moving == 4){
    step_moving = -1;
    exit_moving = false;
    saved = false;
    body_left(15);
  }
}

void hand_shake(bool next_step){
  if(!next_step) return;
  step_moving++;
  static float x_tmp, y_tmp, z_tmp;
  static bool saved = false;
  move_speed = body_move_speed;
  if(step_moving == 0){
    body_right(15);
  }
  else if(step_moving == 1){
    if(saved == false){
      x_tmp = site_now[2][0];
      y_tmp = site_now[2][1];
      z_tmp = site_now[2][2];
      saved = true;
    }
    move_speed = leg_move_speed;
    set_site(2, x_default - 30, y_start + 2 * y_step, 55);
  }
  else if(step_moving == 2){
    move_speed = leg_move_speed;
    set_site(2, x_default - 30, y_start + 2 * y_step, 10);
    if(!exit_moving){
      step_moving = 0; // loop to step 1
    }
  }
  else if(step_moving == 3){
    move_speed = leg_move_speed;
    set_site(2, x_tmp, y_tmp, z_tmp);
  }
  else if(step_moving == 4){
    move_speed = 1;
    body_left(15);
    step_moving = -1;
    exit_moving = false;
    saved = false;
  }
}

/*
  - spot turn to left
  - non-blocking function
  - parameter next step
   ---------------------------------------------------------------------------*/
void turn_left(bool next_step){
  if(!next_step) return;
  step_moving++;
  move_speed = spot_turn_speed;
  if(step_moving == 0){
    //leg 3&1 move
    set_site(3, x_default + x_offset, y_start, z_up);
  }
  else if(step_moving == 1){
    set_site(0, turn_x1 - x_offset, turn_y1, z_default);
    set_site(1, turn_x0 - x_offset, turn_y0, z_default);
    set_site(2, turn_x1 + x_offset, turn_y1, z_default);
    set_site(3, turn_x0 + x_offset, turn_y0, z_up);
  }
  else if(step_moving == 2){
    set_site(3, turn_x0 + x_offset, turn_y0, z_default);
  }
  else if(step_moving == 3){
    set_site(0, turn_x1 + x_offset, turn_y1, z_default);
    set_site(1, turn_x0 + x_offset, turn_y0, z_default);
    set_site(2, turn_x1 - x_offset, turn_y1, z_default);
    set_site(3, turn_x0 - x_offset, turn_y0, z_default);
  }
  else if(step_moving == 4){
    set_site(1, turn_x0 + x_offset, turn_y0, z_up);
  }
  else if(step_moving == 5){
    set_site(0, x_default + x_offset, y_start, z_default);
    set_site(1, x_default + x_offset, y_start, z_up);
    set_site(2, x_default - x_offset, y_start + y_step, z_default);
    set_site(3, x_default - x_offset, y_start + y_step, z_default);
  }
  else if(step_moving == 6){
    set_site(1, x_default + x_offset, y_start, z_default);
  }
  //leg 0&2 move
  else if(step_moving == 7){
    set_site(0, x_default + x_offset, y_start, z_up);
  }
  else if(step_moving == 8){
    set_site(0, turn_x0 + x_offset, turn_y0, z_up);
    set_site(1, turn_x1 + x_offset, turn_y1, z_default);
    set_site(2, turn_x0 - x_offset, turn_y0, z_default);
    set_site(3, turn_x1 - x_offset, turn_y1, z_default);
  }
  else if(step_moving == 9){
    set_site(0, turn_x0 + x_offset, turn_y0, z_default);
  }
  else if(step_moving == 10){
    set_site(0, turn_x0 - x_offset, turn_y0, z_default);
    set_site(1, turn_x1 - x_offset, turn_y1, z_default);
    set_site(2, turn_x0 + x_offset, turn_y0, z_default);
    set_site(3, turn_x1 + x_offset, turn_y1, z_default);
  }
  else if(step_moving == 11){
    set_site(2, turn_x0 + x_offset, turn_y0, z_up);
  }
  else if(step_moving == 12){
    set_site(0, x_default - x_offset, y_start + y_step, z_default);
    set_site(1, x_default - x_offset, y_start + y_step, z_default);
    set_site(2, x_default + x_offset, y_start, z_up);
    set_site(3, x_default + x_offset, y_start, z_default);
  }
  else if(step_moving == 13){
    set_site(2, x_default + x_offset, y_start, z_default);
    step_moving = -1;
  }
}

/*
  - spot turn to right
  - non-blocking function
  - parameter next step
   ---------------------------------------------------------------------------*/
void turn_right(bool next_step){
  if(!next_step) return;
  step_moving++;
  move_speed = spot_turn_speed;
    //leg 2&0 move
  if(step_moving == 0){
    set_site(2, x_default + x_offset, y_start, z_up);
  }
  else if(step_moving == 1){
    set_site(0, turn_x0 - x_offset, turn_y0, z_default);
    set_site(1, turn_x1 - x_offset, turn_y1, z_default);
    set_site(2, turn_x0 + x_offset, turn_y0, z_up);
    set_site(3, turn_x1 + x_offset, turn_y1, z_default);
  }
  else if(step_moving == 2){
    set_site(2, turn_x0 + x_offset, turn_y0, z_default);
  }
  else if(step_moving == 3){
    set_site(0, turn_x0 + x_offset, turn_y0, z_default);
    set_site(1, turn_x1 + x_offset, turn_y1, z_default);
    set_site(2, turn_x0 - x_offset, turn_y0, z_default);
    set_site(3, turn_x1 - x_offset, turn_y1, z_default);
  }
  else if(step_moving == 4){
    set_site(0, turn_x0 + x_offset, turn_y0, z_up);
  }
  else if(step_moving == 5){
    set_site(0, x_default + x_offset, y_start, z_up);
    set_site(1, x_default + x_offset, y_start, z_default);
    set_site(2, x_default - x_offset, y_start + y_step, z_default);
    set_site(3, x_default - x_offset, y_start + y_step, z_default);
  }
  else if(step_moving == 6){
    set_site(0, x_default + x_offset, y_start, z_default);
  }
  //leg 1&3 move
  else if(step_moving == 7){
    set_site(1, x_default + x_offset, y_start, z_up);
  }
  else if(step_moving == 8){
    set_site(0, turn_x1 + x_offset, turn_y1, z_default);
    set_site(1, turn_x0 + x_offset, turn_y0, z_up);
    set_site(2, turn_x1 - x_offset, turn_y1, z_default);
    set_site(3, turn_x0 - x_offset, turn_y0, z_default);
  }
  else if(step_moving == 9){
    set_site(1, turn_x0 + x_offset, turn_y0, z_default);
  }
  else if(step_moving == 10){
    set_site(0, turn_x1 - x_offset, turn_y1, z_default);
    set_site(1, turn_x0 - x_offset, turn_y0, z_default);
    set_site(2, turn_x1 + x_offset, turn_y1, z_default);
    set_site(3, turn_x0 + x_offset, turn_y0, z_default);
  }
  else if(step_moving == 11){
    set_site(3, turn_x0 + x_offset, turn_y0, z_up);
  }
  else if(step_moving == 12){
    set_site(0, x_default - x_offset, y_start + y_step, z_default);
    set_site(1, x_default - x_offset, y_start + y_step, z_default);
    set_site(2, x_default + x_offset, y_start, z_default);
    set_site(3, x_default + x_offset, y_start, z_up);
  }
  else if(step_moving == 13){
    set_site(3, x_default + x_offset, y_start, z_default);
    step_moving = -1;
  }
}

/*
  - go forward
  - non-blocking function
  - parameter next step
   ---------------------------------------------------------------------------*/
void step_forward(bool next_step){
  if(!next_step) return;
  step_moving++;
  move_speed = leg_move_speed;
    //leg 2&1 move
  if(step_moving == 0){
    set_site(2, x_default + x_offset, y_start, z_up);
  }
  else if(step_moving == 1){
    set_site(2, x_default + x_offset, y_start + 2 * y_step, z_up);
  }
  else if(step_moving == 2){
    set_site(2, x_default + x_offset, y_start + 2 * y_step, z_default);
  }
  else if(step_moving == 3){
    move_speed = body_move_speed;
    set_site(0, x_default + x_offset, y_start, z_default);
    set_site(1, x_default + x_offset, y_start + 2 * y_step, z_default);
    set_site(2, x_default - x_offset, y_start + y_step, z_default);
    set_site(3, x_default - x_offset, y_start + y_step, z_default);
  }
  else if(step_moving == 4){
    set_site(1, x_default + x_offset, y_start + 2 * y_step, z_up);
  }
  else if(step_moving == 5){
    set_site(1, x_default + x_offset, y_start, z_up);
  }
  else if(step_moving == 6){
    set_site(1, x_default + x_offset, y_start, z_default);
  }
  //leg 0&3 move
  else if(step_moving == 7){
    set_site(0, x_default + x_offset, y_start, z_up);
  }
  else if(step_moving == 8){
    set_site(0, x_default + x_offset, y_start + 2 * y_step, z_up);
  }
  else if(step_moving == 9){
    set_site(0, x_default + x_offset, y_start + 2 * y_step, z_default);
  }
  else if(step_moving == 10){
    move_speed = body_move_speed;
    set_site(0, x_default - x_offset, y_start + y_step, z_default);
    set_site(1, x_default - x_offset, y_start + y_step, z_default);
    set_site(2, x_default + x_offset, y_start, z_default);
    set_site(3, x_default + x_offset, y_start + 2 * y_step, z_default);
  }
  else if(step_moving == 11){
    set_site(3, x_default + x_offset, y_start + 2 * y_step, z_up);
  }
  else if(step_moving == 12){
    set_site(3, x_default + x_offset, y_start, z_up);
  }
  else if(step_moving == 13){
    set_site(3, x_default + x_offset, y_start, z_default);
    step_moving = -1;
  }
}

/*
  - go back
  - non-blocking function
  - parameter next step
   ---------------------------------------------------------------------------*/
void step_back(bool next_step){
  if(!next_step) return;
  step_moving++;
  move_speed = leg_move_speed;
  //leg 2&1 move
  if(step_moving == 0){
    set_site(3, x_default + x_offset, y_start, z_up);
  }
  else if(step_moving == 1){
    set_site(3, x_default + x_offset, y_start + 2 * y_step, z_up);
  }
  else if(step_moving == 2){
    set_site(3, x_default + x_offset, y_start + 2 * y_step, z_default);
  }
  else if(step_moving == 3){
    move_speed = body_move_speed;
    set_site(0, x_default + x_offset, y_start + 2 * y_step, z_default);
    set_site(1, x_default + x_offset, y_start, z_default);
    set_site(2, x_default - x_offset, y_start + y_step, z_default);
    set_site(3, x_default - x_offset, y_start + y_step, z_default);
  }
  else if(step_moving == 4){
    set_site(0, x_default + x_offset, y_start + 2 * y_step, z_up);
  }
  else if(step_moving == 5){
    set_site(0, x_default + x_offset, y_start, z_up);
  }
  else if(step_moving == 6){
    set_site(0, x_default + x_offset, y_start, z_default);
  }
  //leg 0&3 move
  else if(step_moving == 7){
    set_site(1, x_default + x_offset, y_start, z_up);
  }
  else if(step_moving == 8){
    set_site(1, x_default + x_offset, y_start + 2 * y_step, z_up);
  }
  else if(step_moving == 9){
    set_site(1, x_default + x_offset, y_start + 2 * y_step, z_default);
  }
  else if(step_moving == 10){
    move_speed = body_move_speed;
    set_site(0, x_default - x_offset, y_start + y_step, z_default);
    set_site(1, x_default - x_offset, y_start + y_step, z_default);
    set_site(2, x_default + x_offset, y_start + 2 * y_step, z_default);
    set_site(3, x_default + x_offset, y_start, z_default);
  }
  else if(step_moving == 11){
    set_site(2, x_default + x_offset, y_start + 2 * y_step, z_up);
  }
  else if(step_moving == 12){
    set_site(2, x_default + x_offset, y_start, z_up);
  }
  else if(step_moving == 13){
    set_site(2, x_default + x_offset, y_start, z_default);
    step_moving = -1;
  }
}

/*
  - trans site from cartesian to polar
  - mathematical model 2/2
   ---------------------------------------------------------------------------*/
void cartesian_to_polar(volatile float &alpha, volatile float &beta, volatile float &gamma, volatile float x, volatile float y, volatile float z){
  //calculate w-z degree
  float v, w;
  w = (x >= 0 ? 1 : -1) * (sqrt(pow(x, 2) + pow(y, 2)));
  v = w - length_c;
  alpha = atan2(z, v) + acos((pow(length_a, 2) - pow(length_b, 2) + pow(v, 2) + pow(z, 2)) / 2 / length_a / sqrt(pow(v, 2) + pow(z, 2)));
  beta = acos((pow(length_a, 2) + pow(length_b, 2) - pow(v, 2) - pow(z, 2)) / 2 / length_a / length_b);
  //calculate x-y-z degree
  gamma = (w >= 0) ? atan2(y, x) : atan2(-y, -x);
  //trans degree pi->180
  alpha = alpha / pi * 180;
  beta = beta / pi * 180;
  gamma = gamma / pi * 180;
}

/*
  - trans site from polar to microservos
  - mathematical model map to fact
  - the errors saved in eeprom will be add
   ---------------------------------------------------------------------------*/
void polar_to_servo(int leg, float alpha, float beta, float gamma, bool is_map = true){
  if(is_map){
    if (leg == 0){
      alpha = 90 - alpha;
      beta = beta;
      gamma += 90;
    }
    else if (leg == 1){
      alpha += 90;
      beta = 180 - beta;
      gamma = 90 - gamma;
    }
    else if (leg == 2){
      alpha += 90;
      beta = 180 - beta;
      gamma = 90 - gamma;
    }
    else if (leg == 3){
      alpha = 90 - alpha;
      beta = beta;
      gamma += 90;
    }
  }
  cur_alpha[leg] = alpha;
  cur_beta[leg] = beta;
  cur_gamma[leg] = gamma;
  
  servo[leg][0].write(alpha + offset_alpha[leg]);
  servo[leg][1].write(beta + offset_beta[leg]);
  servo[leg][2].write(180 - (gamma + offset_gamma[leg]));
}

/*
  - microservos service /timer interrupt function/50Hz
  - when set site expected,this function move the end point to it in a straight line
  - temp_speed[4][3] should be set before set expect site,it make sure the end point
   move in a straight line,and decide move speed.
   ---------------------------------------------------------------------------*/
void servo_service(void){
  static float alpha, beta, gamma;
  for (int i = 0; i < 4; i++){
    for (int j = 0; j < 3; j++){
      if (abs(site_now[i][j] - site_expect[i][j]) >= abs(temp_speed[i][j]))
        site_now[i][j] += temp_speed[i][j];
      else
        site_now[i][j] = site_expect[i][j];
    }

    cartesian_to_polar(alpha, beta, gamma, site_now[i][0], site_now[i][1], site_now[i][2]);
    polar_to_servo(i, alpha, beta, gamma);
  }

  rest_counter++;
}

/*
  - set one of end points' expect site
  - this founction will set temp_speed[4][3] at same time
  - non - blocking function
   ---------------------------------------------------------------------------*/
void set_site(int leg, float x, float y, float z){
  float length_x = 0, length_y = 0, length_z = 0;

  if (x != KEEP)
    length_x = x - site_now[leg][0];
  if (y != KEEP)
    length_y = y - site_now[leg][1];
  if (z != KEEP)
    length_z = z - site_now[leg][2];

  float length = sqrt(pow(length_x, 2) + pow(length_y, 2) + pow(length_z, 2));

  temp_speed[leg][0] = length_x / length * move_speed * speed_multiple;
  temp_speed[leg][1] = length_y / length * move_speed * speed_multiple;
  temp_speed[leg][2] = length_z / length * move_speed * speed_multiple;

  if (x != KEEP)
    site_expect[leg][0] = x;
  if (y != KEEP)
    site_expect[leg][1] = y;
  if (z != KEEP)
    site_expect[leg][2] = z;
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  if(type == WStype_TEXT && num == 0){
    if(payload[0] != '#'){
      String data_recive = String((char *)payload);
      if(data_recive.indexOf("Up") == 0){
        int leg = (int)data_recive.charAt(2) - 48;
        if(leg <= 3){
          float *cur_value[3] = {&cur_alpha[leg], &cur_beta[leg], &cur_gamma[leg]};
          int16_t *offset_value[3] = {&offset_alpha[leg], &offset_beta[leg], &offset_gamma[leg]};
          int index = (int)data_recive.charAt(3) - 48;
          if(index <= 2){
            float current_angle = *cur_value[index] + *offset_value[index];
            if(current_angle < 180){
              *offset_value[index] += 1;
            }
            else Serial.println("Reached maximum");
          }
          polar_to_servo(leg, cur_alpha[leg], cur_beta[leg], cur_gamma[leg], false);
        }
      }
      else if(data_recive.indexOf("Down") == 0){
        int leg = (int)data_recive.charAt(4) - 48;
        if(leg <= 3){
          float *cur_value[3] = {&cur_alpha[leg], &cur_beta[leg], &cur_gamma[leg]};
          int16_t *offset_value[3] = {&offset_alpha[leg], &offset_beta[leg], &offset_gamma[leg]};
          int index = (int)data_recive.charAt(5) - 48;
          if(index <= 2){
            float current_angle = *cur_value[index] + *offset_value[index];
            if(current_angle > 0){
              *offset_value[index] -= 1;
            }
            else Serial.println("Reached minimum");
          }
          polar_to_servo(leg, cur_alpha[leg], cur_beta[leg], cur_gamma[leg], false);
        }
      }
      else if(data_recive.indexOf("Save") == 0){
        for(uint8_t i = 0; i < 4; i++){
          int16_t *offset_value[3] = {&offset_alpha[i], &offset_beta[i], &offset_gamma[i]};
          for(uint8_t j = 0; j < 3; j++){
            write_eeprom(j*3 + i*4*3, *offset_value[j]);
          }
        }
        beep_count = 4;
      }
      else if(data_recive.indexOf("Stand") == 0){
        next_command = emStand;
      }
      else if(data_recive.indexOf("Forward") == 0){
        next_command = emForward;
      }
      else if(data_recive.indexOf("Back") == 0){
        next_command = emBack;
      }
      else if(data_recive.indexOf("Left") == 0){
        next_command = emLeft;
      }
      else if(data_recive.indexOf("Right") == 0){
        next_command = emRight;
      }
      else if(data_recive.indexOf("Sit") == 0){
        next_command = emSit;
      }
      else if(data_recive.indexOf("Wave") == 0){
        next_command = emWave;
      }
      else if(data_recive.indexOf("Shake") == 0){
        next_command = emShake;
      }
      else if(data_recive.indexOf("Stomp") == 0){
        next_command = emStomp;
      }
      else if(data_recive.indexOf("Speed") == 0){
        int speed = (int)data_recive.charAt(5) - 48;
        if(data_recive.length() >= 7){
          speed = speed*10 + ((int)data_recive.charAt(6) - 48);
        }
        leg_move_speed = speed;
        body_move_speed = speed / 2.0;
      }
      //Serial.println(data_recive);
    }
  }
  else if(type == WStype_CONNECTED){
    Serial.printf("[%u] Connected\n", num);
    send2wed("Speed" + String(leg_move_speed));
    connected = true;
  }
  else if(type == WStype_DISCONNECTED){
    connected = false;
    Serial.printf("[%u] Disconnected!\n", num);
  }
}

bool site_reached(){
  for (int i = 0; i < 4; i++){
    for(int j = 0; j < 3; j++){
      if (site_now[i][j] != site_expect[i][j]){
        return false;
      }
    }
  }
  return true;
}

bool send_rf(String data, int16_t times){
  radio.stopListening();
  char text[32] = {"\0"};
  bool ret = false;
  data.toCharArray(text,data.length()+1);
  while(--times >= 0){
    if(radio.write(&text, sizeof(text))){
      ret = true;
      break;
    }
  }
  radio.startListening();
  return ret;
}

void print_rfdata(){
  Serial.printf("XL=%d\tYL=%d\tXR=%d\tYR=%d\tMode=%d\tButton=%d%d%d%d%d%d%d%d%d%d%d%d\tEncoder=%d\n",
    rfdata.joys_leftX, rfdata.joys_leftY, rfdata.joys_rightX, rfdata.joys_rightY,
    rfdata.mode_sel1, rfdata.button[0], rfdata.button[1], rfdata.button[2], rfdata.button[3], 
    rfdata.button[4], rfdata.button[5], rfdata.button[6], rfdata.button[7], rfdata.button[8], 
    rfdata.button[9], rfdata.button[10], rfdata.button[11], rfdata.encoder_postion);
}

void parse_data(char *data){
  /* byte 0                      |byte 1  |byte 2  |byte 3  |byte 4  |byte 5       |byte 6
   * 0       |12345 |6   |7      |01234567|01234567|01234567|01234567|0123    |456701234567
   * 0       |12345 |6   |7      |89012345|67890123|45678901|01234567|8901    |234567890123
   * is valid|en_pos|mode|bat_req|joys_LX |joys_LY |joys_RX |joys_RY |dir_joys|button[0:10]
   */
  static int8_t latest_encoder = -1;
  if((int)data[0] & 0x1 == 0)
    return;
  rfdata.time_lastRecive = millis();
  rfdata.encoder_postion = (((int)data[0] >> 1) & 0x1F);
  rfdata.mode_sel1 = ((int)data[0] >> 6) & 0x1;
  if(((int)data[0] >> 7) & 0x1){ // request read battery
    send_rf("Unk", 1);
  }
  rfdata.joys_leftX = (uint8_t)data[1];
  rfdata.joys_leftY = (uint8_t)data[2];
  rfdata.joys_rightX = (uint8_t)data[3];
  rfdata.joys_rightY = (uint8_t)data[4];
  if((((int)data[5] >> 0) & 0x1) == 0) rfdata.joys_leftX *= -1;
  if((((int)data[5] >> 1) & 0x1) == 0) rfdata.joys_leftY *= -1;
  if((((int)data[5] >> 2) & 0x1) == 0) rfdata.joys_rightX *= -1;
  if((((int)data[5] >> 3) & 0x1) == 0) rfdata.joys_rightY *= -1;

  for(uint8_t i = 4; i < 8; i++){
    rfdata.button[i-4] = ((int)data[5] >> i) & 0x1;
  }
  for(uint8_t i = 0; i < 8; i++){
    rfdata.button[i+4] = ((int)data[6] >> i) & 0x1;
  }

  // control the spider robot base rfdata
  if(latest_encoder == -1) latest_encoder = rfdata.encoder_postion;
  if(rfdata.encoder_postion != latest_encoder){
    int diff = rfdata.encoder_postion - latest_encoder;
    if(abs(diff) < 19){
      // do nothing
    }
    else{
      if(rfdata.encoder_postion < 10)
        diff = (19 - latest_encoder) + rfdata.encoder_postion;
      else
        diff = -(latest_encoder + (19 - rfdata.encoder_postion));
    }
    leg_move_speed += diff;
    if(leg_move_speed > 10)     leg_move_speed = 10;
    else if(leg_move_speed < 1) leg_move_speed = 1;
    body_move_speed = leg_move_speed / 2.0;
  }
  
  if(rfdata.button[9] == 1){
    next_command = emWave;
  }
  else if(rfdata.button[11] == 1){
    next_command = emShake;
  }
  else if(rfdata.button[10] == 1){
    next_command = emStomp;
  }
  else if(rfdata.button[8] == 1){
    next_command = emSit;
  }
  else if(rfdata.button[1] == 1){
    next_command = emForward;
  }
  else if(rfdata.button[4] == 1){
    next_command = emBack;
  }
  else if(rfdata.button[5] == 1){
    next_command = emLeft;
  }
  else if(rfdata.button[3] == 1){
    next_command = emRight;
  }
  else if(cur_command != emStand){
    next_command = emStand;
  }

  latest_encoder = rfdata.encoder_postion;
//  print_rfdata();
}

void read_nRF(){
  if(radio.available()){
    char text[8] = "";
    radio.read(&text, sizeof(text));
    String str = String(text);
    if(text[0] & 0x1){
      parse_data(text);
    }
    else{
      if(str.indexOf("$") >= 0){ // tx reads parameter
        // do nothing
      }
      else if(str.indexOf("#") >= 0){ // tx sets parameter
        // do nothing
      }
      else{
        Serial.printf("Unknow command: %s\n", text);
      }
    }
  }
}

void setup_webserver(String ip){
  server.on("/",[](){
    server.send_P(200, "text/html", ControlPage);
  });
  server.on("/setup",[](){
    server.send_P(200, "text/html", SetupPage);
  });
  server.begin();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  Serial.print("Control spider robot at: \n\thttp://");
  Serial.println(ip);
  
  Serial.print("Correct position for servos at: \n\thttp://");
  Serial.print(ip);
  Serial.println("/setup");
}

void setup(){
  delay(100);
  WiFi.begin(ssid, password);
  delay(100); WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  Serial.begin(115200);
  EEPROM.begin(150);
  ledcSetup(BUZZER_PWM_CHANNEL, 2000, 8);
  ledcAttachPin(BUZZER_PIN, BUZZER_PWM_CHANNEL);
  for(uint8_t i = 0; i < 4; i++){
    int16_t *offset_value[3] = {&offset_alpha[i], &offset_beta[i], &offset_gamma[i]};
    for(int8_t j = 0; j < 3; j++){
      *offset_value[j] = read_eeprom(j*3 + i*4*3);
      if(*offset_value[j] >= 180 || *offset_value[j] <= -180){
        write_eeprom(j*3 + i*4*3, 0);
        j--;
        continue;
      }
      //Serial.printf("%d \t", *offset_value[j]);
    }
    //Serial.println();
  }
  if(EEPROM.read(37) > 1){
    EEPROM.write(37, 0);
    EEPROM.commit();
  }
  rf_enable = EEPROM.read(37);
  if(radio.begin()){
    is_nRF_ok = true;
    radio.setPayloadSize(32);
    radio.setDataRate(RF24_2MBPS);
    radio.openWritingPipe(0xEECCEECCEE);
    radio.openReadingPipe(1, 0xEECCEECCEE);
    radio.setPALevel(RF24_PA_MAX);
    radio.startListening();
    if(!rf_enable){
      radio.powerDown();
      Serial.println("RF disabled");
    }
    else{
      Serial.println("RF enabled");
    }
  }
  else{
    is_nRF_ok = false;
    Serial.println("nRF is BAD!");
  }

  set_site(0, x_default - x_offset, y_start + y_step, z_boot);
  set_site(1, x_default - x_offset, y_start + y_step, z_boot);
  set_site(2, x_default + x_offset, y_start, z_boot);
  set_site(3, x_default + x_offset, y_start, z_boot);

  for (int i = 0; i < 4; i++){
    for (int j = 0; j < 3; j++){
      site_now[i][j] = site_expect[i][j];
    }
  }

  for (int i = 0; i < 4; i++){
    for (int j = 0; j < 3; j++){
      if(!servo[i][j].attach(SERVO_PINS[i][j])){
        delay(1000); Serial.println("Initialize servo failed");
        while(true) delay(10);
      }
      delay(10);
    }
  }

  // check and reset(if any) for ssid and password
  for(uint8_t i = 0; i < 50; i++){
    uint8_t c = EEPROM.read(50+i);
    if(c == 0 && i > 0){
      break;
    }
    else if(c < 32 || c > 126){
      char temp[12] = "SpiderRobot";
      for(uint8_t j = 0; j < 11; j++){
        EEPROM.write(50+j, (int)temp[j]);
      }
      EEPROM.write(50+11, 0);
      EEPROM.commit();
      break;
    }
  }
  for(uint8_t i = 0; i < 50; i++){
    uint8_t c = EEPROM.read(100+i);
    if(c == 0 && i >= 8){
      break;
    }
    else if((c < 32 || c > 126) || (i < 8 && c == 0)){
      for(uint8_t j = 0; j < 8; j++){
        EEPROM.write(100+j, 48);
      }
      EEPROM.write(100+8, 0);
      EEPROM.commit();
      break;
    }
  }

  // read ssid and password
  for(uint8_t i = 0; i < 50; i++){
    uint8_t c = EEPROM.read(50+i);
    ssid[i] = (char)c;
    if(c == 0) break;
  }
  for(uint8_t i = 0; i < 50; i++){
    uint8_t c = EEPROM.read(100+i);
    password[i] = (char)c;
    if(c == 0) break;
  }
  
  if(EEPROM.read(38) > 2){
    EEPROM.write(38, 0);
    EEPROM.commit();
  }
  if(EEPROM.read(38) == 1){
    wifi_state = emSetup;
  }
  else if(EEPROM.read(38) == 2){
    wifi_state = emAccessPointSetup;
  }
  Serial.println("Ready!");
}

void loop(){
  if(is_nRF_ok && rf_enable){
    read_nRF();
  }
  if(Serial.available()){
    delay(10);
    String str = "";
    while(Serial.available()){
      str += (char)Serial.read();
    }

    bool invalid_command = true;
    if(str.indexOf("\r") > 0) str.replace("\r","");
    if(str.indexOf("\n") > 0) str.replace("\n","");
    if(str.indexOf("Wifi ") == 0){
      if(str.indexOf(" on") > 0){
        if(wifi_state == emNotConnect){
          EEPROM.write(38, 1);
          EEPROM.commit();
          wifi_state = emSetup;
        }
      }
      else if(str.indexOf(" off") > 0){
        if(wifi_state == emConnected || wifi_state == emAccessPoint){
          if(web_enable){
            webSocket.close();
            server.close();
          }
          WiFi.disconnect();
          WiFi.mode(WIFI_OFF);
          WiFi.disconnect(true);
          wifi_state = emNotConnect;
          web_enable = false;
          Serial.println("Turned off Wifi");
        }
        else if(wifi_state == emConnecting){
          WiFi.disconnect();
          WiFi.mode(WIFI_OFF);
          WiFi.disconnect(true);
          wifi_state = emNotConnect;
          Serial.println("Turned off Wifi");
        }
        EEPROM.write(38, 0);
        EEPROM.commit();
      }
      else if(str.indexOf(" ap") > 0){
        if(wifi_state == emNotConnect){
          EEPROM.write(38, 2);
          EEPROM.commit();
          wifi_state = emAccessPointSetup;
        }
      }
    }
    else if(str.indexOf("RF ") == 0){
      if(is_nRF_ok == true){
        if(str.indexOf(" on") > 0 && !rf_enable){
          rf_enable = true;
          radio.powerUp();delay(10);
          radio.startListening();
          EEPROM.write(37, 1);
          EEPROM.commit();
          Serial.println("Turned on RF");
        }
        else if(str.indexOf(" off") > 0 && rf_enable){
          rf_enable = false;
          radio.powerDown();
          EEPROM.write(37, 0);
          EEPROM.commit();
          Serial.println("Turned off RF");
        }
      }
      else{
        Serial.println("RF initialize failed before");
      }
    }
    else if(str.indexOf("ssid=") == 0){
      str.replace("ssid=","");
      str.toCharArray(ssid, str.length()+1);
      if(str.length() == 0){
        Serial.println("ssid must be a string");
        invalid_command = false;
      }
      else{
        for(uint8_t i = 0; i < str.length()+1 && i < 50; i++){
          EEPROM.write(50+i, ssid[i]);
        }
        EEPROM.commit();
      }
    }
    else if(str.indexOf("pass=") == 0){
      str.replace("pass=","");
      str.toCharArray(password, str.length()+1);
      if(str.length() < 8){
        Serial.println("password must be 8 characters as less");
        invalid_command = false;
      }
      else{
        for(uint8_t i = 0; i < str.length()+1 && i < 50; i++){
          EEPROM.write(100+i, password[i]);
        }
        EEPROM.commit();
      }
    }
    else{
      Serial.println("Usage:");
      Serial.println("  \"ssid=example_ssid\": set ssid for wifi");
      Serial.println("  \"pass=example_pass\": set password for wifi");
      Serial.println("  \"Wifi on\": turn on wifi and webserver");
      Serial.println("  \"Wifi ap\": turn on wifi with access point mode and webserver");
      Serial.println("  \"Wifi off\": turn off wifi and webserver");
      Serial.println("  \"RF on\": turn on RF");
      Serial.println("  \"RF off\": turn off RF");
      invalid_command = false;
    }

    if(invalid_command){
      Serial.printf(">> %s\n", str.c_str());
    }
  }
  
  if(micros() >= execution_time){
    execution_time = micros() + 20000;
    servo_service();
  }

  if(wifi_state == emSetup){
    Serial.printf("Connecting to \"%s\"\n", ssid);
    WiFi.begin(ssid, password);
    reconnect_wifi_time = millis() + 5000;
    wifi_state = emConnecting;
  }
  else if(wifi_state == emConnecting){
    if(WiFi.status() == WL_CONNECTED){
      Serial.println("Wifi is connected");
      setup_webserver(WiFi.localIP().toString());
      wifi_state = emConnected;
      beep_count = 6;
      web_enable = true;
    }
    else{
      if(millis() >= reconnect_wifi_time){
        beep_count = 2;
        Serial.println("Try reconnecting ^^");
        reconnect_wifi_time = millis() + 5000;
        WiFi.reconnect();
      }
    }
  }
  else if(wifi_state == emAccessPointSetup){
    Serial.println("Wifi access point mode");
    WiFi.softAP(ssid, password);
    Serial.printf("ssid    : %s\n", ssid);
    Serial.printf("password: %s\n", password);
    setup_webserver(WiFi.softAPIP().toString());
    web_enable = false;
    wifi_state = emAccessPoint;
  }

  static uint32_t time_check_wifi = 0;
  if(millis() >= time_check_wifi){
    static uint8_t lost_count = 0;
    if(wifi_state == emConnected){
      if(WiFi.status() != WL_CONNECTED){
        if(++lost_count >= 2){
          WiFi.disconnect(); delay(100);
          wifi_state = emSetup;
          beep_count = 8;
        }
        else{
          Serial.printf("Lost wifi connection %d\n", lost_count);
        }
      }
      else{
        lost_count = 0;
      }
    }
    time_check_wifi = millis() + 500;
  }
  
  if(web_enable){
    webSocket.loop();
    server.handleClient();
    static uint32_t time_check = 0;
    if(millis() >= time_check){
      send2wed("Checked");
      time_check = millis() + 1000;
    }
  }

  bool reached = site_reached();
  if(reached && next_command != cur_command){
    if(cur_command == emWave || cur_command == emShake || cur_command == emStomp){
      if(step_moving != -1){
        exit_moving = true;
      }
    }
    if(step_moving == -1){
      cur_command = next_command;
    }
  }
  if(cur_command == emForward){
    step_forward(reached);
  }
  else if(cur_command == emBack){
    step_back(reached);
  }
  else if(cur_command == emLeft){
    turn_left(reached);
  }
  else if(cur_command == emRight){
    turn_right(reached);
  }
  else if(cur_command == emWave){
    hand_wave(reached);
  }
  else if(cur_command == emShake){
    hand_shake(reached);
  }
  else if(cur_command == emStomp){
    stomp(reached);
  }
  else if(cur_command == emStand){
    if(reached){
      step_moving++;
      if(step_moving == 0){
        move_speed = stand_seat_speed;
        for (int leg = 0; leg < 4; leg++){
          set_site(leg, KEEP, KEEP, z_default);
        }
      }
      else if(step_moving == 1){
        step_moving = -1;
        cur_command = emNone;
        next_command = cur_command;
      }
    }
  }
  else if(cur_command == emSit){
    if(reached){
      step_moving++;
      if(step_moving == 0){
        move_speed = stand_seat_speed;
        for (int leg = 0; leg < 4; leg++){
          set_site(leg, KEEP, KEEP, z_boot);
        }
      }
      else if(step_moving == 1){
        step_moving = -1;
        cur_command = emNone;
        next_command = cur_command;
      }
    }
  }

  if(millis() >= beep_run_time && beep_count > 0){
    beep_count--;
    if(beep_count % 2)
      ledcWriteTone(BUZZER_PWM_CHANNEL, 200);
    else
      ledcWriteTone(BUZZER_PWM_CHANNEL, 0);
    beep_run_time = millis() + 100;
  }
}
