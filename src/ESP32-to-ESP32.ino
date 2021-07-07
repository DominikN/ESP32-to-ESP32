#include <AceButton.h>
#include <Husarnet.h>
#include <SPI.h>
#include <TFT_eSPI.h>

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include <WiFi.h>
#include <WiFiMulti.h>

using namespace ace_button;

/* =============== config section start =============== */

#define ENABLE_TFT 1  // tested on TTGO T Display

const int BUTTON_PIN = 0;
const int LED_PIN = 27;
const int PORT = 8001;

#if __has_include("credentials.h")
#include "credentials.h"
#else
/* to get your join code go to https://app.husarnet.com
   -> select network
   -> click "Add element"
   -> select "join code" tab

   Keep it secret!
*/
const char *husarnetJoinCode = "xxxxxxxxxxxxxxxxxxxxxx";
const char *dashboardURL = "default";

// WiFi credentials
#define NUM_NETWORKS 2  // number of Wi-Fi network credentials saved

const char *ssidTab[NUM_NETWORKS] = {
    "wifi-ssid-one",
    "wifi-ssid-two",
};

const char *passwordTab[NUM_NETWORKS] = {
    "wifi-pass-one",
    "wifi-pass-two",
};

const char *hostname = "random";

#endif

/* =============== config section end =============== */

#if ENABLE_TFT == 1
TFT_eSPI tft = TFT_eSPI();  // Invoke custom library
#define LOG(f_, ...)                                                         \
  {                                                                          \
    if (tft.getCursorY() >= tft.height()) {                                  \
      tft.fillScreen(TFT_BLACK);                                             \
      tft.setCursor(0, 0);                                                   \
      tft.printf("IP: %u.%u.%u.%u\r\n", myip[0], myip[1], myip[2], myip[3]); \
      tft.printf("Hostname: %s\r\n--\r\n", Husarnet.getHostname());          \
    }                                                                        \
    tft.printf((f_), ##__VA_ARGS__);                                         \
    Serial.printf((f_), ##__VA_ARGS__);                                      \
  }
#else
#define LOG(f_, ...) \
  { Serial.printf((f_), ##__VA_ARGS__); }
#endif

AceButton btn(BUTTON_PIN);

// you can provide credentials to multiple WiFi networks
WiFiMulti wifiMulti;

/* store index.html content in html constant variable (platformio feature) */
extern const char index_html_start[] asm("_binary_src_index_html_start");
const String html = String((const char*)index_html_start);
AsyncWebServer server(PORT);

HusarnetClient httpClient;

// asyncHTTPrequest httpClient;

IPAddress myip;

void handleEvent(AceButton *, uint8_t, uint8_t);
void taskWifi(void *parameter);
void taskConnection(void *parameter);

void setup() {
  Serial.begin(115200);

#if ENABLE_TFT == 1
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);
#endif
  //--------------------

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  btn.setEventHandler(handleEvent);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  xTaskCreate(taskWifi,   /* Task function. */
              "taskWifi", /* String with name of task. */
              10000,      /* Stack size in bytes. */
              NULL,       /* Parameter passed as input of the task */
              1,          /* Priority of the task. */
              NULL);      /* Task handle. */
}

void loop() {
  while (1) {
    btn.check();
    delay(1);
  }
}

void handleEvent(AceButton *button, uint8_t eventType, uint8_t buttonState) {
  int ledState = 0;
  switch (eventType) {
    case AceButton::kEventPressed:
      Serial.println("pressed");
      ledState = 1;
      break;
    case AceButton::kEventReleased:
      Serial.println("released");
      ledState = 0;
      break;
  }

  for (auto const &host : Husarnet.listPeers()) {
    IPv6Address addr = host.first;
    IP6Address addr2; //based on std:array

    for(int i=0; i<16; i++) {
      addr2.data[i] = addr[i];
    }

    if (!httpClient.connect(addr2, PORT)) {
      Serial.println("connection failed");
    } else {
      String requestURL = "/led/1/state/" + String(ledState);
      Serial.printf("\r\n=========================================\r\n");
      Serial.println(requestURL);
      httpClient.print(String("GET ") + requestURL + + " HTTP/1.1\r\n" + "Host: esp32\r\n" + "Connection: close\r\n\r\n");
    LOG("%s: %s\r\n", host.second.c_str(), host.first.toString().c_str());
  }
}


void taskWifi(void *parameter) {
  uint8_t stat = WL_DISCONNECTED;

  /* Configure Wi-Fi */
  for (int i = 0; i < NUM_NETWORKS; i++) {
    wifiMulti.addAP(ssidTab[i], passwordTab[i]);
    Serial.printf("WiFi %d: SSID: \"%s\" ; PASS: \"%s\"\r\n", i, ssidTab[i],
                  passwordTab[i]);
  }

  while (stat != WL_CONNECTED) {
    stat = wifiMulti.run();
    Serial.printf("WiFi status: %d\r\n", (int)stat);
    delay(100);
  }

  Serial.printf("WiFi connected\r\n", (int)stat);
  Serial.printf("IP address: ");

  myip = WiFi.localIP();
  LOG("IP: %u.%u.%u.%u\r\n", myip[0], myip[1], myip[2], myip[3]);

  /* Start Husarnet */
  Husarnet.selfHostedSetup(dashboardURL);
  Husarnet.join(husarnetJoinCode, hostname);
  Husarnet.start();
  LOG("Hostname: %s\r\n--\r\n", hostname);

    /* Start a web server */
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/html", html);
  });

  // Send a GET request to <IP>/led/<number>/state/<0 or 1>
  server.on("^\\/led\\/([0-9]+)\\/state\\/([0-9]+)$", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String ledNumber = request->pathArg(0); // currently unused - we use only a predefined LED number
    String state = request->pathArg(1);

    digitalWrite(LED_PIN, state.toInt());

    request->send(200, "text/plain", "LED: " + ledNumber + ", with state: " + state);
  });

  server.begin();

  uint8_t oldState = btn.getLastButtonState();

  while (1) {
    while (WiFi.status() == WL_CONNECTED) {
      delay(500);
    }
    LOG("WiFi disconnected, reconnecting\r\n");
    delay(500);
    stat = wifiMulti.run();
    LOG("WiFi status: %d\r\n", (int)stat);
  }
}
