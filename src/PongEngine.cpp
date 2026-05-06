#include "PongEngine.hpp"

#include "Logger.h"

#include <Arduino.h>

PongEngine::PongEngine(CRGB* leds, int numLeds)
    : mLeds(leds)
    , mMaxNumLeds(numLeds)
{
}

void PongEngine::init()
{
    transitionIntoState(WaitingForStartAtAny);
}

void PongEngine::debug(int intervalUs)
{
    static uint32_t lasttd = 0;

    if(micros() - lasttd > intervalUs) 
    {
        Logger::debug(Serial, "* mGameState.state     = ", mGameState.state);
        Logger::debug(Serial, "* mBallState.position  = ", mBallState.position);
        Logger::debug(Serial, "* mBallState.velocity  = ", mBallState.velocity);

        lasttd = micros();
    }
}

void PongEngine::setPaddleHit(Paddle paddle)
{
    uint32_t t = micros();

    if(t - mPaddleHit[paddle].timeUs > mGameParameters.paddle.deadtimeUs) {
        Logger::debug(Serial, "setPaddleHit(", paddle, ")");
        mPaddleHit[paddle].timeUs   = t;
        mPaddleHit[paddle].isActive = true;
    }
}

GameParameters PongEngine::gameParameters() const
{
    return mGameParameters;
}

void PongEngine::setGameParameters(const GameParameters &params)
{
    mGameParameters = params;
}

void PongEngine::update()
{
    mCurrentUpdateTimeUs = micros();

    switch(mGameState.state) 
    {
        case WaitingForStartAtAny:
            updateWaitingForStart(Paddle::A);
            updateWaitingForStart(Paddle::B);
            drawWaitingForStart(std::nullopt);
            break;
        case WaitingForStartAtA:
            updateWaitingForStart(Paddle::A);
            drawWaitingForStart(Paddle::A);
            break;
        case WaitingForStartAtB:
            updateWaitingForStart(Paddle::B);
            drawWaitingForStart(Paddle::B);
            break;
        case Playing:
            updateBallPositionAndSpeed();
            drawPlaying();
            break;
        case PointScoredByA:
            updatePointScoredBy(Paddle::A);
            drawPointScoredBy(Paddle::A);
            break;
        case PointScoredByB:
            updatePointScoredBy(Paddle::B);
            drawPointScoredBy(Paddle::B);
            break;
        default:
            break;
    }

    mLastUpdateTimeUs = mCurrentUpdateTimeUs;
}

void PongEngine::updateWaitingForStart(Paddle at)
{
    if(mPaddleHit[at].isActive) {
        mBallState.position = (at == Paddle::A ? 0.0f : 1.0f);
        mBallState.velocity = (at == Paddle::A ? 1.0f :-1.0f) * mGameParameters.ball.speedInit;
        mPaddleHit[at].isActive = false;
        Logger::debug(Serial, "Started: ", at, ", ", mBallState.position, ", ", mBallState.velocity);
        transitionIntoState(GameState::Playing);
    }
}

void PongEngine::drawWaitingForStart(std::optional<Paddle> at)
{
    draw([](int i, float x) {
        return CRGB{};
    });
}

void PongEngine::updateBallPositionAndSpeed()
{
    uint32_t t     = mCurrentUpdateTimeUs;
    auto& bs       = mBallState;
    const auto& gp = mGameParameters;

    float dt = (t - mLastUpdateTimeUs) / 1000000.0f;
    bs.position += bs.velocity*dt;

    if(bs.position > 1.0f) {
        transitionIntoState(PointScoredByA);
        return;
    } else if(bs.position < 0.0f) {
        transitionIntoState(PointScoredByB);
        return;
    }

    auto paddleToCheck = bs.velocity > 0.0f ? Paddle::B : Paddle::A;
    auto& paddle = mPaddleHit[paddleToCheck];

    if(paddle.isActive) {
        float k = gp.paddle.power;
        float q = normalizeToLength(gp.paddle.range);

        float x = bs.position;
        float d = paddleToCheck == Paddle::A ? x : (1.0f - x);
        float e = k * fl::clamp(1.0f - d / q, 0.0f, 1.0f);
        
        paddle.isActive = false;

        if(e > 0.01f) {
            bs.hitCounter++;

            float dir = bs.velocity > 0.0f ? -1.0f : 1.0f;
            float vel = fabs(e * bs.velocity);
            float min = gp.ball.speedMin + ((float)bs.hitCounter * gp.gameplay.speedMinIncreasePerHit);

            min = fl::clamp(min, gp.ball.speedMin, 0.9f * gp.ball.speedMax);
            vel = fl::clamp(vel, min, gp.ball.speedMax);
            bs.velocity = dir * vel;

            Logger::debug(Serial, "Hit! ", paddleToCheck, ": ", x, ", ", d, ", ", e, " -> ", bs.velocity);
        }
    }
}

void PongEngine::drawPlaying()
{
    draw([this](int, float x) {
        auto ballp = sampleBallAt(x);
        auto paddlep = samplePaddleAt(Paddle::A, x) + samplePaddleAt(Paddle::B, x);
        return CRGB::blend(ballp, paddlep, 128); // Blend ball and paddle colors
    });
}

void PongEngine::updatePointScoredBy(Paddle at)
{
    if(mCurrentUpdateTimeUs - mGameState.transitionUs > mGameParameters.gameplay.scoreAnimationDurationUs)
        transitionIntoState(at == Paddle::A ? WaitingForStartAtA : WaitingForStartAtB);
}

void PongEngine::drawPointScoredBy(Paddle at)
{
    draw([this, at](int, float x) {
        float d = at == Paddle::A ? x : (1.0f - x); 

        if(d < 0.25f)
            return CRGB::Green;
        if(d > 0.75f)
            return CRGB::Red;
            
        return CRGB::Black;
    });
}

CRGB PongEngine::sampleBallAt(float x) const
{
    const auto& bs = mBallState;
    const auto& gp = mGameParameters;

    // Normalize coordinates so that x>0 -> in front of ball, x<0 -> behind ball:
    x = (bs.velocity < 0.0f) ? bs.position - x : x - bs.position;

    float relv = (fabs(bs.velocity) - gp.ball.speedMin) / (gp.ball.speedMax - gp.ball.speedMin);

    // float v = fl::clamp(1.0f - fabs(x) / (normalizeToLength(gp.ball.width) / 2.0f), 0.0f, 1.0f);
    float w = normalizeToLength(gp.ball.width) / 2.0f;
    float v = fl::clamp(1.0f - fabs(x) / w, 0.0f, 1.0f);

    // "Motion blur":
    float b = x < 0.0f 
            ? gp.ball.motionBlurStrength * (1.0f - fabs(x) / (w * (relv + 1.0f) * gp.ball.motionBlurWidth))
            : 0.0f;
    b = fl::clamp(b, 0.0f, 1.0f);

    // v += fl::clamp(b, 0.0f, 1.0f);
    float cc = 255.f * v;
    float bc = 255.f * b;

    auto ballp = CRGB::blend(CRGB(0, cc, 0), CRGB(cc, 0, 0), (relv*255.0f)); 
    auto blurp = CRGB(bc, bc, bc);

    return CRGB::blend(ballp, blurp, 128);
}

CRGB PongEngine::samplePaddleAt(Paddle paddle, float x) const
{
    const auto& hitTimeUs = mPaddleHit[paddle].timeUs;
    const auto& gp        = mGameParameters;
    const uint32_t t      = mCurrentUpdateTimeUs;

    if(!hitTimeUs || t - hitTimeUs > gp.paddle.animationDecayUs)
        return CRGB::Black;

    float paddleSize = normalizeToLength(gp.paddle.range);
    float d = paddle == Paddle::A ? x : (1.0f - x);
    
    if(d > paddleSize) 
        return CRGB::Black;

    // Intensity/paddle shape: falling curve with slight taper:
    float v = fl::clamp(1.1f - (d / paddleSize), 0.0f, 1.0f);
    // Decay over time:
    float k = 1.0f - ((float)(t - hitTimeUs))/((float)gp.paddle.animationDecayUs);

    float c = 255.f * v * k;
    return CRGB(c, c, c);
}

void PongEngine::transitionIntoState(GameState state)
{
    Logger::debug(Serial, "transitionIntoState(", state, ")");

    switch(state) 
    {
        case PointScoredByA:
            Logger::info(Serial, "Point scored by A!");
            break;
        case PointScoredByB:
            Logger::info(Serial, "Point scored by B!");
            break;
        case Playing:
            mBallState.hitCounter = 0;
            break;
        default:
            break;
    }

    mPaddleHit[Paddle::A].isActive = false;
    mPaddleHit[Paddle::B].isActive = false;

    mGameState.state = state;
    mGameState.transitionUs = mCurrentUpdateTimeUs;
}
