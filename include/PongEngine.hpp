#pragma once

#include <FastLED.h>

#include <optional>
#include <functional>

struct GameParameters
{
    struct {
        CRGB color  = CRGB::Green;
        int width   = 3;

        float motionBlurWidth = 5.0f;       // As a multiplier of width
        float motionBlurStrength = 0.33f;

        float speedInit = 0.55f;
        float speedMin  = 0.5f;
        float speedMax  = 2.20f;
    } ball;

    struct {
        float power = 1.5f;
        int range   = 20;

        CRGB color[2]  = {CRGB::Red, CRGB::Yellow};

        uint32_t animationDecayUs = 450*1000;
        uint32_t deadtimeUs       = 250*1000;
    } paddle;

    struct {
        float speedMinIncreasePerHit = 0.075f;
        uint32_t scoreAnimationDurationUs   =  1*1500*1000;
        uint32_t maxWaitForPlayerTimeUs     = 15*1000*1000;
        uint32_t maxWaitUntilIdleAnimation  = 30*1000*1000; 
    } gameplay;
};

class PongEngine
{   
public:
    enum Paddle
    {
        A = 0,
        B = 1
    };

    PongEngine(CRGB* leds, int numLeds);

    void init();
    void debug(int intervalUs);

    void update();
    void setPaddleHit(Paddle paddle);

    GameParameters gameParameters() const;
    void setGameParameters(const GameParameters& params);

private:
    enum GameState
    {
        IdleAnimation,
        WaitingForStartAtAny,
        WaitingForStartAtA,
        WaitingForStartAtB,
        Playing,
        PointScoredByA,
        PointScoredByB
    };

    void updateWaitingForStart(Paddle at);
    void drawWaitingForStart(std::optional<Paddle> at = std::nullopt);
    void drawIdleAnimation();

    void updateBallPositionAndSpeed();
    void drawPlaying();

    void updatePointScoredBy(Paddle at);
    void drawPointScoredBy(Paddle at);

    CRGB sampleBallAt(float x) const;
    CRGB samplePaddleAt(Paddle paddle, float x) const;
    CRGB samplePaddleWaitAnimationAt(Paddle paddle, float x) const;

    void transitionIntoState(GameState state);

    inline float normalizeToLength(float v) const { 
        return v/((float)mMaxNumLeds);
    }

    void draw(std::function<CRGB(int, float)>&& f) {
        for(int i = 0; i < mMaxNumLeds; ++i) {
            mLeds[i] = f(i, normalizeToLength(i + 1));
        }
    }

    struct {
        GameState state = IdleAnimation;
        uint32_t transitionUs = 0;
    } mGameState;

    uint32_t mCurrentUpdateTimeUs = 0;
    uint32_t mLastUpdateTimeUs = 0;


    struct {
        float position = 0.0f; // Ball position (0.0 to 1.0)
        float velocity = 0.0f; // Ball velocity (units per update)

        uint32_t hitCounter = 0;
    } mBallState;

    CRGB* mLeds = nullptr;
    int mMaxNumLeds;

    struct PaddleHit {
        uint32_t timeUs = 0;    // Time of last hit
        bool isActive = false;  // Whether the paddle hit effect is active
    };

    PaddleHit mPaddleHit[2];

    GameParameters mGameParameters;
};