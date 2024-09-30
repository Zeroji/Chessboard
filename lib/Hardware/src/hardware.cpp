#include "hardware.h"

#if defined(USE_FAST_GPIO)
#include "FastGPIO.h"
#endif

// Pin definitions
constexpr uint8_t PIN_LCD_BTN = A0;
constexpr uint8_t PIN_HALL_EN = A5; // Active low
constexpr uint8_t PIN_LOAD    = 12; // Active low, shifts when high
constexpr uint8_t PIN_CLOCK   = 11; // Rising edge trigger
constexpr uint8_t PIN_DATA_0  = A1; // Active high
constexpr uint8_t PIN_DATA_1  = A2;
constexpr uint8_t PIN_DATA_2  = A3;
constexpr uint8_t PIN_DATA_3  = A4;

LCD_KEY getLcdKeyPressed() {
    // Values are not exact but sufficiently precise
    const int val = analogRead(PIN_LCD_BTN);
    if (val < 100)
        return LCD_KEY::Right;
    else if (val < 250)
        return LCD_KEY::Up;
    else if (val < 380)
        return LCD_KEY::Down;
    else if (val < 600)
        return LCD_KEY::Left;
    else if (val < 900)
        return LCD_KEY::Select;
    else
        return LCD_KEY::None;
}

LCD_KEY getLastLcdKeyPressed() {
    static LCD_KEY s_lastKey = LCD_KEY::None;
    LCD_KEY currentKey       = getLcdKeyPressed();
    if (currentKey != s_lastKey) {
        s_lastKey = currentKey;
        return currentKey;
    }
    return LCD_KEY::None;
}

void initChessboard() {
#ifdef USE_POWER_CTRL
    // Sensor power control (active low)
    pinMode(PIN_HALL_EN, OUTPUT);
    digitalWrite(PIN_HALL_EN, HIGH);
#endif

    // Control lines
    pinMode(PIN_LOAD, OUTPUT);
    pinMode(PIN_CLOCK, OUTPUT);
    digitalWrite(PIN_LOAD, HIGH);
    digitalWrite(PIN_CLOCK, LOW);

    // Data lines
    pinMode(PIN_DATA_0, INPUT);
    pinMode(PIN_DATA_1, INPUT);
    pinMode(PIN_DATA_2, INPUT);
    pinMode(PIN_DATA_3, INPUT);
}

uint64_t readChessboard() {
#if defined(USE_POWER_CTRL)
    // Enable sensors and wait for current to stabilize
    digitalWrite(PIN_HALL_EN, LOW);
    delayMicroseconds(15);
#endif

    // Load the data into the registers
    digitalWrite(PIN_LOAD, LOW);
    digitalWrite(PIN_LOAD, HIGH);

#if defined(USE_POWER_CTRL)
    // Turn off sensors
    digitalWrite(PIN_HALL_EN, HIGH);
#endif

    // Read data
    uint64_t state = 0;

#if defined(CHESSBOARD_REV_A) or defined(CHESSBOARD_REV_A_JUMPED)
    constexpr uint64_t baseOffset0 = 1uLL << 0;
    constexpr uint64_t baseOffset1 = 1uLL << 4;
    constexpr uint64_t baseOffset2 = 1uLL << 32;
    constexpr uint64_t baseOffset3 = 1uLL << 36;

#if defined(CHESSBOARD_REV_A_JUMPED)
    for (byte i = 0; i < 32; i++) {
        const byte bitshift = ((i >> 2) | ((i & 3) << 3)) ^ 0b11011; // Optimized with C++23
#else
    for (byte i = 0; i < 16; i++) {
        const byte bitshift = (3 - (i >> 2)) | ((3 - (i & 0b11)) << 3);
#endif
#if defined(USE_FAST_GPIO)
        if (FastGPIO::Pin<PIN_DATA_0>::isInputHigh())
            state |= (baseOffset0 << bitshift);
#if defined(CHESSBOARD_REV_A_JUMPED)
        if (FastGPIO::Pin<PIN_DATA_2>::isInputHigh())
            state |= (baseOffset2 << bitshift);
#else
        if (FastGPIO::Pin<PIN_DATA_1>::isInputHigh())
            state |= (baseOffset1 << bitshift);
        if (FastGPIO::Pin<PIN_DATA_2>::isInputHigh())
            state |= (baseOffset2 << bitshift);
        if (FastGPIO::Pin<PIN_DATA_3>::isInputHigh())
            state |= (baseOffset3 << bitshift);
#endif // Rev A jumped

        // Clock tick
        FastGPIO::Pin<PIN_CLOCK>::setOutputValueHigh();
        FastGPIO::Pin<PIN_CLOCK>::setOutputValueLow();
#else
        if (digitalRead(PIN_DATA_0) == HIGH)
            state |= (baseOffset0 << bitshift);
#if defined(CHESSBOARD_REV_A_JUMPED)
        if (digitalRead(PIN_DATA_2) == HIGH)
            state |= (baseOffset2 << bitshift);
#else
        if (digitalRead(PIN_DATA_1) == HIGH)
            state |= (baseOffset1 << bitshift);
        if (digitalRead(PIN_DATA_2) == HIGH)
            state |= (baseOffset2 << bitshift);
        if (digitalRead(PIN_DATA_3) == HIGH)
            state |= (baseOffset3 << bitshift);
#endif // Rev A jumped

        // Clock tick
        digitalWrite(PIN_CLOCK, HIGH);
        digitalWrite(PIN_CLOCK, LOW);
#endif // USE_FAST_GPIO
    }

#else
    Unknown_board_revision;
#endif // CHESSBOARD_REV_*

    return state;
}
