#pragma once

#include <FastLED.h>

#include <optional>
#include <functional>

struct GameParameters
{
    struct {
        CRGB color  = CRGB::Green;
        int width   = 3;

        float speedMin = 0.5f;
        float speedMax = 10.0f;
    } ball;

    struct {
        float power = 4.0f;
        int range   = 5;

        uint32_t deadtimeUs = 250*1000;
    } paddle;

    struct {
        
    } gameplay;
};

class PongEngine
{   
public:
    enum Paddles
    {
        A = 0,
        B = 1
    };

    PongEngine(CRGB* leds, int numLeds);

    void init();
    void debug(int intervalUs);

    void update();
    void setPaddleHit(Paddles paddle, uint32_t timeUs = 0);

    GameParameters gameParameters() const;
    void setGameParameters(const GameParameters& params);

private:
    enum GameState
    {
        WaitingForStartAtAny,
        WaitingForStartAtA,
        WaitingForStartAtB,
        Playing,
        PointScoredByA,
        PointScoredByB
    };

    void updateWaitingForStart(Paddles at);
    void drawWaitingForStart(std::optional<Paddles> at = std::nullopt);

    void updateBallPositionAndSpeed();
    void drawPlaying();

    void updatePointScoredBy(Paddles at);

    CRGB sampleBallAt(float x) const;
    CRGB samplePaddleAt(Paddles paddle, float x) const;

    void transitionIntoState(GameState state);

    void draw(std::function<CRGB(int, float)>&& f) {
        for(int i = 0; i < mMaxNumLeds; ++i) {
            float x = ((float)i)/((float)(mMaxNumLeds - 1));
            mLeds[i] = f(i, x);
        }
    }

    struct {
        GameState state = WaitingForStartAtAny;
        uint32_t transitionUs = 0;
    } mGameState;

    uint32_t mLastUpdateTimeUs = 0;

    struct {
        float position = 0.0f; // Ball position (0.0 to 1.0)
        float velocity = 0.0f; // Ball velocity (units per update)
    
        float width = 0.01f; // Ball width (0.0 to 1.0)
        CRGB color  = CRGB::Green; // Ball color
    } mBallState;

    CRGB* mLeds = nullptr;
    int mMaxNumLeds;

    struct PaddleHit {
        uint32_t timeUs = 0;    // Time of last hit
        bool isActive = false;  // Whether the paddle hit effect is active
    };

    PaddleHit mPaddleHit[2];
};