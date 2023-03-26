#include <Arduino.h>

#include <WiFi.h>
#include <WebServer.h>
#include <time.h>
#include <Preferences.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

# define LOWERPIN 27

Preferences prefs;

const char* ssid     = "Casa";
const char* password = "056b04fcde";
const char* ntpServer = "europe.pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;

WebServer server(80);
uint8_t lowerpin = 27;
uint8_t relaypin = 26;

String sb = "<!DOCTYPE html>"
              "<html>"
              "<body>"
              "<h1>Blinders alarm esp32</h1>"
              "<h1>Set the <strong>alarm time</strong> for the blinders to raise:</h1>"
              "<form method =\"POST\" action=\"setAlarm\" >"
              "  <label for=\"appt\">Select an alarmtime:</label>"
              "  <input type=\"time\" id=\"appt\" name=\"appt\">"
              "  <input type=\"submit\" value=\"Submit\">"
              "</form>"
              "<h2>Raise the blinders now</h2>"
              "<form action=\"Raise\" method=\"POST\">"
              " <button type=\"submit\"> Raise </button>"
              "</form>"
              "<h2>Lower the blinders now</h2>"
              "<form action=\"Lower\" method=\"POST\">"
              " <button type=\"submit\"> Lower </button>"
              "</form>"
              "<p><strong>Note:</strong> type=\"time\" is not supported in Safari or Internet Explorer 12 (or earlier).</p>"
              "</body>";

void handleAlarm();
void handleStatus();
void raiseBlinders();
void lowerBlinders();
void setAlarmTime();

#define ONBOARD_LED 2
struct tm alarmtime;
struct tm timeinfo;

void setup()
{
    Serial.begin(115200);

    // Set up relay pins
    // Raise pin
    pinMode(relaypin, OUTPUT);
    digitalWrite(relaypin,HIGH);
    // Lower pin
    pinMode(lowerpin, OUTPUT);
    digitalWrite(lowerpin,HIGH);
    
    prefs.begin("alarmhour");
    prefs.begin("alarmmin");

    // We start by connecting to a WiFi network

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
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    /* Handlers for client calls */

    server.on("/", handleAlarm);
    server.on("/setAlarm", HTTP_POST, setAlarmTime);
    server.on("/Raise", HTTP_POST, raiseBlinders);
    server.on("/Lower", HTTP_POST, lowerBlinders);
    server.on("/Status", handleStatus);   //Associate the handler function to the path

    //start the server
    
    server.begin();
    Serial.println("Server listening");

    //time configurations (alarm and system time)
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);


    alarmtime.tm_hour = prefs.getInt("alarmhour", -1);
    alarmtime.tm_min = prefs.getInt("alarmmin", -1);

    Serial.println("Alarm set from flash to: ");
    Serial.print(alarmtime.tm_hour);
    Serial.print(':');
    Serial.print(alarmtime.tm_min);

    //Arduino IDE wifi programming handler
    // Set OTA port to 3232
    ArduinoOTA.setPort(3232);

    ArduinoOTA
    .onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
        else // U_SPIFFS
        type = "filesystem";

        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
        Serial.println("Start updating " + type);
    })
    .onEnd([]() {
        Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

    ArduinoOTA.begin();

    pinMode(ONBOARD_LED, OUTPUT);
    delay(1000);
    digitalWrite(ONBOARD_LED, LOW);
}


void loop()
{
  server.handleClient();    //Handling of incoming requests

  getLocalTime(&timeinfo);

  Serial.println("Actual time:" + (String)timeinfo.tm_hour + ':' + (String)timeinfo.tm_min);

  if (timeinfo.tm_hour == alarmtime.tm_hour && timeinfo.tm_min == alarmtime.tm_min && timeinfo.tm_sec < 10) {
    raiseBlinders();

  };

  ArduinoOTA.handle();
  


}

void setAlarmTime() { //Handler
  String message = "Alarm is set for: ";
  String times = server.arg(0);
  String minutes = "-1" ;
  String hours = "-1";
  minutes[0] = times[3];
  minutes[1] = times[4];
  hours[0] = times[0];
  hours[1] = times[1];
  alarmtime.tm_min = minutes.toInt();
  alarmtime.tm_hour = hours.toInt();

  prefs.putInt("alarmhour", alarmtime.tm_hour);
  prefs.putInt("alarmmin", alarmtime.tm_min);
  //prefs.end();

  if (alarmtime.tm_hour < 10) {
    message += '0';
  }
  message += (String)alarmtime.tm_hour;
  message += ':';
  if (alarmtime.tm_min < 10) {
    message += '0';
  }
  message += (String)alarmtime.tm_min;
  server.send(200, "text/plain", message);

  
  Serial.println(times);

};

void handleStatus() {

  String message = "Alarm is set for: ";
  message += (String)alarmtime.tm_hour;
  message += ':';
  message += (String)alarmtime.tm_min;


  server.send(200, "text/plain", message);

};

void handleAlarm() {
  server.send(
    200,
    "text/html",
    sb + 
    + "\nThe time is: " + (String)timeinfo.tm_hour + ':' + (String)timeinfo.tm_min
    + "\nAlarm is set for: " + (String)alarmtime.tm_hour + ':' + (String)alarmtime.tm_min
    );

};

void raiseBlinders() {
  digitalWrite(relaypin, LOW);  //raiseBlinders();
  Serial.println("Blinders raised");
  handleAlarm();
  delay(20000);
  digitalWrite(relaypin, HIGH);
};

void lowerBlinders() {
  digitalWrite(lowerpin, LOW);  //lowerBlinders();
  Serial.println("Blinders lowered");
  handleAlarm();
  delay(20000);
  digitalWrite(lowerpin, HIGH);
};
