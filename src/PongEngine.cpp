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
    transitionIntoState(WaitingForStartAtA);
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

void PongEngine::setPaddleHit(Paddles paddle, uint32_t timeUs)
{
    Logger::debug(Serial, "setPaddleHit(", paddle, ")");
    mPaddleHit[paddle].timeUs   = timeUs > 0 ? timeUs : micros();
    mPaddleHit[paddle].isActive = true;
}

void PongEngine::update()
{
    switch(mGameState.state) 
    {
        case WaitingForStartAtAny:
            updateWaitingForStart(Paddles::A);
            // updateWaitingForStart(Paddles::B);
            drawWaitingForStart(std::nullopt);
            break;
        case WaitingForStartAtA:
            updateWaitingForStart(Paddles::A);
            drawWaitingForStart(Paddles::A);
            break;
        case WaitingForStartAtB:
            updateWaitingForStart(Paddles::B);
            drawWaitingForStart(Paddles::B);
            break;
        case Playing:
            updateBallPositionAndSpeed();
            drawPlaying();
            break;
        case PointScoredByA:
            updatePointScoredBy(Paddles::A);
            break;
        case PointScoredByB:
            updatePointScoredBy(Paddles::B);
            break;
        default:
            break;
    }

    mLastUpdateTimeUs = micros();
}

void PongEngine::updateWaitingForStart(Paddles at)
{
    if(mPaddleHit[at].isActive) {
        mBallState.position = (at == Paddles::A ? 0.0f : 1.0f); // ## MAKEPARAM
        mBallState.velocity = (at == Paddles::A ? 1.0f :-1.0f);
        mPaddleHit[at].isActive = false;
        Logger::debug(Serial, "Started: ", at, ", ", mBallState.position, ", ", mBallState.velocity);
        transitionIntoState(GameState::Playing);
    }
}

void PongEngine::drawWaitingForStart(std::optional<Paddles> at)
{
    draw([](int i, float x) {
        return CRGB{};
    });
}

void PongEngine::updateBallPositionAndSpeed()
{
    uint32_t t = micros();
    float dt = (t - mLastUpdateTimeUs) / 1000000.0f;

    mBallState.position += mBallState.velocity*dt;

    auto paddleToCheck = mBallState.velocity > 0.0f ? Paddles::B : Paddles::A;
    auto& paddle = mPaddleHit[paddleToCheck];

    if(paddle.isActive) {
        float k = 4.0; // #MAKEPARAM
        float x = mBallState.position;
        float d = paddleToCheck == Paddles::A ? x : (1.0f - x);
        float e = 1.0f - 6.0f * d; // #MAKEPARAM
        e = k*fl::clamp(e, 0.0f, 1.0f);
        
        paddle.isActive = false;

        if(e > 0.01f) {
            mBallState.velocity *= (-1.0f * e);
            Logger::debug(Serial, "Hit! ", paddleToCheck, ": ", x, ", ", d, ", ", e, " -> ", mBallState.velocity);
        }
    }

    if(mBallState.position > 1.01f)
        transitionIntoState(PointScoredByA);
    else if(mBallState.position <-0.01f)
        transitionIntoState(PointScoredByB);
}

void PongEngine::drawPlaying()
{
    draw([this](int, float x) {
        auto ballp = sampleBallAt(x);
        auto paddlep = samplePaddleAt(Paddles::A, x) + samplePaddleAt(Paddles::B, x);
        return CRGB::blend(ballp, paddlep, 128); // Blend ball and paddle colors
    });
}

void PongEngine::updatePointScoredBy(Paddles at)
{
    (void)at;
    transitionIntoState(at == Paddles::A ? WaitingForStartAtA : WaitingForStartAtB);
}

CRGB PongEngine::sampleBallAt(float x) const
{
    // TODO: use two linear functions for ball? Could be smoother...
    float v = (-1.f / mBallState.width) * pow(x - mBallState.position, 2.f) + 1.f;
    v = fl::clamp(v, 0.0f, 1.0f);
    float c = 255.f * v;
    return CRGB(c, 0, 0);
}

CRGB PongEngine::samplePaddleAt(Paddles paddle, float x) const
{
    const auto& hitTimeUs = mPaddleHit[paddle].timeUs;

    if(!hitTimeUs || micros() - hitTimeUs > 500000) {
        return CRGB::Black; // Paddle effect lasts for 0.5 seconds
    } 

    const float paddleSize = 0.1f; // Size of the paddle in normalized units ##MAKEPARAM
    
    if(paddle == Paddles::A) {
        if(x > paddleSize) return CRGB::Black; // Paddle A is on the left
    } else {
        if(x < 1.0f - paddleSize) return CRGB::Black; // Paddle B is on the right
    }

    float dt = (micros() - hitTimeUs) / 1000000.0f; // Time since last hit in seconds
    float v = 1.0f - (dt / 0.5f); // Paddle effect fades over 0.5 seconds
    float c = 255.f * v;
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
        default:
            break;
    }

    mPaddleHit[Paddles::A].isActive = false;
    mPaddleHit[Paddles::B].isActive = false;

    mGameState.state = state;
    mGameState.transitionUs = micros();
}
