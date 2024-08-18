/*
  Compile this with MiniCore (LTO enabled) to save space.

   Beware of code which is disabled to save PGM space
   Removed things include:
   RTC
   DHCP
   EEPROM
*/


#include <DHT.h>
#include <SPI.h>
#include <Ethernet.h>
#include <avr/wdt.h>
//#include <EEPROM.h>
#include <SD.h>
#include <EthernetUdp.h>
#include <NTPClient.h>
//#include <Wire.h>
//#include <DS1307RTC.h>

#include "config.h"
#include "strings.h"

float temp = -999;
float hum = -999;
int leak = 9999;
bool leak_detected = false;

bool sound = true;

DHT dht(DHT_PIN, DHT_TYPE);

#define WEBBUF 64
EthernetServer server(80);

EthernetUDP nUDP;
NTPClient ntp(nUDP, NTP_SERVER);

#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char* sbrk(int incr);
#else   // __ARM__
extern char* __brkval;
#endif  // __arm__

int freeMemory() {
  char top;
#ifdef __arm__
  return &top - reinterpret_cast<char*>(sbrk(0));
#elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
  return &top - __brkval;
#else   // __arm__
  return __brkval ? &top - __brkval : &top - __malloc_heap_start;
#endif  // __arm__
}

void setup() {
  wdt_enable(WDTO_8S);
  wdt_reset();

  // put your setup code here, to run once:
  Serial.begin(9600);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  //Beeper
  pinMode(BEEPER_PIN, OUTPUT);

  //Leak Sensor
  pinMode(WATER_PIN, INPUT_PULLUP);

  //Flasher
  pinMode(FLASHER_PIN, OUTPUT);

  //Virtual power
  pinMode(7, OUTPUT);
  digitalWrite(7, LOW);
  pinMode(3, OUTPUT);
  digitalWrite(3, LOW);

  dht.begin();
  wdt_reset();

  if (!SD.begin(4))
    for (uint8_t i = 0; i < 10; i++) {
      Serial.println(F("SD error!"));
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
      delay(500);
    }
  wdt_reset();

  Ethernet.begin(ETH_MAC, ETH_IP, ETH_DNS, ETH_GATEWAY, ETH_SUBNET);
  /* DOES NOT WORK! (no idea why)
     throws error even tho hardware is connected and working
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println(F("Ethernet error!"));
    wdt_enable(WDTO_4S);
    while (true) {
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
      delay(500);
    }
    }*/

  // print your local IP address:
  Serial.print(F("IP: "));
  Serial.println(Ethernet.localIP());
  wdt_reset();

  ntp.begin();
  ntp.update();
  Serial.println(F("got time"));
  /*if (RTC.chipPresent()) {
    //Check if NTP valid, then write
    if (ntp.getEpochTime() > 10000) {
      RTC.set(ntp.getEpochTime());
    }
    }*/
  wdt_reset();

  //sound = EEPROM.read(43);

  server.begin();
  wdt_reset();

  report_test();
  wdt_reset();

  digitalWrite(LED_BUILTIN, LOW);
}


void readSensors() {
  temp = dht.readTemperature();
  hum = dht.readHumidity();

  uint8_t dontCrash = 0;
  dontCrash = 0;
  while (isnan(temp) || isnan(hum)) {
    delay(500);
    temp = dht.readTemperature();
    hum = dht.readHumidity();
    dontCrash++;
    if (dontCrash > 3) {
      temp = -999;
      hum = -999;
      break;
    }
  }

  uint16_t sum = 0;
  for (uint8_t i = 0; i < 10; i++) {
    sum += analogRead(WATER_PIN);
    delay(5);
  }
  leak = sum / 10;

  //Serial.println(temp);
  //Serial.println(hum);
  //Serial.println(leak);
}

void logData() {
  bool add_head = !SD.exists(LOG_PATH);
  File logfile = SD.open(LOG_PATH, FILE_WRITE);
  if (add_head) {
    logfile.println(F("epoch,temp,hum,leak"));
  }
  //if (RTC.chipPresent()) {
  //  logfile.print(RTC.get());
  //}
  //else {
  logfile.print(ntp.getEpochTime());
  //}
  logfile.print(F(","));
  logfile.print(temp);
  logfile.print(F(","));
  logfile.print(hum);
  logfile.print(F(","));
  logfile.print(leak);
  logfile.println();
  logfile.close();
}


#define addJsonKV(s, k, v) \
  s += '"'; \
  s += k; \
  s += "\":"; \
  s += v;

void handleServer() {
  EthernetClient client = server.available();

  if (client) {
    char clientline[WEBBUF];
    int index = 0;
    digitalWrite(LED_BUILTIN, HIGH);
    // an http request ends with a blank line
    boolean current_line_is_blank = true;

    // reset the input buffer
    index = 0;

    while (client.connected()) {
      if (client.available()) {
        char c = client.read();

        // If it isn't a new line, add the character to the buffer
        if (c != '\n' && c != '\r') {
          clientline[index] = c;
          index++;
          // are we too big for the buffer? start tossing out data
          if (index >= WEBBUF)
            index = WEBBUF - 1;

          // continue to read more data!
          continue;
        }

        // got a \n or \r new line, which means the string is done
        clientline[index] = 0;

        // Print it out for debugging
        Serial.println(clientline);

        char* filename;

        filename = clientline + 4;  // look after the "GET " (4 chars)
        // a little trick, look for the " HTTP/1.1" string and
        // turn the first character of the substring into a 0 to clear it out.
        (strstr(clientline, " HTTP"))[0] = 0;

        // print the file we want
        Serial.println(filename);

        if (0 == strcmp(filename, "/reboot")) {
          client.println(OKheader);
          client.println(CT_TEXT);
          client.println();
          client.println(F("rebooting..."));
          client.stop();
          wdt_enable(WDTO_15MS);
          while (1)
            ;
        } else if (0 == strcmp(filename, "/")) {
          client.println(OKheader);
          client.println(F("Content-Type: text/html; charset=utf-8"));
          //client.println(F("Refresh: 15"));
          client.println();
          client.print(F("<!DOCTYPE html>\n<html>\n<head>\n<title>Keller Sensor</title>\n<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n<style>body {background-color: #111; color: #EEE; font-family: monospace;}\nul {background-color: #222; max-width: max-content;}\na {background-color: #EEE; color: #222;}\nhr {color: #333;}\n.an {color: lime;}\n.aus{color: red;}\n.version{font-size: 8pt;}</style>\n</head>\n<body>\n<h1>Keller Sensor</h1>\n<ul>\n<li>Temperatur: "));
          client.print(temp);
          client.print(F("°C</li>\n<li>LuftFeuchte: "));
          client.print(hum);
          client.print(F("%</li>\n<li>Wasser: "));
          client.print((1023 - leak) / 10);
          client.print(F("%</li>\n</ul>\n<hr>\n "));
          client.print(F("<p>Freier Speicher: "));
          client.print(freeMemory());
          client.print(F("</p>\n<p>UNIX Zeit: "));
          //if (RTC.chipPresent()) {
          //  client.print(RTC.get());
          //}
          //else {
          client.print(ntp.getEpochTime());
          //}
          //client.print(F("</p>\n<p> Zeit Quelle: "));
          //client.print(RTC.chipPresent() ? F("RTC") : F("NTP"));
          client.print(F("</p>\n</body>\n</html>"));

          /*client.println(F("Content-Type: text/html; charset=utf-8"));
            client.println();
            client.print(F("<h1>Keller Sensor</h1>\n<ul>\n<li>"));
            client.print(temp);
            client.print(F("°C</li>\n<li>"));
            client.print(hum);
            client.print(F("%</li>\n<li>"));
            client.print(leak);
            client.print(F("/1023</li>\n</ul>\n"));
            client.print(ntp.getEpochTime());*/

          break;
        }

        else if (0 == strcmp(filename, "/data")) {
          client.println(OKheader);
          client.println(F("Content-Type: application/json"));

          String json = "";

          json += "{";
          addJsonKV(json, "temp", temp);
          json += ',';
          addJsonKV(json, "hum", temp);
          json += ',';
          addJsonKV(json, "leak_raw", leak);
          json += ',';
          addJsonKV(json, "leak", leak_detected);
          json += "}";

          client.print(F("Content-Length: "));
          client.println(json.length());
          client.println();
          client.print(json);
          break;
        }

        else if (0 == strcmp(filename, "/log")) {
          client.println(OKheader);
          client.println(F("Content-Type: text/csv"));
          client.println();
          File f = SD.open(LOG_PATH);
          while (f.available()) {
            client.write(f.read());
            digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
            if (!client.connected()) {
              break;
            }
            if (!client.available()) {
              break;
            }
          }
          f.close();
          break;
        }

        else if (0 == strcmp(filename, "/doLog")) {
          client.println(OKheader);
          client.println(CT_TEXT);
          client.println();
          logData();
          client.print(F("OK"));
          break;
        }

        /*else if (0==strcmp(filename,"/toggleSound")) {
          client.println(OKheader);
          client.println(CT_TEXT);
          client.println();
          sound = !sound;
          EEPROM.write(43, sound);
          client.print(F("Sound "));
          client.print(sound ? F("ON") : F("OFF"));
          break;
        }*/

        else {
          client.println(F("HTTP/1.1 404 Not Found"));
          /*client.println(CT_TEXT);
          client.println();
          client.println(F("404 Not Found"));*/
          break;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    client.stop();
    digitalWrite(LED_BUILTIN, LOW);
  }
}

bool ntfy_message(uint8_t prio, const char* tags, const char* msg) {
  Serial.print(F("conn "));
  EthernetClient client;
  client.stop();
  Serial.print(ntfy_server);
  const bool status = client.connect(ntfy_server, ntfy_port);
  Serial.print(" -> ");
  Serial.print(client.remoteIP());
  Serial.println(status ? F(" OK") : F(" ERR"));
  if (!status) return false;
  //http
  client.print(F("POST /"));
  client.print(ntfy_topic);
  client.println(F(" HTTP/1.1"));
  //headers
  client.println("Connection: close");
  client.print(F("Host: "));
  client.println(ntfy_server);
  client.print(F("X-Priority: "));
  client.println(prio);
  client.print(F("X-Tags: "));
  client.println(tags);
  client.print(F("X-Title: "));
  client.println(msg);
  client.println();
  //message
  //client.println(msg);

  delay(100);

  /*
  const char* ok_str = "HTTP/1.1 200 ";

  for (uint8_t b = 0; b < sizeof(ok_str); b++)
    if (client.read() != ok_str[i]) return false;  //if not 200 OK return error
  */

  while (client.available()) Serial.write(client.read());
  Serial.println();
  client.stop();

  return true;
}

bool report_leak() {
  wdt_reset();
  bool status = ntfy_message(ntfy_alert_prio, ntfy_alert_tags, ntfy_alert_message);
  wdt_reset();
  return status;
}

bool report_clear() {
  wdt_reset();
  ntfy_message(ntfy_clear_prio, ntfy_clear_tags, ntfy_clear_message);
  wdt_reset();
}

bool report_test() {
  wdt_reset();
  ntfy_message(ntfy_test_prio, ntfy_test_tags, ntfy_test_message);
  wdt_reset();
}

void handleSensorData() {
  static bool sent_alert = false;

  if (leak_detected) {
    if (!sent_alert) sent_alert = report_leak();  //if not already successfully sent, send alert

    if (leak > CLEAR_THRESH) {
      report_clear();
      leak_detected = false;
    }
  } else {
    sent_alert = false;

    noTone(BEEPER_PIN);
    digitalWrite(FLASHER_PIN, LOW);

    if (leak < LEAK_THRESH) leak_detected = true;
  }
}

void alert_people_nearby() {
  uint8_t slope = millis();
  //if (slope < 128) {
  if (sound) tone(BEEPER_PIN, 500 + (slope * 4));
  //}
  //else noTone(BEEPER_PIN);

  analogWrite(FLASHER_PIN, 0xFF - slope);
}

void loop() {
  // put your main code here, to run repeatedly:
  handleServer();
  //Ethernet.maintain(); //only needed with DHCP
  //ntp.update(); //crashes server after 2 minutes (no idea why tho)

  static uint32_t sensMillis = 0;
  if ((millis() - sensMillis) > 2000) {
    readSensors();
    sensMillis = millis();
    handleSensorData();
  }

  if (leak_detected) alert_people_nearby();

  //Log data each hour
  static uint32_t logMillis = 0;
  if ((millis() - logMillis) > 3600000) {
    logData();
    logMillis = millis();
  }

  //Scheduled Reboot after 14 days (just to be sure)
  if (millis() > 1209600000) {
    wdt_enable(WDTO_15MS);
    while (1)
      ;
  }

  //Reboot on bad NTP time after 3 hours
  if ((millis() > 10800000) and (ntp.getEpochTime() < 25000)) {
    wdt_enable(WDTO_15MS);
    while (1)
      ;
  }

  wdt_reset();
  delay(1);
}
