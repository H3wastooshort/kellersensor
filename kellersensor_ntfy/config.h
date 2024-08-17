#define WATER_PIN A0
#define DHT_PIN 6
#define BEEPER_PIN 5
#define LED_BUILTIN 8
#define FLASHER_PIN 9

const char* LOG_PATH = "/log.csv";

#define NTP_SERVER "europe.pool.ntp.org"
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0x69, 0x69 };
byte ip[] = { 192, 168, 178, 128 };

const uint16_t ntfy_port = 80;
const char* ntfy_server = "ntfy.sh";
const char* ntfy_topic = "h3_test";
const uint8_t ntfy_prio = 5;  //5=max
const char* ntfy_tags = "rotating_light";
const char* ntfy_message = "WATER IN BASEMENT!";
