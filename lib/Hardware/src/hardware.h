#pragma once

#include <Arduino.h>
#include <LiquidCrystal.h>
#include <U8g2lib.h>

// Whether sensor power should be controlled
#define USE_POWER_CTRL

// Whether faster GPIO functions should be used for certain calls
#define USE_FAST_GPIO

// Hardware revision of the chessboard
#define CHESSBOARD_REV_A_JUMPED

// Pin definitions for LCD (not sure how to move these to .cpp file)

// Pin definitions for SD
constexpr uint8_t PIN_SD = 99;

// LCD buttons
enum LCD_KEY {
    None = 0,
    Select,
    Up,
    Down,
    Left,
    Right,
};

// Gets current pressed LCD button
LCD_KEY getLcdKeyPressed();

// Gets last rising edge on an LCD button
LCD_KEY getLastLcdKeyPressed();

// Initialize chessboard
void initChessboard();

// Read current state of chessboard, LSB=A1, B1... MSB = H8
uint64_t readChessboard();

// Stabilize the chessboard state
uint64_t stabilizeBoardState(uint64_t p_boardState);
