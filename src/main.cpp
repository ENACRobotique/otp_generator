#include <Arduino.h>
#include <WiFi.h>
#include "time.h"
#include "base32.h"
#include "otp.h"


#define NB_KEYS 2
const char *secrets[NB_KEYS] = {
  "AADHCKELQQSSDDFF",
  "AZERTYUIOPQSDFGHJKLMWXCVBN234567",
};
const char *names[NB_KEYS] = {
  "Github ENAC",
  "Twitch",
  
};

char ckey[40] = {0};

const char* ssid = "plop";
const char* password = "plop";

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600 * 1;
const int   daylightOffset_sec = 3600 * 0;


bool on_time = false;
uint32_t blink_period = 100;




void OTPGeneratorTask( void *pvParameters )
{
  while(true)
  {
    if(on_time) {
      time_t now;
      time(&now);
      for(int i=0; i<NB_KEYS; i++) {
        const char* secret = secrets[i];
        const size_t secretLength = strlen(secret);
        size_t ckey_len = decode32(secret, secretLength, ckey, 40);
        if(ckey_len == 0) {
          Serial.printf("Key decoding failed for %s!\n", names[i]);
          continue;
        }
        uint32_t otp = getOTP((const unsigned char *)ckey, ckey_len, now/30);
        uint32_t upper = otp/1000;
        uint32_t lower = otp - upper*1000;
        Serial.printf("%s: ", names[i]);
        Serial.printf("%03lu.%03lu\n", upper, lower);
      }
      Serial.println("------------");

    }
    vTaskDelay( pdMS_TO_TICKS( 1000 ) );
  }
}


void blinker( void *pvParameters )
{
  pinMode(LED_BUILTIN, OUTPUT);
  while(true)
  {
    digitalWrite(LED_BUILTIN, HIGH);
    vTaskDelay(pdMS_TO_TICKS(blink_period));
    digitalWrite(LED_BUILTIN, LOW);
    vTaskDelay(pdMS_TO_TICKS(blink_period));
  }
}

void clockTask( void *pvParameters )
{
  blink_period = 100;
  WiFi.mode(WIFI_STA); // Optional
  WiFi.begin(ssid, password);
  Serial.println("\nConnecting");

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }

  blink_period = 1000;

  Serial.println("\nConnected to the WiFi network");
  Serial.print("Local ESP32 IP: ");
  Serial.println(WiFi.localIP());

  // On configure le seveur NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  while(true)
  {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
  } else {
    on_time = true;
  }
  vTaskDelay( pdMS_TO_TICKS( 2000 ) );
  }
}


void setup()
{
  Serial.begin(115200);
  while (!Serial);
  Serial.println("Start");

  xTaskCreate(blinker, "blinker", 1024, NULL, 1, NULL);
  xTaskCreate(clockTask, "NTP", 10000, NULL, 1, NULL);
  xTaskCreate(OTPGeneratorTask, "OTP generator", 10000, NULL, 1, NULL);
}

void loop()
{
  //Serial.printf("Task loop() %d\n", xPortGetCoreID());
  //delay(1000);  
}
