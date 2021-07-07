# ESP32-to-ESP32
*ESP32 to ESP32 communication example using Arduino framework. Secure, P2P, low latency connection between devices is established. Button connected to the first ESP32 controlls LED connected to second ESP32.*

- ESP32 acts both as a HTTP server and HTTP client (listening on port 8001)
- ESP32 automatically detects all peers in the same network
- when the button is pressed, HTTP request is sent to other peers and turn the LED on
- when the button is released, HTTP request is sent to other peers and turn on the LED off

## Default hardware configuration:

- LED is connected to pin: **27**
- button is connected to pin: **0**

## Running a project

1. Clone this repo
2. Open the repo folder using VSC with installed Platformio extension
3. Provide your Wi-Fi networks credentials here:

```cpp
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
