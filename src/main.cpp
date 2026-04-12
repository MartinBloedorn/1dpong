#include <Arduino.h>
#include <FastLED.h>
#include <OneButton.h>

#include "ConfigWebServer.hpp"
#include "PongEngine.hpp"
#include "Logger.h"

// WiFi credentials
const char *WIFI_SSID = "MagentaWLAN-QZUJ";
const char *WIFI_PASSWORD = "25183127771347211799";

// WS2812 LED configuration
#define LED_PIN 21
#define NUM_LEDS 1

#define LED_STRIP_PIN 1
#define NUM_STRIP_LEDS 30

// Button configuration
#define BUTTON_PIN 6

CRGB leds[NUM_LEDS];
CRGB ledStrip[NUM_STRIP_LEDS];

PongEngine pongEngine(ledStrip, NUM_STRIP_LEDS);
OneButton button(BUTTON_PIN, true);    // Active low with pull-up
ConfigWebServer *webServer = nullptr;

// Callback for when configuration is saved
void onConfigSaved(const ConfigParameters &params)
{
    Serial.println("\n=== Configuration Saved ===");
    Serial.print("IP Address: ");
    Serial.println(params.ipAddress.c_str());
    Serial.print("Port: ");
    Serial.println(params.port);
    Serial.print("Slider Value: ");
    Serial.println(params.sliderValue);
    Serial.println("============================\n");
}

// Callback for button press
void onButtonPressed()
{
    // Serial.println("Button pressed!");
    pongEngine.setPaddleHit(PongEngine::Paddles::A);
    pongEngine.setPaddleHit(PongEngine::Paddles::B);
    // Add your button press logic here
}

// Test pattern: color chase through the strip
void testLEDStrip()
{
    static uint8_t hue = 0;
    static uint8_t position = 0;

    // Clear the strip
    fill_solid(ledStrip, NUM_STRIP_LEDS, CRGB::Black);

    // Light up current position with a color
    ledStrip[position] = CHSV(hue, 255, 255);

    FastLED.show();

    position++;
    if (position >= NUM_STRIP_LEDS)
    {
        position = 0;
        hue += 30;  // Change color every cycle
    }
}

void setup()
{
    Serial.begin(115200);
    while (!Serial)
    {
    };

    // Initialize button
    button.attachPress(onButtonPressed);
    button.setClickMs(0);  // Debounce time in ms

    // Initialize FastLED
    FastLED.addLeds<WS2812, LED_PIN, RGB>(leds, NUM_LEDS);
    FastLED.addLeds<WS2812, LED_STRIP_PIN, RGB>(ledStrip, NUM_STRIP_LEDS);
    FastLED.setBrightness(100);

    pongEngine.init();

    Serial.println("\n1D Pong Started!");

    ConfigParameters initialConfig = {"192.168.1.100", 9000, 3.25f};

    // webServer = new ConfigWebServer(WIFI_SSID, WIFI_PASSWORD, 80);
    // webServer->setInitialConfig(initialConfig);
    // webServer->setConfigCallback(onConfigSaved);
    // webServer->begin();
}

void loop()
{
    static unsigned long lastBlink = 0;
    static bool ledState = false;

    // Handle button input with debouncing
    button.tick();

    pongEngine.update();
    // pongEngine.debug(2000*1000);
    FastLED.show();

    // Update LED strip test pattern every 100ms
    // testLEDStrip();

    // Blink single LED every 500ms
    // if (millis() - lastBlink >= 500)
    // {
    //     lastBlink = millis();
    //     ledState = !ledState;
    //     leds[0] = ledState ? CRGB::Red : CRGB::Black;
    //     FastLED.show();
    // }

    delay(5);
}