
/* *************************************************************************************
 *
 * Tittle: aqua_controller.ino
 *
 * Description: Applicaiton to control aquarium
 *
 * Version: 8
 * Date:    19/05/2016  10:06:52
 * Author:  Gustavo Celani
 *
 * ************************************************************************************/

/* Libraries */
#include <Adafruit_GFX.h>    //Graphic
#include <Adafruit_TFTLCD.h> //LCD
#include <TouchScreen.h>     //TouchScreen
#include <OneWire.h>         //Temperature
#include <DS1307.h>          //Real Time Control

/* Pins TouchScreen Display */
#define YP A3 //ADC (Analog Digital Converter)
#define XM A2 //ADC (Analog Digital Converter)
#define YM 9
#define XP 8

/* Touch Pressure Setup */
#define MINPRESSURE 1
#define MAXPRESSURE 1000
#define SENSIBILITY 300

/* Touch Margin of Error */
short TS_MINX = 150;
short TS_MINY = 120;
short TS_MAXX = 920;
short TS_MAXY = 940;

/* Touch Screen Inicialization */
TouchScreen ts = TouchScreen(XP, YP, XM, YM, SENSIBILITY);

/* Touch X Y Position */
unsigned int X, Y;
/* Touch Pressure */
unsigned int Z;

/* LCD Pins */
#define LCD_CS    A3   
#define LCD_CD    A2   
#define LCD_WR    A1
#define LCD_RD    A0
#define LCD_RESET A4

/* Colors */
#define BLACK   0x0000
#define BLUE    0x001F  
#define RED     0xF800  
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

/* LCD Inicializations */
Adafruit_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);

/* Frames Control */
unsigned int frame;
bool transition = false; //FALSE => Frame Ok //TRUE => Wait frame transition
#define BLOCKED    0
#define UNBLOCKED  1

/* Theme Colors */
unsigned int BACKGROUND         = BLACK; //Cor do background
unsigned int TEXTCOLOR          = WHITE; //Cor do texto
unsigned int FIGCOLOR           = BLUE;  //Cor das figuras gráficas
unsigned int SELECTIONFIGCOLOR  = WHITE; //Cor da figura em seleção
unsigned int SELECTIONTEXTCOLOR = BLUE;  //Cor do texto em seleção

/* Hardware Control Variables */
#define COLORON  GREEN;
#define COLOROFF RED;
unsigned int STATUSCOLOR; //Indications of ON/OFF on manual_configuration_frame
bool light = false;       //FALSE => Off //TRUE => On
bool cooler = false;      //FALSE => Off //TRUE => On
bool heater = false;      //FALSE => Off //TRUE => On

/* Control and Temporization of Open Cover */
unsigned int row_time_cover = 3; //Time Open Cover
char times_cover[12][3] {
  {'0','0','0'}, {'3','0','s'}, {'0','1','m'}, {'0','3','m'}, {'0','5','m'}, {'1','0','m'}, 
  {'1','5','m'}, {'2','0','m'}, {'3','0','m'}, {'4','5','m'}, {'0','1','h'}, {'0','2','h'}
};

/* Control and Temporization of Feeding */
unsigned int row_time_food = 8; //Time beetween feeding
bool check_food = false; //Inicialization without feed
char times_food[12][3] {
  {'0','0','0'}, {'0','2','h'}, {'0','3','h'}, {'0','5','h'}, {'0','8','h'}, {'1','0','h'},
  {'1','2','h'}, {'1','6','h'}, {'0','1','d'}, {'3','2','h'}, {'4','0','h'}, {'0','2','d'}
};

/* Control and Temporizatino of Filter Maintenance */
unsigned int row_time_filter = 4; //Time beetween Filter Maintenance
bool check_filter = false; //Inicialization without maintenance
char times_filter[12][3] {
  {'0','0','0'}, {'0','2','d'}, {'0','5','d'}, {'0','1','s'}, {'1','0','d'}, {'1','2','d'},
  {'0','2','s'}, {'1','5','d'}, {'2','0','d'}, {'0','3','s'}, {'2','5','d'}, {'0','1','m'}
};

/*Control and Temporization of Lighting */
unsigned int row_time_light = 4; //Time beetween Lighting
bool check_light = false; //Inicialization without Lighing
bool automatic_light = true; //Automatic Light
char times_light[12][3] {
  {'0','0','0'}, {'0','6','h'}, {'0','8','h'}, {'1','0','h'}, {'1','2','h'}, {'1','4','h'},
  {'1','4','h'}, {'1','6','h'}, {'1','8','h'}, {'2','0','h'}, {'2','2','d'}, {'0','1','d'}
};

/* Temperature Control */
#define TEMPERATURE_SENSOR_PIN 51
#define TEMPERATURE_MARGIN 0.2 //Termperature Margin Ok
OneWire temperature_sensor(TEMPERATURE_SENSOR_PIN);
byte present = 0;
byte type_s;
byte data[12];
byte addr[8];
float desired_temperature = 27.5;
float current_temperature = -1;
bool measured_temperature;

/* Rele Control */
#define HEATER_PIN  46 //Low-Active
#define COOLERS_PIN 48 //Low-Active
#define LIGHT_PIN_1 50 //Low-Active
#define LIGHT_PIN_2 52 //Low-Active

/* RTC */
#define SDA A14
#define SCL A15

DS1307 RTC(SDA, SCL);
Time current_time;     //Current Time Always Read
Time aux_time;         //Time to Comparations
Time temperature_time;
Time display_time;
Time cover_time;
Time food_time;
Time filter_time;
Time light_time;
bool flag_time_read;

/* Time Lighting Control */
Time time_light_on;
Time time_light_off;

/* Cover Buttom */
#define BUTTOM_PIN 42
bool COVER; //TRUE => Open //FALSE => Closed
bool cover_read = true;

/* Buzzer */
#define BUZZER_PIN 40

/*--------------------------------------------------------------------------------------*/
/*                                   Auxiliar Functions                                 */
/*--------------------------------------------------------------------------------------*/

void read_touch(void) {
  
  TSPoint p = ts.getPoint();
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);

  /* Screen (320x240) */
  X = map(p.x, TS_MINX, TS_MAXX, 0, tft.width());
  Y = map(p.y, TS_MINY, TS_MAXY, 0, tft.height());
  Z = p.z;
}

void read_temperature(void) {
  if (!temperature_sensor.search(addr)) {
    temperature_sensor.reset_search();
    delay(250);
    return;
  }

  /* Sensor Type */
  switch (addr[0]) {
    case 0x10:
      type_s = 1; break;
    case 0x28:
      type_s = 0; break;
    case 0x22:
      type_s = 0; break;
    default: return;
  } 

  temperature_sensor.reset();
  temperature_sensor.select(addr);
  temperature_sensor.write(0x44, 1);
  delay(750);
  present = temperature_sensor.reset();
  temperature_sensor.select(addr);    
  temperature_sensor.write(0xBE);
  
  for (int i = 0; i < 9; i++) data[i] = temperature_sensor.read();

  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3;
    if (data[7] == 0x10) raw = (raw & 0xFFF0) + 12 - data[6];
  } else {
    byte cfg = (data[4] & 0x60);
    if (cfg == 0x00) raw = raw & ~7;
    else if (cfg == 0x20) raw = raw & ~3;
    else if (cfg == 0x40) raw = raw & ~1;
  }
  current_temperature = (float) raw / 16.0;
}


void light_time_init() {
  time_light_on.hour = 9;
  time_light_on.min = 0;
  time_light_on.sec = 0;

  time_light_off.hour = 21;
  time_light_off.min = 0;
  time_light_off.sec = 0;
}

void select_buzzer() {
  tone(BUZZER_PIN, 1500);
  delay(200);
  noTone(BUZZER_PIN);
}

void alert_buzzer() {
  tone(BUZZER_PIN, 1500);
  delay(200);
  noTone(BUZZER_PIN);
  delay(25);
  tone(BUZZER_PIN, 1500);
  delay(200);
  noTone(BUZZER_PIN);
  delay(1000);
  tone(BUZZER_PIN, 1500);
  delay(200);
  noTone(BUZZER_PIN);
  delay(25);
  tone(BUZZER_PIN, 1500);
  delay(200);
  noTone(BUZZER_PIN);
  delay(1000);
  tone(BUZZER_PIN, 1500);
  delay(200);
  noTone(BUZZER_PIN);
  delay(25);
  tone(BUZZER_PIN, 1500);
  delay(200);
  noTone(BUZZER_PIN);
}

void initialization_buzzer() {
  tone(BUZZER_PIN, 1500);
  delay(600);
  noTone(BUZZER_PIN);
  delay(300);
  tone(BUZZER_PIN, 1500);
  delay(200);
  noTone(BUZZER_PIN);
  delay(25);
  tone(BUZZER_PIN, 1500);
  delay(200);
  noTone(BUZZER_PIN);
}


void check_cover() { //TRUE => Open //FALSE => False
  if(digitalRead(BUTTOM_PIN) == LOW) COVER = true;
  else {
    COVER = false;
    cover_read = false;
  }
}

/*--------------------------------------------------------------------------------------*/
/*                                      Time Functions                                  */
/*--------------------------------------------------------------------------------------*/

bool time_compare(Time t1, Time t2) { //TRUE => t1>=t2 //FALSE => t2>t1
  if((int)t1.year > (int)t2.year) return true;
  else if ((int)t1.year < (int)t2.year) return false;
  else {
    if((int)t1.mon  > (int)t2.mon) return true;
    else if((int)t1.mon  < (int)t2.mon) return false;
    else {
      if((int)t1.date  > (int)t2.date) return true;
      else if((int)t1.date  < (int)t2.date) return false;
      else {
        if((int)t1.hour  > (int)t2.hour) return true;
        else if((int)t1.hour  < (int)t2.hour) return false;
        else {
          if((int)t1.min  > (int)t2.min) return true;
          else if((int)t1.min  < (int)t2.min) return false;
          else {
            if((int)t1.sec  > (int)t2.sec) return true;
            else if((int)t1.sec  < (int)t2.sec) return false;
            else return true;
          }
        }
      }
    }
  }
}


bool equal_times(Time t1, Time t2) { //TRUE => Equals //FALSE => Different
  if((int)t1.hour == (int)t2.hour && (int)t1.min == (int)t2.min && (int)t1.sec == (int)t2.sec) return true;
  else return false;
}


Time sum_seconds(Time t, int x) { //Sum X(0 a 59) SECONDSS on Time t
  int year = (int)t.year;
  int month = (int)t.mon;
  int day = (int)t.date;
  int hour = (int)t.hour;
  int minute = (int)t.min;
  int second = (int)t.sec;

  if( (x + (int)t.sec) < 60) second = (int)t.sec + x;
  else {
    second = (x + (int)t.sec) - 60;
    if( (int)t.min == 59) {
      minute = 0;
      hour = (int)t.hour + 1;
    } else minute = (int)t.min + 1;
  }

  t.year = year;
  t.mon = month;
  t.date = day;
  t.hour = hour;
  t.min = minute;
  t.sec = second;
  return t;
}

Time sum_minuts(Time t, int x) { //Sum X (0 a 59) MINUTS in Time t
  int year = (int)t.year;
  int month = (int)t.mon;
  int day = (int)t.date;
  int hour = (int)t.hour;
  int minute = (int)t.min;
  int second = (int)t.sec;

  if( (x + (int)t.min) < 60) minute = (int)t.min + x;
  else {
    minute = (x + (int)t.min) - 60;
    if( (int)t.hour == 23) {
      hour = 0;
      day = (int)t.date + 1;
    } else hour = (int)t.hour + 1;
  }

  t.year = year;
  t.mon = month;
  t.date = day;
  t.hour = hour;
  t.min = minute;
  t.sec = second;
  return t;
}

Time sum_hours(Time t, int x) { //Sum X (0 a 23) HOURS in Time t
  int year = (int)t.year;
  int month = (int)t.mon;
  int day = (int)t.date;
  int hour = (int)t.hour;
  int minute = (int)t.min;
  int second = (int)t.sec;

  if( (x + (int)t.hour) < 23) hour = (int)t.hour + x;
  else {
    hour = (x + (int)t.hour) - 24;
    if( (int)t.date == 30) {
      day = 1;
      if( (int)t.mon != 12) month = (int)t.mon + 1;
      else month = 1;
    } else day = (int)t.date + 1;
  }

  t.year = year;
  t.mon = month;
  t.date = day;
  t.hour = hour;
  t.min = minute;
  t.sec = second;
  return t;
}

Time sum_days(Time t, int x) { //Sum X (0 a 29) DAYS in Time t
  int year = (int)t.year;
  int month = (int)t.mon;
  int day = (int)t.date;
  int hour = (int)t.hour;
  int minute = (int)t.min;
  int second = (int)t.sec;

  if( (x + (int)t.date) < 29) day = (int)t.date + x;
  else {
    day = (x + (int)t.date) - 30;
    if( (int)t.mon == 12) {
      month = 1;
      year = (int)t.year + 1;
    } else month = (int)t.mon + 1;
  }

  t.year = year;
  t.mon = month;
  t.date = day;
  t.hour = hour;
  t.min = minute;
  t.sec = second;
  return t;
}

/*--------------------------------------------------------------------------------------*/
/*                                      Alerts Function                                 */
/*--------------------------------------------------------------------------------------*/

void open_cover_alert() { //Open Cover Alert

  tft.fillScreen(BACKGROUND);
  tft.setTextSize(4);
  tft.setTextColor(TEXTCOLOR);
  tft.fillRect(0, 10, 240, 60, RED);
  tft.setCursor(30, 25);
  tft.print("ALERTA !");
  tft.fillRect(0, 90, 240, 140, RED);
  tft.setCursor(55, 120);
  tft.print("COVER");
  tft.setCursor(45, 170);
  tft.print("ABERTA");

  /* Ok Buttom */
  tft.fillRoundRect(15, 240, 210, 60, 15, FIGCOLOR);
  tft.setTextSize(3);
  tft.setTextColor(TEXTCOLOR);
  tft.setCursor(100, 260);
  tft.println("OK");

  alert_buzzer();
  
  while(true) {

    check_cover();
    
    if(COVER == false) {
      flag_time_read = false;
      measured_temperature = false;
      cover_read = false;
      transition = true;
      blocked_frame();
    }

    read_touch();
    
    if((X>15 && X<225) && (Y>10 && Y<70) && (Z>MINPRESSURE && Z<MAXPRESSURE)) { //OK
      select_buzzer();
      tft.fillRoundRect(15, 240, 210, 0, 15, SELECTIONFIGCOLOR);
      tft.setTextSize(3);
      tft.setTextColor(SELECTIONTEXTCOLOR);
      tft.setCursor(90, 275);
      tft.println("OK");

      transition = true;
      flag_time_read = false;
      measured_temperature = false;
      cover_read = false;
      blocked_frame();
    }
  }
}

void feeding_alert() { //Feeding Alert

  tft.fillScreen(BACKGROUND);
  tft.setTextSize(4);
  tft.setTextColor(TEXTCOLOR);
  tft.fillRect(0, 10, 240, 60, RED);
  tft.setCursor(30, 25);
  tft.print("ALERTA !");
  tft.fillRect(0, 90, 240, 140, RED);
  tft.setTextSize(3);
  tft.setCursor(50, 120);
  tft.print("HORARIO");
  tft.setCursor(15, 170);
  tft.print("ALIMENTACAO");

  /* Ok Buttom */
  tft.fillRoundRect(15, 240, 210, 60, 15, FIGCOLOR);
  tft.setTextSize(3);
  tft.setTextColor(TEXTCOLOR);
  tft.setCursor(100, 260);
  tft.println("OK");

  alert_buzzer();
  
  while(true) {

    read_touch();
    
    if((X>15 && X<225) && (Y>10 && Y<70) && (Z>MINPRESSURE && Z<MAXPRESSURE)) { //OK
      select_buzzer();
      tft.fillRoundRect(15, 240, 210, 0, 15, SELECTIONFIGCOLOR);
      tft.setTextSize(3);
      tft.setTextColor(SELECTIONTEXTCOLOR);
      tft.setCursor(90, 275);
      tft.println("OK");

      transition = true;
      flag_time_read = false;
      measured_temperature = false;
      cover_read = false;
      blocked_frame();
    }
  }
}

void filter_alert() { //Filter Maintenance Alert

  tft.fillScreen(BACKGROUND);
  tft.setTextSize(4);
  tft.setTextColor(TEXTCOLOR);
  tft.fillRect(0, 10, 240, 60, RED);
  tft.setCursor(30, 25);
  tft.print("ALERTA !");
  tft.fillRect(0, 90, 240, 140, RED);
  tft.setCursor(25, 120);
  tft.print("MANUTENCAO");
  tft.setCursor(45, 170);
  tft.print("FILTROS");

  /* Ok Buttom */
  tft.fillRoundRect(15, 240, 210, 60, 15, FIGCOLOR);
  tft.setTextSize(3);
  tft.setTextColor(TEXTCOLOR);
  tft.setCursor(100, 260);
  tft.println("OK");

  alert_buzzer();
  
  while(true) {

    read_touch();
    
    if((X>15 && X<225) && (Y>10 && Y<70) && (Z>MINPRESSURE && Z<MAXPRESSURE)) { //OK
      select_buzzer();
      tft.fillRoundRect(15, 240, 210, 0, 15, SELECTIONFIGCOLOR);
      tft.setTextSize(3);
      tft.setTextColor(SELECTIONTEXTCOLOR);
      tft.setCursor(90, 275);
      tft.println("OK");

      transition = true;
      flag_time_read = false;
      measured_temperature = false;
      cover_read = false;
      blocked_frame();
    }
  }
}

/*--------------------------------------------------------------------------------------*/
/*                                       Timers Frame                                   */
/*--------------------------------------------------------------------------------------*/

void timers_frame(void) {
  if(transition == true) {
    tft.fillScreen(BACKGROUND);
    
    /* Cover Buttom */
    tft.fillRoundRect(15, 20, 210, 60, 15, FIGCOLOR);
    tft.setTextSize(3);
    tft.setTextColor(TEXTCOLOR);
    tft.setCursor(70, 40);
    tft.println("COVER");

    /* Feed Buttom */
    tft.fillRoundRect(15, 100, 210, 60, 15, FIGCOLOR);
    tft.setTextSize(3);
    tft.setTextColor(TEXTCOLOR);
    tft.setCursor(25, 120);
    tft.println("ALIMENTACAO");

    /* Filter Buttom */
    tft.fillRoundRect(15, 180, 210, 60, 15, FIGCOLOR);
    tft.setTextSize(3);
    tft.setTextColor(TEXTCOLOR);
    tft.setCursor(60, 200);
    tft.println("FILTROS");

    /*Back Buttom */
    tft.fillRoundRect(15, 260, 210, 55, 15, FIGCOLOR);
    tft.setTextSize(3);
    tft.setTextColor(TEXTCOLOR);
    tft.setCursor(70, 275);
    tft.println("VOLTAR");

    transition = false;
  }

  while(true) {
    read_touch();
    if((X>15 && X<225) && (Y>230 && Y<360) && (Z>MINPRESSURE && Z<MAXPRESSURE)) { //Cover
      select_buzzer();
      tft.fillRoundRect(15, 20, 210, 60, 15, SELECTIONFIGCOLOR);
      tft.setTextSize(3);
      tft.setTextColor(SELECTIONTEXTCOLOR);
      tft.setCursor(70, 40);
      tft.println("TAMPA");

      transition = true;
      cover_frame();
    }
    if((X>15 && X<225) && (Y>160 && Y<220) && (Z>MINPRESSURE && Z<MAXPRESSURE)) { //Feed
      select_buzzer();
      tft.fillRoundRect(15, 100, 210, 60, 15, SELECTIONFIGCOLOR);
      tft.setTextSize(3);
      tft.setTextColor(SELECTIONTEXTCOLOR);
      tft.setCursor(25, 120);
      tft.println("ALIMENTACAO");

      transition = true;
      feeding_frame();
    }
    if((X>15 && X<225) && (Y>80 && Y<150) && (Z>MINPRESSURE && Z<MAXPRESSURE)) { //Filter
      select_buzzer();
      tft.fillRoundRect(15, 180, 210, 60, 15, SELECTIONFIGCOLOR);
      tft.setTextSize(3);
      tft.setTextColor(SELECTIONTEXTCOLOR);
      tft.setCursor(60, 200);
      tft.println("FILTROS");

      transition = true;
      filter_frame();
    }
    if((X>15 && X<225) && (Y>10 && Y<70) && (Z>MINPRESSURE && Z<MAXPRESSURE)) { //Back
      select_buzzer();
      tft.fillRoundRect(15, 260, 210, 55, 15, SELECTIONFIGCOLOR);
      tft.setTextSize(3);
      tft.setTextColor(SELECTIONTEXTCOLOR);
      tft.setCursor(70, 275);
      tft.println("VOLTAR");

      transition = true;
      automatic_configuration_frame();
    }
  }
}

/*--------------------------------------------------------------------------------------*/
/*                                        Cover Frame                                   */
/*--------------------------------------------------------------------------------------*/

void cover_frame(void) {
  if(transition == true) {
    tft.fillScreen(BACKGROUND);

    /* Tittle */
    tft.fillRect(0, 20, 240, 100, FIGCOLOR);
    tft.setTextSize(3);
    tft.setTextColor(TEXTCOLOR);
    tft.setCursor(40, 40);
    tft.println("ALERTA DE");
    tft.setCursor(15, 80);
    tft.println("TAMPA ABERTA");

    /* Time */
    tft.setTextSize(5);
    tft.setTextColor(TEXTCOLOR);
    tft.setCursor(80, 175);
    for(int i=0; i<=2; i++) tft.print(times_cover[row_time_cover][i]);

    /* - Buttom */
    tft.fillRoundRect(10, 170, 50, 50, 15, FIGCOLOR);
    tft.setTextSize(3);
    tft.setTextColor(TEXTCOLOR);
    tft.setCursor(20, 185);
    tft.println("<<");

    /* + Buttom */
    tft.fillRoundRect(180, 170, 50, 50, 15, FIGCOLOR);
    tft.setTextSize(3);
    tft.setTextColor(TEXTCOLOR);
    tft.setCursor(190, 185);
    tft.println(">>");

    /* Back Buttom */
    tft.fillRoundRect(15, 260, 210, 55, 15, FIGCOLOR);
    tft.setTextSize(3);
    tft.setTextColor(TEXTCOLOR);
    tft.setCursor(70, 275);
    tft.println("VOLTAR");

    transition = false;
  }

  while(true) {
    read_touch();
    
    if((X>180 && X<230) && (Y>70 && Y<140) && (Z>MINPRESSURE && Z<MAXPRESSURE)) { //Minus
      select_buzzer();
      tft.fillRoundRect(10, 170, 50, 50, 15, SELECTIONFIGCOLOR);
      tft.setTextSize(3);
      tft.setTextColor(SELECTIONTEXTCOLOR);
      tft.setCursor(20, 185);
      tft.println("<<");

      if(row_time_cover > 1) {
        tft.fillRect(65, 125, 110, 120, BACKGROUND);
        tft.setTextSize(5);
        tft.setTextColor(TEXTCOLOR);
        tft.setCursor(80, 175);
        row_time_cover--;
        for(int i=0; i<=2; i++) tft.print(times_cover[row_time_cover][i]);
      }
      
      tft.fillRoundRect(10, 170, 50, 50, 15, FIGCOLOR);
      tft.setTextSize(3);
      tft.setTextColor(TEXTCOLOR);
      tft.setCursor(20, 185);
      tft.println("<<");
    }
    if((X>10 && X<60) && (Y>70 && Y<140) && (Z>MINPRESSURE && Z<MAXPRESSURE)) { //Plus
      select_buzzer();
      tft.fillRoundRect(180, 170, 50, 50, 15, SELECTIONFIGCOLOR);
      tft.setTextSize(3);
      tft.setTextColor(SELECTIONTEXTCOLOR);
      tft.setCursor(190, 185);
      tft.println(">>");

      if(row_time_cover < 11) {
        tft.fillRect(65, 125, 110, 120, BACKGROUND);
        tft.setTextSize(5);
        tft.setTextColor(TEXTCOLOR);
        tft.setCursor(80, 175);
        row_time_cover++;
        for(int i=0; i<=2; i++) tft.print(times_cover[row_time_cover][i]);
      }

      tft.fillRoundRect(180, 170, 50, 50, 15, FIGCOLOR);
      tft.setTextSize(3);
      tft.setTextColor(TEXTCOLOR);
      tft.setCursor(190, 185);
      tft.println(">>");
    }
    if((X>15 && X<210) && (Y>10 && Y<70) && (Z>MINPRESSURE && Z<MAXPRESSURE)) { //Back
      select_buzzer();
      tft.fillRoundRect(15, 260, 210, 55, 15, SELECTIONFIGCOLOR);
      tft.setTextSize(3);
      tft.setTextColor(SELECTIONTEXTCOLOR);
      tft.setCursor(70, 275);
      tft.println("VOLTAR");

      transition = true;
      timers_frame();
    }
  }
}

/*--------------------------------------------------------------------------------------*/
/*                                         Feed Frame                                   */
/*--------------------------------------------------------------------------------------*/

void feeding_frame(void) {
  if(transition == true) {
    tft.fillScreen(BACKGROUND);

    /* Tittle */
    tft.fillRect(0, 20, 240, 100, FIGCOLOR);
    tft.setTextSize(3);
    tft.setTextColor(TEXTCOLOR);
    tft.setCursor(10, 40);
    tft.println("INTERVALO DE");
    tft.setCursor(15, 80);
    tft.println("ALIMENTACAO");

    /* Time */
    tft.setTextSize(5);
    tft.setTextColor(TEXTCOLOR);
    tft.setCursor(80, 175);
    for(int i=0; i<=2; i++) tft.print(times_food[row_time_food][i]);

    /* - Buttom */
    tft.fillRoundRect(10, 170, 50, 50, 15, FIGCOLOR);
    tft.setTextSize(3);
    tft.setTextColor(TEXTCOLOR);
    tft.setCursor(20, 185);
    tft.println("<<");

    /* + Buttom */
    tft.fillRoundRect(180, 170, 50, 50, 15, FIGCOLOR);
    tft.setTextSize(3);
    tft.setTextColor(TEXTCOLOR);
    tft.setCursor(190, 185);
    tft.println(">>");

    /* Back Buttom */
    tft.fillRoundRect(15, 260, 210, 55, 15, FIGCOLOR);
    tft.setTextSize(3);
    tft.setTextColor(TEXTCOLOR);
    tft.setCursor(70, 275);
    tft.println("VOLTAR");

    transition = false;
  }

  while(true) {
    read_touch();
    
    if((X>180 && X<230) && (Y>70 && Y<140) && (Z>MINPRESSURE && Z<MAXPRESSURE)) { //Minus
      select_buzzer();
      tft.fillRoundRect(10, 170, 50, 50, 15, SELECTIONFIGCOLOR);
      tft.setTextSize(3);
      tft.setTextColor(SELECTIONTEXTCOLOR);
      tft.setCursor(20, 185);
      tft.println("<<");

      if(row_time_food > 1) {
        tft.fillRect(65, 125, 110, 120, BACKGROUND);
        tft.setTextSize(5);
        tft.setTextColor(TEXTCOLOR);
        tft.setCursor(80, 175);
        row_time_food--;
        for(int i=0; i<=2; i++) tft.print(times_food[row_time_food][i]);
      }
      
      tft.fillRoundRect(10, 170, 50, 50, 15, FIGCOLOR);
      tft.setTextSize(3);
      tft.setTextColor(TEXTCOLOR);
      tft.setCursor(20, 185);
      tft.println("<<");
    }
    if((X>10 && X<60) && (Y>70 && Y<140) && (Z>MINPRESSURE && Z<MAXPRESSURE)) { //Plus
      select_buzzer();
      tft.fillRoundRect(180, 170, 50, 50, 15, SELECTIONFIGCOLOR);
      tft.setTextSize(3);
      tft.setTextColor(SELECTIONTEXTCOLOR);
      tft.setCursor(190, 185);
      tft.println(">>");

      if(row_time_food < 11) {
        tft.fillRect(65, 125, 110, 120, BACKGROUND);
        tft.setTextSize(5);
        tft.setTextColor(TEXTCOLOR);
        tft.setCursor(80, 175);
        row_time_food++;
        for(int i=0; i<=2; i++) tft.print(times_food[row_time_food][i]);
      }

      tft.fillRoundRect(180, 170, 50, 50, 15, FIGCOLOR);
      tft.setTextSize(3);
      tft.setTextColor(TEXTCOLOR);
      tft.setCursor(190, 185);
      tft.println(">>");
    }
    if((X>15 && X<210) && (Y>10 && Y<70) && (Z>MINPRESSURE && Z<MAXPRESSURE)) { //Back
      select_buzzer();
      tft.fillRoundRect(15, 260, 210, 55, 15, SELECTIONFIGCOLOR);
      tft.setTextSize(3);
      tft.setTextColor(SELECTIONTEXTCOLOR);
      tft.setCursor(70, 275);
      tft.println("VOLTAR");
      
      current_time = RTC.getTime();
      if(row_time_food == 1) food_time = sum_hours(current_time, 2);
      if(row_time_food == 2) food_time = sum_hours(current_time, 3);
      if(row_time_food == 3) food_time = sum_hours(current_time, 5);
      if(row_time_food == 4) food_time = sum_hours(current_time, 8);
      if(row_time_food == 5) food_time = sum_hours(current_time, 10);
      if(row_time_food == 6) food_time = sum_hours(current_time, 12);
      if(row_time_food == 7) food_time = sum_hours(current_time, 16);
      if(row_time_food == 8) food_time = sum_days(current_time, 1);
      if(row_time_food == 9) {
        food_time = sum_days(current_time, 1);
        food_time = sum_hours(food_time, 8);
      }
      if(row_time_food == 10) {
        food_time = sum_days(current_time, 1);
        food_time = sum_hours(food_time, 16);
      }
      if(row_time_food == 11) food_time = sum_days(current_time, 2);

      check_food = true;
      transition = true;
      timers_frame();
    }
  }
}

/*--------------------------------------------------------------------------------------*/
/*                                       Filter Frame                                   */
/*--------------------------------------------------------------------------------------*/

void filter_frame(void) {
  if(transition == true) {
    tft.fillScreen(BACKGROUND);

    /* Tittle */
    tft.fillRect(0, 20, 240, 100, FIGCOLOR);
    tft.setTextSize(3);
    tft.setTextColor(TEXTCOLOR);
    tft.setCursor(30, 40);
    tft.println("MANUTENCAO");
    tft.setCursor(30, 80);
    tft.println("DOS FILTROS");

    /* Time */
    tft.setTextSize(5);
    tft.setTextColor(TEXTCOLOR);
    tft.setCursor(80, 175);
    for(int i=0; i<=2; i++) tft.print(times_filter[row_time_filter][i]);

    /* - Buttom */
    tft.fillRoundRect(10, 170, 50, 50, 15, FIGCOLOR);
    tft.setTextSize(3);
    tft.setTextColor(TEXTCOLOR);
    tft.setCursor(20, 185);
    tft.println("<<");

    /* + Buttom */
    tft.fillRoundRect(180, 170, 50, 50, 15, FIGCOLOR);
    tft.setTextSize(3);
    tft.setTextColor(TEXTCOLOR);
    tft.setCursor(190, 185);
    tft.println(">>");

    /* Back Buttom */
    tft.fillRoundRect(15, 260, 210, 55, 15, FIGCOLOR);
    tft.setTextSize(3);
    tft.setTextColor(TEXTCOLOR);
    tft.setCursor(70, 275);
    tft.println("VOLTAR");

    transition = false;
  }

  while(true) {
    read_touch();
    
    if((X>180 && X<230) && (Y>70 && Y<140) && (Z>MINPRESSURE && Z<MAXPRESSURE)) { //Minus
      select_buzzer();
      tft.fillRoundRect(10, 170, 50, 50, 15, SELECTIONFIGCOLOR);
      tft.setTextSize(3);
      tft.setTextColor(SELECTIONTEXTCOLOR);
      tft.setCursor(20, 185);
      tft.println("<<");

      if(row_time_filter > 1) {
        tft.fillRect(65, 125, 110, 120, BACKGROUND);
        tft.setTextSize(5);
        tft.setTextColor(TEXTCOLOR);
        tft.setCursor(80, 175);
        row_time_filter--;
        for(int i=0; i<=2; i++) tft.print(times_filter[row_time_filter][i]);
      }
      
      tft.fillRoundRect(10, 170, 50, 50, 15, FIGCOLOR);
      tft.setTextSize(3);
      tft.setTextColor(TEXTCOLOR);
      tft.setCursor(20, 185);
      tft.println("<<");
    }
    if((X>10 && X<60) && (Y>70 && Y<140) && (Z>MINPRESSURE && Z<MAXPRESSURE)) { //Plus
      select_buzzer();
      tft.fillRoundRect(180, 170, 50, 50, 15, SELECTIONFIGCOLOR);
      tft.setTextSize(3);
      tft.setTextColor(SELECTIONTEXTCOLOR);
      tft.setCursor(190, 185);
      tft.println(">>");

      if(row_time_filter < 11) {
        tft.fillRect(65, 125, 110, 120, BACKGROUND);
        tft.setTextSize(5);
        tft.setTextColor(TEXTCOLOR);
        tft.setCursor(80, 175);
        row_time_filter++;
        for(int i=0; i<=2; i++) tft.print(times_filter[row_time_filter][i]);
      }

      tft.fillRoundRect(180, 170, 50, 50, 15, FIGCOLOR);
      tft.setTextSize(3);
      tft.setTextColor(TEXTCOLOR);
      tft.setCursor(190, 185);
      tft.println(">>");
    }
    if((X>15 && X<210) && (Y>10 && Y<70) && (Z>MINPRESSURE && Z<MAXPRESSURE)) { //Back
      select_buzzer();
      tft.fillRoundRect(15, 260, 210, 55, 15, SELECTIONFIGCOLOR);
      tft.setTextSize(3);
      tft.setTextColor(SELECTIONTEXTCOLOR);
      tft.setCursor(70, 275);
      tft.println("VOLTAR");

      current_time = RTC.getTime();
      if(row_time_filter == 1) filter_time = sum_days(current_time, 2);
      if(row_time_filter == 2) filter_time = sum_days(current_time, 5);
      if(row_time_filter == 3) filter_time = sum_days(current_time, 7);
      if(row_time_filter == 4) filter_time = sum_days(current_time, 10);
      if(row_time_filter == 5) filter_time = sum_days(current_time, 12);
      if(row_time_filter == 6) filter_time = sum_days(current_time, 14);
      if(row_time_filter == 7) filter_time = sum_days(current_time, 15);
      if(row_time_filter == 8) filter_time = sum_days(current_time, 20);
      if(row_time_filter == 9) filter_time = sum_days(current_time, 21);
      if(row_time_filter == 10) filter_time = sum_days(current_time, 25);
      if(row_time_filter == 11) filter_time = sum_days(current_time, 29);

      check_filter = true;
      transition = true;
      timers_frame();
    }
  }
}

/*--------------------------------------------------------------------------------------*/
/*                                        Light Frame                                   */
/*--------------------------------------------------------------------------------------*/

void light_frame(void) {
  if(transition == true) {
    tft.fillScreen(BACKGROUND);

    /* Tittle */
    tft.fillRect(0, 20, 240, 100, FIGCOLOR);
    tft.setTextSize(3);
    tft.setTextColor(TEXTCOLOR);
    tft.setCursor(30, 40);
    tft.println("TEMPO DE");
    tft.setCursor(30, 80);
    tft.println("ILUMINACAO");

    /* Time */
    tft.setTextSize(5);
    tft.setTextColor(TEXTCOLOR);
    tft.setCursor(80, 175);
    for(int i=0; i<=2; i++) tft.print(times_light[row_time_light][i]);

    /* - Buttom */
    tft.fillRoundRect(10, 170, 50, 50, 15, FIGCOLOR);
    tft.setTextSize(3);
    tft.setTextColor(TEXTCOLOR);
    tft.setCursor(20, 185);
    tft.println("<<");

    /* + Buttom */
    tft.fillRoundRect(180, 170, 50, 50, 15, FIGCOLOR);
    tft.setTextSize(3);
    tft.setTextColor(TEXTCOLOR);
    tft.setCursor(190, 185);
    tft.println(">>");

    /* Back Buttom */
    tft.fillRoundRect(15, 260, 210, 55, 15, FIGCOLOR);
    tft.setTextSize(3);
    tft.setTextColor(TEXTCOLOR);
    tft.setCursor(70, 275);
    tft.println("VOLTAR");

    transition = false;
  }

  while(true) {
    read_touch();
    
    if((X>180 && X<230) && (Y>70 && Y<140) && (Z>MINPRESSURE && Z<MAXPRESSURE)) { //Minus
      select_buzzer();
      tft.fillRoundRect(10, 170, 50, 50, 15, SELECTIONFIGCOLOR);
      tft.setTextSize(3);
      tft.setTextColor(SELECTIONTEXTCOLOR);
      tft.setCursor(20, 185);
      tft.println("<<");

      if(row_time_light > 1) {
        tft.fillRect(65, 125, 110, 120, BACKGROUND);
        tft.setTextSize(5);
        tft.setTextColor(TEXTCOLOR);
        tft.setCursor(80, 175);
        row_time_light--;
        for(int i=0; i<=2; i++) tft.print(times_light[row_time_light][i]);
      }
      
      tft.fillRoundRect(10, 170, 50, 50, 15, FIGCOLOR);
      tft.setTextSize(3);
      tft.setTextColor(TEXTCOLOR);
      tft.setCursor(20, 185);
      tft.println("<<");
    }
    if((X>10 && X<60) && (Y>70 && Y<140) && (Z>MINPRESSURE && Z<MAXPRESSURE)) { //Plus
      select_buzzer();
      tft.fillRoundRect(180, 170, 50, 50, 15, SELECTIONFIGCOLOR);
      tft.setTextSize(3);
      tft.setTextColor(SELECTIONTEXTCOLOR);
      tft.setCursor(190, 185);
      tft.println(">>");

      if(row_time_light < 11) {
        tft.fillRect(65, 125, 110, 120, BACKGROUND);
        tft.setTextSize(5);
        tft.setTextColor(TEXTCOLOR);
        tft.setCursor(80, 175);
        row_time_light++;
        for(int i=0; i<=2; i++) tft.print(times_light[row_time_light][i]);
      }

      tft.fillRoundRect(180, 170, 50, 50, 15, FIGCOLOR);
      tft.setTextSize(3);
      tft.setTextColor(TEXTCOLOR);
      tft.setCursor(190, 185);
      tft.println(">>");
    }
    if((X>15 && X<210) && (Y>10 && Y<70) && (Z>MINPRESSURE && Z<MAXPRESSURE)) { //Back
      select_buzzer();
      tft.fillRoundRect(15, 260, 210, 55, 15, SELECTIONFIGCOLOR);
      tft.setTextSize(3);
      tft.setTextColor(SELECTIONTEXTCOLOR);
      tft.setCursor(70, 275);
      tft.println("VOLTAR");
      
      current_time = RTC.getTime();
      if(row_time_light == 1) light_time = sum_hours(current_time, 4);
      if(row_time_light == 2) light_time = sum_hours(current_time, 6);
      if(row_time_light == 3) light_time = sum_hours(current_time, 8);
      if(row_time_light == 4) light_time = sum_hours(current_time, 10);
      if(row_time_light == 5) light_time = sum_hours(current_time, 12);
      if(row_time_light == 6) light_time = sum_hours(current_time, 14);
      if(row_time_light == 7) light_time = sum_hours(current_time, 16);
      if(row_time_light == 8) light_time = sum_hours(current_time, 18);
      if(row_time_light == 9) light_time = sum_hours(current_time, 20);
      if(row_time_light == 10) light_time = sum_hours(current_time, 22);
      if(row_time_light == 11) light_time = sum_days(current_time, 1);

      check_light = true;
      transition = true;
      automatic_configuration_frame();
    }
  }
}

/*--------------------------------------------------------------------------------------*/
/*                                      Blocked Frame                                   */
/*--------------------------------------------------------------------------------------*/

void blocked_frame(void) {

  if(transition == true) {
    tft.fillScreen(BACKGROUND);

    /* Time */
    current_time = RTC.getTime();
    tft.setTextColor(TEXTCOLOR);
    tft.setCursor(40, 125);
    tft.setTextSize(5);
    tft.print(current_time.hour);
    tft.print(":");
    if(current_time.min < 10) tft.print("0");
    tft.print(current_time.min);
    display_time = sum_minuts(current_time, 1);

    /* Temperature */
    tft.setTextSize(3);
    tft.setTextColor(TEXTCOLOR);
    tft.setCursor(20, 20);
    tft.print("Temperatura");
    tft.setCursor(50, 50);
    tft.setTextSize(2);
    tft.print("Calibrando...");
    measured_temperature = false;

    /* Hardware Status */
    tft.setTextSize(4);
    if(heater == true) tft.setTextColor(GREEN);
    if(heater == false) tft.setTextColor(TEXTCOLOR);
    if(current_temperature <= (desired_temperature - TEMPERATURE_MARGIN - 1)) {
      tft.setTextColor(RED);
      alert_buzzer();
    }
    tft.setCursor(30, 185);
    tft.print("AQ");

    if(cooler == true) tft.setTextColor(GREEN);
    if(cooler == false) tft.setTextColor(TEXTCOLOR);
    if(current_temperature >= (desired_temperature + TEMPERATURE_MARGIN + 1)) {
      tft.setTextColor(RED);
      alert_buzzer();
    }
    tft.setCursor(100, 185);
    tft.print("CL");

    if(light == true) tft.setTextColor(GREEN);
    if(light == false) tft.setTextColor(TEXTCOLOR);
    tft.setCursor(160, 185);
    tft.print("IL");
    
    /* Unlock Buttom */
    tft.fillRoundRect(15, 240, 210, 60, 15, FIGCOLOR);
    tft.setTextSize(3);
    tft.setTextColor(TEXTCOLOR);
    tft.setCursor(20, 260);
    tft.println("DESBLOQUEAR");
    
    transition = false;
  }
  
  while(true) {

    read_touch();
    
    if(measured_temperature == false) {
      read_temperature();
      tft.fillRect(10, 50, 200, 60, BACKGROUND);
      tft.setCursor(15, 55);
      tft.setTextSize(6);
      tft.print(current_temperature, 1);
      tft.setTextSize(5);
      tft.print((char)167);
      tft.print("C");
      measured_temperature = true;
    }

    if(current_temperature >= desired_temperature-0,5 && current_temperature <= desired_temperature+0,5) {
      digitalWrite(HEATER_PIN, HIGH);
      digitalWrite(COOLERS_PIN, HIGH);
      heater = false;
      cooler = false;
    }
    if(current_temperature <= desired_temperature - TEMPERATURE_MARGIN){
      digitalWrite(HEATER_PIN, LOW);
      digitalWrite(COOLERS_PIN, HIGH);
      heater = true;
      cooler = false;
    }
    if(current_temperature >= desired_temperature + TEMPERATURE_MARGIN){
      digitalWrite(HEATER_PIN, HIGH);
      digitalWrite(COOLERS_PIN, LOW);
      heater = false;
      cooler = true;
    }
    
    current_time = RTC.getTime();
    if(flag_time_read == false) {
      temperature_time = sum_seconds(current_time, 10);
      flag_time_read = true;
    }
    
    if(time_compare(current_time, temperature_time) == true) {
      measured_temperature = false;
      flag_time_read = false;
      transition = true;
      blocked_frame();
    }
        
    if((((int) current_time.hour >= (int) time_light_on.hour) && ((int) current_time.hour < (int) time_light_off.hour)) && automatic_light == true) {
      light = true;
      digitalWrite(LIGHT_PIN_1, LOW);
      digitalWrite(LIGHT_PIN_2, LOW);
      
    }
    if((((int) current_time.hour >= (int) time_light_off.hour) || ((int) current_time.hour < (int) time_light_on.hour)) && automatic_light == true) {
      light = false;
      digitalWrite(LIGHT_PIN_1, HIGH);
      digitalWrite(LIGHT_PIN_2, HIGH);
    }

    COVER = false;
    if(COVER == true && cover_read == false) {
      current_time = RTC.getTime();
      if(row_time_cover == 1) cover_time = sum_seconds(current_time, 30);
      if(row_time_cover == 2) cover_time = sum_minuts(current_time, 1);
      if(row_time_cover == 3) cover_time = sum_minuts(current_time, 3);
      if(row_time_cover == 4) cover_time = sum_minuts(current_time, 5);
      if(row_time_cover == 5) cover_time = sum_minuts(current_time, 10);
      if(row_time_cover == 6) cover_time = sum_minuts(current_time, 15);
      if(row_time_cover == 7) cover_time = sum_minuts(current_time, 20);
      if(row_time_cover == 8) cover_time = sum_minuts(current_time, 30);
      if(row_time_cover == 9) cover_time = sum_minuts(current_time, 45);
      if(row_time_cover == 10) cover_time = sum_hours(current_time, 1);
      if(row_time_cover == 11) cover_time = sum_hours(current_time, 2);
      if(row_time_cover == 12) cover_time = sum_hours(current_time, 3);
      cover_read = true;
    }
    if(COVER == true && time_compare(current_time, cover_time) == true) {
      open_cover_alert();
      flag_time_read = false;
      measured_temperature = false;
      cover_read == false;
    }

    read_touch();
    
    /* Unlock Buttom */
    if((X>15 && X<225) && (Y>10 && Y<70) && (Z>MINPRESSURE && Z<MAXPRESSURE)) {
      select_buzzer();
      tft.fillRoundRect(15, 240, 210, 60, 15, SELECTIONFIGCOLOR);
      tft.setTextSize(3);
      tft.setTextColor(SELECTIONTEXTCOLOR);
      tft.setCursor(20, 260);
      tft.println("DESBLOQUEAR");
      transition = true;
      unblocked_frame();
    }
  }
}

/*--------------------------------------------------------------------------------------*/
/*                                     Unblocked Frame                                  */
/*--------------------------------------------------------------------------------------*/

void unblocked_frame(void) {
  if(transition == true) {
    tft.fillScreen(BACKGROUND);
    
    /* Manual Control Buttom */
    tft.fillRoundRect(15, 20, 210, 60, 15, FIGCOLOR);
    tft.setTextSize(3);
    tft.setTextColor(TEXTCOLOR);
    tft.setCursor(50, 25);
    tft.println("CONTROLE");
    tft.setCursor(70, 50);
    tft.println("MANUAL");

    /* Automatic Control Buttom */
    tft.fillRoundRect(15, 100, 210, 60, 15, FIGCOLOR);
    tft.setTextSize(3);
    tft.setTextColor(TEXTCOLOR);
    tft.setCursor(50, 105);
    tft.println("CONTROLE");
    tft.setCursor(35, 130);
    tft.println("AUTOMATICO");

    /* Themes Button */
    tft.fillRoundRect(15, 180, 210, 60, 15, FIGCOLOR);
    tft.setTextSize(3);
    tft.setTextColor(TEXTCOLOR);
    tft.setCursor(70, 200);
    tft.println("TEMAS");

    /* Unlock Buttom */
    tft.fillRoundRect(15, 260, 210, 55, 15, FIGCOLOR);
    tft.setTextSize(3);
    tft.setTextColor(TEXTCOLOR);
    tft.setCursor(50, 275);
    tft.println("BLOQUEAR");

    transition = false;
  }

  while(true) {
    read_touch();

    if((X>15 && X<225) && (Y>230 && Y<360) && (Z>MINPRESSURE && Z<MAXPRESSURE)) { //Manual
      select_buzzer();
      tft.fillRoundRect(15, 20, 210, 60, 15, SELECTIONFIGCOLOR);
      tft.setTextSize(3);
      tft.setTextColor(SELECTIONTEXTCOLOR);
      tft.setCursor(50, 25);
      tft.println("CONTROLE");
      tft.setCursor(70, 50);
      tft.println("MANUAL");

      transition = true;
      manual_configuration_frame();
    }
    if((X>15 && X<225) && (Y>160 && Y<220) && (Z>MINPRESSURE && Z<MAXPRESSURE)) { //Automatic
      select_buzzer();
      tft.fillRoundRect(15, 100, 210, 60, 15, SELECTIONFIGCOLOR);
      tft.setTextSize(3);
      tft.setTextColor(SELECTIONTEXTCOLOR);
      tft.setCursor(50, 105);
      tft.println("CONTROLE");
      tft.setCursor(35, 130);
      tft.println("AUTOMATICO");

      transition = true;
      automatic_configuration_frame();
    }
    if((X>15 && X<225) && (Y>80 && Y<150) && (Z>MINPRESSURE && Z<MAXPRESSURE)) { //Themes
      select_buzzer();
      tft.fillRoundRect(15, 180, 210, 60, 15, SELECTIONFIGCOLOR);
      tft.setTextSize(3);
      tft.setTextColor(SELECTIONTEXTCOLOR);
      tft.setCursor(70, 200);
      tft.println("TEMAS");

      transition = true;
      themes_frame();
    }
    if((X>15 && X<225) && (Y>10 && Y<70) && (Z>MINPRESSURE && Z<MAXPRESSURE)) { //Block
      select_buzzer();
      tft.fillRoundRect(15, 260, 210, 55, 15, SELECTIONFIGCOLOR);
      tft.setTextSize(3);
      tft.setTextColor(SELECTIONTEXTCOLOR);
      tft.setCursor(50, 275);
      tft.println("BLOQUEAR");

      transition = true;
      measured_temperature = false;
      blocked_frame();
    }
  }
}

/*--------------------------------------------------------------------------------------*/
/*                               Manual Configuration Frame                             */
/*--------------------------------------------------------------------------------------*/

void manual_configuration_frame(void) {
  if(transition == true) {
    tft.fillScreen(BACKGROUND);
    
    /* Light Buttom */
    if(light == true) STATUSCOLOR = COLORON;
    if(light == false) STATUSCOLOR = COLOROFF;
    tft.fillRoundRect(15, 20, 210, 60, 15, STATUSCOLOR);
    tft.setTextSize(3);
    tft.setTextColor(WHITE);
    tft.setCursor(30, 40);
    tft.println("ILUMINACAO");

    /* Coolers Buttom */
    if(cooler == true) STATUSCOLOR = COLORON;
    if(cooler == false) STATUSCOLOR = COLOROFF;
    tft.fillRoundRect(15, 100, 210, 60, 15, STATUSCOLOR);
    tft.setTextSize(3);
    tft.setTextColor(WHITE);
    tft.setCursor(60, 120);
    tft.println("COOLERS");

    /* Heater Buttom */
    if(heater == true) STATUSCOLOR = COLORON;
    if(heater == false) STATUSCOLOR = COLOROFF;
    tft.fillRoundRect(15, 180, 210, 60, 15, STATUSCOLOR);
    tft.setTextSize(3);
    tft.setTextColor(WHITE);
    tft.setCursor(40, 200);
    tft.println("AQUECEDOR");

    /* Back Buttom */
    tft.fillRoundRect(15, 260, 210, 55, 15, FIGCOLOR);
    tft.setTextSize(3);
    tft.setTextColor(TEXTCOLOR);
    tft.setCursor(70, 275);
    tft.println("VOLTAR");

    transition = false;
  }

  while(true) {
    read_touch();

    if((X>15 && X<225) && (Y>230 && Y<360) && (Z>MINPRESSURE && Z<MAXPRESSURE)) { //Light
      select_buzzer();
      light = !light;
      if(light == true) {
        STATUSCOLOR = COLORON;
        digitalWrite(LIGHT_PIN_1, LOW);
        digitalWrite(LIGHT_PIN_2, LOW);

        current_time = RTC.getTime();
        if(((int) current_time.hour >= (int) time_light_on.hour) && ((int) current_time.hour < (int) time_light_off.hour)) automatic_light = true;
        if(((int) current_time.hour >= (int) time_light_off.hour) || ((int) current_time.hour < (int) time_light_on.hour)) automatic_light = false;
      }
      if(light == false) {
        STATUSCOLOR = COLOROFF;
        digitalWrite(LIGHT_PIN_1, HIGH);
        digitalWrite(LIGHT_PIN_2, HIGH);

        current_time = RTC.getTime();
        if(((int) current_time.hour >= (int) time_light_on.hour) && ((int) current_time.hour < (int) time_light_off.hour)) automatic_light = false;
        if(((int) current_time.hour >= (int) time_light_off.hour) || ((int) current_time.hour < (int) time_light_on.hour)) automatic_light = true;
      }
      tft.fillRoundRect(15, 20, 210, 60, 15, STATUSCOLOR);
      tft.setTextSize(3);
      tft.setTextColor(WHITE);
      tft.setCursor(40, 40);
      tft.println("ILUMINACAO");
    }
    if((X>15 && X<225) && (Y>160 && Y<220) && (Z>MINPRESSURE && Z<MAXPRESSURE)) { //Cooler
      select_buzzer();
      cooler = !cooler;
      if(cooler == true) {
        STATUSCOLOR = COLORON;
        digitalWrite(COOLERS_PIN, LOW);
      }
      if(cooler == false) {
        STATUSCOLOR = COLOROFF;
        digitalWrite(COOLERS_PIN, HIGH);
      }
      tft.fillRoundRect(15, 100, 210, 60, 15, STATUSCOLOR);
      tft.setTextSize(3);
      tft.setTextColor(WHITE);
      tft.setCursor(60, 120);
      tft.println("COOLERS");
    }
    if((X>15 && X<225) && (Y>80 && Y<150) && (Z>MINPRESSURE && Z<MAXPRESSURE)) { //Heater
      select_buzzer();
      heater = !heater;
      if(heater == true) {
        STATUSCOLOR = COLORON;
        digitalWrite(HEATER_PIN, LOW);
      }
      if(heater == false) {
        STATUSCOLOR = COLOROFF;
        digitalWrite(HEATER_PIN, HIGH);
      }
      tft.fillRoundRect(15, 180, 210, 60, 15, STATUSCOLOR);
      tft.setTextSize(3);
      tft.setTextColor(WHITE);
      tft.setCursor(40, 200);
      tft.println("AQUECEDOR");
    }
    if((X>15 && X<225) && (Y>10 && Y<70) && (Z>MINPRESSURE && Z<MAXPRESSURE)) { //Back
      select_buzzer();
      tft.fillRoundRect(15, 260, 210, 55, 15, SELECTIONFIGCOLOR);
      tft.setTextSize(3);
      tft.setTextColor(SELECTIONTEXTCOLOR);
      tft.setCursor(70, 275);
      tft.println("VOLTAR");

      transition = true;
      unblocked_frame();
    }
  }
}

/*--------------------------------------------------------------------------------------*/
/*                              Automatic Configuration Frame                           */
/*--------------------------------------------------------------------------------------*/

void automatic_configuration_frame(void) {
  if(transition == true) {
    tft.fillScreen(BACKGROUND);
    
    /* Temperature Buttom */
    tft.fillRoundRect(15, 20, 210, 60, 15, FIGCOLOR);
    tft.setTextSize(3);
    tft.setTextColor(TEXTCOLOR);
    tft.setCursor(20, 40);
    tft.println("TEMPERATURA");

    /* Timers Buttom */
    tft.fillRoundRect(15, 100, 210, 60, 15, FIGCOLOR);
    tft.setTextSize(3);
    tft.setTextColor(TEXTCOLOR);
    tft.setCursor(70, 120);
    tft.println("TIMERS");

    /* Lighting Buttom */
    tft.fillRoundRect(15, 180, 210, 60, 15, FIGCOLOR);
    tft.setTextSize(3);
    tft.setTextColor(TEXTCOLOR);
    tft.setCursor(25, 200);
    tft.println("ILUMINACAO");

    /* Back Buttom */
    tft.fillRoundRect(15, 260, 210, 55, 15, FIGCOLOR);
    tft.setTextSize(3);
    tft.setTextColor(TEXTCOLOR);
    tft.setCursor(70, 275);
    tft.println("VOLTAR");

    transition = false;
  }

  while(true) {
    read_touch();

    if((X>15 && X<225) && (Y>230 && Y<360) && (Z>MINPRESSURE && Z<MAXPRESSURE)) { //Temperature
      select_buzzer();
      tft.fillRoundRect(15, 20, 210, 60, 15, SELECTIONFIGCOLOR);
      tft.setTextSize(3);
      tft.setTextColor(SELECTIONTEXTCOLOR);
      tft.setCursor(20, 40);
      tft.println("TEMPERATURA");
      
      transition = true;
      temperature_frame();
    }
    if((X>15 && X<225) && (Y>160 && Y<220) && (Z>MINPRESSURE && Z<MAXPRESSURE)) { //Timers
      select_buzzer();
      tft.fillRoundRect(15, 100, 210, 60, 15, SELECTIONFIGCOLOR);
      tft.setTextSize(3);
      tft.setTextColor(SELECTIONTEXTCOLOR);
      tft.setCursor(70, 120);
      tft.println("TIMERS");
    
      transition = true;
      timers_frame();
    }
    if((X>15 && X<225) && (Y>80 && Y<150) && (Z>MINPRESSURE && Z<MAXPRESSURE)) { //Light
      select_buzzer();
      tft.fillRoundRect(15, 180, 210, 60, 15, STATUSCOLOR);
      tft.setTextSize(3);
      tft.setTextColor(SELECTIONTEXTCOLOR);
      tft.setCursor(25, 200);
      tft.println("ILUMINACAO");

      transition = true;
      light_frame();
    }
    if((X>15 && X<225) && (Y>10 && Y<70) && (Z>MINPRESSURE && Z<MAXPRESSURE)) { //Back
      select_buzzer();
      tft.fillRoundRect(15, 260, 210, 55, 15, SELECTIONFIGCOLOR);
      tft.setTextSize(3);
      tft.setTextColor(SELECTIONTEXTCOLOR);
      tft.setCursor(70, 275);
      tft.println("VOLTAR");

      transition = true;
      unblocked_frame();
    }
  }
}

/*--------------------------------------------------------------------------------------*/
/*                                        Theme Frame                                   */
/*--------------------------------------------------------------------------------------*/

void themes_frame(void) {
  if(transition == true) {
    tft.fillScreen(BACKGROUND);
    
    /* Theme 1 */
    tft.fillRoundRect(15, 20, 210, 60, 15, FIGCOLOR);
    tft.setTextSize(3);
    tft.setTextColor(TEXTCOLOR);
    tft.setCursor(70, 40);
    tft.println("TEMA 1");

    /* Theme 2 */
    tft.fillRoundRect(15, 100, 210, 60, 15, FIGCOLOR);
    tft.setTextSize(3);
    tft.setTextColor(TEXTCOLOR);
    tft.setCursor(70, 120);
    tft.println("TEMA 2");

    /* Theme 3 */
    tft.fillRoundRect(15, 180, 210, 60, 15, FIGCOLOR);
    tft.setTextSize(3);
    tft.setTextColor(TEXTCOLOR);
    tft.setCursor(70, 200);
    tft.println("TEMA 3");

    /* Back Buttom */
    tft.fillRoundRect(15, 260, 210, 55, 15, FIGCOLOR);
    tft.setTextSize(3);
    tft.setTextColor(TEXTCOLOR);
    tft.setCursor(70, 275);
    tft.println("VOLTAR");

    transition = false;
  }

  while(true) {
    read_touch();

    if((X>15 && X<225) && (Y>230 && Y<360) && (Z>MINPRESSURE && Z<MAXPRESSURE)) { //Theme 1
      select_buzzer();
      tft.fillRoundRect(15, 20, 210, 60, 15, SELECTIONFIGCOLOR);
      tft.setTextSize(3);
      tft.setTextColor(SELECTIONTEXTCOLOR);
      tft.setCursor(70, 40);
      tft.println("TEMA 1");

      BACKGROUND         = BLACK;
      TEXTCOLOR          = WHITE;
      FIGCOLOR           = BLUE;
      SELECTIONFIGCOLOR  = WHITE;
      SELECTIONTEXTCOLOR = BLUE;
      
      transition = true;
      blocked_frame();
    }
    if((X>15 && X<225) && (Y>160 && Y<220) && (Z>MINPRESSURE && Z<MAXPRESSURE)) { //Theme 2
      select_buzzer();
      tft.fillRoundRect(15, 100, 210, 60, 15, SELECTIONFIGCOLOR);
      tft.setTextSize(3);
      tft.setTextColor(SELECTIONTEXTCOLOR);
      tft.setCursor(70, 130);
      tft.println("TEMA 2");

      BACKGROUND         = WHITE;
      TEXTCOLOR          = BLUE;
      FIGCOLOR           = BLACK;
      SELECTIONFIGCOLOR  = BLUE;
      SELECTIONTEXTCOLOR = BLACK;
      
      transition = true;
      blocked_frame();
    }
    if((X>15 && X<225) && (Y>80 && Y<150) && (Z>MINPRESSURE && Z<MAXPRESSURE)) { //Theme 3
      select_buzzer();
      tft.fillRoundRect(15, 180, 210, 60, 15, SELECTIONFIGCOLOR);
      tft.setTextSize(3);
      tft.setTextColor(SELECTIONTEXTCOLOR);
      tft.setCursor(70, 200);
      tft.println("TEMA 3");

      BACKGROUND         = BLUE;
      TEXTCOLOR          = WHITE;
      FIGCOLOR           = BLACK;
      SELECTIONFIGCOLOR  = WHITE;
      SELECTIONTEXTCOLOR = BLACK;
      
      transition = true;
      blocked_frame();
    }
    if((X>15 && X<225) && (Y>10 && Y<700) && (Z>MINPRESSURE && Z<MAXPRESSURE)) { //Back
      select_buzzer();
      tft.fillRoundRect(15, 260, 210, 55, 15, SELECTIONFIGCOLOR);
      tft.setTextSize(3);
      tft.setTextColor(SELECTIONTEXTCOLOR);
      tft.setCursor(70, 275);
      tft.println("VOLTAR");

      transition = true;
      unblocked_frame();
    }
  }
}

/*--------------------------------------------------------------------------------------*/
/*                                     Temperature Frame                                */
/*--------------------------------------------------------------------------------------*/

void temperature_frame(void) {
  if(transition == true) {
    tft.fillScreen(BACKGROUND);

    /* Tittle */
    tft.fillRect(0, 20, 240, 100, FIGCOLOR);
    tft.setTextSize(3);
    tft.setTextColor(TEXTCOLOR);
    tft.setCursor(25, 40);
    tft.println("TEMPERATURA");
    tft.setCursor(50, 80);
    tft.println("DESEJADA");

    /* Time */
    tft.setTextSize(4);
    tft.setTextColor(TEXTCOLOR);
    tft.setCursor(75, 180);
    tft.print(desired_temperature, 1);

    /* - Buttom */
    tft.fillRoundRect(10, 170, 50, 50, 15, FIGCOLOR);
    tft.setTextSize(3);
    tft.setTextColor(TEXTCOLOR);
    tft.setCursor(20, 185);
    tft.println("<<");

    /* + Buttom */
    tft.fillRoundRect(180, 170, 50, 50, 15, FIGCOLOR);
    tft.setTextSize(3);
    tft.setTextColor(TEXTCOLOR);
    tft.setCursor(190, 185);
    tft.println(">>");

    /* Back Buttom */
    tft.fillRoundRect(15, 260, 210, 55, 15, FIGCOLOR);
    tft.setTextSize(3);
    tft.setTextColor(TEXTCOLOR);
    tft.setCursor(70, 275);
    tft.println("VOLTAR");

    transition = false;
  }

  while(true) {
    read_touch();
    
    if((X>180 && X<230) && (Y>70 && Y<140) && (Z>MINPRESSURE && Z<MAXPRESSURE)) { //Minus
      select_buzzer();
      tft.fillRoundRect(10, 170, 50, 50, 15, SELECTIONFIGCOLOR);
      tft.setTextSize(3);
      tft.setTextColor(SELECTIONTEXTCOLOR);
      tft.setCursor(20, 185);
      tft.println("<<");

      tft.fillRect(65, 125, 110, 120, BACKGROUND);
      tft.setTextSize(4);
      tft.setTextColor(TEXTCOLOR);
      tft.setCursor(75, 180);
      desired_temperature -= 0.5;
      tft.print(desired_temperature, 1);
      
      tft.fillRoundRect(10, 170, 50, 50, 15, FIGCOLOR);
      tft.setTextSize(3);
      tft.setTextColor(TEXTCOLOR);
      tft.setCursor(20, 185);
      tft.println("<<");
    }
    if((X>10 && X<60) && (Y>70 && Y<140) && (Z>MINPRESSURE && Z<MAXPRESSURE)) { //Plus
      select_buzzer();
      tft.fillRoundRect(180, 170, 50, 50, 15, SELECTIONFIGCOLOR);
      tft.setTextSize(3);
      tft.setTextColor(SELECTIONTEXTCOLOR);
      tft.setCursor(190, 185);
      tft.println(">>");

      tft.fillRect(65, 125, 110, 120, BACKGROUND);
      tft.setTextSize(4);
      tft.setTextColor(TEXTCOLOR);
      tft.setCursor(75, 180);
      desired_temperature += 0.5;
      tft.print(desired_temperature, 1);

      tft.fillRoundRect(180, 170, 50, 50, 15, FIGCOLOR);
      tft.setTextSize(3);
      tft.setTextColor(TEXTCOLOR);
      tft.setCursor(190, 185);
      tft.println(">>");
    }
    if((X>15 && X<210) && (Y>10 && Y<70) && (Z>MINPRESSURE && Z<MAXPRESSURE)) { //Back
      select_buzzer();
      tft.fillRoundRect(15, 260, 210, 55, 15, SELECTIONFIGCOLOR);
      tft.setTextSize(3);
      tft.setTextColor(SELECTIONTEXTCOLOR);
      tft.setCursor(70, 275);
      tft.println("VOLTAR");

      transition = true;
      automatic_configuration_frame();
    }
  }
}

/*--------------------------------------------------------------------------------------*/
/*                                           Setup                                      */
/*--------------------------------------------------------------------------------------*/

void setup(void) {
  tft.begin(0x9341);  //Initializing Display with II9341 Controller
  tft.setRotation(2); //Vertical
  transition = true;
  
  /* RTC DS1307 */
  RTC.halt(false);
  RTC.setSQWRate(SQW_RATE_1);
  RTC.enableSQW(true);

  RTC.setTime(20, 18, 0); //Current Time
  //RTC.setDate(10, 8, 2016);

  /* I/O */
  pinMode(HEATER_PIN,  OUTPUT);
  pinMode(COOLERS_PIN, OUTPUT);
  pinMode(LIGHT_PIN_1, OUTPUT);
  pinMode(LIGHT_PIN_2, OUTPUT);
  pinMode(BUZZER_PIN,  OUTPUT);
  pinMode(BUTTOM_PIN,  INPUT);

  /* Initializations */
  light = true;
  digitalWrite(HEATER_PIN,  HIGH);
  digitalWrite(COOLERS_PIN, HIGH);
  digitalWrite(LIGHT_PIN_1, LOW);
  digitalWrite(LIGHT_PIN_2, LOW);

  light_time_init();

  initialization_buzzer();
  frame = BLOCKED;
}

/*--------------------------------------------------------------------------------------*/
/*                                            Loop                                      */
/*--------------------------------------------------------------------------------------*/

void loop(void) {
  if(frame == BLOCKED) blocked_frame(); //Initial State
}
