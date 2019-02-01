#include <WiFi.h>
#include <Husarnet.h>
#include <AceButton.h>

using namespace ace_button;

/* =============== config section start =============== */

#define DEV_TYPE 0    // type "0" for 1st ESP32, and "1" for 2nd ESP32

const int BUTTON_PIN = 22;
const int LED_PIN = 16;

// Husarnet credentials
const char* hostName0 = "esp2esp0";  //this will be the name of the 1st ESP32 device at https://app.husarnet.com
const char* hostName1 = "esp2esp1";  //this will be the name of the 2nd ESP32 device at https://app.husarnet.com

/* to get your join code go to https://app.husarnet.com
   -> select network
   -> click "Add element"
   -> select "join code" tab

   Keep it secret!
*/
const char* husarnetJoinCode = "xxxxxxxxxxxxxxxxxxxxxx";

// WiFi credentials
#define NUM_NETWORKS 2  //number of Wi-Fi network credentials saved

const char* ssidTab[NUM_NETWORKS] = {
  "wifi-ssid-one",
  "wifi-ssid-two",
};

const char* passwordTab[NUM_NETWORKS] = {
  "wifi-pass-one",
  "wifi-pass-two",
};

/* =============== config section end =============== */

AceButton btn(BUTTON_PIN);

HusarnetClient client;

uint16_t port = 8001;

#if DEV_TYPE == 0
HusarnetServer server(port);
#endif

void handleEvent(AceButton*, uint8_t, uint8_t);
void taskWifi( void * parameter );
void taskConnection( void * parameter );

void setup() {
  Serial.begin(115200);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  btn.setEventHandler(handleEvent);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  xTaskCreate(
    taskWifi,          /* Task function. */
    "taskWifi",        /* String with name of task. */
    10000,            /* Stack size in bytes. */
    NULL,             /* Parameter passed as input of the task */
    1,                /* Priority of the task. */
    NULL);            /* Task handle. */

  xTaskCreate(
    taskConnection,          /* Task function. */
    "taskConnection",        /* String with name of task. */
    10000,            /* Stack size in bytes. */
    NULL,             /* Parameter passed as input of the task */
    1,                /* Priority of the task. */
    NULL);            /* Task handle. */
}

void loop() {
  while (1) {
    btn.check();
    delay(1);
  }
}

void handleEvent(AceButton* button, uint8_t eventType,
                 uint8_t buttonState) {
  switch (eventType) {
    case AceButton::kEventPressed:
      Serial.println("pressed");
      break;
    case AceButton::kEventReleased:
      Serial.println("released");
      break;
  }
}

void taskWifi( void * parameter ) {
  while (1) {
    for (int i = 0; i < NUM_NETWORKS; i++) {
      Serial.print("Connecting to ");
      Serial.print(ssidTab[i]);
      WiFi.begin(ssidTab[i], passwordTab[i]);
      for (int j = 0; j < 10; j++) {
        if (WiFi.status() != WL_CONNECTED) {
          delay(500);
          Serial.print(".");
        } else {
          Serial.println("done");
          Serial.print("IP address: ");
          Serial.println(WiFi.localIP());
#if DEV_TYPE == 0
          Husarnet.join(husarnetJoinCode, hostName0);
#elif DEV_TYPE == 1
          Husarnet.join(husarnetJoinCode, hostName1);
#endif
          Husarnet.start();
          while (WiFi.status() == WL_CONNECTED) {
            delay(500);
          }
        }
      }
    }
  }
}

void taskConnection( void * parameter ) {
  uint8_t oldState = btn.getLastButtonState();

  while (1) {
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
    }

#if DEV_TYPE == 0

    server.begin();
    Serial.println("Waiting for client");

    do {
      delay(500);
      client = server.available();
      client.setTimeout(3);
    } while (client < 1);

#elif DEV_TYPE == 1

    Serial.printf("Connecting to %s\r\n", hostName0);
    while (client.connect(hostName0, port) == 0) {
      delay(500);
    }

#endif

    Serial.printf("Client connected: %d\r\n", (int)client.connected());

    unsigned long lastMsg = millis();
    auto lastPing = 0;

    while (client.connected()) {
      //ping RX
      if (millis() - lastMsg > 6000) {
        Serial.println("ping timeout");
        break;
      }

      //ping TX
      auto now = millis();
      if (now > lastPing + 4000) {
        client.print('p');
        lastPing = now;
      }

      if (oldState != btn.getLastButtonState()) {
        oldState = btn.getLastButtonState();
        if (oldState == 0) {
          client.print('a');
        } else {
          client.print('b');
        }
      }

      if (client.available()) {
        char c = client.read();

        if (c == 'p') {
          lastMsg = millis();
        }
        if (c == 'a') {
          digitalWrite(LED_PIN, HIGH);
        }
        if (c == 'b') {
          digitalWrite(LED_PIN, LOW);
        }

        Serial.printf("read: %c\r\n", c);
      }
    }
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}
