#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include <ClickEncoder.h>
#include <TimerOne.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMono9pt7b.h>
#include "OBD2UART.h"
#include "Wire.h" // For I2C

int ID = 0;
int IDt = 0;
int paramID = 0;
int paramIDt = 0;
int page = 1;

COBD obd;

const int8_t BL_PIN = 6;  // LCD backlight pin
const int8_t red_light_pin= 9;
const int8_t green_light_pin = 10;
const int8_t blue_light_pin = 11;
int cnt = 0;

String menuItem[3] = {"Parameters", "Speed Limit", "Light: ON"};

boolean backlight = false;
int speed_limit = 65;

String parameters[] = {"Intk. C", 
                       "Cool. C", 
                       "Amb. C", 
                       "Fuel %",
                       "Load %",
                       "MAF g/s",
                       "Thtl %",
                       "PB kPa"};

const byte pids[] = {PID_SPEED,
                     PID_RPM,
                     PID_INTAKE_TEMP,
                     PID_COOLANT_TEMP,
                     PID_AMBIENT_TEMP,
                     PID_FUEL_LEVEL,
                     PID_ENGINE_LOAD,
                     PID_MAF_FLOW,
                     PID_THROTTLE,
                     PID_BAROMETRIC};

boolean up = false;
boolean down = false;
boolean middle = false;

ClickEncoder *encoder;
int16_t last = 0, value;

int var[sizeof(pids)];   
int Speed = 0, RPM = 0;
int sampling = 1000;
unsigned long time_now = 0;

// Adafruit_PCD8544 display = Adafruit_PCD8544(CLK_PIN, DIN_PIN, DC_PIN, CE_PIN, RST_PIN);
Adafruit_PCD8544 display = Adafruit_PCD8544(3, 4, 5, 8, 7); //Download the latest Adafruit Library in order to use this constructor

void timerIsr() {
  encoder->service();
}

void readRotaryEncoder() {
  
  value += encoder->getValue();
  
  if (value > last) {
    last = value;
    down = true;
    delay(150);
  } 
  else if (value < last) {
    last = value;
    up = true;
    delay(150);
  }
}

void RGB_color(int red_light_value, int green_light_value, int blue_light_value) {
  analogWrite(red_light_pin, red_light_value);
  analogWrite(green_light_pin, green_light_value);
  analogWrite(blue_light_pin, blue_light_value);
}

void Speed_check(int Speed, int speed_limit) {
  if (Speed > speed_limit and Speed <= speed_limit + 5) {
    cnt = 5;
    RGB_color(200,200,0);
  } else if (Speed > speed_limit + 5) {
    cnt++;
    if (cnt%2) {
      RGB_color(200,0,0);
      cnt = 5;
    }else{
      RGB_color(0,0,0);
    }
  } else if (Speed <= speed_limit) {
    if (cnt > 0) {
      RGB_color(0,200,0);
    } else {
      RGB_color(0,0,0);
    }
  }
}

void setup() {

  pinMode(BL_PIN, OUTPUT);
  digitalWrite(BL_PIN,HIGH);
  pinMode(red_light_pin, OUTPUT);
  pinMode(green_light_pin, OUTPUT);
  pinMode(blue_light_pin, OUTPUT);
  
  encoder = new ClickEncoder(A0, A1, A2, 4);
  encoder->setAccelerationEnabled(false);

  display.begin();     
  display.clearDisplay(); 
  display.setRotation(2);
  display.setContrast(50);  

  Timer1.initialize(1000);  // 1 ms timer for interrupt
  Timer1.attachInterrupt(timerIsr); 

  /*  initiate OBD-II connection until success */
  obd.begin();
  while (!obd.init());
}

void loop() {

  readRotaryEncoder();
  ClickEncoder::Button b = encoder->getButton();
  if (b != ClickEncoder::Open) {
   switch (b) {
      case ClickEncoder::Clicked:
        middle=true;
        break;
    }
  }

  if (millis() >= time_now + sampling){
    time_now += sampling;
    if (obd.readPID(pids, sizeof(pids), var) == sizeof(pids)) {
      var[0] = var[0]*0.621371;
      Speed = round(var[0]);
      RPM = round(var[1]);
    }
    if (Speed <= speed_limit){
      cnt--;
    }
  }

  if (middle) { // Middle Button is Pressed
    middle = false;
    if (ID == 0 or ID == 1){
      if (page == 1){
        page = 2;
      } else if (page == 2) {
        page = 1;
        ID = 0;
        IDt = 0;
      }
    } else if (ID == 2 and page == 1) {
      if (backlight) {
        backlight = false;
        menuItem[2] = "Light: OFF";
        digitalWrite(BL_PIN,LOW);
      } else {
        backlight = true; 
        menuItem[2] = "Light: ON";
        digitalWrite(BL_PIN,HIGH);
      }
    }
  }

  switch (page) {
    case 1:
      if (up) {
        up = false;
        IDt--;
      } else if (down){
        down = false;
        IDt++;
      }
      ID = abs(IDt);
      if (ID > sizeof(menuItem)/sizeof(menuItem[0])-1) {
        ID = sizeof(menuItem)/sizeof(menuItem[0])-1;
      }
      break;
    case 2:
      if (ID == 1){
        if (up) {
          up = false;
          speed_limit--;
        } else if (down){
          down = false;
          speed_limit++;
        }
      } else if (ID == 0){
        if (up) {
          up = false;
          paramIDt--;
        } else if (down){
          down = false;
          paramIDt++;
        }
        paramID = abs(paramIDt);
        if (paramID > sizeof(parameters)/sizeof(parameters[0])-1) {
          paramID = sizeof(parameters)/sizeof(parameters[0])-1;
        }
      }
      break;
  }
    
  drawMenu(); 
  Speed_check(Speed, speed_limit);
}
  
void drawMenu() {  
  display.clearDisplay(); 
  display.drawRoundRect(0,0,36,24,3,BLACK);
  display.drawRoundRect(37,0,47,24,3,BLACK);
  display.drawRoundRect(0,25,84,15,3,BLACK);
  display.setTextColor(BLACK, WHITE);
  display.setTextSize(1);
  display.setCursor(2, 2);
  display.print(speed_limit);
  display.setCursor(16, 1);
  display.print("mph");
  display.setCursor(64, 1);
  display.print("rpm");
  
  display.setFont(&FreeMonoBold9pt7b);
  if (Speed >= 0 and Speed < 10) {
    display.setCursor(23, 21); //1 digit
  } else if (Speed >= 10 and Speed < 100) {
    display.setCursor(13, 21); //2 digits
  } else if (Speed >= 100) {
    display.setCursor(2, 21); //3 digits
  }
  display.print(Speed);

  if (RPM >= 0 and RPM < 10) {
    display.setCursor(72, 21); //1 digit
  } else if (RPM >= 10 and RPM < 100) {
    display.setCursor(61, 21); //2 digits
  } else if (RPM >= 100 and RPM < 1000) {
    display.setCursor(50, 21); //3 digits
  } else if (RPM >= 1000) {
    display.setCursor(39, 21); //4 digits
  }
  display.print(RPM);
  
  display.setFont(&FreeMono9pt7b);
  display.setCursor(50, 37);
  display.print(round(var[paramID+2]));
  display.setFont();
  display.setTextSize(1);
  display.setCursor(2,27);
  display.print(parameters[paramID]);
  display.setCursor(0,41);
  if (page == 1) {
    display.print(">"+menuItem[ID]);
  } else if (page == 2) {
    if (ID == 1) {
      display.print(">>Speed limit");
    } else if (ID == 0) {
      display.print(">>OBDII item");
    }
  }
  display.display();
}
