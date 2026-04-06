#include <Arduino.h>
#include <FastLED.h>
#include "ConfigWebServer.hpp"

// WiFi credentials
const char *WIFI_SSID = "MagentaWLAN-QZUJ";
const char *WIFI_PASSWORD = "25183127771347211799";

// WS2812 LED configuration
#define LED_PIN 21
#define NUM_LEDS 1

CRGB leds[NUM_LEDS];
ConfigWebServer *webServer = nullptr;

// Callback for when configuration is saved
void onConfigSaved(const ConfigParameters &params)
{
    Serial.println("\n=== Configuration Saved ===");
    Serial.print("IP Address: ");
    Serial.println(params.ipAddress.c_str());
    Serial.print("Port: ");
    Serial.println(params.port);
    Serial.println("============================\n");
}

void setup()
{
    Serial.begin(115200);
    while (!Serial)
    {
    };

    // Initialize FastLED
    FastLED.addLeds<WS2812, LED_PIN, RGB>(leds, NUM_LEDS);
    FastLED.setBrightness(100);

    Serial.println("\n\nLED blinky started!");

    // Initialize and start web server
    webServer = new ConfigWebServer(WIFI_SSID, WIFI_PASSWORD, 80);

    // Set initial configuration values
    ConfigParameters initialConfig = {"192.168.1.100", 9000};
    webServer->setInitialConfig(initialConfig);

    // Set the callback for configuration changes
    webServer->setConfigCallback(onConfigSaved);

    // Start the web server
    webServer->begin();
}

void loop()
{
    // Serial.println("Turning LED on and off...");

    // Turn LED red
    leds[0] = CRGB::Red;
    FastLED.show();
    delay(500);

    // Turn LED off
    leds[0] = CRGB::Black;
    FastLED.show();
    delay(500);
}