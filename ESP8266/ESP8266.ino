
/*
 *  This sketch sends data via HTTP GET requests to data.sparkfun.com service.
 *
 *  You need to get streamId and privateKey at data.sparkfun.com and paste them
 *  below. Or just customize this script to talk to other HTTP servers.
 *
 */


/**
 * 
 * EEPROM Adress
 * 
 * 0 = DeviceID
 * 1 = NrofLED
 * 2 = PIN
 * 3 = AnimationID
 * 4 = AnimationDelay
 * 5 = NrofColors
 * 6 - 10 = Empty
 * 11 - 311 = LED (max 300 LEDs, bit 1-7 = Color, bit 8 = State)
 * 312 - 511 = Color (max 66 Colors) 
 */
 

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <NeoPixelBus.h>
#include <EEPROM.h>

typedef NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> NeoBus;


const char* ssid     = "";
const char* password = "";

#define PORT 80

int configAddr = 0;
int LedAddr = 11;
int ColorAddr = 312;

// Config
uint8_t led_count = 10;
uint8_t color_count = 1;
uint8_t pin = 2;
int deviceID = 0;
int AnimationID = 0;
int AnimationDelay = 0;


// Leds
uint8_t *leds;

// Colors
RgbColor *colors;
//Default Colors
RgbColor black(0);

// Setup Server
ESP8266WebServer server(PORT);
NeoBus* strip = NULL;

void setup() {
  Serial.begin(115200);
  delay(10);

  // INIT EEPROM
   
  EEPROM.begin(512);
  deviceID = EEPROM.read(0);
  led_count = EEPROM.read(1);
  pin = EEPROM.read(2);
  AnimationID = EEPROM.read(3);
  AnimationDelay = EEPROM.read(4);
  color_count = EEPROM.read(5);

  if(led_count == 255){
    led_count = 10;
  }

  initColors(color_count);
  initLeds(led_count);
  
  int val = analogRead(A0) / 4;

  // We start by connecting to a WiFi network
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Setup Strip
  setupStrip();
    
  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
  server.on("/", HTTP_GET, handleRoot);               // Call the 'handleRoot' function when a client requests URI "/"
  server.on("/toogle", HTTP_GET, handleToogle);
  server.on("/update", HTTP_POST, handleUpdate);  
  server.onNotFound(handleNotFound);        // When a client requests an unknown URI (i.e. something other than "/"), call function "handleNotFound"

  server.begin();                           // Actually start the server
  Serial.println("HTTP server started on ");
  Serial.print(WiFi.localIP());
  Serial.print(":");
  Serial.print(PORT);
  Serial.println();
}

void setupStrip(){
  if(strip != NULL){
    delete strip;
    while (!strip->CanShow()) // wait for pin to clear
      {
          yield();
      }
    strip = NULL;
  }
  strip = new NeoBus(led_count);
  strip->Begin();
}

void initColors(uint8_t nColors){
  colors = (RgbColor *)malloc(nColors);
  for(int i=0; i< nColors; i++){
    colors[i] = RgbColor(EEPROM.read(ColorAddr+3*i),EEPROM.read(ColorAddr+3*i+1),EEPROM.read(ColorAddr+3*i+2));
  }
}

void initLeds(uint8_t nLeds){
  leds = (uint8_t *)malloc(nLeds);
  for(int i=0; i< nLeds; i++){
    leds[i] = EEPROM.read(LedAddr+i);
   }
}



void loop(void){
  server.handleClient();                    // Listen for HTTP requests from clients
}

void handleRoot() {
  server.send(200, "text/plain", "Im Here");   // Send HTTP status 200 (Ok) and send some text to the browser/client
}

void handleToogle() {
  server.send(200, "text/plain", "Im Here");   // Send HTTP status 200 (Ok) and send some text to the browser/client
}

void handleUpdate() {
  Serial.print(server.arg("plain"));
  const int capacity = 30*JSON_ARRAY_SIZE(3) + JSON_ARRAY_SIZE(30) + 31*JSON_OBJECT_SIZE(2) + 2*JSON_OBJECT_SIZE(3); // test for 30 leds
  DynamicJsonDocument json(capacity);
  DeserializationError err = deserializeJson(json, server.arg("plain"));
  if(err) {
    Serial.print(F("deserializeJson() failed with code "));
    Serial.println(err.c_str());
    server.send(400);
    return ;
  }

  JsonObject device = json["device"];
  int device_ID = device["ID"];
  const char* device_Name = device["Name"];
  Serial.println(device_Name);
  int device_PIN = device["PIN"];
  JsonArray leds = json["leds"];
  if(leds.size() != strip->PixelCount()){
    Serial.print("Create new Stripe");
     led_count = leds.size();
     setupStrip();
  }
  
  Serial.println(leds.size());
  for(int i=0; i< leds.size(); i++){
    
      bool state = leds[i]["State"];
      if(state){
          Serial.print(".");
          strip->SetPixelColor(i, RgbColor(leds[i]["Color"][0], leds[i]["Color"][1], leds[i]["Color"][2]));
      }else{
          Serial.print("-");
          strip->SetPixelColor(i, black);
      }
  }
  strip->Show();
  server.send(200);   // Send HTTP status 200 (Ok) and send some text to the browser/client
}


void handleNotFound(){
  server.send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}
