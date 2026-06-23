#include "StatusLight.h"
#include <Adafruit_NeoPixel.h>

StatusLight::State StatusLight::_state = State::OFF;
StatusLight::State StatusLight::_prevState = State::OFF;
unsigned long StatusLight::_stateStartMs = 0;
unsigned long StatusLight::_lastUpdateMs = 0;

uint8_t  StatusLight::_transientBlinkCount = 0;
uint8_t  StatusLight::_transientBlinkTarget = 0;
unsigned long StatusLight::_transientDurationMs = 0;
bool     StatusLight::_transientDone = false;

static Adafruit_NeoPixel pixel(16, 48, NEO_GRB + NEO_KHZ800);

// ====================== Core ======================
void StatusLight::_fillColor(uint8_t r, uint8_t g, uint8_t b) {
    uint32_t c = pixel.Color((r*BRIGHTNESS)/255, (g*BRIGHTNESS)/255, (b*BRIGHTNESS)/255);
    for (int i = 0; i < LED_COUNT; i++) pixel.setPixelColor(i, c);
    pixel.show();
}
void StatusLight::_off() { pixel.clear(); pixel.show(); }

// ====================== Buzzer ======================
void StatusLight::_beep(uint16_t ms) {
    digitalWrite(BUZZER_PIN, LOW); delay(ms); digitalWrite(BUZZER_PIN, HIGH);
}
void StatusLight::_successBeep() { _beep(100); delay(100); _beep(100); }
void StatusLight::_failBeep()    { _beep(500); delay(100); _beep(500); }

// ====================== Basic animations ======================
void StatusLight::_breathe(uint8_t r, uint8_t g, uint8_t b, uint16_t periodMs) {
    unsigned long t = millis() - _stateStartMs;
    float v = (sinf(fmodf((float)t / periodMs, 1.0f) * 2.0f * PI) + 1.0f) / 2.0f;
    _fillColor((uint8_t)(r*v), (uint8_t)(g*v), (uint8_t)(b*v));
}
void StatusLight::_blink(uint8_t r, uint8_t g, uint8_t b, uint16_t onMs, uint16_t offMs) {
    unsigned long t = millis() - _stateStartMs;
    if ((uint16_t)(t % (onMs + offMs)) < onMs) _fillColor(r, g, b); else _off();
}
void StatusLight::_blinkAlt(uint8_t r1, uint8_t g1, uint8_t b1, uint8_t r2, uint8_t g2, uint8_t b2, uint16_t eachMs) {
    unsigned long t = millis() - _stateStartMs;
    if ((uint16_t)(t % (eachMs * 2)) < eachMs) _fillColor(r1, g1, b1); else _fillColor(r2, g2, b2);
}

// ====================== 濞翠焦鎸夐悘?/ 鐠烘垿鈹堥悘?======================
void StatusLight::_chase(uint8_t r, uint8_t g, uint8_t b, uint16_t speedMs) {
    unsigned long t = millis() - _stateStartMs;
    int pos = (t / speedMs) % LED_COUNT;
    pixel.clear();
    uint32_t bright = pixel.Color((r*BRIGHTNESS)/255, (g*BRIGHTNESS)/255, (b*BRIGHTNESS)/255);
    uint32_t dim    = pixel.Color((r*BRIGHTNESS)/510, (g*BRIGHTNESS)/510, (b*BRIGHTNESS)/510);
    pixel.setPixelColor(pos, bright);
    pixel.setPixelColor((pos - 1 + LED_COUNT) % LED_COUNT, dim);
    pixel.setPixelColor((pos - 2 + LED_COUNT) % LED_COUNT, dim);
    pixel.show();
}

// ====================== 閺冨娴嗗〒鎰綁 ======================
void StatusLight::_rotate(uint8_t r, uint8_t g, uint8_t b, uint16_t speedMs) {
    unsigned long t = millis() - _stateStartMs;
    int offset = (t / speedMs) % LED_COUNT;
    for (int i = 0; i < LED_COUNT; i++) {
        int dist = abs(i - offset);
        if (dist > LED_COUNT/2) dist = LED_COUNT - dist;
        uint8_t v = 255 - (uint8_t)(dist * 255 / (LED_COUNT/2));
        pixel.setPixelColor(i, pixel.Color((r*v)/255, (g*v)/255, (b*v)/255));
    }
    pixel.show();
}

// ====================== 閹舵牕濮╅梻顏嗗剨 ======================
void StatusLight::_shake(uint8_t r, uint8_t g, uint8_t b) {
    unsigned long t = millis() - _stateStartMs;
    uint8_t phase = (t / 60) % 3;
    if (phase == 0) _fillColor(r, g, b);
    else if (phase == 1) _fillColor(r/4, g/4, b/4);
    else _off();
}


// ====================== ???? ======================
static void _hsv2rgb(float h, float s, float v, uint8_t& r, uint8_t& g, uint8_t& b) {
    int i = (int)(h * 6.0f); float f = h * 6.0f - i;
    float p = v * (1.0f - s), q = v * (1.0f - f * s), t = v * (1.0f - (1.0f - f) * s);
    float rr, gg, bb;
    switch (i % 6) {
        case 0: rr=v; gg=t; bb=p; break; case 1: rr=q; gg=v; bb=p; break;
        case 2: rr=p; gg=v; bb=t; break; case 3: rr=p; gg=q; bb=v; break;
        case 4: rr=t; gg=p; bb=v; break; default: rr=v; gg=p; bb=q; break;
    }
    r = (uint8_t)(rr * 255.0f); g = (uint8_t)(gg * 255.0f); b = (uint8_t)(bb * 255.0f);
}

void StatusLight::_rainbowBreathe(uint16_t periodMs, uint16_t huePeriodMs) {
    unsigned long t = millis() - _stateStartMs;
    float brightness = (sinf(fmodf((float)t / periodMs, 1.0f) * 2.0f * PI) + 1.0f) / 2.0f;
    float hue = fmodf((float)t / huePeriodMs, 1.0f);
    uint8_t r, g, b;
    _hsv2rgb(hue, 1.0f, brightness, r, g, b);
    _fillColor(r, g, b);
}

// ====================== Transient ======================
void StatusLight::_beginTransient(uint8_t blinkTarget, unsigned long durationMs) {
    _transientBlinkCount = 0;
    _transientBlinkTarget = blinkTarget;
    _transientDurationMs = durationMs;
    _transientDone = false;
}
bool StatusLight::isTransient() {
    return _state == State::NFC_SUCCESS || _state == State::NFC_FAIL;
}

// ====================== State switch ======================
void StatusLight::setStatus(State s) {
    if (_state == s) return;

    if (s == State::NFC_SUCCESS || s == State::NFC_FAIL) _prevState = _state;

    _state = s;
    _stateStartMs = millis();

    switch (s) {
        case State::NFC_SUCCESS: _beginTransient(0, 3000); _successBeep(); break;
        case State::NFC_FAIL:    _beginTransient(6, 0);    _failBeep();    break;
        case State::ERROR:       _failBeep(); break;
        default: _transientDone = true; break;
    }
}

// ====================== Update ======================
void StatusLight::update() {
    if (_state == State::NFC_SUCCESS) {
        if (millis() - _stateStartMs >= _transientDurationMs) { _state = State::IDLE; _stateStartMs = millis(); return; }
    }
    if (_state == State::NFC_FAIL) {
        unsigned long t = millis() - _stateStartMs;
        if (t >= (unsigned long)_transientBlinkTarget * 400) { _state = State::IDLE; _stateStartMs = millis(); return; }
    }

    switch (_state) {
        case State::OFF:                _off(); break;
        case State::IDLE:               _breathe(0, 0, 255, 3000); break;
        case State::NFC_WAITING:        _rainbowBreathe(5000, 10000); break;
        case State::NFC_AUTHENTICATING: _chase(0, 200, 255, 40); break;
        case State::NFC_SUCCESS:        _rotate(0, 255, 0, 60); break;
        case State::NFC_FAIL:           _blink(255, 0, 0, 250, 150); break;
        case State::BLE_SCANNING:       _breathe(0, 0, 255, 2000); break;
        case State::BLE_CONNECTED:      _fillColor(0, 0, 255); break;
        case State::BLE_PAIRING:        _blinkAlt(0,0,255, 0,255,0, 300); break;
        case State::OTA:                _breathe(128, 0, 255, 2000); break;
        case State::ERROR:              _shake(180, 0, 255); break;
        case State::LOW_BATTERY:        _breathe(255, 0, 0, 2000); break;
    }
    _lastUpdateMs = millis();
}

// ====================== Init ======================
void StatusLight::init() {
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, HIGH);
    pixel.begin();
    pixel.setBrightness(BRIGHTNESS);
    _off();
    _state = State::OFF;
    _stateStartMs = millis();
}