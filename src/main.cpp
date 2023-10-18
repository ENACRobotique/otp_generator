#include <Arduino.h>
#include <WiFi.h>
#include <LiquidCrystal.h>
#include "time.h"
#include "base32.h"
#include "otp.h"

// secrets and associated names
// TODO: save in flash/eeprom, not in the code!
#define NB_KEYS 2
const char *secrets[NB_KEYS] = {
  "AADHCKELQQSSDDFF",
  "AZERTYUIOPQSDFGHJKLMWXCVBN234567",
};
const char *names[NB_KEYS] = {
  "Github ENAC",
  "OnShape",
};

// WiFi credentials
// TODO: remove from code (flash/eeprom?)
const char* ssid = "plop";
const char* password = "plop";

// NTP server
const char* ntpServer = "pool.ntp.org";

// LED blink period
uint32_t blink_period = 100;

// current code to display
volatile size_t current_code = 0;


// LCD(rs, enable, d4, d5, d6, d7)
LiquidCrystal lcd(14, 27, 26, 25, 33, 32);
constexpr uint8_t BUTTON = 13;



void OTPGeneratorTask( void *pvParameters )
{
  static uint8_t time_char[8]={B00000,B11111,B11111,B11111,B11111,B11111,B11111, B00000};
  while(true)
  {
    // get current timestamp
    time_t now;
    time(&now);

    // Create character representing the time left for the current OTP
    time_t tleft = 30 - now%30;
    for(int i=0; i<6; i++) {
      for(int j=0; j<5; j++) {
        if(5*i+j < tleft) {
          time_char[6-i] |= 1<<(4-j);
        } else {
          time_char[6-i] &= ~(1<<(4-j));
        }
      }
    }
    lcd.createChar(0 , time_char);

    // Decode the secret (base32 encoded)
    const char* secret = secrets[current_code];
    const size_t secretLength = strlen(secret);
    char ckey[40] = {0};
    size_t ckey_len = decode32(secret, secretLength, ckey, 40);
    if(ckey_len == 0) {
      Serial.printf("Key decoding failed for %s!\n", names[current_code]);
      continue;
    }
    // compute OTP
    uint32_t otp = getOTP((const unsigned char *)ckey, ckey_len, now/30);
    // split OTP in upper and lower halfs to make it easier to read
    uint32_t upper = otp/1000;
    uint32_t lower = otp - upper*1000;

    Serial.printf("%s: ", names[current_code]);
    Serial.printf("%03lu.%03lu\n", upper, lower);
  
    // display code name
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(names[current_code]);
    // display time left
    lcd.setCursor(15, 0);
    lcd.write((uint8_t)0x00); // remaining time
    // display OTP
    lcd.setCursor(0,1);
    lcd.printf("%03lu.%03lu", upper, lower);
    
    // wait for 1s, or a button notification
    xTaskNotifyWait(0, 0, NULL, pdMS_TO_TICKS(1000));
  }
}

// blink status led at blink_period.
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

// counter and ISR to debouce button hits
volatile uint32_t _button_counter = 0;
void buttonISR() {
  _button_counter = 10;
}
// button debounce thread
void buttonHandler( void *arg ) {
  TaskHandle_t* otp_handle = (TaskHandle_t*) arg;
  pinMode(BUTTON, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON), buttonISR, FALLING);

  while(true) {
    // The button is considered pressed if the counter is at 1 and if it's currently down.
    if(_button_counter == 1 && !digitalRead(BUTTON)) {
      // increment current_code
      current_code = (current_code + 1) % NB_KEYS;
      // notify OTP generator
      xTaskNotify(*otp_handle, current_code, eNoAction);
    }

    // Decrement the counter until it reaches 0.
    if(_button_counter >= 1) {
      _button_counter--;
    }

    // When the counter reaches 0, it stay there until the ISR reset it.

    vTaskDelay(pdMS_TO_TICKS(1));
  }
}


void setup()
{
  Serial.begin(115200);
  // wait for serial to be ready
  while (!Serial);
  Serial.println("Start");

  // display loading screen
  lcd.begin(20, 4);
  lcd.clear();
  lcd.print("Connecting...");
  lcd.blink();

  // Connecting to WiFi
  blink_period = 100;
  WiFi.mode(WIFI_STA);
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
  // NTP configuration
  // We don't really care about the local time
  configTime(0, 0, ntpServer);

  // Wait until the timestamp is somewhat realistic.
  time_t now = 0;
  while(now < 1e9) {
    time(&now);
    vTaskDelay(pdMS_TO_TICKS(10)); 
  }
  lcd.noBlink();
  
  TaskHandle_t otp_handle;
  xTaskCreate(blinker, "blinker", 1024, NULL, 1, NULL);
  xTaskCreate(OTPGeneratorTask, "OTP generator", 10000, NULL, 2, &otp_handle);
  xTaskCreate(buttonHandler, "buttonHandler", 1024, &otp_handle, 3, NULL);
}

void loop()
{
  // do nothing
  vTaskDelay(pdMS_TO_TICKS(2000));  
}
