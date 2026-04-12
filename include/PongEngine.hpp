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

        float speedInit = 0.75f;
        float speedMin  = 0.5f;
        float speedMax  = 2.25f;
    } ball;

    struct {
        float power = 2.75f;
        int range   = 10;

        CRGB color[2]  = {CRGB::Red, CRGB::Yellow};

        uint32_t animationDecayUs = 450*1000;
        uint32_t deadtimeUs       = 250*1000;
    } paddle;

    struct {
        uint32_t scoreAnimationDurationUs = 1000*1000;
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
    void setPaddleHit(Paddles paddle);

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
    void drawPointScoredBy(Paddles at);

    CRGB sampleBallAt(float x) const;
    CRGB samplePaddleAt(Paddles paddle, float x) const;

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
        GameState state = WaitingForStartAtAny;
        uint32_t transitionUs = 0;
    } mGameState;

    uint32_t mCurrentUpdateTimeUs = 0;
    uint32_t mLastUpdateTimeUs = 0;

    struct {
        float position = 0.0f; // Ball position (0.0 to 1.0)
        float velocity = 0.0f; // Ball velocity (units per update)

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