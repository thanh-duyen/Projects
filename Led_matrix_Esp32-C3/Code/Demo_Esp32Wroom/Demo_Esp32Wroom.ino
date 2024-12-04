/* Display  | ESP32-WROOM 
 * GND      | GND
 * 5V       | VIN
 * TX       | RX2 (IO16)
 * RX       | TX2 (IO17)
 * 
 * Color defination:
 * red  : 1
 * green: 2
 * both : 3 (means yellow)
 * 
 * All command and usage:
  help
  print message
  drawPixel x y color
  drawFastVLine x y h color
  drawFastHLine x y w color
  fillRect x y w h color
  fillScreen color
  drawLine x0 y0 x1 y1 color
  drawRect x y w h color
  drawCircle x y r color
  drawCircleHelper x y r cornername color
  fillCircle x y r color
  fillCircleHelper x y r cornername color
  drawTriangle x0 y0 x1 y1 x2 y2 color
  fillTriangle x0 y0 x1 y1 x2 y2 color
  drawRoundRect x y w h r color
  fillRoundRect x y w h r color
  setCursor 
    x y (Normal set) 
    0 (Off cursor/blink)
    mode x y color (On cursor(1)/blink(2))
  setTextColor 
    color 
    color bg_color
  drawChar 
    x y char color bg size 
    x y char color bg x_size y_size
  setTextSize 
    size 
    x_size y_size
  time 
    yyyy mm dd hh mm ss (Set time) 
    No parameter (Get time: hh:mm:ss wday mmm dd.yyyy)
  clockDisplay 
    No parameter (change to clock display)
    color (change color for clock)
  setDisplay width_block height_block buffer board_defination (0: sub, 1: main)
  setScroll 
    0 (Off scroll)
    0 index (change index display, default is 0)
    1 max_index delay notify (On scroll)
 */

bool send_command(String command, uint32_t timeout = 50){
  if(command.indexOf("\r\n") < 0) command += "\r\n";
  Serial2.print(command);
  command = "";
  timeout += millis();
  while(millis() < timeout){ // wait for receive response from display or reach time out
    if (Serial2.available()) {
      command += Serial2.read();
      if(command.indexOf("\r\n") >= 0){
        if(command.indexOf("OK") >= 0) // command valid
          return true;
        else                           // command and input parameters are not correctly
          return false;
      }
    }
  }
  return false;
}

bool wait_ScrollNotify(uint32_t timeout = 50){
  timeout += millis();
  String data_rx = "";
  while(millis() < timeout){
    if (Serial2.available()) {
      data_rx += (char)Serial2.read();
      if(data_rx.indexOf("\r\n") >= 0){
        if(data_rx.indexOf("ScrollNotify") == 0){
          return true;
        }
      }
    }
  }
  return false;
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200);
  delay(1000);
  send_command("setScroll 0"); // off scroll
  send_command("setCursor 0"); // off cursor/blink
  send_command("fillScreen 0"); // clear screen
}

void loop() {
  send_command("setCursor 0 0");
  send_command("setTextColor 3"); // set text color is yellow
  send_command("print hello!");
  send_command("setCursor 1 36 7 1"); // on cursor mode
  delay(300);
  for(int8_t i = 5; i >= 0; i--){
    send_command("setCursor " + String(i*6) + " 7");
    delay(300);
  }
  delay(1000);
  send_command("setCursor 2 0 7 1"); // on blink mode
  delay(2000);
  send_command("setCursor 0 0");
  send_command("setTextColor 3 0"); // set text color is yellow and back ground color is no color
  send_command("print H");
  delay(2000);
  send_command("setCursor 0");
  send_command("fillScreen 0");
  for(uint16_t i = 0; i < 48; i++){
    send_command("drawPixel " + String(i) + " 0 2"); // draw pixel with green color
  }
  for(uint16_t i = 0; i < 8; i++){
    send_command("drawPixel 47 " + String(i) + " 2");
  }
  for(int16_t i = 47; i >= 0; i--){
    send_command("drawPixel " + String(i) + " 7 2");
  }
  for(int16_t i = 7; i >= 0; i--){
    send_command("drawPixel 0 " + String(i) + " 2");
  }
  send_command("fillScreen 0");
  delay(100);
  for(uint8_t i = 5; i < 48; i+=7){
    send_command("drawCircle " + String(i) + " 3 3 2"); // draw circle with green color
    delay(100);
  }
  for(uint8_t i = 5; i < 48; i+=7){
    send_command("drawCircle " + String(i) + " 3 3 1"); // draw circle with red color
    delay(100);
  }
  for(uint8_t i = 5; i < 48; i+=7){
    send_command("drawCircle " + String(i) + " 3 3 3"); // draw circle with yellow color
    delay(100);
  }
  send_command("fillScreen 0");
  delay(100);
  for(uint8_t i = 2; i < 48; i+=5){
    send_command("drawRect " + String(i) + " 1 4 6 1"); // draw rectangle with red color
    delay(100);
  }
  send_command("fillScreen 0");
  send_command("drawLine 0 " + String(0) + " 47 " + String(0) + " 3"); // draw line with yellow color
  for(uint8_t i = 1; i < 8; i++){
    send_command("drawLine 0 " + String(i-1) + " 47 " + String(i-1) + " 0"); // draw line with no color
    send_command("drawLine 0 " + String(i) + " 47 " + String(i) + " 3 ");
    delay(100);
  }
  send_command("drawLine 0 " + String(7) + " 47 " + String(7) + " 0 ");
  send_command("drawLine " + String(0) + " 0 " + String(0) + " 7 3 ");
  for(uint8_t i = 1; i < 48; i++){
    send_command("drawLine " + String(i) + " 0 " + String(i) + " 7 3 ");
    send_command("drawLine " + String(i-1) + " 0 " + String(i-1) + " 7 0");
  }
  send_command("drawLine " + String(47) + " 0 " + String(47) + " 7 0 ");
  for(uint8_t i = 0; i < 5; i++){
    for(uint8_t j = 1; j < 4; j++){
      send_command("setCursor 0 0");
      send_command("setTextColor " + String(j));
      send_command("print Hello!");delay(200);
    }
  }
  send_command("fillScreen 0");
  send_command("setCursor 0 0");
  send_command("setTextColor 2");
  send_command("print Hello world!");
  delay(1000);
  send_command("setScroll 1 72 100 1"); // on scroll with delay 100ms
  wait_ScrollNotify(100000);            // wait for complete 1 times scrolling
  send_command("setScroll 0");
  send_command("setCursor 0 0");
  send_command("setTextColor 1");
  send_command("print Hello world!");
  delay(1000);
  send_command("setScroll 1 72 70 1"); // on scroll with delay 70ms
  wait_ScrollNotify(100000);
  send_command("setScroll 0");
  delay(1000);
  send_command("setCursor 0 0");
  send_command("setTextColor 3");
  send_command("print Hello world!");
  delay(1000);
  send_command("setScroll 1 72 50 1"); // on scroll with delay 50ms
  wait_ScrollNotify(100000);
  send_command("setScroll 0");
  delay(1000);
  send_command("fillScreen 0");
  send_command("setCursor 0 0");
  send_command("setTextColor 2");
  send_command("print 8x48 uart display");
  delay(1000);
  send_command("setScroll 1 108 100 1");
  wait_ScrollNotify(100000);
  send_command("setScroll 0");
  delay(1000);
  send_command("fillScreen 0");
  send_command("setCursor 0 0");
  send_command("setTextColor 2");
  send_command("print Elec Channel");
  delay(1000);
  send_command("setScroll 1 78 100 1");
  wait_ScrollNotify(100000);
  send_command("setScroll 0");
  delay(1000);
  send_command("fillScreen 0");
  send_command("clockDisplay"); // change to clock display mode (previous mode is uard display mode)
  delay(5000);
  send_command("clockDisplay 1"); // change clock color to red color
  delay(5000);
  send_command("clockDisplay 2"); // change clock color to green color
  delay(5000);
  send_command("clockDisplay 3"); // change clock color to yellow color
  while(1){
    // the clock color will change randomly for each minute
    delay(100);
  }
}
