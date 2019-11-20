#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include"uptime_formatter.h"

AsyncWebServer server(80);

// REPLACE WITH YOUR NETWORK CREDENTIALS

const char* PARAM_INT = "inputInt";
const char* PARAM_FLOAT = "inputInt2";
String webtime = "system_up_time";

// HTML web page to handle 2 input fields (inputInt, inputInt2)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>Water Pump Settings Form</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <script>
    function submitMessage() {
      alert("Saved value to ESP SPIFFS");
      setTimeout(function(){ document.location.reload(false); }, 500);   
    }
  </script></head><body>

  <form action="/get" target="hidden-form">
    Pump RunTime (current value = %inputInt%ms): <input type="number " name="inputInt">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form><br>
  <form action="/get" target="hidden-form">
    Pump idleTime (current value = %inputInt2%ms): <input type="number " name="inputInt2">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form>
  <br>
  <form action="/get" target="hidden-form">
    System Up Time = %system_up_time%
  <iframe style="display:none" name="hidden-form"></iframe>
</body></html>)rawliteral";

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

String readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\r\n", path);
  File file = fs.open(path, "r");
  if(!file || file.isDirectory()){
    Serial.println("- empty file or failed to open file");
    return String();
  }
  Serial.println("- read from file:");
  String fileContent;
  while(file.available()){
    fileContent+=String((char)file.read());
  }
  Serial.println(fileContent);
  return fileContent;
  file.close();
}

void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\r\n", path);
  File file = fs.open(path, "w");
  if(!file){
    Serial.println("- failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
  file.close();
}

// Replaces placeholder with stored values
String processor(const String& var){
  //Serial.println(var);
  if(var == "inputInt"){
    return readFile(SPIFFS, "/inputInt.txt");
  }
  else if(var == "inputInt2"){
    return readFile(SPIFFS, "/inputInt2.txt");
  }
    else if(var == "system_up_time"){
    return readFile(SPIFFS, "/time.txt");
  }
  return String();
}

void setup() {

  pinMode(15, OUTPUT);    // sets the digital pin 5 as output
  
  Serial.begin(115200);

   WiFi.softAP("GS-System", "winning1");
  Serial.println();
  // Initialize SPIFFS
  #ifdef ESP32
    if(!SPIFFS.begin(true)){
      Serial.println("An Error has occurred while mounting SPIFFS");
      return;
    }
  #else
    if(!SPIFFS.begin()){
      Serial.println("An Error has occurred while mounting SPIFFS");
      return;
    }
  #endif


  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  // Send web page with input fields to client
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  // Send a GET request to <ESP_IP>/get?inputString=<inputMessage>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;

    // GET inputInt value on <ESP_IP>/get?inputInt=<inputMessage>
    if (request->hasParam(PARAM_INT)) {
      inputMessage = request->getParam(PARAM_INT)->value();
      writeFile(SPIFFS, "/inputInt.txt", inputMessage.c_str());
    }
    // GET inputInt2 value on <ESP_IP>/get?inputInt2=<inputMessage>
    else if (request->hasParam(PARAM_FLOAT)) {
      inputMessage = request->getParam(PARAM_FLOAT)->value();
      writeFile(SPIFFS, "/inputInt2.txt", inputMessage.c_str());
    }
    else {
      inputMessage = "No message sent";
    }

    Serial.println(inputMessage);
    request->send(200, "text/text", inputMessage);
  });
  server.onNotFound(notFound);
  server.begin();
}

void loop() {
  
  String newUptime = uptime_formatter::getUptime();
  writeFile(SPIFFS, "/time.txt", newUptime.c_str());
  String up_time = readFile(SPIFFS, "/time.txt");
  webtime = up_time;
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());
  int yourInputInt = readFile(SPIFFS, "/inputInt.txt").toInt();
  Serial.print("*** Your Pump Runtime: ");
  int newPumpTime = yourInputInt * 60000;
  Serial.println(newPumpTime);
  
  int yourinputInt2 = readFile(SPIFFS, "/inputInt2.txt").toFloat();
  Serial.print("*** Your Pump idleTime: ");
  int newIdleTime = yourinputInt2 * 60000;
  Serial.println(newIdleTime);
  digitalWrite(15, HIGH); // sets the digital pin 5 on
  delay(newPumpTime);           // waits for a 1 minute
  digitalWrite(15, LOW);  // sets the digital pin 5 off
  delay(newIdleTime);            // waits for a 19 minutes

  
  
}
