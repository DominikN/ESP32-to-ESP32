#include <WiFi.h>
#include <WiFiMulti.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Husarnet.h>
#include <AceButton.h>

#include <SPI.h>
#include <TFT_eSPI.h>

#define ENABLE_TFT 1  // tested on TTGO T Display

#if ENABLE_TFT == 1

TFT_eSPI tft = TFT_eSPI(); 

#define LOG(f_, ...)                                                         \
  {                                                                          \
    if (tft.getCursorY() >= tft.height() || tft.getCursorY() == 0) {         \
      tft.fillScreen(TFT_BLACK);                                             \
      tft.setCursor(0, 0);                                                   \
      IPAddress myip = WiFi.localIP();                                       \
      tft.printf("IP: %u.%u.%u.%u\r\n", myip[0], myip[1], myip[2], myip[3]); \
      tft.printf("Hostname: %s\r\n--\r\n", Husarnet.getHostname().c_str());  \
    }                                                                        \
    tft.printf((f_), ##__VA_ARGS__);                                         \
    Serial.printf((f_), ##__VA_ARGS__);                                      \
  }
#else
#define LOG(f_, ...) \
  { Serial.printf((f_), ##__VA_ARGS__); }
#endif

/* =============== config section start =============== */
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
const char* wifiNetworks[][2] = {
  {"wifi-ssid-one", "wifi-pass-one"},
  {"wifi-ssid-two", "wifi-pass-two"},
};

const char *hostname = "random";

#endif
/* =============== config section end =============== */

using namespace ace_button;

const int BUTTON_PIN = 0;
const int LED_PIN = 27;
const int PORT = 8001;

int ledState = 0;

// Push button
AceButton btn(BUTTON_PIN);
void handleButtonEvent(AceButton *, uint8_t, uint8_t);

// you can provide credentials to multiple WiFi networks
WiFiMulti wifiMulti;

// store index.html content in html constant variable (platformio feature)
extern const char index_html_start[] asm("_binary_src_index_html_start");
const String html = String((const char*)index_html_start);
AsyncWebServer server(PORT);

// Task functions
void taskWifi(void *parameter);

void setup() {
  Serial.begin(115200);

#if ENABLE_TFT == 1
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);
#endif

  // LED and Button config
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  btn.setEventHandler(handleButtonEvent);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Save Wi-Fi credentials
  for (int i = 0; i < (sizeof(wifiNetworks)/sizeof(wifiNetworks[0])); i++) {
    wifiMulti.addAP(wifiNetworks[i][0], wifiNetworks[i][1]);
    Serial.printf("WiFi %d: SSID: \"%s\" ; PASS: \"%s\"\r\n", i, wifiNetworks[i][0], wifiNetworks[i][1]);
  }

  // Husarnet VPN configuration 
  Husarnet.selfHostedSetup(dashboardURL);
  Husarnet.join(husarnetJoinCode, hostname);

  // A dummy web server (see index.html)
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

void taskWifi(void *parameter) {
  uint8_t stat = WL_DISCONNECTED;

  while (stat != WL_CONNECTED) {
    stat = wifiMulti.run();
    Serial.printf("WiFi status: %d\r\n", (int)stat);
    delay(100);
  }

  Serial.printf("WiFi connected\r\n");

  // Start Husarnet VPN Client
  Husarnet.start();

  // Start HTTP server
  server.begin();

  LOG("READY!\r\n");

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

void handleButtonEvent(AceButton *button, uint8_t eventType, uint8_t buttonState) {
  ledState = (buttonState==1?0:1);

  for (auto const &host : Husarnet.listPeers()) {
    IPv6Address peerAddr = host.first;
    if(host.second == "master") {
      ;
    } else {
      AsyncClient* client_tcp = new AsyncClient;
      
      client_tcp->onConnect([](void *arg, AsyncClient *client) {
        String requestURL = "/led/1/state/" + String(ledState);
        String GETreq = String("GET ") + requestURL + " HTTP/1.1\r\n" + "Host: esp32\r\n" + "Connection: close\r\n\r\n";

        if ( client->canSend() && (client->space() > GETreq.length())){
          client->add(GETreq.c_str(), strlen(GETreq.c_str()));
	        client->send();
        } else {
          Serial.printf("\r\nSENDING ERROR!\r\n");
        }
      }, client_tcp);

      client_tcp->onData([](void *arg, AsyncClient *client, void *data, size_t len) {
        Serial.printf("\r\nResponse from %s\r\n", client->remoteIP().toString().c_str());
	      Serial.write((uint8_t *)data, len);
        client->close();
      }, client_tcp);

      client_tcp->onDisconnect([](void* arg, AsyncClient* client) {
        Serial.println("[CALLBACK] discconnected");
        delete client;
      }, client_tcp);
      
      client_tcp->onError([](void* arg, AsyncClient* client, int8_t error) {
        Serial.printf("[CALLBACK] error: %d\r\n", error);
      }, NULL);

      client_tcp->onTimeout([](void* arg, AsyncClient* client, uint32_t time) {
        Serial.println("[CALLBACK] ACK timeout");
      }, NULL);
      
      client_tcp->connect(peerAddr, PORT);

      LOG("Sending HTTP req to:\r\n%s:\r\n%s\r\n\r\n", host.second.c_str(), host.first.toString().c_str());
    }
  }
}