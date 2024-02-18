
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Adafruit_HTU21DF.h"
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>




const char* ssid = "wifi_ssid";
const char* password = "wifi_password";

//Flask API post parameters
const char* serverAddress = "http://192.168.1.26:5000/add_data"; 

//flask server address, in the future i will need auth
String location = "Etimesgut";

//API GET parameters
const char* host = "api.openweathermap.org";
const char* apiKey = "OpenWeatherMapAPI_key"; // OpenWeatherMap API anahtarını buraya girin
const float lat = 39.9760; // Enlem koordinatını buraya girin -- etimesgut için girdim
const float lon = 32.6550; // Boylam koordinatını buraya girin
const char* exclude = "minutely,hourly,daily,alerts"; // Hariç tutulacak hava durumu verilerini buraya girin
const char* units = "metric";


WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

//Week Days
String weekDays[7]={"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

//Month names
String months[12]={"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

Adafruit_HTU21DF htu = Adafruit_HTU21DF();

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

int interval = 1000; // delay time between screens

void setup() {

  Serial.begin(115200);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }

  WiFi.begin(ssid, password);
  display.clearDisplay();
  while (WiFi.status() != WL_CONNECTED) {
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);

    display.println("Connecting WIFI");
    display.display(); 
    delay(250);
    
  }

  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println("Connected");
  display.display();
  delay(1500);


  timeClient.begin();
  timeClient.setTimeOffset(3600*3); //gmt zone in second gmt +3 means 3*3600

  if (!htu.begin()) {
    Serial.println("Check circuit. HTU21D not found!");
    while (1);
  }
  delay(2000);
  

 
}

void loop() {

    // display time
  String hours_minutes = get_hours_minutes();
  display.setTextSize(4);
  display.clearDisplay();
  display.setCursor(6, 16);
  display.println(hours_minutes);
  display.display(); 
  delay(interval*45);

  // display date
  String day_name = get_day_name();

  int day;
  String month;
  int year;
  get_date(day, month, year);
    
  display.clearDisplay();
  display.setTextSize(3);
  display.setCursor(0, 0);
  display.print(day);
  display.print(" ");
  display.println(month);
  display.println(year);
  display.setTextSize(2);
  display.print(day_name);
  display.display(); 
  delay(interval*10);
  
  // read temp and hum from sensor
  float temp = htu.readTemperature();
  float hum = htu.readHumidity();
  
  // display temp and hum from sensor
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(21, 0);
  display.println("Temp C");
  display.print("\t\t");
  display.println("Inside");
  display.setTextSize(4);
  display.println(temp);
  display.display();
  delay(interval*10);

  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(30, 0);
  display.println("Hum %");
  display.print("\t\t");
  display.println("Inside");
  display.setTextSize(4);
  display.println(hum);
  display.display(); 
  delay(interval*10);

  // display temp and hum from online source
  String outside_data = getWeatherData();
  
  /*
  int split_index = outside_data.indexOf("Nem:");
  String outside_temp = outside_data.substring(11, split_index - 3);
  String outside_hum = outside_data.substring(split_index + 5);
  */
  int split_index = outside_data.indexOf(" ");
  String outside_temp = outside_data.substring(0, split_index);
  String outside_hum = outside_data.substring(split_index + 1);

  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(21, 0);
  display.println("Temp C");
  display.print("\t\t");
  display.println("Outside");
  display.setTextSize(4);
  display.println(outside_temp);
  display.display();
  delay(interval*10);

  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(21, 0);
  display.println("Hum %");
  display.print("\t\t");
  display.println("Outside");
  display.setTextSize(4);
  display.println(outside_hum + ".00");
  display.display();
  delay(interval*10);

  String date = String(day) + month+ String(year); // şu formatı düzeltmek lazım. 

  sendDataToServer(date, hours_minutes, temp, hum, outside_temp, outside_hum, location); // location programatically gelse cok iyi olur 


  

}
/*
String get_hours_minutes() {
  int currentHour = timeClient.getHours();
  int currentMinute = timeClient.getMinutes();
  String timeString = "";
  if (currentHour < 10){
    
    timeString = "0" + String(currentHour) + ":" + String(currentMinute);
  }
  else{
    timeString = String(currentHour) + ":" + String(currentMinute);
  }
  // String timeString = "11:59";
   // Saat ve dakika değerlerini birleştir
  // https://randomnerdtutorials.com/esp8266-nodemcu-date-time-ntp-client-server-arduino/
  return timeString; // Oluşturulan saat:dakika stringini döndür
} */

String get_hours_minutes() {
  timeClient.update();
  int currentHour = timeClient.getHours();
  int currentMinute = timeClient.getMinutes();
  
  String hourString = (currentHour < 10) ? "0" + String(currentHour) : String(currentHour); // Saati düzelt
  String minuteString = (currentMinute < 10) ? "0" + String(currentMinute) : String(currentMinute); // Dakikayı düzelt
  
  String timeString = hourString + ":" + minuteString; // Saat ve dakika değerlerini birleştir
  
  return timeString; // Oluşturulan saat:dakika stringini döndür
}

String get_day_name(){
  timeClient.update();
  String weekDay = weekDays[timeClient.getDay()];
  return weekDay;  
}

void get_date(int &day, String &month, int &year){
  timeClient.update();
  time_t epochTime = timeClient.getEpochTime();
  struct tm *ptm = gmtime ((time_t *)&epochTime); 

  day = ptm->tm_mday;
  int currentMonth = ptm->tm_mon+1;
  month = months[currentMonth-1];
  year = ptm->tm_year+1900;

  // Gün ve ay bileşenlerini düzeltme
  if (day < 10) {
    // Önünde sıfır varsa kaldır
    day = day % 10;
  }
}

String getWeatherData() {
  // HTTP isteği yap
  WiFiClient client;
  const int httpPort = 80; // HTTP bağlantı noktası
  String url = "/data/2.5/weather?lat=" + String(lat) + "&lon=" + String(lon) + "&exclude=" + exclude + "&appid=" + apiKey + "&units=" + units;
  Serial.print("Requesting URL: ");
  Serial.println(url);

  if (!client.connect(host, httpPort)) {
    Serial.println("Connection failed");
    return "";
  }

  // HTTP GET isteği gönder
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");

  // HTTP yanıtını al
  String response = "";
  while (client.connected() || client.available()) {
    if (client.available()) {
      response = client.readStringUntil('\n');
      Serial.println(response);
    }
  }

  // Bağlantıyı kapat
  client.stop();

  // JSON yanıtını ayrıştır ve "temp" ve "humidity" değerlerini al
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, response);
  float temp = doc["main"]["temp"];
  int humidity = doc["main"]["humidity"];

  // Alınan değerleri bir string olarak döndür
  //return "Sıcaklık: " + String(temp) + "C, Nem: " + String(humidity) + "%";
  return String(temp) + " " + String(humidity);
}

void sendDataToServer(String date, String time, float sensor_temp, float sensor_hum, String openweather_temp, String openweather_hum, String location) {
  WiFiClient client;
  HTTPClient http;

  if (http.begin(client, serverAddress)) {
    http.addHeader("Content-Type", "application/json");

    String postData = "{\"date\":\"" + date + "\",\"time\":\"" + time + "\",\"sensor_temp\":" + String(sensor_temp) + ",\"sensor_hum\":" + String(sensor_hum) + ",\"openweather_temp\":" + String(openweather_temp) + ",\"openweather_hum\":" + String(openweather_hum) + ",\"location\":\"" + location + "\"}";

    int httpResponseCode = http.POST(postData);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("HTTP Response code: " + String(httpResponseCode));
      Serial.println("Response: " + response);
    } else {
      Serial.println("Error on HTTP request");
    }

    http.end();
  } else {
    Serial.println("Unable to connect to server");
  }
}

/*
void sendDataToServer() {
  WiFiClient client;
  HTTPClient http;

  if (http.begin(client, serverAddress)) {
    http.addHeader("Content-Type", "application/json");

    String postData = "{\"date\":\"2024-02-08\",\"time\":\"13:30:00\",\"sensor_temp\":25.8,\"sensor_hum\":49.5,\"openweather_temp\":23.0,\"openweather_hum\":45.2,\"location\":\"Bayburt\"}";

    int httpResponseCode = http.POST(postData);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("HTTP Response code: " + String(httpResponseCode));
      Serial.println("Response: " + response);
    } else {
      Serial.println("Error on HTTP request");
    }

    http.end();
  } else {
    Serial.println("Unable to connect to server");
  }
}
*/
