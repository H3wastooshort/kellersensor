#include <DHT.h>
#include <SPI.h>
#include <Ethernet.h>
#include <ArduinoJson.h>
#include <avr/wdt.h>
#include <EEPROM.h>

#define LED_BUILTIN 8

const char OKheader[] = "HTTP/1.1 200 Okay";

float temp = -999;
float hum = -999;
int leak = 9999;
bool sound = true;
uint32_t dht_errors = 0;

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0x69, 0x69 };
byte ip[] = { 192, 168, 178, 128 };

DHT dht(6, DHT11);

#define WEBBUF 64
EthernetServer server(80);

#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char* sbrk(int incr);
#else  // __ARM__
extern char *__brkval;
#endif  // __arm__

int freeMemory() {
  char top;
#ifdef __arm__
  return &top - reinterpret_cast<char*>(sbrk(0));
#elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
  return &top - __brkval;
#else  // __arm__
  return __brkval ? &top - __brkval : &top - __malloc_heap_start;
#endif  // __arm__
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  pinMode(5, OUTPUT);

  //Leak Sensor
  pinMode(A0, INPUT_PULLUP);

  //Virtual power
  pinMode(7, OUTPUT);
  digitalWrite(7, LOW);
  pinMode(3, OUTPUT);
  digitalWrite(3, LOW);

  dht.begin();

  Ethernet.init(10);
  Ethernet.begin(mac, ip);
  Ethernet.setRetransmissionCount(20);
  Ethernet.setRetransmissionTimeout(100);
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println(F("Ethernet error!"));
    wdt_enable(WDTO_8S);
    while (true) {
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
      delay(500);
    }
  }
  while (Ethernet.linkStatus() == LinkOFF) {
    Serial.println(F("No Link!"));
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    delay(1000);
  }
  digitalWrite(LED_BUILTIN, HIGH);

  // print your local IP address:
  Serial.print(F("IP address: "));
  Serial.println(Ethernet.localIP());

  sound = EEPROM.read(43);

  server.begin();

  digitalWrite(LED_BUILTIN, LOW);
}
void readSensors() {
  temp = dht.readTemperature();
  hum = dht.readHumidity();

  uint8_t dontCrash = 0;
  dontCrash = 0;
  while (isnan(temp) || isnan(hum)) {
    dht_errors++;
    delay(100);
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
    sum += analogRead(A0);
    delay(5);
  }
  leak = sum / 10;

  Serial.println(temp);
  Serial.println(hum);
  Serial.println(leak);
}

void handleServer() {
  char clientline[WEBBUF];
  int index = 0;

  EthernetClient client = server.available();
  if (client) {
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

        char *filename;

        filename = clientline + 4; // look after the "GET " (4 chars)
        // a little trick, look for the " HTTP/1.1" string and
        // turn the first character of the substring into a 0 to clear it out.
        (strstr(clientline, " HTTP"))[0] = 0;

        // print the file we want
        Serial.println(filename);

        if (String(filename) == F("/reboot")) {
          client.println(OKheader);
          client.println(F("Content-Type: text/plain"));
          client.println(F("Refresh: 5; url=/"));
          client.println();
          client.println(F("rebooting..."));
          client.stop();
          wdt_enable(WDTO_15MS);
          while (1);
        }
        else if (String(filename) == F("/")) {
          client.println(OKheader);
          client.println(F("Content-Type: text/html; charset=utf-8"));
          client.println(F("Refresh: 15"));
          client.println();
          client.print(F("<!DOCTYPE html>\n<html>\n<head>\n<title>Keller Sensor</title>\n<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n<style>body {background-color: #111; color: #EEE; font-family: monospace;}\nul {background-color: #222; max-width: max-content;}\na {background-color: #EEE; color: #222;}\nhr {color: #333;}\n.an {color: lime;}\n.aus{color: red;}\n.version{font-size: 8pt;}</style>\n</head>\n<body>\n<h1>Keller Sensor</h1>\n<ul>\n<li>Temperatur: "));
          client.print(temp);
          client.print(F("°C</li>\n<li>Feuchte: "));
          client.print(hum);
          client.print(F("%</li>\n<li>Leck Rohwert: "));
          client.print(leak);
          client.print(F("/1023</li>\nDer Leck Rowert geht von 0 bis 1023, wobei 1023 trocken, und 0 extrem nass ist.\n</ul>\n<hr>\n<p>Ton "));
          client.print(sound ? F("<span class=\"an\">AN</span>") : F("<span class=\"aus\">AUS</span>"));
          client.print(F(" <a href=\"/toggleSound\">[Ändern]</a></p>\n<p>Freier Speicher: "));
          client.print(freeMemory());
          client.print(F("</p>\n<p class=\"version\"><small>"));
          //client.print(__FILE__);
          client.print(F("Kompiliert am "));
          client.print(__DATE__);
          client.print(F(" um "));
          client.print(__TIME__);
          client.print(F("</small></p>\n</body>\n</html>"));

          break;
        }

        else if (String(filename) == F("/data")) {
          client.println(OKheader);
          client.println(F("Content-Type: application/json"));
          client.println(F("Refresh: 15"));
          DynamicJsonDocument doc(128);
          doc["temp"] = temp;
          doc["hum"] = hum;
          doc["leak_raw"] = leak;
          doc["leak_percent"] = map(leak, 1024, 0, 0, 100);
          doc["leak_bool"] = leak < 512;
          doc["sound"] = sound;
          doc["dht_errors"] = dht_errors;
          char output[128];
          client.print(F("Content-Length: "));
          client.println(serializeJson(doc, output));
          client.println();
          client.print(output);
          break;
        }

        else if (String(filename) == F("/toggleSound")) {
          client.println(OKheader);
          client.println(F("Content-Type: text/plain"));
          client.println(F("Refresh: 1; url=/"));
          client.println();
          sound = !sound;
          EEPROM.write(43, sound);
          client.print(F("Sound "));
          client.print(sound ? F("ON") : F("OFF"));
          break;
        }

        else {
          client.println(F("HTTP/1.1 404 Not Found"));
          client.println(F("Content-Type: text/plain"));
          client.println();
          client.println(F("404 Not Found"));
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

void loop() {
  // put your main code here, to run repeatedly:
  handleServer();
  //Ethernet.maintain(); //only needed with DHCP
  //ntp.update(); //crashes server after 2 minutes

  static uint32_t sensMillis = 0;
  if ((millis() - sensMillis) > 2000) {
    readSensors();
    sensMillis = millis();
  }

  //Scheduled Reboot after 14 days (just to be sure)
  if (millis() > 1209600000) {
    wdt_enable(WDTO_15MS);
    while (1);
  }

  if (leak < 512) {
    if (uint8_t(millis()) > 127) {
      if (sound) {
        tone(5, 1000);
      }
      digitalWrite(4, LOW);
    }
    else {
      noTone(5);
      digitalWrite(4, HIGH);
    }
  }
  else {
    noTone(5);
    digitalWrite(4, LOW);
  }

  delay(1);
}
