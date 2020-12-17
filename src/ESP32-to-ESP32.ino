#include <WiFi.h>
#include <WiFiMulti.h>
#include <Husarnet.h>
#include <AceButton.h>

#include <SPI.h>
#include <TFT_eSPI.h>

using namespace ace_button;

/* =============== config section start =============== */

#define DEV_TYPE 1 // type "0" for 1st ESP32, and "1" for 2nd ESP32

const int BUTTON_PIN = 35;
const int LED_PIN = 17;
const int PORT = 8001;

#if DEV_TYPE == 0
const char *myHostName = "esp-a";
const char *peerHostName = "esp-b";
#else
const char *myHostName = "esp-b";
const char *peerHostName = "esp-a";
#endif

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
#define NUM_NETWORKS 2 //number of Wi-Fi network credentials saved

const char *ssidTab[NUM_NETWORKS] = {
    "wifi-ssid-one",
    "wifi-ssid-two",
};

const char *passwordTab[NUM_NETWORKS] = {
    "wifi-pass-one",
    "wifi-pass-two",
};

#endif

/* =============== config section end =============== */

AceButton btn(BUTTON_PIN);

// you can provide credentials to multiple WiFi networks
WiFiMulti wifiMulti;

HusarnetServer server(PORT);
HusarnetClient clientTx;
HusarnetClient clientRx;

void handleEvent(AceButton *, uint8_t, uint8_t);
void taskWifi(void *parameter);
void taskConnection(void *parameter);

TFT_eSPI tft = TFT_eSPI(); // Invoke custom library

void setup()
{
  Serial.begin(115200);

  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE,TFT_BLACK);  
  tft.setTextSize(2);
  Serial.begin(115200);
  tft.printf("Hostname:\r\n>%s", myHostName);

  //--------------------

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  btn.setEventHandler(handleEvent);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  xTaskCreate(
      taskWifi,   /* Task function. */
      "taskWifi", /* String with name of task. */
      10000,      /* Stack size in bytes. */
      NULL,       /* Parameter passed as input of the task */
      1,          /* Priority of the task. */
      NULL);      /* Task handle. */
}

void loop()
{
  while (1)
  {
    btn.check();
    delay(1);
  }
}

void handleEvent(AceButton *button, uint8_t eventType,
                 uint8_t buttonState)
{
  switch (eventType)
  {
  case AceButton::kEventPressed:
    Serial.println("pressed");
    break;
  case AceButton::kEventReleased:
    Serial.println("released");
    break;
  }
}

void taskWifi(void *parameter)
{
  uint8_t stat = WL_DISCONNECTED;

  /* Configure Wi-Fi */
  for (int i = 0; i < NUM_NETWORKS; i++)
  {
    wifiMulti.addAP(ssidTab[i], passwordTab[i]);
    Serial.printf("WiFi %d: SSID: \"%s\" ; PASS: \"%s\"\r\n", i, ssidTab[i], passwordTab[i]);
  }

  while (stat != WL_CONNECTED)
  {
    stat = wifiMulti.run();
    Serial.printf("WiFi status: %d\r\n", (int)stat);
    delay(100);
  }

  Serial.printf("WiFi connected\r\n", (int)stat);
  Serial.printf("IP address: ");
  Serial.println(WiFi.localIP());

  /* Start Husarnet */
  Husarnet.selfHostedSetup(dashboardURL);
  Husarnet.join(husarnetJoinCode, myHostName);
  Husarnet.start();

  uint8_t oldState = btn.getLastButtonState();
  while (1)
  {
    while (WiFi.status() == WL_CONNECTED)
    {
      server.begin();
      if (clientTx.connected() == false)
      {
        clientTx = server.available();
      }

      if (clientRx.connected() == false)
      {
        clientRx.connect(peerHostName, PORT);
      }

      if ((clientRx.connected() == true) && (clientTx.connected() == true))
      {
        if (oldState != btn.getLastButtonState())
        {
          char txch;
          oldState = btn.getLastButtonState();
          if (oldState == 0)
          {
            txch = 'a';
          }
          else
          {
            txch = 'b';
          }
          clientTx.print('a');
          Serial.printf("clientTx: write: %c\r\n", txch);
          tft.printf("clientTx: write: %c\r\n", txch);
        }

        if (clientRx.available())
        {
          char c = clientRx.read();

          if (c == 'a')
          {
            digitalWrite(LED_PIN, HIGH);
          }
          if (c == 'b')
          {
            digitalWrite(LED_PIN, LOW);
          }

          Serial.printf("clientRx: read: %c\r\n", c);
          tft.printf("clientRx: read: %c\r\n", c);
        }
      }

      delay(5);
    }
    Serial.printf("WiFi disconnected, reconnecting\r\n");
    delay(500);
    stat = wifiMulti.run();
    Serial.printf("WiFi status: %d\r\n", (int)stat);
  }
}
