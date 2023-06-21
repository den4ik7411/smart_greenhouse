/*
Here was used:
      - ESP8266
      - DHT11
      - LCD i2c display
      - LED
      - LM393
      - Pump pump
      - Power board
      - Voltage step down board
      - 5v relay
*/
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ThingSpeak.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <UniversalTelegramBot.h>

// number of your LED 
#define NUMPIXELS 10

#define ssid_s "SSID"
#define password_s "YOUR_SSID_PASSWORD"
#define myWriteAPIKey "THINGSPEAK_API_KEY"
#define BOT_TOKEN "YOUR_BOT_TOKEN"


const unsigned long BOT_MTBS = 1000;
X509List cert(TELEGRAM_CERTIFICATE_ROOT);
WiFiClient secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);
unsigned long bot_lasttime;
LiquidCrystal_I2C lcd(0x27, 16, 2);
#define pomp_1 D0
#define rainPin A0
#define thresholdValue 28
int last_temperature = 0;
int last_humidity = 0;
int last_sensorValue = 0;
bool GrSt = false;
int grhum = 0;
int counter = 0;
#define DHTPIN D3
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
WiFiClient client;
unsigned long myChannelNumber = 632407;
int temperature, humidity, sensorValue;
Adafruit_NeoPixel pixels(NUMPIXELS, D5, NEO_GRB + NEO_KHZ800);


void handleNewMessages(int numNewMessages) {
  Serial.print(numNewMessages);
  int grhum = map(1024 - analogRead(rainPin), 0, 1024, 0, 100);
  int sensorValue = grhum;
  int temperature = dht.readTemperature();
  int humidity = dht.readHumidity();

  for (int i = 0; i < numNewMessages; i++) {

    String chat_id = String(bot.messages[i].chat_id);
    String text = bot.messages[i].text;
    String from_name = bot.messages[i].from_name;

    String keyboardJson = "[[{ \"text\" : \"Temperature\", \"callback_data\" : \"/temp\" }],[{ \"text\" : \"Humidity\", \"callback_data\" : \"/humid\" }], [{ \"text\" : \"Ground humidity\", \"callback_data\" : \"/grhum\" }], [{ \"text\" : \"All data\", \"callback_data\" : \"/stats\" }]]";
    
    if (from_name == "") {
      from_name = "Guest";
    }
    if (text == "/temp") {
      bot.sendMessageWithInlineKeyboard(chat_id, "<b><em>Temperature</em> : " + String(temperature) + "</b>C", "HTML", keyboardJson);
    }

    if (text == "/humid") {
      bot.sendMessageWithInlineKeyboard(chat_id, "<b><em>Humidity</em> : " + String(humidity) + "</b>%", "HTML", keyboardJson);
    }
    if (text == "/grhum") {
      bot.sendMessageWithInlineKeyboard(chat_id, "<b><em>Gr. humidity</em> : " + String(sensorValue) + "</b>%", "HTML", keyboardJson);
    }

    if (text == "/stats") {
      String stats = "<em>Temperature</em> : <b>" + String(temperature) + "C</b>\n\n";
      stats += "<em>Humidity</em> : <b>" + String(humidity) + "%</b>\n\n";
      stats += "<em>G. Humidity</em> : <b>" + String(sensorValue) + "%</b>";
      bot.sendMessageWithInlineKeyboard(chat_id, stats, "HTML", keyboardJson);
    }
    if (text == "/rgb") {
      pixels.begin();
      pixels.clear();
      for (int i;i<=3;i++){
        if (i == 1){
          for (int j;j<=3;j++){
            pixels.setPixelColor(j, pixels.Color(255, 0, 0));
          }
        }
        else if (i == 2){
          for (int j;j<=3;j++){
            pixels.setPixelColor(j, pixels.Color(0, 255, 0));
          }
        }
        else if (i == 3){
          for (int j;j<=3;j++){
            pixels.setPixelColor(j, pixels.Color(0, 0, 255));
          }
        }
      }
      pixels.show();
    }

    if (text == "/start")
    {
      String welcome = "Welcome to GreenhouseBot <em>" + from_name + "</em>.\n\n";
      welcome += "/temp : Returns current <b><em>temperature</em></b>\n";
      welcome += "/humid : Returns current <b><em>humidity</em></b>\n";
      welcome += "/stats : Returns current <b><em>temp and hum</em></b>\n";
      welcome += "/grhum : Returns current <b><em>gr. humidity</em></b>";
      bot.sendMessageWithInlineKeyboard(chat_id, welcome, "HTML", keyboardJson);
    }
  }
}
void bot_checka() {
  if (millis() - bot_lasttime > BOT_MTBS) {
    Serial.println("On a tick bot");
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages) {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
  }
  bot_lasttime = millis();
  }
}

void checka() {

  grhum = map(1024 - analogRead(rainPin), 0, 1024, 0, 100);
  sensorValue = grhum;
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();

  if (temperature != last_temperature) {
    ThingSpeak.writeField(myChannelNumber, 1, temperature, myWriteAPIKey);
    last_temperature = temperature;
  }
  else if (humidity != last_humidity) {
    ThingSpeak.writeField(myChannelNumber, 2, humidity, myWriteAPIKey);
    last_humidity = humidity;
  }
  else if (sensorValue != last_sensorValue) {
    ThingSpeak.writeField(myChannelNumber, 3, sensorValue, myWriteAPIKey);
    sensorValue = last_sensorValue;
  }

  Serial.print(sensorValue);

  if (GrSt == true) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Ground Humidity");
    lcd.setCursor(0, 1);
    lcd.print(String(grhum) + "%");
    GrSt = false;
  }
  else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Temp      Humid");
    lcd.setCursor(0, 1);
    lcd.print(temperature);
    lcd.print("C");
    lcd.setCursor(10, 1);
    lcd.print(humidity);
    lcd.print("%");
    GrSt = true;
  }

  if (grhum > thresholdValue) {
    Serial.println(" Полив не нужен");
  }
  else if (grhum == 0) {
    Serial.println("Датчику плохо");
  }
  else {
    Serial.println(" Пора полить");
    digitalWrite(pomp_1, 0);
    delay(1000);
    digitalWrite(pomp_1, 1);
  }

}


void setup() {
  Serial.begin(9600);
  pixels.begin();
  pixels.clear();
  Serial.print("Connecting to ");
  Serial.println(ssid_s);
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting to");
  lcd.setCursor(0, 1);
  lcd.print(ssid_s);
  WiFi.begin(ssid_s, password_s);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.print("\nWiFi connected. IP address: ");
  Serial.println(WiFi.localIP());
  configTime(0, 0, "time.cloudflare.com");
  ThingSpeak.begin(client);

  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(127, 229, 26));
    pixels.show();
    delay(500);
  }
  dht.begin();
  pinMode(rainPin, INPUT);
  pinMode(pomp_1, OUTPUT);
  digitalWrite(pomp_1, 1);
  checka();
}

void loop(){
 checka()
 bot_checka()
}
