#define WATER_PIN A0
#define DHT_PIN 6
#define DHT_TYPE DHT22
#define BEEPER_PIN 5
#define LED_BUILTIN 8
#define FLASHER_PIN 9

const char* LOG_PATH = "/log.csv";

#define NTP_SERVER "europe.pool.ntp.org"
const uint8_t ETH_MAC[6] = { 0xDE, 0xAD, 0xBE, 0xEF, 0x69, 0x69 };
const uint8_t ETH_IP[4] = { 192, 168, 178, 128 };
const uint8_t ETH_DNS[4] = { 192, 168, 178, 1 };
const uint8_t ETH_GATEWAY[4] = { 192, 168, 178, 1 };
const uint8_t ETH_SUBNET[4] = { 255, 255, 255, 0 };

#define LEAK_THRESH 512
#define CLEAR_THRESH 768

const uint16_t ntfy_port = 80;
const char* ntfy_server = "ntfy.sh";
const char* ntfy_topic = "h3_test";

const uint8_t ntfy_alert_prio = 5;  //5=max
const char* ntfy_alert_tags = "rotating_light";
const char* ntfy_alert_message = "WASSER ERKANNT!";

const uint8_t ntfy_clear_prio = 1;  //min
const char* ntfy_clear_tags = "ok";
const char* ntfy_clear_message = "trocken";

const uint8_t ntfy_test_prio = 1;  //min
const char* ntfy_test_tags = "information_source";
const char* ntfy_test_message = "system rebooted";
