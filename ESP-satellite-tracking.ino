#include<WiFi.h>
#include<ArduinoJson.h>
#include<Wire.h>
#include<RTClib.h>
#include <GxEPD2_BW.h>
#include <Fonts/FreeSans9pt7b.h>
#include "images.h"

GxEPD2_BW<GxEPD2_154, GxEPD2_154::HEIGHT> display(GxEPD2_154(/*CS=5*/ SS, /*DC=*/ 17, /*RST=*/ 16, /*BUSY=*/ 4));



WiFiClient web;
const char* Web_Address = "www.n2yo.com";
#define Web_Address_Port 80
String response = "";

#define Satellite_ID 27607 // SO-50
float Observer_Lat = 41.702;
float Observer_Lon = -76.014;
#define Altitude 0
#define Days 1
#define Minimum_Seconds 300
String API_KEY = "YOUR API KEY HERE";


#define SSID "Your WiFi Name"
#define Password "WiFi Password"


struct satellite_parameters{
  int start_azimuth;
  int end_azimuth;
  int max_azimuth;
  int start_elevation;
  int end_elevation;
  int max_elevation;
  int duration;
  DateTime start_time;
  DateTime max_elevation_time;
  DateTime end_time;
} pass_predictions[10];


boolean only_next_pass = true;// display only next pass
int passes_count;



void parseJsonData();
bool getPassPredictions(void);
void display_passes(void);
void display_init(void);
void connect_to_wifi(void);
void display_logo(void);
void display_error(String err);

void setup(){
  Serial.begin(115200);
  Serial.println("// booted");
  display_init();
  connect_to_wifi();
  display_logo();
  delay(3000);

  
  if(getPassPredictions()){
    parseJsonData();
    display_passes();
  }
  else{
    display_error("fetch error");
  }
}


void loop(){

  
}






void parseJsonData(){
  StaticJsonDocument<3000> jsonBuffer;
  DeserializationError error = deserializeJson(jsonBuffer, response);
  if (error) {
    Serial.println("There was an error while deserializing");
    display_error("JSON parse err");
    return;
  }

  
  JsonObject root = jsonBuffer.as<JsonObject>();
  JsonObject info = root["info"];
  int info_satid = info["satid"]; // 27607
  const char* info_satname = info["satname"]; // "SAUDISAT 1C"
  int info_transactionscount = info["transactionscount"]; // 13
  passes_count = info["passescount"]; // 18);
  Serial.println(info_satid);
  Serial.println(info_satname);
  Serial.println(info_transactionscount);
  Serial.println(passes_count);
  JsonArray passes = root["passes"];

  
  for(int i=0; i<passes_count;i++){

    if(i < 10){
      pass_predictions[i].start_azimuth = passes[i]["startAz"]; // 205.44
      pass_predictions[i].start_elevation = passes[i]["startEl"]; // 0.29
      pass_predictions[i].start_time = long(passes[i]["startUTC"]) + 19800; // 1565401750
      pass_predictions[i].max_azimuth = passes[i]["maxAz"]; // 118.47
      pass_predictions[i].max_elevation = passes[i]["maxEl"]; // 71.42
      pass_predictions[i].max_elevation_time = long(passes[i]["maxUTC"]) + 19800; // 1565402175
      pass_predictions[i].end_azimuth = passes[i]["endAz"]; // 35.81
      pass_predictions[i].end_elevation = passes[i]["endEl"]; // 0
      pass_predictions[i].end_time = long(passes[i]["endUTC"]) + 19800; // 1565402605
      pass_predictions[i].duration = passes[i]["duration"]; // 855
    }
    
  }
}


bool getPassPredictions(void){
  String path = "/rest/v1/satellite/visualpasses/" + String(Satellite_ID) + "/" + String(Observer_Lat) + "/" + String(Observer_Lon) + "/" + String(Altitude) + "/" + String(Days) + "/" + String(Minimum_Seconds) + "/&apiKey=" + String(API_KEY);
  boolean status = false;
  if(web.connect(Web_Address,Web_Address_Port)){
    Serial.println("Connected to : " + String(Web_Address));
    Serial.println("Requesting : " + path);
    web.print("GET " + path + " HTTP/1.0\r\n");
    web.print("Host: " + String(Web_Address) + "\r\n");
    web.print("User-Agent: Mozilla/4.0 (compatible; MSIE5.01; Windows NT)\r\n");
    web.print("Content-Type: application/x-www-form-urlencoded\r\n");
    web.print("Connection: Keep-Alive\r\n\r\n");
    Serial.println("Posted");
    status = true;
  }
  else{
    Serial.println("failed to connect to : " + String(Web_Address));
    return false;
  }
  delay(1000);
  unsigned long start_millis = millis();
  while(!web.available()){
    if(millis() - start_millis > 30000){
      display_error("NO RESPONSE");
    }
  }
  char c='a';
  if(web.find("\r\n\r\n\r\n")){
    while(web.available()){
      response = web.readStringUntil('\n');
      //response += c;
      Serial.print(response);
    }
  }
  Serial.println("");
  Serial.println(response.length());
  web.stop();
  return status;
}



void display_passes(void){
  int display_passes_count = 0;
  if(only_next_pass){
    display_passes_count = 1;
  }
  else{
    display_passes_count = passes_count;
  }

  for(int i=0;i<display_passes_count;i++){
    display.firstPage();
    do{
      display.fillScreen(GxEPD_WHITE);
      display.setCursor(10, 15);
      display.println("Start time : " + String(pass_predictions[i].start_time.hour()) + ":" + String(pass_predictions[i].start_time.minute()) + ":" + String(pass_predictions[i].start_time.second()));
      display.setCursor(10, 34);
      display.println("End time : " + String(pass_predictions[i].end_time.hour()) + ":" + String(pass_predictions[i].end_time.minute()) + ":" + String(pass_predictions[i].end_time.second()));
      display.setCursor(10, 53);
      display.println("MAX @ " + String(pass_predictions[i].max_elevation_time.hour()) + ":" + String(pass_predictions[i].max_elevation_time.minute()) + ":" + String(pass_predictions[i].max_elevation_time.second()));
      display.setCursor(10, 72);
      display.println("Duration : " + String(pass_predictions[i].duration) + " sec");
      display.drawRect(0,76,200,1, GxEPD_BLACK);
      display.drawRect(110,79,1,120, GxEPD_BLACK);
      display.drawRect(110,160,90,1, GxEPD_BLACK);
      display.setCursor(10, 93);
      display.println("S_AZ : " + String(pass_predictions[i].start_azimuth));
      display.setCursor(10, 112);
      display.println("E_AZ : " + String(pass_predictions[i].end_azimuth));
      display.setCursor(10, 131);
      display.println("M_AZ : " + String(pass_predictions[i].max_azimuth));
      display.setCursor(10, 150);
      display.println("M_EL : " + String(pass_predictions[i].max_elevation));
      display.setCursor(10, 169);
      display.println("S_EL : " + String(pass_predictions[i].start_elevation));
      display.setCursor(10, 188);
      display.println("E_EL : " + String(pass_predictions[i].end_elevation));
      display.setCursor(112, 188);
      display.println(String(pass_predictions[i].start_time.day()) + "/" + String(pass_predictions[i].start_time.month()) + "/" + String(pass_predictions[i].start_time.year()));
      display.drawBitmap(120, 80, radio_bits, radio_width, radio_height, GxEPD_BLACK);
    }while(display.nextPage());
    delay(5000);
  }
}


void display_init(void){
  display.init(115200);
  display.setRotation(0);
  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
  display.setFont(&FreeSans9pt7b);
}


void connect_to_wifi(void){
  display.firstPage();
  do{
    display.setCursor(20, 50);
    display.println("Connecting to WiFi");
    display.setCursor(20, 80);
    display.println("SSID : " + String(SSID));
    display.drawBitmap(50,110, wifi_bits, wifi_width, wifi_height, GxEPD_BLACK);
  }while(display.nextPage());
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID , Password);
  while(WiFi.status() != WL_CONNECTED){
    Serial.print('.');
    delay(500);
  }
  Serial.print("WiFi local IP : ");
  Serial.println(WiFi.localIP());
}


void display_logo(void){
  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
  display.firstPage();
  do{
    display.drawBitmap(25,0,sat_logo_bits, sat_logo_width, sat_logo_height,GxEPD_BLACK);
    display.setCursor(35,175);
    display.println("Fetching DATA");
  }while(display.nextPage());
}

void display_error(String err){
  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
  display.firstPage();
  do{
    display.drawBitmap(25,0, error_bits, error_width, error_height,GxEPD_BLACK);
    display.setCursor(35,175);
    display.println(err);
  }while(display.nextPage()); 
  while(1){
    delay(500);
  }
}
