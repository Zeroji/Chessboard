#include <Arduino.h>
#include <LiquidCrystal.h>
#include <U8g2lib.h>

#include "chess.h"
#include "hardware.h"

LiquidCrystal lcd(PIN_LCD_RS, PIN_LCD_EN,
                  PIN_LCD_D0, PIN_LCD_D1, PIN_LCD_D2, PIN_LCD_D3);

U8G2_SSD1306_128X32_UNIVISION_1_HW_I2C oled(U8G2_R0);

Game game;

void setup() {
    initChessboard();
    lcd.begin(16, 2);
    oled.begin();
    Serial.begin(115200);
    initializeGame(&game, 0x6600000000000066uLL);
}

void loop() {
    const LCD_KEY keyPressed = getLastLcdKeyPressed();

    const uint64_t boardState = readChessboard();

    if (keyPressed == LCD_KEY::Select)
        initializeGame(&game, boardState);

    // Update game
    evolveGame(&game, boardState);

    // Display moves on LCD screen
    lcd.setCursor(0, 0);
    int moveCounter = game.moves / 2 + 1;
    if (game.moves && (game.moves % 2 == 0) && (game.state.status != (bits::White | bits::ToPlay))) {
        lcd.print(moveCounter + 1);
        lcd.write(".              ");
    } else {
        lcd.print(moveCounter);
        lcd.write(". ");
        if (game.lastMoveW.piece != EPiece::Empty) {
            lcd.write(getMoveStr(game.lastMoveW));
            if (((game.moves & 1) == 0) && (game.lastMoveB.piece != EPiece::Empty)) {
                lcd.print(" ");
                lcd.write(getMoveStr(game.lastMoveB));
            }
        }
        lcd.write("             ");
    }
    lcd.setCursor(0, 1);
    lcd.print(getStatusStr(game.state.status));

    // Display board state on OLED screen
    oled.firstPage();
    do {
        oled.drawLine(30, 0, 30, 31);
        oled.drawLine(97, 0, 97, 31);

        for (byte lx = 0; lx < 8; lx++)
            for (byte ly = 0; ly < 8; ly++)
                if (boardState & (1uLL << ((7 - ly) * 8 + lx)))
                    oled.drawBox(lx * 8 + 32, ly * 4, 8, 4);
    } while (oled.nextPage());
}
