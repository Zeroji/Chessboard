#include <Arduino.h>
#include <LiquidCrystal.h>
#include <U8x8lib.h>

#include <chess.h>
#include <hardware.h>

LiquidCrystal lcd(PIN_LCD_RS, PIN_LCD_EN,
                  PIN_LCD_D0, PIN_LCD_D1, PIN_LCD_D2, PIN_LCD_D3);

U8X8_SSD1306_128X32_UNIVISION_HW_I2C oled;

uint8_t SENSOR_TILES[32] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0,
                            0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F,
                            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

Game game;

uint64_t lastBoardState = DEFAULT_SENSORS_STATE;

void setup() {
    initChessboard();
    lcd.begin(16, 2);
    oled.begin();
    Serial.begin(115200);
    initializeGame(&game, lastBoardState);
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
    const uint64_t boardState = readChessboard();
#endif

    if (keyPressed == LCD_KEY::Select)
        initializeGame(&game, boardState);

    // Update game
#ifdef USE_SERIAL_CHESSBOARD
    if (boardState != lastBoardState)
#endif
    {
        evolveGame(&game, boardState);
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

    for (byte lx = 0; lx < 8; lx++)
        for (byte ly = 0; ly < 4; ly++) {
            uint8_t* tile = SENSOR_TILES;
            if (boardState & (1uLL << ((3 - ly) * 16 + lx)))
                tile += 8;
            if (boardState & (1uLL << ((3 - ly) * 16 + lx + 8)))
                tile += 16;
            oled.drawTile(lx + 4, ly, 1, tile);
        }

    lastBoardState = boardState;
}
