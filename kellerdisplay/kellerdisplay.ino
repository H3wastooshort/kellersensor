#include <ESP8266HTTPClient.h>
#include <ArduinoOTA.h>
#include <strings_en.h>
#include <WiFiManager.h>
#include <LiquidCrystal.h>
#include <ArduinoJson.h>
#include <Ticker.h>

#define KELLERSENSOR ""
#define STATS_SERVER ""
uint8_t led_brightness = 64;

const char *conf_ssid = "KellerDisp";
const char *conf_pass = "randomizeme";

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(D2, D1, D5, D6, D7, D8);
HTTPClient shttp;
HTTPClient khttp;

Ticker siren;

byte WiFiSymbol[8] = {
  0b00000,
  0b01110,
  0b10001,
  0b00100,
  0b01010,
  0b00000,
  0b00100,
  0b00000
};

void wm_ap_c(WiFiManager *myWiFiManager) {
  analogWrite(D4, led_brightness);
  Serial.print(F("WiFi not found. Please configure via AP."));
  lcd.print("ERROR");
  lcd.setCursor(0, 1);
  lcd.print(F("Configure via AP"));
  delay(1000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("SSID: "));
  lcd.print(conf_ssid);
  lcd.setCursor(0, 1);
  lcd.print(F("PSK:  "));
  lcd.print(conf_pass);
}

void setup() {
  // put your setup code here, to run once:
  pinMode(D4, OUTPUT);
  Serial.begin(115200);
  lcd.begin(16, 2);
  lcd.print("WiFi... ");
  WiFiManager wm;
  wm.setConfigPortalTimeout(180);
  WiFi.hostname("KellerDisplay");
  wm.setAPCallback(wm_ap_c);
  if (!wm.autoConnect(conf_ssid, conf_pass)) {
    ESP.restart();
  }
  lcd.print("OK");
  delay(500);

  lcd.clear();
  lcd.print("OTA... ");
  ArduinoOTA.setHostname("KellerDisplay");
  ArduinoOTA.onStart([]() {
    lcd.clear();
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
    lcd.clear();
    lcd.print("OTA... " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
    lcd.clear();
    lcd.print("OTA... End");
    analogWrite(D4, led_brightness);
    delay(250);
    analogWrite(D4, 0);
    delay(250);
    analogWrite(D4, led_brightness);
    delay(250);
    analogWrite(D4, 0);
    delay(250);
    analogWrite(D4, led_brightness);
    delay(250);
    analogWrite(D4, 0);
    lcd.clear();
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    analogWrite(D4, led_brightness);
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    lcd.setCursor(0, 1);
    lcd.printf("Progress: %u%%", (progress / (total / 100)));
    delay(50);
    analogWrite(D4, 0);
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
      lcd.clear();
      lcd.print("Auth Failed");
      analogWrite(D4, led_brightness);
      delay(5000);
      analogWrite(D4, 0);
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
      lcd.clear();
      lcd.print("Begin Failed");
      analogWrite(D4, led_brightness);
      delay(5000);
      analogWrite(D4, 0);
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.print("Connect Failed");
      lcd.clear();
      lcd.println("Connect Failed");
      analogWrite(D4, led_brightness);
      delay(5000);
      analogWrite(D4, 0);
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.print("Receive Failed");
      lcd.clear();
      lcd.print("Receive Failed");
      analogWrite(D4, led_brightness);
      delay(5000);
      analogWrite(D4, 0);
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
      lcd.clear();
      lcd.print("End Failed");
      analogWrite(D4, led_brightness);
      delay(5000);
      analogWrite(D4, 0);
    }
    lcd.clear();
  });
  ArduinoOTA.begin();
  lcd.print("OK");
  delay(50);
  lcd.clear();
}

void siren_cb(/*int f_start, int f_end*/) {
  int f_start = 1500;
  int f_end = 500;
  static uint8_t cnt = 1;
  uint16_t freq = 0;
  freq =  map(cnt, 1, 100, f_start, f_end);
  tone(D0, freq);
  cnt++;
  if (cnt > 100) {
    cnt = 1;
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  analogWrite(D4, led_brightness);
  while (WiFi.status() != WL_CONNECTED) {
    lcd.createChar(0, WiFiSymbol);
    lcd.home();
    lcd.setCursor(14, 0);
    lcd.write((uint8_t)0);
    lcd.setCursor(15, 0);
    lcd.print("!");
    analogWrite(D4, led_brightness);
    delay(250);
    analogWrite(D4, 0);
    lcd.setCursor(14, 0);
    lcd.print("  ");
    delay(250);
    Serial.print(".");
  }
  lcd.setCursor(14, 0);
  lcd.print("  ");

  static bool page = false;
  page = !page;

  if (page) {
    khttp.begin(KELLERSENSOR);
    int httpCode = khttp.GET();
    if (httpCode == 200) {
      String payload = khttp.getString();
      Serial.println(payload);
      DynamicJsonDocument doc(256);
      deserializeJson(doc, payload);
      int leak = doc["leak_raw"];
      float temp = doc["temp"];
      float hum = doc["hum"];
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("Keller "));
      if (leak > 768) {
        lcd.print(F("Trocken."));
        noTone(D0);
        siren.detach();
      }
      else if (leak > 512) {
        lcd.print(F("Feucht..."));
        noTone(D0);
        siren.detach();
      }
      else if (leak > 256) {
        lcd.print(F("NASS!"));
        //tone(D0, 1000, 1000);
        siren.attach_ms(10, siren_cb);
      }
      else {
        lcd.print(F("FLUT!"));
        //tone(D0, 1500, 1000);
        siren.attach_ms(10, siren_cb);
      }
      lcd.setCursor(0, 1);
      lcd.print(temp);
      lcd.print("C ");
      lcd.print(hum);
      lcd.print("% ");
    }
    else {
      lcd.clear();
      lcd.home();
      lcd.print(F("KS HTTP Error "));
      lcd.print(httpCode);
      lcd.setCursor(0, 1);
      lcd.print(khttp.errorToString(httpCode));
    }
    khttp.end();
  }
  else {
    shttp.begin(STATS_SERVER);
    int httpCode = shttp.GET();
    if (httpCode == 200) {
      String payload = shttp.getString();
      Serial.println(payload);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("WX Voltage:"));
      lcd.setCursor(0, 1);
      lcd.print(payload);
    }
    else {
      lcd.clear();
      lcd.home();
      lcd.print(F("WX HTTP Error "));
      lcd.print(httpCode);
      lcd.setCursor(0, 1);
      lcd.print(shttp.errorToString(httpCode));
    }
    shttp.end();
  }
  analogWrite(D4, 0);
  delay(5000);
}
