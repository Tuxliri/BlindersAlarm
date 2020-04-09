#include <WiFi.h>
#include <WebServer.h>
#include <time.h>
#include <Preferences.h>

Preferences prefs;

const char* ssid     = "Casa";
const char* password = "056b04fcde";
const char* ntpServer = "europe.pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;

WebServer server(80);

uint8_t relaypin = 26;


struct tm alarmtime;
struct tm timeinfo;
void setup()
{
  Serial.begin(115200);
  pinMode(relaypin, OUTPUT);
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

  server.on("/AlarmTime", handleAlarmTime);

  server.begin();                                    //start the server
  Serial.println("Server listening");

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  server.on("/Status", handleStatus);   //Associate the handler function to the path

  alarmtime.tm_hour = prefs.getInt("alarmhour", -1);
  alarmtime.tm_min = prefs.getInt("alarmmin", -1);

  Serial.println("Alarm set from flash to: ");
  Serial.print(alarmtime.tm_hour);
  Serial.print(':');
  Serial.print(alarmtime.tm_min);


}


void loop()
{
  server.handleClient();    //Handling of incoming requests

  getLocalTime(&timeinfo);

  Serial.println("Actual time:" + (String)timeinfo.tm_hour + ':' + (String)timeinfo.tm_min);

  if (timeinfo.tm_hour == alarmtime.tm_hour && timeinfo.tm_min == alarmtime.tm_min && timeinfo.tm_sec < 10) {
    digitalWrite(relaypin, HIGH);  //raiseBlinders();
    Serial.println("Blinders raised");
    delay(20000);
    digitalWrite(relaypin, LOW);
  }

  delay(1000);


}

void handleAlarmTime() { //Handler
  String message = "Set alarm time: ";
  message += server.arg(0);        //Get number of parameteres
  message += ":";                 //Add separator
  message += server.arg(1);
  alarmtime.tm_hour = server.arg(0).toInt();
  alarmtime.tm_min = server.arg(1).toInt();

  prefs.putInt("alarmhour", alarmtime.tm_hour);
  prefs.putInt("alarmmin", alarmtime.tm_min);
  //prefs.end();
  
  Serial.println("Alarm time set = " + (String)alarmtime.tm_hour + ':' + (String)alarmtime.tm_min);
  server.send(200, "text/plain", message);     //Response to the HTTP request
}

void handleStatus() {

  String message = "Alarm is set for: ";
  message += (String)alarmtime.tm_hour;
  message += ':';
  message += (String)alarmtime.tm_min;


  server.send(200, "text/plain", message);

}
