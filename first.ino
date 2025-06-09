#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_camera.h"
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <WebServer.h>

// ===== Wi-Fi Credentials =====
const char* ssid = "Redmi";           
const char* password = "Suryakarna";  

// ===== Telegram Bot =====
String BOTtoken = "7563909791:AAFoUosTYEOETjU8DA8RGrdbtAM8vayMIkw";  // Replace with your token
String CHAT_ID = "809775231";     // Replace with your chat ID

// ===== Pin Definitions =====
#define RELAY_PIN       12
#define FLASH_LED_PIN   4
#define GPS_RX_PIN      13
#define GPS_TX_PIN      14

// ===== Camera Pins (OV2640) =====
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// ===== Globals =====
HardwareSerial mySerial(1); 
TinyGPSPlus gps;
float latitude = 0.0, longitude = 0.0;

WiFiClientSecure clientTCP;
UniversalTelegramBot bot(BOTtoken, clientTCP);
WebServer server(80);

bool flashState = false;
bool relayState = false;
unsigned long lastTimeBotRan = 0;
int botRequestDelay = 1000;

// ===== Camera Setup =====
void configInitCamera() {
  camera_config_t config;
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
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if(psramFound()){
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_CIF;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    delay(1000);
    ESP.restart();
  }
}

// ===== GPS Location =====
String getLocation() {
  while (mySerial.available()) {
    gps.encode(mySerial.read());
    if (gps.location.isUpdated()) {
      latitude = gps.location.lat();
      longitude = gps.location.lng();
    }
  }
  return "📍 Latitude: " + String(latitude, 6) + "\n📍 Longitude: " + String(longitude, 6);
}

// ===== Telegram Photo =====
String sendPhotoTelegram() {
  const char* myDomain = "api.telegram.org";
  String getAll = "";

  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return "Camera capture failed";
  }

  if (clientTCP.connect(myDomain, 443)) {
    String head = "--AaB03x\r\nContent-Disposition: form-data; name=\"chat_id\"\r\n\r\n" + CHAT_ID + 
                  "\r\n--AaB03x\r\nContent-Disposition: form-data; name=\"photo\"; filename=\"cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--AaB03x--\r\n";
    uint32_t imageLen = fb->len;
    uint32_t totalLen = head.length() + imageLen + tail.length();

    String request = "POST /bot" + BOTtoken + "/sendPhoto HTTP/1.1\r\n";
    request += "Host: " + String(myDomain) + "\r\n";
    request += "Content-Length: " + String(totalLen) + "\r\n";
    request += "Content-Type: multipart/form-data; boundary=AaB03x\r\n\r\n";

    clientTCP.print(request);
    clientTCP.print(head);
    clientTCP.write(fb->buf, imageLen);
    clientTCP.print(tail);

    esp_camera_fb_return(fb);

    long timeout = millis() + 5000;
    while (clientTCP.connected() && millis() < timeout) {
      while (clientTCP.available()) {
        char c = clientTCP.read();
        getAll += c;
      }
      if (getAll.length() > 0) break;
    }
  } else {
    return "Connection to Telegram failed.";
  }
  return "📸 Photo sent!";
}

// ===== Stream Handler =====
void handle_stream() {
  WiFiClient client = server.client();
  String response = "HTTP/1.1 200 OK\r\n";
  response += "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";
  server.sendContent(response);

  while (client.connected()) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) continue;

    response = "--frame\r\n";
    response += "Content-Type: image/jpeg\r\n\r\n";
    server.sendContent(response);
    server.sendContent((const char *)fb->buf, fb->len);
    server.sendContent("\r\n");

    esp_camera_fb_return(fb);
    if (!client.connected()) break;
    delay(100);
  }
}

// ===== Telegram Command Handling =====
void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = bot.messages[i].chat_id;
    if (chat_id != CHAT_ID) {
      bot.sendMessage(chat_id, "⛔ Unauthorized user", "");
      continue;
    }

    String text = bot.messages[i].text;
    String from = bot.messages[i].from_name;

    if (text == "/start") {
      bot.sendMessage(CHAT_ID, "👋 Hi " + from + ", available commands:\n\n/photo - Capture image\n/flash - Toggle flash\n/Getloc - Get GPS location\n/on - Relay ON\n/off - Relay OFF\n/stream - View live stream", "");
    }
    else if (text == "/flash") {
      flashState = !flashState;
      digitalWrite(FLASH_LED_PIN, flashState);
      bot.sendMessage(CHAT_ID, flashState ? "💡 Flash ON" : "💡 Flash OFF", "");
    }
    else if (text == "/photo") {
      bot.sendMessage(CHAT_ID, "Capturing photo...", "");
      bot.sendMessage(CHAT_ID, sendPhotoTelegram(), "");
    }
    else if (text == "/Getloc") {
      bot.sendMessage(CHAT_ID, getLocation(), "Markdown");
    }
    else if (text == "/on") {
      digitalWrite(RELAY_PIN, HIGH);
      relayState = true;
      bot.sendMessage(CHAT_ID, "🔌 Relay ON", "");
    }
    else if (text == "/off") {
      digitalWrite(RELAY_PIN, LOW);
      relayState = false;
      bot.sendMessage(CHAT_ID, "🔌 Relay OFF", "");
    }
    else if (text == "/stream") {
      String streamURL = "http://" + WiFi.localIP().toString() + "/stream";
      bot.sendMessage(CHAT_ID, "🔴 Live Stream\n[Click to view stream](" + streamURL + ")", "Markdown");
    }
  }
}

// ===== Setup =====
void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 
  Serial.begin(115200);

  pinMode(FLASH_LED_PIN, OUTPUT);
  digitalWrite(FLASH_LED_PIN, LOW);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  configInitCamera();
  mySerial.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);

  WiFi.begin(ssid, password);
  clientTCP.setCACert(TELEGRAM_CERTIFICATE_ROOT);

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi connected");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  // Start Web Server
  server.on("/stream", HTTP_GET, handle_stream);
  server.begin();
  Serial.println("📡 Stream server started");
}

// ===== Loop =====
void loop() {
  server.handleClient();

  if (millis() > lastTimeBotRan + botRequestDelay) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages) {
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }
}