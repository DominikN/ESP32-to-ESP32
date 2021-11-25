# ESP32-to-ESP32
*ESP32 to ESP32 communication example using Arduino framework. Secure, P2P, low latency connection between devices is established. Button connected to the first ESP32 controlls LED connected to second ESP32.*

- ESP32 acts both as a HTTP server (based on `ESPAsyncWebServer` library) and HTTP client (based on `AsyncTCP`)
- ESP32 automatically detects all peers in the same Husarnet VPN network
- when the button is pressed, HTTP request is sent to all other peers and turn the LED on
- when the button is released, HTTP request is sent to all other peers and turn on the LED off

## Default hardware configuration:

- LED is connected to pin: **27**
- button is connected to pin: **0**

## Running a project

1. Clone this repo

2. Open the repo folder using VSC with installed Platformio extension

3. Provide your Wi-Fi networks credentials here:

```cpp
// WiFi credentials
const char* wifiNetworks[][2] = {
  {"wifi-ssid-one", "wifi-pass-one"},
  {"wifi-ssid-two", "wifi-pass-two"},
} 
```

4. Get your Husarnet VPN Join Code (allowing you to connect devices to the same VPN network)

> You will find your Join Code at **https://app.husarnet.com  
> -> Click on the desired network  
> -> `Add element` button  
> -> `Join Code` tab** 

5. Place your Husarnet Join Code here:

```cpp
const char *husarnetJoinCode = "fc94:b01d:1803:8dd8:b293:5c7d:7639:932a/xxxxxxxxxxxxxxxxxxxxxx";
```

6. Flash at least two ESP32 devices and enjoy

> Note that each ESP32 runs it's own HTTP server, so you can test not only ESP32 <> ESP32 communication but also web browser <> ESP32 (by just typing `http://random:8001/led/1/state/0`). Remember to use the same Join Code to connect your laptop to the same VPN network as ESP32.

## Extra notes

If you want to add Over The Air (OTA) firmware update from GitHub Actions workflow, check out this project - https://github.com/husarnet/esp32-internet-ota