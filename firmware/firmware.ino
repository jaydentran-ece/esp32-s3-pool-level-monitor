#include <ArduinoJson.h>
#include <UniversalTelegramBot.h>

#include "esp_camera.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "board_config.h"
#include "secrets.h"

#include <vector>
#include <algorithm>

const int trigPin = 21;
const int echoPin = 47;
const int target_size = 5;
const int max_attempts = 15;
const float calibration_offset = 17.3f;
camera_fb_t *telegramPhoto = nullptr;
size_t telegramPhotoIndex = 0;

WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

camera_config_t config;

bool camera_init();
float takeMeasurement();
bool connectWiFi();

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("Setup started");
  bool init_success = camera_init();
  if (!init_success) {
    Serial.println("Camera Initialization Failed...");
    Serial.println("Trying again...");
    for (int i {0}; i < 3; i++) {
      delay(1000);
      if (camera_init()) {
        init_success = true;
        Serial.printf("Camera initialized on attempt %d\n", i + 2);
        break;
      }
    }
    if (!init_success) {
      Serial.println("Camera cannot be initialized after 4 attempts...");
    }
  }
  if (init_success) {
    Serial.println("Camera Initialization Success");
  }

  camera_fb_t* fb = nullptr;
  if (init_success) {
    fb = esp_camera_fb_get();
  }
  if (fb == nullptr) {
    Serial.println("Photo capture failed.");
  } 
  else {
    Serial.printf("Photo captured: %u bytes\n", fb->len);
    Serial.printf("Width: %d\n", fb->width);
    Serial.printf("Height: %d\n", fb->height);
    Serial.printf("Format: %d\n", fb->format);
  }
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  std::vector<float> numbers {};
  for (int i = 0; i < max_attempts && static_cast<int>(numbers.size()) < target_size; i++) {
    float num = takeMeasurement();
    if (num == -9999.9f) {
      Serial.println("Error getting a measurement in attempt number ");
      Serial.print(i + 1);
    }
    else {
      numbers.push_back(num);
      Serial.print("Valid measurement: ");
      Serial.print(num, 2);
      Serial.println(" inches");
    }
    delay(1000);
  }
  float median = 0.0f;
  if (numbers.empty()) {
    Serial.println("Error occurred where no measurements were taken");
  }
  else {
    std::sort(numbers.begin(), numbers.end());
    if (numbers.size() % 2 == 1) {
      median = numbers[numbers.size() / 2];
    } 
    else {
      int upperIndex = numbers.size() / 2;
      int lowerIndex = upperIndex - 1;
      median = (numbers[lowerIndex] + numbers[upperIndex]) / 2.0;
    }
    Serial.print("Median distance: ");
    Serial.print(median, 2);
    Serial.println(" inches");
  }
  if (connectWiFi()) {
    String message;
    if (numbers.empty()) {
      message = "Pool Monitor Bot Here!\n\nWater level measurement failed.";
    }
    else if (median >= 0) {
      message = "Pool Monitor Bot Here!\n\nInches Above the Water Line: " + String(median, 2);
    }
    else {
      message = "Pool Monitor Bot Here!\n\nInches Below the Water Line: " + String(abs(median), 2);
    }
    bool success = bot.sendMessage(CHAT_ID,message,"");
    if (success) {
      Serial.println("Telegram message sent!");
    }
    else {
      Serial.println("Telegram message failed.");
    }
    
    if (fb != nullptr) {
      telegramPhoto = fb;
      telegramPhotoIndex = 0;
      bool photoSent = bot.sendPhotoByBinary(CHAT_ID, "image/jpeg", fb->len, morePhotoData, getNextPhotoByte, nullptr, nullptr);
      if (photoSent) {
        Serial.println("Photo sent!");
      } 
      else {
        Serial.println("Photo failed to send.");
      }
      telegramPhoto = nullptr;
    }
    else {
      Serial.println("No valid photo can be sent...");
    }
  }
  else {
    Serial.println("Could not connect to Wifi...");
  }
  if (fb != nullptr) {
    esp_camera_fb_return(fb);
    fb = nullptr;
  } 
  Serial.println("sleeping for 15 minutes");
  Serial.flush();
  esp_sleep_enable_timer_wakeup(15ULL * 60ULL * 1000000ULL);
  esp_deep_sleep_start();
}

void loop() {
}

bool camera_init() {
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 10000000;
  config.frame_size = FRAMESIZE_QVGA;
  config.pixel_format = PIXFORMAT_JPEG; // JPEG image capture
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 10;
  config.fb_count = 1;
  
  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x\n", err);
    return false;
  }

  sensor_t * s = esp_camera_sensor_get();
  if (s == nullptr) {
    Serial.println("Could not access camera sensor.");
    esp_camera_deinit();
    return false;
  }
  // drop down frame size for higher initial frame rate
  s->set_hmirror(s, 1);
  s->set_vflip(s, 0);
  return true;
}


float takeMeasurement() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(5);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(20);
  digitalWrite(trigPin, LOW);
  unsigned long duration = pulseIn(echoPin, HIGH, 40000);
  if (duration == 0) {
    return -9999.9f;
  }
  else {
    float distance = (duration * 0.0343 / 2) / 2.54;
    float water = distance - calibration_offset;
    return water;
  }
}

bool connectWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 15000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected");
    configTime(0, 0, "pool.ntp.org");
    unsigned long timeStart = millis();
    while (time(nullptr) < 1700000000 && millis() - timeStart < 10000) {
      delay(500);
    }
    if (time(nullptr) < 1700000000) {
      Serial.println("Time synchronization failed");
      return false;
    }
    client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
    return true;
  }
  Serial.println("WiFi connection failed");
  return false;
}

bool morePhotoData() {
  return telegramPhotoIndex < telegramPhoto->len;
}

uint8_t getNextPhotoByte() {
  return telegramPhoto->buf[telegramPhotoIndex++];
}