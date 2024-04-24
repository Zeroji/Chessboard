#include <Arduino.h>
#include <LiquidCrystal.h>
#include <U8g2lib.h>

constexpr uint8_t PIN_LCD_BTN = A0;
constexpr uint8_t PIN_HALL_VCC = A5;
constexpr uint8_t PIN_DATA_1 = A1;
constexpr uint8_t PIN_DATA_2 = A2;
constexpr uint8_t PIN_DATA_3 = A3;
constexpr uint8_t PIN_DATA_4 = A4;
constexpr uint8_t PIN_SHIFT = 12;
constexpr uint8_t PIN_CLK = 11;

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

U8G2_SSD1306_128X32_UNIVISION_1_HW_I2C oled(U8G2_R0);

enum LCD_KEY {
    None,
    Select,
    Up,
    Down,
    Left,
    Right,
};

LCD_KEY getLCDKeyPressed() {
    int val = analogRead(PIN_LCD_BTN);
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
void print_byte(uint8_t b) {
    for (uint8_t k = 0; k < 8; k++) {
        lcd.write((b & 0x80) ? '1' : '0');
        b <<= 1;
    }
}

uint8_t load_lsb(uint8_t data, uint8_t clk) {
    uint8_t res;
    for (uint8_t k = 0; k < 8; k++) {
        res >>= 1;
        res |= (digitalRead(data) << 7);
        digitalWrite(clk, HIGH);
        digitalWrite(clk, LOW);
    }
    return res;
}

uint8_t shiftIn165(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder, int bitTime_us) {
    uint8_t value = 0;
    uint8_t i;

    for (i = 0; i < 8; ++i) {
        digitalWrite(clockPin, LOW);
        delayMicroseconds(bitTime_us / 4);
        if (bitOrder == LSBFIRST) {
            value |= digitalRead(dataPin) << i;
        } else {
            value |= digitalRead(dataPin) << (7 - i);
        }
        delayMicroseconds(bitTime_us / 4);
        digitalWrite(clockPin, HIGH);
        delayMicroseconds(bitTime_us / 2);
    }
    return value;
}

void shiftInN(uint8_t size, const uint8_t *dataPin, uint8_t clockPin, byte *buffer, uint8_t bitOrder) {
    uint8_t value = 0;
    uint8_t i, j;

    for (i = 0; i < 8; ++i) {
        digitalWrite(clockPin, LOW);
        for (j = 0; j < size; j++) {
            const auto bit = digitalRead(dataPin[j]);
            if (bitOrder == LSBFIRST) {
                buffer[j] |= bit << i;
            } else {
                buffer[j] |= bit << (7 - i);
            }
        }
        digitalWrite(clockPin, HIGH);
    }
}

void setup() {
    lcd.begin(16, 2);
    lcd.print("Sensor:");
    lcd.setCursor(0, 1);
    lcd.print("bT=");

    pinMode(PIN_LCD_BTN, INPUT);
    pinMode(PIN_HALL_VCC, OUTPUT);
    pinMode(PIN_DATA_1, INPUT);
    pinMode(PIN_DATA_2, INPUT);
    pinMode(PIN_DATA_3, INPUT);
    pinMode(PIN_DATA_4, INPUT);
    pinMode(PIN_SHIFT, OUTPUT);
    pinMode(PIN_CLK, OUTPUT);

    oled.begin();
    oled.clearBuffer();
    oled.drawBox(20, 4, 8, 16);
    oled.drawBox(104, 4, 8, 16);
    oled.drawBox(32, 16, 64, 16);
    oled.sendBuffer();

    Serial.begin(115200);
}

// #define DATASHEET

// Time counter
int loops = 0;
unsigned long busy = 0;
unsigned long idle = 0;

// UI
LCD_KEY prevKey = LCD_KEY::None;
const char *anim = "/-\\|";
int x = 1;
int y = 0;

void loop() {
    loops++;
    LCD_KEY lcdKey = getLCDKeyPressed();
    unsigned long t1 = micros();

    if (prevKey == LCD_KEY::None && lcdKey == LCD_KEY::Up && x < 20)
        x++;
    if (prevKey == LCD_KEY::None && lcdKey == LCD_KEY::Down && x > 0)
        x--;
    if (prevKey == LCD_KEY::None && lcdKey == LCD_KEY::Right && y < 20)
        y++;
    if (prevKey == LCD_KEY::None && lcdKey == LCD_KEY::Left && y > 0)
        y--;
#ifdef DATASHEET
    digitalWrite(PIN_HALL_VCC, LOW);
    digitalWrite(PIN_CLK, LOW);
    digitalWrite(PIN_SHIFT, LOW);
    delayMicroseconds(x * 10);
    digitalWrite(PIN_SHIFT, HIGH);

    uint16_t data;
    for (int k = 0; k < 16; k++) {
        delayMicroseconds(1);
        data >>= 1;
        if (digitalRead(PIN_DATA) == HIGH)
            data |= (1 << 15);

        digitalWrite(PIN_CLK, HIGH);
        delayMicroseconds(1);
        digitalWrite(PIN_CLK, LOW);
    }

    int bitTime_us = 999;
    uint8_t leftmost = data >> 8;
    uint8_t rightmost = data & 0xff;
#else
    // Load data
    digitalWrite(PIN_SHIFT, LOW);
    digitalWrite(PIN_CLK, HIGH);
    digitalWrite(PIN_HALL_VCC, LOW);
    delayMicroseconds(x * 10);
    digitalWrite(PIN_SHIFT, HIGH);
    digitalWrite(PIN_HALL_VCC, HIGH);

    int bitTime_us = (1 << y);
    // Read data
    // uint8_t rightmost = shiftIn(PIN_DATA_2, PIN_CLK, LSBFIRST);
    // uint8_t leftmost = shiftIn(PIN_DATA_2, PIN_CLK, LSBFIRST);
    // uint8_t rightmost = shiftIn165(PIN_DATA, PIN_CLK, LSBFIRST, bitTime_us);
    // uint8_t leftmost = shiftIn165(PIN_DATA, PIN_CLK, LSBFIRST, bitTime_us);
    // uint8_t rightmost = load_lsb(PIN_DATA, PIN_CLK);
    // uint8_t leftmost = load_lsb(PIN_DATA, PIN_CLK);
    uint8_t buf[8];
    memset(buf, 0, 8);
    const uint8_t pins[4] = {PIN_DATA_1, PIN_DATA_2, PIN_DATA_3, PIN_DATA_4};
    shiftInN(4, pins, PIN_CLK, buf, LSBFIRST);
    shiftInN(4, pins, PIN_CLK, &buf[4], LSBFIRST);
    uint8_t rightmost = buf[0];
    uint8_t leftmost = buf[2];
#endif

    uint8_t combined = (leftmost & 0x0f) << 4 | (rightmost & 0x0f);

    // Write data
    lcd.setCursor(0, 0);
    lcd.print(anim[loops % 4]);
    lcd.write("x=");
    lcd.print(x);
    lcd.print(" ");
    lcd.setCursor(8, 0);
    print_byte(leftmost);
    lcd.setCursor(3, 1);
    lcd.print(bitTime_us);
    lcd.print(" ");
    lcd.setCursor(8, 1);
    print_byte(rightmost);

    unsigned long t2 = micros();
    busy += t2 - t1;

    // OLED tests
    oled.firstPage();
    do {
#ifdef BAR
        oled.drawFrame(5, 7, 116, 18);
        for (byte k = 0; k < 8; k++)
            if (leftmost & (128 >> k))
                oled.drawBox(k * 14 + 8, 10, 12, 6);
        for (byte k = 0; k < 8; k++)
            if (rightmost & (128 >> k))
                oled.drawBox(k * 14 + 8, 16, 12, 6);
#else
        oled.drawLine(30, 0, 30, 31);
        oled.drawLine(97, 0, 97, 31);
        for (byte j = 0; j < 8; j++)
            for (byte k = 0; k < 8; k++)
                if (buf[j] & (128 >> k))
                    oled.drawBox((((j & 1) * 2 + 1 - (j >> 2)) * 2 + (k >> 2)) * 8 + 32,
                                 (7 - ((j << 1) & 4) - (k % 4)) * 4, 8, 4);

                    // for (byte k = 0; k < 8; k++)
                    //     if (leftmost & (128 >> k))
                    //         oled.drawBox((k >> 2) * 8 + 48, (3 - (k % 4)) * 8, 8, 8);
                    // for (byte k = 0; k < 8; k++)
                    //     if (rightmost & (128 >> k))
                    //         oled.drawBox((k >> 2) * 8 + 64, (3 - (k % 4)) * 8, 8, 8);
#endif
    } while (oled.nextPage());

    t1 = micros();
    delay(10);
    t2 = micros();
    idle += t2 - t1;

    if (loops % 100 == 0) {
        Serial.print("Busy: ");
        Serial.print(busy);
        Serial.print(" µs, idle: ");
        Serial.print(idle);
        Serial.println(" us");
    }

    prevKey = lcdKey;
}
