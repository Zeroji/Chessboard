#include <Arduino.h>
#include <LiquidCrystal.h>
#include <RTClib.h>
#include <U8g2lib.h>

#include <chess.h>
#include <hardware.h>
#include <writer.h>

LiquidCrystal lcd(PIN_LCD_RS, PIN_LCD_EN,
                  PIN_LCD_D0, PIN_LCD_D1, PIN_LCD_D2, PIN_LCD_D3);

// U8G2_SSD1306_128X32_UNIVISION_1_HW_I2C oled(U8G2_R0);

RTC_DS1307 rtc;

Game game;

// Write games to SD
File history;
bool gameStarted = false;
uint8_t lastGameStatus;

#undef USE_SERIAL_CHESSBOARD
uint64_t lastBoardState = DEFAULT_SENSORS_STATE;

void setup() {
    initChessboard();
    lcd.begin(16, 2);
    // oled.begin();
    Serial.begin(115200);
    initializeGame(&game, lastBoardState);

    if (!rtc.begin()) {
        Serial.write("Couldn't find RTC!");
    }

    initWriter(PIN_SD_CS);

    lcd.clear();
    lcd.setCursor(2, 1);
    lcd.write("Press Select");
}

void loop() {
    const LCD_KEY keyPressed = getLastLcdKeyPressed();

#ifdef USE_SERIAL_CHESSBOARD
    uint64_t boardState = lastBoardState;

    {
        int c = Serial.read();
        if ((c == '+' || c == '-') && Serial.available() >= 2) {
            char buffer[2];
            Serial.readBytes(buffer, 2);
            uint8_t square = getSquareFromStr(buffer);
            if (c == '+') {
                boardState |= (1uLL << square);
            } else {
                boardState &= ~(1uLL << square);
            }
        }
        if (c == '=' && Serial.available() >= 16) {
            char hexBuffer[17];
            if (16 == Serial.readBytes(hexBuffer, 16)) {
                hexBuffer[16] = 0;
                uint32_t low  = strtoul(&hexBuffer[8], nullptr, 16);
                hexBuffer[8]  = 0;
                uint32_t high = strtoul(hexBuffer, nullptr, 16);
                boardState    = ((uint64_t)high << 32) | low;
            }
        }
        if (c == 'Z') {
            boardState = DEFAULT_SENSORS_STATE;
            initializeGame(&game, boardState);
        }
    }
#else
    const uint64_t boardState = stabilizeBoardState(readChessboard());
#endif

    // // Display board state on OLED screen
    // oled.firstPage();
    // do {
    //     oled.drawLine(30, 0, 30, 31);
    //     oled.drawLine(97, 0, 97, 31);

    //     for (byte lx = 0; lx < 8; lx++)
    //         for (byte ly = 0; ly < 8; ly++)
    //             if (boardState & (1uLL << ((7 - ly) * 8 + lx)))
    //                 oled.drawBox(lx * 8 + 32, ly * 4, 8, 4);
    // } while (oled.nextPage());

    if (keyPressed == LCD_KEY::Select) {
        initializeGame(&game, boardState);
        if (rtc.isrunning()) {
            DateTime now = rtc.now();
            history      = openFile(now);
        } else {
            history = openFile();
        }
        lastGameStatus = bits::White | bits::ToPlay;
        gameStarted    = true;
    }

    if (false == gameStarted) {
        lcd.setCursor(0, 0);
        if (rtc.isrunning()) {
            DateTime now = rtc.now();
            char buf[17];
            sprintf(buf, "%04d-%02d-%02d %02d:%02d", now.year(), now.month(), now.day(), now.hour(), now.minute());
            lcd.write(buf);
        } else {
            lcd.write("RTC not running!");
        }
        delay(10);
        return;
    }

    bool played = false;
    // Update game
#ifdef USE_SERIAL_CHESSBOARD
    if (boardState != lastBoardState)
#endif
    {
        played = evolveGame(&game, boardState);
    }

    // Display moves on LCD screen
    lcd.setCursor(0, 0);
    lcd.print(game.fullmoveClock);
    lcd.write(". ");
    if (game.state.status == (bits::White | bits::ToPlay)) {
        if (game.lastMoveW.piece != EPiece::Empty) {
            lcd.write(getMoveStr(game.lastMoveW));
            lcd.write(" ");
            lcd.write(getMoveStr(game.lastMoveB));
        }
    } else if (game.state.status == (bits::Black | bits::ToPlay)) {
        lcd.write(getMoveStr(game.lastMoveW));
        lcd.write("             ");
    }
    lcd.setCursor(0, 1);
    lcd.print(getStatusStr(game.state.status));

    // Write move to file
    if (played) {
        Move* move;
        if ((lastGameStatus & bits::White) != 0) {
            // white played
            writeToFile(&history, game.fullmoveClock);
            writeToFile(&history, ".");
            move = &game.lastMoveW;
        } else {
            // black played
            move = &game.lastMoveB;
        }

        writeToFile(&history, " ");
        writeToFile(&history, getMoveStr(*move));

        if ((game.state.status & bits::Draw) != 0) {
            writeToFile(&history, " 1/2-1/2");
            closeFile(&history);
        } else if ((game.state.status & bits::Finished) != 0) {
            if ((game.state.status & bits::White) != 0) {
                writeToFile(&history, " 1-0");
            } else {
                writeToFile(&history, " 0-1");
            }
            closeFile(&history);
        } else {
            history.flush();
        }

        lastGameStatus = game.state.status;
    }

    lastBoardState = boardState;
}
