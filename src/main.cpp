#include <Arduino.h>
#include "ESP8266WiFi.h"
#include <WebSocketClient.h>

//Private Config
#include "config.h"

//Pins
#define STEP_ENABLE_PIN D0
#define STEP_PIN D3
#define DIR_PIN D4 

//Global variables
const int stepsPerRev = 200; //Steps required to turn stepper 360 degrees
const int revToChangeState = 4; //Number of revolutions required to open/close blinds
WebSocketClient webSocketClient;
WiFiClient client;
int currentPos = 0;

void wifiCheck(){
    if(WiFi.status() != WL_CONNECTED){
        Serial.print("Connecting to WiFi...");
    }
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("");
}
void websocketLoad() {
    if (client.connect(WEBSOCKET_URL, 80)) {
        Serial.println("Connected");
    } else {
        Serial.println("Connection failed.");
    }
    
    webSocketClient.path = "/";
    webSocketClient.host = strdup(WEBSOCKET_URL);
    if (webSocketClient.handshake(client)) {
        Serial.println("Handshake successful");
    } else {
        Serial.println("Handshake failed.");
    }
}

//Turn stepper by N steps
//Dir CW if true, CCW if false
void turnSteps(int nSteps, bool dir){
    digitalWrite(STEP_ENABLE_PIN, LOW);
    delay(500);
    digitalWrite(DIR_PIN, dir);
    for(int x = 0; x < nSteps; x++) {
        digitalWrite(STEP_PIN,HIGH); 
        delayMicroseconds(500); 
        digitalWrite(STEP_PIN,LOW); 
        delayMicroseconds(500); 
    }
    if(dir){
        currentPos = currentPos + nSteps;
    } else {
        currentPos = currentPos - nSteps;
    }
    delay(500);
    digitalWrite(STEP_ENABLE_PIN, HIGH);
    digitalWrite(DIR_PIN, HIGH);
}


void setup() {
    Serial.begin(9600);
    pinMode(STEP_ENABLE_PIN, OUTPUT);
    pinMode(STEP_PIN, OUTPUT);
    pinMode(DIR_PIN, OUTPUT);
    digitalWrite(STEP_ENABLE_PIN, HIGH);
    digitalWrite(STEP_PIN, LOW);
    WiFi.begin(AP_SSID, AP_PASSWORD);
    wifiCheck();    
    websocketLoad();
    digitalWrite(DIR_PIN, HIGH);
}

std::vector<String> splitStringToVector(String msg, char delim){
  std::vector<String> subStrings;
  int j=0;
  for(int i =0; i < msg.length(); i++){
    if(msg.charAt(i) == delim){
      subStrings.push_back(msg.substring(j,i));
      j = i+1;
    }
  }
  subStrings.push_back(msg.substring(j,msg.length())); //to grab the last value of the string
  return subStrings;
}

void openWindow(){
    Serial.println("Opening Window!");
    if(currentPos < (stepsPerRev * revToChangeState)) {
        turnSteps((revToChangeState*stepsPerRev) - currentPos, true);
    }
}

void closeWindow(){
    Serial.println("Closing Window!");
    if(currentPos > 0) {
        turnSteps(currentPos, false);
    }
}

String data;
void loop() {
    if (client.connected()) {
        webSocketClient.getData(data);
        if (data.length() > 0) {
            Serial.print("Received data: ");
            Serial.println(data);
            std::vector<String> split_string = splitStringToVector(data.c_str(), ':');
            String msgType = split_string[0];
            String deviceName = split_string[1];
            String fname = split_string[2];
            String command = split_string[3];
            if(deviceName == "window" && fname == "power"){
                if(command == "off"){
                    closeWindow();
                }
                if(command == "on"){
                    openWindow();
                }
            }
        }
        data = "";
    } else {
        Serial.println("Client disconnected.");
        websocketLoad();
    }
}