#include <WiFiClient.h>
//#include <ESP32WebServer.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <ESPmDNS.h>
#include "SPIFFS.h"
#include "FastLED.h"
#include "holobot.h"

// objects and globals
WiFiMulti wifi_multi;
CRGB leds[LED_COUNT];
AsyncWebServer server(80);
File image_file;
holo_anim_params_t stripe_anim_params;
holo_picture_params_t picture_params;
holo_last_error_t last_error;
holo_state_t state = IDLE;
long int last_batt_check = 0;
int upload_counter = 0;

// gamma correction
extern const uint8_t gamma8[];

// info HTML
extern String info_html;

// blink the internal LED
void blink() {
    digitalWrite(STATUS_PIN, HIGH);
    delay(100);
    digitalWrite(STATUS_PIN, LOW);
    delay(100);
}


// read battery voltage
float holo_get_battery() {
    // enable read
    digitalWrite(BATT_EN_PIN, HIGH);
    delay(1000); // wait for signals to stabilize
    // read analog pin
    uint16_t analog = analogRead(BATT_V_PIN);
    digitalWrite(BATT_EN_PIN, LOW);
    Serial.print("Analog: ");
    Serial.println(analog);
    // make conversion, reference is 3.3V
    float voltage_measured = (analog * 3.3)/4096.0;
    return voltage_measured * BATT_DIVISION * BATT_CORRECTION;
}

// Server handles
void handle_root(AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "OK");
    state = BLINK;
}


void handle_not_found(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "404 NOT FOUND");
    state = BLINK;
}

void handle_stripes(AsyncWebServerRequest *request) {
    stripe_anim_params.time = 1000;
    stripe_anim_params.r = 255;
    stripe_anim_params.g = 255;
    stripe_anim_params.b = 255;
    stripe_anim_params.width = 2;
    stripe_anim_params.steps = 0;
    stripe_anim_params.step_delay = 20;
    
    // parse parameters
    if (request->hasParam("t")) {
        stripe_anim_params.time = request->getParam("t")->value().toInt();
    }
    if (request->hasParam("w")) {
        stripe_anim_params.width = request->getParam("w")->value().toInt();
    }
    if (request->hasParam("s")) {
        stripe_anim_params.steps = request->getParam("s")->value().toInt();
    }
    if (request->hasParam("d")) {
        stripe_anim_params.step_delay = request->getParam("d")->value().toInt();
    }
    if (request->hasParam("r")) {
        stripe_anim_params.r = gamma8[request->getParam("r")->value().toInt()];
    }
    if (request->hasParam("g")) {
        stripe_anim_params.g = gamma8[request->getParam("g")->value().toInt()];
    }
    if (request->hasParam("b")) {
        stripe_anim_params.b = gamma8[request->getParam("b")->value().toInt()];
    }
            
    state = STRIPES;
    request->send(200, "text/plain", "OK");   
    
}

void handle_rainbow(AsyncWebServerRequest *request) {
    stripe_anim_params.time = 1000;
    stripe_anim_params.steps = 0;
    stripe_anim_params.step_delay = 20;
    
    // parse parameters
    if (request->hasParam("t")) {
        stripe_anim_params.time = request->getParam("t")->value().toInt();
    }
    if (request->hasParam("s")) {
        stripe_anim_params.steps = request->getParam("s")->value().toInt();
    }
    if (request->hasParam("d")) {
        stripe_anim_params.step_delay = request->getParam("d")->value().toInt();
    }
                                                        
    state = RAINBOW;
    request->send(200, "text/plain", "OK");     
}
    

void handle_picture(AsyncWebServerRequest *request) {
    // default values
    picture_params.anim_delay = DEFAULT_SPEED;
    picture_params.filename = "/test.bmp";

    // parse parameters
    if (request->hasParam("d")) {
        picture_params.anim_delay = request->getParam("d")->value().toInt();
    }
    if (request->hasParam("d")) {
        picture_params.filename = request->getParam("f")->value();
        picture_params.filename = "/" + picture_params.filename;
    }
    
    request->send(200, "text/plain", "OK");

    state = PICTURE;
}

void handle_brightness(AsyncWebServerRequest *request) {
    int brightness;
    // parse parameters
    if (request->hasParam("b")) {
        brightness = request->getParam("b")->value().toInt();
        FastLED.setBrightness(brightness);
        request->send(200, "text/plain", "OK");
    } else {
        request->send(500, "text/plain", "Missing parameter");
    }

    state = IDLE;
}

void handle_list(AsyncWebServerRequest *request) {
    String resp = "";
    File root = SPIFFS.open("/");
    if(!root){
        request->send(500, "text/plain", "Can't open /");
        return;
    }
                      
    // search for files in current directory
    File file = root.openNextFile();
    Serial.println("Files:");
    while(file){
        Serial.println(file.name());
        if(file.isDirectory()){
            resp = resp + "  DIR : ";
            resp = resp + file.name() + "\n";
        } else {
            resp = resp + "  FILE: ";
            resp = resp + file.name();
            resp = resp + "\tSIZE: ";
            resp = resp + file.size() + "\n";
        }
        file = root.openNextFile();
    }
    request->send(200, "text/plain", resp);
    state = BLINK;
}


void handle_info(AsyncWebServerRequest *request) {
    String output_html = info_html;
    output_html.replace("[[[BATTERY]]]", String(holo_get_battery(), 3));
    output_html.replace("[[[ERROR]]]", "" + String(last_error.code) + " : " + last_error.desc);
    request->send(200, "text/html", output_html.c_str());
    state = BLINK;
}

void handle_test(AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "OK");
    state = LED_TEST;
}
                                                        
void handle_upload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
  if (!index) {
    Serial.println("Upload start");
    // open the file on first call and store the file handle in the request object
    request->_tempFile = SPIFFS.open("/" + filename, "w");
    upload_counter = 0;
    // clear bar
    fill_solid(leds, LED_COUNT, CRGB( 0, 0, 0));
    FastLED.show();
  }

  if (len) {
    // stream the incoming chunk to the opened file
    request->_tempFile.write(data, len);
    leds[upload_counter] = CRGB(0, 50, 0);
    FastLED.show();
    upload_counter++;
    if (upload_counter == LED_COUNT) {
        upload_counter = 0;
        fill_solid(leds, LED_COUNT, CRGB( 0, 0, 0));
        FastLED.show();
    }
  }

  if (final) {
    Serial.println("Upload Complete: " + String(filename) + ", size: " + String(index + len));
    // close the file handle as the upload is now done
    request->_tempFile.close();
    request->redirect("/");
    // clear bar
    fill_solid(leds, LED_COUNT, CRGB( 0, 0, 0));
    FastLED.show();
    state = BLINK;
  }
}

void handle_status(AsyncWebServerRequest *request) {
    String error_msg = "" + String(last_error.code) + '\n' + last_error.desc; 
    Serial.println(error_msg);
    request->send(200, "text/plain", "" + error_msg);
    state = BLINK;
}

void handle_battery(AsyncWebServerRequest *request) {
    String batt_msg = "" + String(holo_get_battery()) + '\n'; 
    Serial.println(batt_msg);
    request->send(200, "text/plain", "" + batt_msg);
    state = BLINK;
}


///////////// SETUP ////////////////

void setup()
{  
    // Debug
    Serial.begin(115200);

    // GPIO
    pinMode(STATUS_PIN, OUTPUT);
    pinMode(BATT_EN_PIN, OUTPUT);
    blink();
    
    digitalWrite(STATUS_PIN, LOW);
    digitalWrite(BATT_EN_PIN, LOW);
    
    analogReadResolution(12);
    
    // Initialize SPIFFS
    if(!SPIFFS.begin(true)){
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }
    
    // initialize LEDs
    FastLED.addLeds<NEOPIXEL, STRIPE_PIN>(leds, LED_COUNT);
    FastLED.setMaxPowerInVoltsAndMilliamps(5, MAX_MILLIAMPS);
    FastLED.showColor(CRGB::Black);
    //led_test();

    // start wifi
    wifi_multi.addAP("HIRIKILABS", "Hal_2016");
    wifi_multi.addAP("REAMDE", "holahola");
    wifi_multi.addAP("SSID3", "password3");
    Serial.println("Connecting to WiFi...");

    // Wait for connection
    if(wifi_multi.run() == WL_CONNECTED) {
        Serial.println("");
        Serial.print("Connected to ");
        Serial.println(WiFi.SSID());
    }
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    // handle URIs  
    server.on("/", handle_root);
    server.on("/test", handle_test);
    server.on("/picture", handle_picture);
    server.on("/list", handle_list);
    server.on("/stripes", handle_stripes);
    server.on("/rainbow", handle_rainbow);
    server.on("/info", handle_info);
    server.on("/status", handle_status);
    server.on("/battery", handle_battery);
    server.on("/brightness", handle_brightness);
    server.onNotFound(handle_not_found);
    server.serveStatic("/test.bmp", SPIFFS, "/test.bmp");
    server.serveStatic("/style.css", SPIFFS, "/style.css");
    server.serveStatic("/favicon.ico", SPIFFS, "/favicon.ico");
    server.onFileUpload(handle_upload);    

    // start MDNS
    if (MDNS.begin(MDNS_DOMAIN)) {
        Serial.println("MDNS responder started");
    }
    
    // start HTTP server
    server.begin();
    Serial.println("HTTP Server started");
    
    // error codes
    last_error.code = NO_ERROR;
    last_error.desc = "No error";

    // reset counter
    last_batt_check = millis();
    
    // everythin ok, blink two times;
    blink(); blink();
    
    // initial state
    state = IDLE;
}

///////////// LOOP ////////////////

void loop()
{
    // requests are handled by AsyncWebServer, 
    // we just need to implement a state machine
    switch(state) {
        case IDLE:
            yield();
            break;
        case LED_TEST:
            holo_led_test();
            state = IDLE;
            break;
        case STRIPES:
            holo_stripes();
            state = IDLE;
            break;
        case RAINBOW:
            holo_rainbow();
            state = IDLE;
            break;
        case PICTURE:
            holo_picture();
            state = IDLE;
            break;
        case LOW_BATT:
            holo_low_batt();
            state = IDLE;
            break;
        case BLINK:
            blink();
            state = IDLE;
            break;
    }
    
    // check battery
    if (millis() > last_batt_check + BATT_CHECK_TIME) {
        float batt = holo_get_battery();
        if (batt < BATT_EMPTY) {
            //state = LOW_BATT;
        }
        last_batt_check = millis();
    }
}
