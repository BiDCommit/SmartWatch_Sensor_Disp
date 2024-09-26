#include <Arduino.h>
//БИБЛИОТЕКА ДЛЯ РАБОТЫ С БЛЮТУЗ 
#include <BluetoothSerial.h>
// БИБЛИОТЕКА ДЛЯ РАБОТЫ С ВНУТРЕННИМИ ЧАСАМИ РЕАЛЬНОГО ВРЕМЕНИ 
#include <ESP32Time.h>
// ХРАНИЛИЩЕ ЗНАЧКОВ 
#include "ICONS.c"
#include "logo2.c"
// БИБЛИОТЕКА ДЛЯ РАБОТЫ С ЭКРАНОМ НА МИКРОСХЕМЕ GC9A01
#include <Arduino_GFX_Library.h>
// Подключаем библиотеку для работы с шиной SPI
#include <SPI.h> 
// БИБЛИОТЕКА ДЛЯ РАБОТЫ С КНОПКАМИ 
#include "GyverButton.h"   
// БИБЛИОТЕКА ДЛЯ РАБОТЫ ТАЧСКРИНА (СЕНСОР ЭКРАНА)        
#include <CST816S.h>    
#include <DallasTemperature.h>
#include <OneWire.h>
// БИБЛИОТЕКИ ДЛЯ РАБОТЫ ДАТЧИКА СЕРДЦЕБИЕНИЯ И НАСЫЩЕНИЯ КИСЛОРОДОМ КРОВИ 
//#include <Wire.h>
//#include "MAX30105.h"
//#include "spo2_algorithm.h"
// Ловим степ с накликиванием для управления яркостью экрана 
#define BTN_PIN 4  // кнопка подключена сюда (BTN_PIN --- КНОПКА --- GND)
GButton butt1(BTN_PIN ,LOW_PULL, NORM_OPEN);
// Датчик света ТЕМТ6000
#define LIGHTSENSOR 34
#define TermoSensro 32 
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(TermoSensro);
// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);
// arrays to hold device address
DeviceAddress insideThermometer;
// Пины для подключения диспленя 
// ESP32 various dev board: CS:5, DC:27, RST:33, BL:2, SCK:18, MOSI:23, MISO:19
// переменные для отсчета минут и секунд
int SEC = 0;
int MIN = 0;
unsigned long timer1;
/////РАЗМЕР ЛОГОТИПА
#define IMG_WIDTH 100
#define IMG_HEIGHT 100
////РАЗМЕР ЗНАЧКОВ 
#define IMG_WIDTH2 30
#define IMG_HEIGHT2 30
//// ПИН ВИБРОМОТОРА 
#define MOTORPIN 15
//// ПИН СВЕТОДИОДА- ФОНАРИК 
#define LEDPIN 14
// TOUCHSCREEN CST816 and for MAX30102(HEART SENSOR)
#define TP_SDA 21
#define TP_SCL 22
#define TP_INT 26
#define TP_RST 25
#define MAX_BRIGHTNESS 255
CST816S touch(TP_SDA,TP_SCL,TP_RST,TP_INT);	// sda, scl, rst, irq                        
//// объявление параметра часов (RTC)
ESP32Time rtc;  
char incoming;//переменная для оправки
#define GFX_BL DF_GFX_BL // пин яркости дисплея !!!
/// инициализация микросхемы джисплея для вывода на экран 
#if defined(DISPLAY_DEV_KIT)
Arduino_GFX *gfx = create_default_Arduino_GFX();
#else 
Arduino_DataBus *bus = create_default_Arduino_DataBus();
Arduino_GFX *gfx = new Arduino_GC9A01(bus, DF_GFX_RST, 0 , true);
#endif
///////
bool bellmode = false;
///
bool CHR = false;
//////
bool FlashMode = false;
bool Logos = false;
////////////////
extern uint8_t SmallFont[], MediumNumbers[];
static bool deviceConnected = false;
bool rotate = false, flip = false, hr24 = true, notify = true, screenOff = false, scrOff = false, b1;
//////////
uint16_t dutyCycle;
bool Flash = false;
struct Tellemetry{
byte Battery;
byte Temp;
};
/////
#if !defined(CONFIG_BT_SPP_ENABLED)
#error Serial Bluetooth not available or not enabled. It is only available for the ESP32 chip.
#endif
//////
BluetoothSerial SerialBT;
const char *pin = "1234"; // Change this to reflect the pin expected by the real slave BT device
String slaveName = "ESP32-BT-Slave"; // Change this to reflect the real name of your slave BT device
String myName = "SMARTWATCH_V2";
//// СИНХРОНИЗАИЦЯ ДАННЫХ ВРЕМЕНИ И ДАТЫ С ДАННЫМИ СМАРТФОНА ПРИ BLE 
void printLocalTime() {
  if (!hr24) {
    gfx->setCursor(170, 187);
    gfx->setTextColor(WHITE,BLACK);
    gfx->setTextSize(2);
    gfx->println(rtc.getAmPm(true));
  }
    gfx->setCursor(80, 175);
    gfx->setTextColor(RED,BLACK);
    gfx->setTextSize(2);
    gfx->println(rtc.getTime("%H:%M:%S"));
    gfx->drawRect(80, 172, 100, 25, BLUE);
}

void printLocalDate(){//ИНИЦИАЛИЗАЦИЯ ЧАСОВ ESP ОТ ВНУТРЕННЕГО RTC 
struct tm timeinfo = rtc.getTimeStruct();
  // запоминаем время отправки
uint32_t last_time = micros();
gfx->setCursor(80, 53);
gfx->setTextColor(GREEN,BLACK);
gfx->setTextSize(2);
   // gfx->println(rtc.getTime("%A, %B %d %Y")); Friday, May 27, 2022 <-- стиль вывода 
gfx->println(rtc.getTime("%x")); // 08/23/01 <--стиль вывода даты 
gfx->drawRect(75, 46, 100, 25, RED);}

void BTSTATUS(){
bool connected;
connected = SerialBT.connect(slaveName);
if(connected){gfx->draw16bitRGBBitmap(190, 80, (const uint16_t*)bluetooth, IMG_WIDTH2, IMG_HEIGHT2);} 
else {gfx->draw16bitRGBBitmap(190, 80, (const uint16_t*)bluetooth2, IMG_WIDTH2, IMG_HEIGHT2);}}

void printTemperature(DeviceAddress deviceAddress)
{
  // method 2 - faster
  float tempC = sensors.getTempC(deviceAddress);
  if (tempC == DEVICE_DISCONNECTED_C)
  {
    Serial.println("Error: Could not read temperature data");
    return;
  }
  Serial.print("Temp C: ");
  Serial.print(tempC);
  Serial.print(" Temp F: ");
  Serial.println(DallasTemperature::toFahrenheit(tempC)); // Converts tempC to Fahrenheit
}

// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

void setup() {
if (!sensors.getAddress(insideThermometer, 0)) Serial.println("Unable to find address for Device 0");
Serial.print("Device 0 Address: ");
  printAddress(insideThermometer);
  Serial.begin(9600);
  SerialBT.begin(myName);
  ///////////////////////
  timer1 = millis();
   touch.begin();
  //////////////////////
    pinMode(MOTORPIN,OUTPUT);
    pinMode(BTN_PIN,INPUT);
    pinMode(LEDPIN,OUTPUT);
    pinMode(GFX_BL, OUTPUT);
    pinMode(LIGHTSENSOR, INPUT);
/// установка времени ( секунды, минуты, часы(пишем на час меньше), день, месяц, год)
    rtc.setTime(0, 15, 15, 04, 03, 2024); 
    gfx->begin();
    gfx->fillScreen(BLACK);
#ifdef GFX_BL
   pinMode(GFX_BL, OUTPUT);
#endif
    digitalWrite(GFX_BL,HIGH);
    gfx->setCursor(90, 35);
    gfx->setTextColor(RED);
    gfx->setTextSize(3);
    gfx->println("HERO");
//////////////////////
    gfx->setCursor(80, 200);
    gfx->setTextColor(RED);
    gfx->setTextSize(3);
    gfx->println("WATCH");
    gfx->draw16bitRGBBitmap(70, 70, (const uint16_t*)logo2, IMG_WIDTH, IMG_HEIGHT);
    delay(1500);
/////////////////////////
    gfx->setCursor(90, 35);
    gfx->setTextColor(BLACK);
    gfx->setTextSize(3);
    gfx->println("HERO");
/////////////////////////
    gfx->setCursor(80, 200);
    gfx->setTextColor(BLACK);
    gfx->setTextSize(3);
    gfx->println("WATCH");
///////////////////////
    digitalWrite(MOTORPIN,HIGH);
    delay(500);
    digitalWrite(MOTORPIN,LOW);
}

////ЦИКЛ ДЛЯ РАБОТЫ С ВЫВОДОМ ДАННЫХ НА ДИСПЛЕЙ//////
void loop() {
 sensors.requestTemperatures();
Tellemetry Buf;
Buf.Battery = 50;
Buf.Temp= sensors.getTempCByIndex(0);
SerialBT.write((byte*)&Buf,sizeof(Buf));//отправляем температуру в цельсиях
if(SerialBT.available()){
 incoming=SerialBT.read();
 if(incoming=='0'){
Flash =true;
 }
 if(incoming=='1'){
Flash =false;
 }
 if(incoming=='H'){
Logos=false;
 }
if(incoming=='A'){
Logos=true;
 }
}

//МОНИТОРИНГ СОСТОЯНИЯ КНОПОК
butt1.tick();
//СЧИТЫВАНИЕ СОСТОЯНИЯ КНОПОК В ПОЛОЖЕНИИ (HIGH|LOW)
int Push_button_state = digitalRead(BTN_PIN);
/////ВЫВОД НА ДИСПЛЕЙ ЗНАЧКОВ 
gfx->draw16bitRGBBitmap(10, 120, (const uint16_t*)helmet, IMG_WIDTH2, IMG_HEIGHT2);
////ЗНАЧОК ДАТЧИКА СВЕТА 
gfx->draw16bitRGBBitmap(200, 120, (const uint16_t*)lightsensor, IMG_WIDTH2, IMG_HEIGHT2);
////ЗНАЧОК ТЕРМОМЕТРА 
gfx->draw16bitRGBBitmap(130, 205, (const uint16_t*)Thermometer, IMG_WIDTH2, IMG_HEIGHT2);
if(touch.available()){
gfx->setCursor(100, 25);
gfx->setTextColor(BLUE,BLACK);
gfx->setTextSize(0.3);
gfx->println(touch.gesture());}
//// ВЫВОД ЗНАЧЕНИЯ ПОТЕНЦИОМЕТРА 
dutyCycle =  analogRead(35); 
int val = map(dutyCycle,0,1023,0,180);
gfx->setCursor(60, 30);
gfx->setTextColor(BLUE,BLACK);
gfx->setTextSize(0.1);
gfx->println(dutyCycle);
////ВЫВОД ЗНАЧЕНИЯ ДАТЧИКА СВЕТА 
float  light_s =  analogRead(LIGHTSENSOR); 
float val2 = (light_s * 100)/10230;
gfx->setCursor(190, 153);
gfx->setTextColor(RED,BLACK);
gfx->setTextSize(1);
gfx->println(val2*2.5);
/////////TERMOSENSOR///////////////
gfx->setCursor(110, 220);
gfx->setTextColor(RED,BLACK);
gfx->setTextSize(2);
gfx->println(Buf.Temp);
//////////////Flashlight//////////////////////////
if(dutyCycle >300 && dutyCycle < 400 ){
    gfx->fillCircle(65,155,7,ORANGE); ////Flashlight
    gfx->fillCircle(55,135,7,BLACK); /////Helmet
    gfx->fillCircle(60,100,7,BLACK); ////Bell
    gfx->fillCircle(67,85,7,BLACK); ///MP3
}
//////////////Helmet////////////////////////
if(dutyCycle >200 && dutyCycle < 300 ){
    gfx->fillCircle(65,155,7,BLACK); ////Flashlight
    gfx->fillCircle(55,135,7,BLUE); /////Helmet
    gfx->fillCircle(60,100,7,BLACK); ////Bell
    gfx->fillCircle(67,85,7,BLACK); ///MP3
}
//////////////BELL///////////////////
if(dutyCycle >100 && dutyCycle < 200 ){
    gfx->fillCircle(65,155,7,BLACK); ////Flashlight
    gfx->fillCircle(55,135,7,BLACK); /////Helmet
    gfx->fillCircle(60,100,7,RED); ////Bell
    gfx->fillCircle(67,85,7,BLACK); ///MP3
}
//////////////MP3///////////////////
/*if(dutyCycle >100 && dutyCycle < 200){
    gfx->fillCircle(65,155,7,BLACK); ////Flashlight
    gfx->fillCircle(55,135,7,BLACK); /////Helmet
    gfx->fillCircle(60,100,7,BLACK); ////Bell
    gfx->fillCircle(67,85,7,YELLOW); ///MP3
    }*/
////////прибавляем одну секунду к переменной SEC
if (millis() - timer1 > 1000){timer1 = millis();SEC = SEC + 1;}
if (SEC > 59){ SEC = 0; MIN = MIN + 1; }
if (SEC < 59 && MIN < 0  ){digitalWrite(GFX_BL, HIGH);}
////////butt2-Screen-ON////////
if(MIN > 0 ){ digitalWrite(GFX_BL, LOW);}
///////CHANGE LOGO
if(touch.data.gestureID==1 || Logos==false){gfx->draw16bitRGBBitmap(75, 70, (const uint16_t*)logo2, IMG_WIDTH, IMG_HEIGHT);}
if(touch.data.gestureID==2 || Logos==true){gfx->draw16bitRGBBitmap(75, 70, (const uint16_t*)Alex, IMG_WIDTH, IMG_HEIGHT);}
////////FLASHLIGHT//////
if(touch.data.gestureID==4 && dutyCycle >300 && dutyCycle < 400 || Flash == true){
SEC = 0;MIN = 0;digitalWrite(LEDPIN,HIGH);
gfx->draw16bitRGBBitmap(20, 160, (const uint16_t*)fl_on, IMG_WIDTH2, IMG_HEIGHT2);}
if(touch.data.gestureID==3 && dutyCycle >300 && dutyCycle < 400 || Flash == false){
gfx->draw16bitRGBBitmap(20, 160, (const uint16_t*)fl_off, IMG_WIDTH2, IMG_HEIGHT2);}
////////Flashlight - OFF///
if( touch.data.gestureID==3 && dutyCycle >300 && dutyCycle < 400 ){digitalWrite(LEDPIN,LOW);gfx->draw16bitRGBBitmap(20, 160, (const uint16_t*)fl_off, IMG_WIDTH2, IMG_HEIGHT2);}
/////////Helmet-Radio-send////////
if( butt1.isPress()==HIGH){SEC = 0;MIN = 0;digitalWrite(GFX_BL, HIGH);}
/////bell-mode //////////////////
if (touch.data.gestureID==4 && dutyCycle >100 && dutyCycle < 200){bellmode = true;}
if(touch.data.gestureID==3 && dutyCycle >100 && dutyCycle < 200){bellmode = false;}
if(bellmode == false ){gfx->draw16bitRGBBitmap(20, 80, (const uint16_t*)nobell, IMG_WIDTH2, IMG_HEIGHT2);}
if(bellmode == true){gfx->draw16bitRGBBitmap(20, 80, (const uint16_t*)bell, IMG_WIDTH2, IMG_HEIGHT2);}
//BATTERY
gfx->setCursor(175, 85);
gfx->setTextColor(BLUE,BLACK);
gfx->setTextSize(2);
gfx->println("75");
gfx->draw16bitRGBBitmap(196, 80, (const uint16_t*)b75, IMG_WIDTH2, IMG_HEIGHT2);
gfx->draw16bitRGBBitmap(196, 163, (const uint16_t*)heart_pulse, IMG_WIDTH2, IMG_HEIGHT2);
gfx->setCursor(195, 193);
gfx->setTextColor(RED,BLACK);
gfx->setTextSize(1);
gfx->println("72");
//gfx->draw16bitRGBBitmap(150, 10, (const uint16_t*)no_charge, IMG_WIDTH2, IMG_HEIGHT2);
//gfx->draw16bitRGBBitmap(175, 40, (const uint16_t*)no_mess, IMG_WIDTH2, IMG_HEIGHT2);
/////////INPUT MESSAGE//////////////////////////////////
printLocalTime();   // display time
printLocalDate(); // display date
//BTSTATUS();
}

