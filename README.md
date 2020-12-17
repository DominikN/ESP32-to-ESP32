# ESP32-to-ESP32
*ESP32 to ESP32 communication example using Arduino framework. Secure, P2P, low latency connection between devices is established. Button connected to the first ESP32 controlls LED connected to second ESP32.*


Edit: This project was reorganized for Platfromio. So just clone this repo, and open the project folder using platformio and Visual Studio Code.


~~Open Arduino IDE and follow these steps:~~

~~**1. Install Husarnet IDF for ESP32:**~~
~~* open ```File -> Preferences```~~
~~* in a field **Additional Board Manager URLs** add this link: `https://files.husarion.com/arduino/package_esp32_index.json`~~
~~* open ```Tools -> Board: ... -> Boards Manager ...```~~
~~* Search for `esp32-husarnet by Husarion`~~
~~* Click Install button~~

~~**2. Select ESP32 dev board:**~~
~~* open ```Tools -> Board```~~
~~* select ***ESP32 Dev Module*** under "ESP32 Arduino (Husarnet)" section~~

~~**3. Run demo:**
~~* Open **ESP32_ledstrip_webserver.ino** project~~
~~* modify line 25 with your Husarnet join code (get on https://app.husarnet.com)~~
~~* modify lines 28 - 38 to add your Wi-Fi network credentials~~
~~* upload project to your ESP32 board. Repeat the previous steps with ```#define DEV_TYPE 1``` for 2nd ESP32 module~~
~~* power both ESP32 modules and wait about 15 seconds to allow your ESP32 devices connect to Wi-Fi network and establish P2P connection (works both in LAN and through the internet).~~


