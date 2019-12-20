#include<WiFi.h>
#include<ArduinoJson.h>
#include<Wire.h>
#include<RTClib.h>
#include <GxEPD2_BW.h>
#include <Fonts/FreeSans9pt7b.h>
#include "images.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include<WebServer.h>

GxEPD2_BW<GxEPD2_154, GxEPD2_154::HEIGHT> display(GxEPD2_154(/*CS=5*/ SS, /*DC=*/ 17, /*RST=*/ 16, /*BUSY=*/ 4));
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "in.pool.ntp.org", 19800, 60000);
int today;
int current_hour;
int current_minute;
int current_second;;

WebServer server(80);


WiFiClient web;
const char* Web_Address = "www.n2yo.com";
#define Web_Address_Port 80
String response = "";

String Satellite_ID = "43017"; // SO-50

//Enter your latitude, longitude and Altitude
float Observer_Lat = 17.4800;
float Observer_Lon = 78.4667;
#define Altitude 0
#define Days 1

String API_KEY = "PASTE N2YO API KEY HERE";
String IFTTT_API_KEY = "PASTE IFTTT WEBHOOKS API KEY HERE";
String IFTTT_EVENT_NAME = "PASTE IFTTT WEBHOOKS TRIGGER NAME HERE";

#define SSID "YOUR WIFI NAME/SSID"
#define Password "YOUR WIFI PASSWORD"


struct satellite_parameters {
  int start_azimuth;
  int end_azimuth;
  int max_azimuth;
  int start_elevation;
  int end_elevation;
  int max_elevation;
  DateTime start_time;
  DateTime max_elevation_time;
  DateTime end_time;
  unsigned long start_utc_time;
  String satellite_name;
} pass_predictions[10] , temporary;


int passes_count;

const int satellites_count = 5; // count should be matched with below list


String satellites_list[satellites_count] = {
  "44829", //ISS
  "27607", //SO 50
  "43017", //AO 91
  "24278", //FO 29
  "40967", //AO 85
}; // add the NORAD ID's of satellites you want to track but make sure to match the above satellite_count variable to this list count.


void home_page();
void test_notify();
void parse_json_data(int position_in_list);
bool get_pass_predictions(String NORAD_ID);
void display_passes(void);
void display_init(void);
void connect_to_wifi(void);
void display_logo(void);
void display_error(String err);
void send_notification(int i);
boolean get_passes_for_satellites_list();
boolean get_passes_for_single_satellite();
void sort_satellites_list_passes();
void notification_stuff();
void update_time();

int next_pass_difference_minutes;
boolean single_satellite = false;
int minutes_to_complete_current_pass;

void setup() {
  Serial.begin(115200);
  Serial.println("// booted");
  display_init();
  connect_to_wifi();
  server.on("/", home_page);
  server.on("/test_notify", test_notify);
  server.begin();
  timeClient.begin();
  display_logo();
  delay(3000);
  timeClient.update();
  delay(3000);
}


void loop() {
  update_time();



  if (single_satellite) {
    // for single satellite data
    if (get_pass_predictions(Satellite_ID)) {
      parse_json_data(0);
      notification_stuff();
    }
    update_time();
  }
  else {
    // for list of satellites
    if (get_passes_for_satellites_list()) {// parsing is already done
      sort_satellites_list_passes();
      notification_stuff();
      update_time();
    }
  }
  display_passes();
  unsigned long t = millis();
  Serial.println("Server Enabled");
  while (millis() - t < 30000) {
    server.handleClient();
  }
  Serial.println("back to work");
}







void parse_json_data(int position_in_list) {
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

  if (position_in_list) {
    pass_predictions[position_in_list].satellite_name = info_satname; // "SAUDISAT 1C"
    pass_predictions[position_in_list].start_azimuth = passes[0]["startAz"]; // 205.44
    pass_predictions[position_in_list].start_elevation = passes[0]["startEl"]; // 0.29
    pass_predictions[position_in_list].start_time = long(passes[0]["startUTC"]) + 19800; // 1565401750
    pass_predictions[position_in_list].max_azimuth = passes[0]["maxAz"]; // 118.47
    pass_predictions[position_in_list].max_elevation = passes[0]["maxEl"]; // 71.42
    pass_predictions[position_in_list].max_elevation_time = long(passes[0]["maxUTC"]) + 19800; // 1565402175
    pass_predictions[position_in_list].end_azimuth = passes[0]["endAz"]; // 35.81
    pass_predictions[position_in_list].end_elevation = passes[0]["endEl"]; // 0
    pass_predictions[position_in_list].end_time = long(passes[0]["endUTC"]) + 19800; // 1565402605


    pass_predictions[position_in_list].start_utc_time = long(passes[0]["endUTC"]) + 19800; //used to sort multiple satellites data
  }
  else {
    for (int i = 0; i < passes_count; i++) {
      if (i < 10) {
        pass_predictions[i].satellite_name = info_satname;// "SAUDISAT 1C"
        pass_predictions[i].start_azimuth = passes[i]["startAz"]; // 205.44
        pass_predictions[i].start_elevation = passes[i]["startEl"]; // 0.29
        pass_predictions[i].start_time = long(passes[i]["startUTC"]) + 19800; // 1565401750
        pass_predictions[i].max_azimuth = passes[i]["maxAz"]; // 118.47
        pass_predictions[i].max_elevation = passes[i]["maxEl"]; // 71.42
        pass_predictions[i].max_elevation_time = long(passes[i]["maxUTC"]) + 19800; // 1565402175
        pass_predictions[i].end_azimuth = passes[i]["endAz"]; // 35.81
        pass_predictions[i].end_elevation = passes[i]["endEl"]; // 0
        pass_predictions[i].end_time = long(passes[i]["endUTC"]) + 19800; // 1565402605

        pass_predictions[position_in_list].start_utc_time = long(passes[0]["endUTC"]) + 19800; //no use in single satellite, just a junk variable
      }
    }
  }
}


bool get_pass_predictions(String NORAD_ID) {
  display_logo();
  String path = "/rest/v1/satellite/radiopasses/" + String(NORAD_ID) + "/" + String(Observer_Lat) + "/" + String(Observer_Lon) + "/" + String(Altitude) + "/" + String(Days) + "/0/&apiKey=" + String(API_KEY);
  boolean status = false;
  if (web.connect(Web_Address, Web_Address_Port)) {
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
  else {
    Serial.println("failed to connect to : " + String(Web_Address));
    return false;
  }
  delay(1000);
  unsigned long start_millis = millis();
  while (!web.available()) {
    if (millis() - start_millis > 30000) {
      display_error("NO RESPONSE");
    }
  }
  char c = 'a';
  if (web.find("\r\n\r\n\r\n")) {
    while (web.available()) {
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



void display_passes(void) {
  int display_passes_count = 0;
  if (single_satellite) {
    display_passes_count = passes_count;
  }
  else {
    display_passes_count = satellites_count;
  }

  if (passes_count == 0) {
    display_error("No future Passes");
  }

  for (int i = 0; i < display_passes_count; i++) {
    display.firstPage();
    do {
      display.fillScreen(GxEPD_WHITE);
      display.setCursor(10, 15);
      display.println("Start time : " + String(pass_predictions[i].start_time.hour()) + ":" + String(pass_predictions[i].start_time.minute()) + ":" + String(pass_predictions[i].start_time.second()));
      display.setCursor(10, 34);
      display.println("End time : " + String(pass_predictions[i].end_time.hour()) + ":" + String(pass_predictions[i].end_time.minute()) + ":" + String(pass_predictions[i].end_time.second()));
      display.setCursor(10, 53);
      display.println("MAX @ " + String(pass_predictions[i].max_elevation_time.hour()) + ":" + String(pass_predictions[i].max_elevation_time.minute()) + ":" + String(pass_predictions[i].max_elevation_time.second()));
      display.setCursor(180, 40);
      display.drawCircle(185,36,12,GxEPD_BLACK);
      display.println(i+1);// pass number
      display.drawRect(0, 57, 200, 1, GxEPD_BLACK);
      display.drawRect(110, 57, 1, 83, GxEPD_BLACK);
      display.setCursor(10, 74);
      display.println("S_AZ : " + String(pass_predictions[i].start_azimuth));
      display.setCursor(10, 93);
      display.println("E_AZ : " + String(pass_predictions[i].end_azimuth));
      display.setCursor(10, 112);
      display.println("M_AZ : " + String(pass_predictions[i].max_azimuth));
      display.setCursor(10, 131);
      display.println("M_EL : " + String(pass_predictions[i].max_elevation));
      display.drawRect(0, 140, 200, 1, GxEPD_BLACK);
      display.setCursor(10, 159);
      display.println(String(current_hour) + ":" + String(current_minute) + ":" + String(current_second));
      display.setCursor(112, 159);
      display.println(String(pass_predictions[i].start_time.day()) + "/" + String(pass_predictions[i].start_time.month()) + "/" + String(pass_predictions[i].start_time.year()).substring(2));
      display.drawBitmap(120, 60, radio_bits, radio_width, radio_height, GxEPD_BLACK);
      display.setCursor(10, 190);
      display.println(pass_predictions[i].satellite_name);
    } while (display.nextPage());
    delay(10000);
  }
}


void display_init(void) {
  display.init(115200);
  display.setRotation(0);
  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
  display.setFont(&FreeSans9pt7b);
}


void connect_to_wifi(void) {
  display.firstPage();
  do {
    display.setCursor(20, 50);
    display.println("Connecting to WiFi");
    display.setCursor(20, 80);
    display.println("SSID : " + String(SSID));
    display.drawBitmap(50, 110, wifi_bits, wifi_width, wifi_height, GxEPD_BLACK);
  } while (display.nextPage());
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID , Password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(500);
  }
  Serial.print("WiFi local IP : ");
  Serial.println(WiFi.localIP());
}


void display_logo(void) {
  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
  display.firstPage();
  do {
    display.drawBitmap(25, 0, sat_logo_bits, sat_logo_width, sat_logo_height, GxEPD_BLACK);
    display.setCursor(35, 175);
    display.println("Fetching DATA");
  } while (display.nextPage());
}

void display_error(String err) {
  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
  display.firstPage();
  do {
    display.drawBitmap(25, 0, error_bits, error_width, error_height, GxEPD_BLACK);
    display.setCursor(35, 175);
    display.println(err);
  } while (display.nextPage());
  while (1) {
    delay(500);
  }
}


void send_notification(int i) {
  String satellite_name_with_replaced_character = String(pass_predictions[i].satellite_name);
  satellite_name_with_replaced_character.replace(" ","+");
  String path = "/trigger/" + IFTTT_EVENT_NAME + "/with/key/" + IFTTT_API_KEY + "?value1=Satellite+name+" + satellite_name_with_replaced_character;
  path += "+Start+time+" + (String(pass_predictions[i].start_time.hour()) + "+" + String(pass_predictions[i].start_time.minute())) + "+with+max+elevation+" + String(pass_predictions[i].max_elevation) + "+degrees";
  boolean status = false;
  if (web.connect("maker.ifttt.com", Web_Address_Port)) {
    Serial.println("Connected to : " + String("maker.ifttt.com"));
    Serial.println("Requesting : " + path);
    web.print("GET " + path + " HTTP/1.0\r\n");
    web.print("Host: " + String("maker.ifttt.com") + "\r\n");
    web.print("User-Agent: Mozilla/4.0 (compatible; MSIE5.01; Windows NT)\r\n");
    web.print("Content-Type: application/x-www-form-urlencoded\r\n");
    web.print("Connection: Keep-Alive\r\n\r\n");
    Serial.println("Posted");
  }
  else {
    Serial.println("failed to connect to : " + String("maker.ifttt.com"));
  }
  delay(1000);
  unsigned long start_millis = millis();
  while (!web.available()) {
    if (millis() - start_millis > 30000) {
      display_error("NO RESPONSE");
    }
  }
  char c = 'a';
  while (web.available()) {
    response = web.readStringUntil('\n');
    //response += c;
    Serial.print(response);
  }
  Serial.println("");
  Serial.println(response.length());
  web.stop();
}



boolean get_passes_for_satellites_list() {
  display_logo();
  bool success = true;
  for (int i = 0; i < satellites_count; i++) {
    if (get_pass_predictions(satellites_list[i])) {
      parse_json_data(i);
    }
    else {
      Serial.println("LIST SAT PARSE FAIL");
      display_error("LIST SAT PARSE FAIL");
      success = false;
    }
  }
  return success;
}


void home_page() {
  String page PROGMEM = "<html><p>Do not put space in any of  the fields!</p><form action=\"/test_notify\" method=\"GET\">Satellite name:<br><input type=\"text\" name=\"sat\"><br>Start @ :<br><input type=\"text\" name=\"start\"><br><br>MAX Elevation :<br><input type=\"text\" name=\"elevation\"><br><br><input type=\"submit\" value=\"Submit\"></form></html>";
  server.send(200, "text/html", page);
}


void test_notify() {
  String path = "/trigger/" + IFTTT_EVENT_NAME + "/with/key/" + IFTTT_API_KEY + "?value1=Satellite+name+" + String(server.arg("sat"));
  path += "+Start+time+" + String(server.arg("start")) + "+with+max+elevation+" + String(server.arg("elevation")) + "+degrees";
  boolean status = false;
  if (web.connect("maker.ifttt.com", Web_Address_Port)) {
    Serial.println("Connected to : " + String("maker.ifttt.com"));
    Serial.println("Requesting : " + path);
    web.print("GET " + path + " HTTP/1.0\r\n");
    web.print("Host: " + String("maker.ifttt.com") + "\r\n");
    web.print("User-Agent: Mozilla/4.0 (compatible; MSIE5.01; Windows NT)\r\n");
    web.print("Content-Type: application/x-www-form-urlencoded\r\n");
    web.print("Connection: Keep-Alive\r\n\r\n");
    Serial.println("Posted");
  }
  else {
    Serial.println("failed to connect to : " + String("maker.ifttt.com"));
  }
  delay(1000);
  unsigned long start_millis = millis();
  while (!web.available()) {
    if (millis() - start_millis > 30000) {
      display_error("NO RESPONSE");
    }
  }
  char c = 'a';
  while (web.available()) {
    response = web.readStringUntil('\n');
    //response += c;
    Serial.print(response);
  }
  Serial.println("");
  Serial.println(response.length());
  web.stop();
  server.send(200, "text/html", "<h2>Test Trigger - OK</h2>");
}


void sort_satellites_list_passes() {
  Serial.println("Before sort");
  for (int k = 0; k < satellites_count; k++) {
    Serial.println(pass_predictions[k].start_utc_time);
  }
  Serial.println("----------------");

  for (int i = 0; i < satellites_count; i++) {
    for (int j = i + 1; j < satellites_count; j++) {
      if (pass_predictions[i].start_utc_time > pass_predictions[j].start_utc_time) {
        temporary = pass_predictions[i];
        pass_predictions[i] = pass_predictions[j];
        pass_predictions[j] = temporary;
      }
    }
  }
  Serial.println("After sort");
  for (int l = 0; l < satellites_count; l++) {
    Serial.println(pass_predictions[l].start_utc_time);
  }
  Serial.println("----------------");
}

void notification_stuff() {
  update_time();
  next_pass_difference_minutes = ((pass_predictions[0].start_time.hour() - current_hour) * 60) + (pass_predictions[0].start_time.minute() - current_minute);
  Serial.print("diff : "); //0 in above line because the very next pass is 0th position in the list
  Serial.println(next_pass_difference_minutes);
  Serial.print("pass start ");
  Serial.print(pass_predictions[0].start_time.hour());
  Serial.print(":");
  Serial.println(pass_predictions[0].start_time.minute());
  Serial.print("current ");
  Serial.print(current_hour);
  Serial.print(":");
  Serial.println(current_minute);
  if ((next_pass_difference_minutes < 30 && next_pass_difference_minutes > 20)) {
    send_notification(0);
    Serial.println("Notification SENT");
    Serial.println("Entering IDLE/Display only mode");
    unsigned long current = millis();
    while (millis() - current < 600000) {
      display_passes();
      next_pass_difference_minutes = ((pass_predictions[0].start_time.hour() - current_hour) * 60) + (pass_predictions[0].start_time.minute() - current_minute);
      Serial.print("diff : "); //0 in above line because the very next pass is 0th position in the list
      Serial.println(next_pass_difference_minutes);
      update_time();
    }
    Serial.println("Exiting IDLE/Display only mode");
  }
  else if ((next_pass_difference_minutes < 20 && next_pass_difference_minutes > 10)) {
    send_notification(0);
    Serial.println("Notification SENT");
    Serial.println("Entering IDLE/Display only mode");
    unsigned long current = millis();
    while (millis() - current < 600000) {
      display_passes();
      next_pass_difference_minutes = ((pass_predictions[0].start_time.hour() - current_hour) * 60) + (pass_predictions[0].start_time.minute() - current_minute);
      Serial.print("diff : "); //0 in above line because the very next pass is 0th position in the list
      Serial.println(next_pass_difference_minutes);
      update_time();
    }
    Serial.println("Exiting IDLE/Display only mode");
  }
}


void update_time() {
  timeClient.update();
  today = timeClient.getDay();
  current_hour = timeClient.getHours();
  current_minute = timeClient.getMinutes();
  current_second = timeClient.getSeconds();
  Serial.println(timeClient.getFormattedTime());
}
