#include <WiFi.h>
#include <HTTPClient.h>

#include <OneWire.h>
#include <DallasTemperature.h>

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const char* ssid     = "__tu_wpisz_ssid_sieci__";
const char* password = "__tu_wpisz_haslo__";

String url = "__tu_url_do_zdalnego_systemu__";

unsigned long lastTime = 0;
unsigned long timerDelay = 5 * 1000  ; //60*1000; // 1 minuta

const int oneWireBus = 4;  

OneWire oneWire(oneWireBus);

DallasTemperature sensors( &oneWire );

DeviceAddress Thermometer;

void printAddress(DeviceAddress deviceAddress) { 
  for (uint8_t i = 0; i < 8; i++) {
    Serial.print("0x");
    if (deviceAddress[i] < 0x10) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
    if (i < 7) Serial.print(", ");
  }
  Serial.println("");
}

void setup() {

    pinMode(LED_BUILTIN, OUTPUT);
    Serial.begin(115200);

    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    
    sensors.begin();

    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
        Serial.println(F("SSD1306 allocation failed"));
   //     for(;;); // Don't proceed, loop forever
    }
    Serial.println("lcd done");
    delay(2000); // Pause for 2 second
    display.clearDisplay();
    Serial.println("lcd operations");
    display.setTextColor(SSD1306_WHITE);        // Draw white text   
    
    // sprawdzenie sensorów
    /*byte i;
    byte addr[8];
  
    if (!oneWire.search( addr )) {
        Serial.println(" No more addresses.");
        Serial.println();
        oneWire.reset_search();
        return;
    }
    Serial.print(" ROM =");
    for (i = 0; i < 8; i++) {
        Serial.write(' ');
        Serial.print(addr[i], HEX);
    }*/
/*
    // locate devices on the bus
  Serial.println("Locating devices...");
  Serial.print("Found ");
  int deviceCount = sensors.getDeviceCount();
  Serial.print(deviceCount, DEC);
  Serial.println(" devices.");
  Serial.println("");
  
  Serial.println("Printing addresses...");
  for (int i = 0;  i < deviceCount;  i++)
  {
    Serial.print("Sensor ");
    Serial.print(i+1);
    Serial.print(" : ");
    sensors.getAddress(Thermometer, i);
    printAddress(Thermometer);
  }

  */   
}

char s [32] = "";
int cntr = 0;

uint8_t sensor1[8] = { 0x10, 0x6F, 0xF1, 0x72, 0x03, 0x08, 0x00, 0x4A }; // in
uint8_t sensor2[8] = { 0x10, 0x1E, 0x1B, 0x73, 0x03, 0x08, 0x00, 0x0D }; // out

bool inDoor = true;

bool selToPost = true;

int requestCntr = 0;

float tempIndoor = 0;
float tempOutdoor = 0;

void PostTempData( float f, String name ) {
  char s [32] = "";
  sprintf( s, "%5.2f", f );
  String ss(s);
  ss.trim();
  String endPoint = url + "?sensor=" + name + "&value=" + ss;
  Serial.println( endPoint );
  
  HTTPClient http;            
  http.begin( endPoint.c_str() );
  Serial.println( "Remote call" );        
  int httpResponseCode = http.GET();          
  if (httpResponseCode>0) {
    Serial.print( "HTTP Response code: " );
    Serial.println( httpResponseCode );
    String payload = http.getString();
    Serial.println( payload );
  }
  else {
    Serial.print( "HTTP Error code: " );
    Serial.println( httpResponseCode );
  }
  http.end();  
} 

void loop() {
  
    while ( 1 ) {
        display.clearDisplay();
        display.setTextSize(1);
        display.setCursor(0,4);
        sprintf( s, "%03d", cntr++ );
        display.println( String(s) + " IP:" + WiFi.localIP().toString() );                

        display.setCursor(0,52);
        display.println( inDoor ? "       INDOOR " :  "OUTDOOR       " );                


        sprintf(s, "%5.1f", inDoor ? tempIndoor : tempOutdoor );
        display.setCursor(0,22);    
        display.setTextSize(3);             
        display.println( String(s) + String((char)247) + "C");        
                
        display.display();

        if ( ( millis() - lastTime) > timerDelay) {            
            lastTime = millis();
            inDoor ^= true;
            if( WiFi.status() != WL_CONNECTED){
                Serial.println("No WiFi");
                ESP.restart();
            }
            sensors.requestTemperatures();
            if ( inDoor ) {
              tempIndoor = sensors.getTempC( sensor1 );              
            }
            else {
              tempOutdoor = sensors.getTempC( sensor2 );                            
            }
            
            requestCntr++; // co 5 sek
            if ( requestCntr > 6 ){
                requestCntr = 0;
                selToPost ^= true;
                if ( selToPost ) {
                  PostTempData(tempIndoor, "ESP32IN");
                }
                else {
                  PostTempData(tempOutdoor, "ESP32OUT");                  
                }
            }            
        }  
    } // while        
} // loop
