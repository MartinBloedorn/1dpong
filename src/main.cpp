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

#define LED_STRIP_PIN 6
#define NUM_STRIP_LEDS 190

// Button configuration
#define BUTTON_PIN 6

static const int BUTTON_PADDLE_LOCAL = 5;
static const int BUTTON_PADDLE_REMOTE = 2;

CRGB leds[NUM_LEDS];
CRGB ledStrip[NUM_STRIP_LEDS];

PongEngine pongEngine(ledStrip, NUM_STRIP_LEDS);
OneButton button(BUTTON_PIN, true);    // Active low with pull-up
ConfigWebServer *webServer = nullptr;

// Interrupt handler for button press
void IRAM_ATTR onButtonFallingEdge()
{
    pongEngine.setPaddleHit(PongEngine::Paddle::A);
    pongEngine.setPaddleHit(PongEngine::Paddle::B);
}

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

static bool buttonFell = false;

// Callback for button press
void onButtonPressed()
{
    // Serial.println("Button pressed!");
    // pongEngine.setPaddleHit(PongEngine::Paddle::A);
    // pongEngine.setPaddleHit(PongEngine::Paddle::B);
    buttonFell = true;
    // Add your button press logic here
}

// Fast GPIO read directly from register
bool fastDigitalRead(uint8_t pin)
{
    return (GPIO.in >> pin) & 1;
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
    // while (!Serial) {};
    delay(250);
    Logger::setEnabled(Serial);

    // Attach interrupt to button pin (falling edge, active-low)
    // attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), onButtonFallingEdge, FALLING);

    // Initialize button
    // button.attachPress(onButtonPressed);
    // button.setClickMs(0);  // Debounce time in ms

    // Initialize FastLED
    FastLED.addLeds<WS2812, LED_PIN, RGB>(leds, NUM_LEDS);
    FastLED.addLeds<WS2812, LED_STRIP_PIN, GRB>(ledStrip, NUM_STRIP_LEDS);
    FastLED.setBrightness(100);

    pinMode(BUTTON_PADDLE_LOCAL, INPUT_PULLUP);
    pinMode(BUTTON_PADDLE_REMOTE, INPUT);

    pongEngine.init();

    Serial.println("\n1D Pong Started!");

    ConfigParameters initialConfig = {"192.168.1.100", 9000, 3.25f};

    // webServer = new ConfigWebServer(WIFI_SSID, WIFI_PASSWORD, 80);
    // webServer->setInitialConfig(initialConfig);
    // webServer->setConfigCallback(onConfigSaved);
    // webServer->begin();
}

static bool localPaddleActive = false;
static bool remotePaddleActive = false;

void loop()
{
    // Handle button input with debouncing
    // button.tick();

    bool localPaddlePressed = !fastDigitalRead(BUTTON_PADDLE_LOCAL);
    bool remotePaddlePressed = !fastDigitalRead(BUTTON_PADDLE_REMOTE);

    // if(localPaddlePressed)
    //     Serial.println("LOCAL PADDLE!");

    if(!localPaddleActive && localPaddlePressed) {
        localPaddleActive = true;
        pongEngine.setPaddleHit(PongEngine::Paddle::A);
    } else if(localPaddleActive && !localPaddlePressed) {
        localPaddleActive = false;
    }

    if(!remotePaddleActive && remotePaddlePressed) {
        remotePaddleActive = true;
        pongEngine.setPaddleHit(PongEngine::Paddle::B);
    } else if(remotePaddleActive && !remotePaddlePressed) {
        remotePaddleActive = false;
    }

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