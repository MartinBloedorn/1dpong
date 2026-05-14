#include "PongEngine.hpp"

#include "Logger.h"

#include <cmath>
#include <Arduino.h>

PongEngine::PongEngine(CRGB* leds, int numLeds)
    : mLeds(leds)
    , mMaxNumLeds(numLeds)
{
}

void PongEngine::init()
{
    transitionIntoState(IdleAnimation);
}

void PongEngine::debug(int intervalUs)
{
    static uint32_t lasttd = 0;

    if(micros() - lasttd > intervalUs) 
    {
        Logger::debug("* mGameState.state     = ", mGameState.state);
        Logger::debug("* mBallState.position  = ", mBallState.position);
        Logger::debug("* mBallState.velocity  = ", mBallState.velocity);

        lasttd = micros();
    }
}

void PongEngine::setPaddleHit(Paddle paddle)
{
    uint32_t t = micros();

    if(t - mPaddleHit[paddle].timeUs > mGameParameters.paddle.deadtimeUs) {
        Logger::debug("setPaddleHit(", paddle, ")");
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
        case IdleAnimation:
            updateWaitingForStart(Paddle::A);
            updateWaitingForStart(Paddle::B);
            drawIdleAnimation();
            break;
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
    const auto& gp = mGameParameters;

    if(mPaddleHit[at].isActive) {
        mBallState.position = (at == Paddle::A ? 0.0f : 1.0f);
        mBallState.velocity = (at == Paddle::A ? 1.0f :-1.0f) * gp.ball.speedInit;
        mPaddleHit[at].isActive = false;
        Logger::debug("Started: ", at, ", ", mBallState.position, ", ", mBallState.velocity);
        transitionIntoState(Playing);
    } 
    else if(
        (at == Paddle::A 
            && mGameState.state == WaitingForStartAtA 
            && mCurrentUpdateTimeUs - mGameState.transitionUs > gp.gameplay.maxWaitForPlayerTimeUs) ||
        (at == Paddle::B
            && mGameState.state == WaitingForStartAtB
            && mCurrentUpdateTimeUs - mGameState.transitionUs > gp.gameplay.maxWaitForPlayerTimeUs)) 
    {
        transitionIntoState(WaitingForStartAtAny);
    }
    else if(
        mGameState.state == WaitingForStartAtAny 
        && mCurrentUpdateTimeUs - mGameState.transitionUs > gp.gameplay.maxWaitUntilIdleAnimation)
    {
        transitionIntoState(IdleAnimation);
    }
}

void PongEngine::drawWaitingForStart(std::optional<Paddle> at)
{
    bool animateBoth = !at.has_value();
    auto other = (at && *at == Paddle::A) ? Paddle::B : Paddle::A;

    draw([this, animateBoth, at, other](int i, float x) 
    {
        if(animateBoth) {
            return samplePaddleWaitAnimationAt(Paddle::A, x) 
                 + samplePaddleWaitAnimationAt(Paddle::B, x);
        } 
        return samplePaddleWaitAnimationAt(*at, x) 
             + samplePaddleAt(other, x);
    });
}

void PongEngine:: drawIdleAnimation()
{
    float t = mCurrentUpdateTimeUs * 0.000001f;

    const auto falloff = [](float d, float radius) -> float {
        float v = 1.0f - (d / radius);
        if (v <= 0) return 0;
        return v * v * (3 - 2 * v); // smoothstep
    };

    draw([this, t, falloff](int, float x) {
        // float time = t * 0.000001f;

        struct Bubble {
            float speed;
            float offset;
            float size;
            float hueOffset;
        };

        Bubble bubbles[3] = {
            { 0.07f, 0.1f, 0.18f, 0.0f },
            { -0.05f, 0.5f, 0.22f, 0.3f },
            { 0.03f, 0.8f, 0.15f, 0.6f }
        };

        float r = 0, g = 0, b = 0;

        for (int i = 0; i < 3; i++) {
            // Position wraps around [0,1]
            float pos = fmodf(bubbles[i].offset + t * bubbles[i].speed, 1.0f);
            if (pos < 0) pos += 1.0f;

            // Distance on a loop (important!)
            float d = fabsf(x - pos);
            d = fminf(d, 1.0f - d);

            // Smooth falloff
            float influence = 1.0f - (d / bubbles[i].size);
            if (influence <= 0) continue;

            influence = influence * influence * (3 - 2 * influence);

            // Color per bubble
            float hue = fmodf(bubbles[i].hueOffset + t * 0.02f, 1.0f);

            CRGB c;
            return c.setHSV(hue * 255.f, 0.8f * 255.f, influence * 255.f);

            r += c.r;
            g += c.g;
            b += c.b;
        }

        // Clamp BEFORE conversion
        r = fminf(r, 1.0f);
        g = fminf(g, 1.0f);
        b = fminf(b, 1.0f);

        // Convert to 0–255 with proper rounding
        uint8_t R = (uint8_t)(r * 255.0f + 0.5f);
        uint8_t G = (uint8_t)(g * 255.0f + 0.5f);
        uint8_t B = (uint8_t)(b * 255.0f + 0.5f);

        return CRGB{R, G, B};
    });

    // draw([this, t](int, float x) 
    // {
    //     // Layered waves
    //     float wave1 = sinf(2.0f * PI * (x * 1.0f + t * 0.05f));
    //     float wave2 = sinf(2.0f * PI * (x * 2.3f - t * 0.08f));
    //     float wave3 = sinf(2.0f * PI * (x * 0.7f + t * 0.02f));
    //     // Combine waves
    //     float combined = (wave1 + wave2 * 0.5f + wave3 * 0.8f);
    //     // Normalize to 0–1
    //     float hue = (combined * 0.5f + 0.5f);
    //     // Optional: slow global hue drift
    //     hue += t * 0.075f;
    //     // Wrap hue
    //     hue = fmodf(hue, 1.0f);
    //     if (hue < 0) hue += 1.0f;
    //     // Convert HSV → RGB
    //     CRGB c;
    //     return c.setHSV(hue * 255.f, 0.6f * 255.f, 0.4f * 255.f);
    // });
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

            Logger::debug("Hit! ", paddleToCheck, ": ", x, ", ", d, ", ", e, " -> ", bs.velocity);
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
    // draw([this, at](int, float x) {
    //     float d = at == Paddle::A ? x : (1.0f - x); 
    //     if(d < 0.25f)
    //         return CRGB::Green;
    //     if(d > 0.75f)
    //         return CRGB::Red;  
    //     return CRGB::Black;
    // });

    uint32_t dt = mCurrentUpdateTimeUs - mGameState.transitionUs;
    float progress = ((float)dt) / ((float)mGameParameters.gameplay.scoreAnimationDurationUs);

    float particles[6] = {0.05, 0.1, 0.15, 0.2, 0.25, 0.3};

    draw([this, at, dt, progress, particles](int, float x) -> CRGB {
        float d = at == Paddle::B ? x : (1.0f - x); // Flip around
        
        if(d < 0.66f) {
            // float k = 1.f - d / (0.25f * (.1f + pow(progress, 0.33f)));
            // k = k * (1.f - progress) * fmaxf((1.f * std::sin(12.f * M_PI * progress)) + .5f, 0.f);
            // k = fl::clamp(k, 0.f, 1.f);
            // float c = 255.f * k;
            // return CRGB(c, 0, 0);

            const float w = 0.01;
            float v = 0.f;

            for(int i = 0; i < 6; ++i) {
                float distance = fabs(particles[i]*progress - d);
                float k = 1.f - distance / w;
                k = fl::clamp(k, 0.f, 1.f);
                v += k;
                // make particle's velocity affect decay/width
            }
            v *= (1.f - progress);
            v = fl::clamp(v, 0.f, 1.f);
            float c = 255.f * v;
            return CRGB(c, 0, 0);
        }

        if(d > 0.66f) {
            return dt & (1 << 17) ? CRGB::Green : CRGB::Black;
        }

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

    // if(!hitTimeUs || t - hitTimeUs > gp.paddle.animationDecayUs)
    //     return CRGB::Black;

    float paddleSize = normalizeToLength(gp.paddle.range);
    float d = paddle == Paddle::A ? x : (1.0f - x);
    
    if(d > paddleSize) 
        return CRGB::Black;

    // Intensity/paddle shape: falling curve with slight taper:
    float v = fl::clamp(1.1f - (d / paddleSize), 0.0f, 1.0f);
    // Decay over time:
    float k = 1.0f - ((float)(t - hitTimeUs))/((float)gp.paddle.animationDecayUs);
    k = fl::clamp(k, 0.0f, 1.0f);

    // Indicators for the paddles: red-ish for non-hit, white for hit:
    float r =  64.f * v; 
    float c = 255.f * v * k;

    return CRGB::blend(CRGB(r, r/2, 0), CRGB(c, c, c), 255.f * k);
}

CRGB PongEngine::samplePaddleWaitAnimationAt(Paddle paddle, float x) const
{
    const auto& gp = mGameParameters;
    const float t  = mCurrentUpdateTimeUs;

    float paddleSize = 2.f * normalizeToLength(gp.paddle.range);
    float d = paddle == Paddle::A ? x : (1.0f - x);

    if(d > paddleSize) 
        return CRGB::Black;

    // Add oscillation over time
    float w = 6.f * t / 1000000.f;
    float s = paddle == Paddle::A ? std::sin(w) : std::sin(w + M_PI);
    // Intensity ramp + oscillation:
    float k = 1.f - d / paddleSize;
    float c = 255.f * k * ((0.4f * s) + 0.5f);

    return CRGB(c/2, c, 0);
}

void PongEngine::transitionIntoState(GameState state)
{
    Logger::debug("transitionIntoState(", state, ")");

    switch(state) 
    {
        case PointScoredByA:
            Logger::info("Point scored by A!");
            break;
        case PointScoredByB:
            Logger::info("Point scored by B!");
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
