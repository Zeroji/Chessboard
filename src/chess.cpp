#include "../include/chess.h"

#include <stdio.h>
#include <stdlib.h>
#include <cstring>

#ifdef ARDUINO_ARCH_AVR
#include <Arduino.h>
#define LOG(X) Serial.println(X)
#define LOG_INDEX(X, IDX)         \
    do {                          \
        Serial.print(X);          \
        Serial.print(" (index "); \
        Serial.print(IDX);        \
        Serial.println(')');      \
    } while (false)
#else
#define HAS_PRINTF
#define LOG(X) printf(X "\n")
#define LOG_INDEX(X, IDX) printf(X " (index %d)\n", IDX)
#endif

const uint8_t NULL_INDEX = 64;

//-----------------------------------------------------------------------------
void initializeGame(Game* p_game, uint64_t p_mask)
//-----------------------------------------------------------------------------
{
    if (NULL == p_game) {
        LOG("Unable to initialize null game");
        return;
    }

    // Empty everything
    for (uint8_t i = 0; i < 64; i++) {
        p_game->board[i] = Empty;
    }

    // Pawns
    for (uint8_t i = 0; i < 8; i++) {
        p_game->board[1 * 8 + i] = WPawn;
        p_game->board[6 * 8 + i] = BPawn;
    }

    // White pieces
    p_game->board[0] = WRook;
    p_game->board[1] = WKnight;
    p_game->board[2] = WBishop;
    p_game->board[3] = WQueen;
    p_game->board[4] = WKing;
    p_game->board[5] = WBishop;
    p_game->board[6] = WKnight;
    p_game->board[7] = WRook;

    // Black pieces
    p_game->board[7 * 8 + 0] = BRook;
    p_game->board[7 * 8 + 1] = BKnight;
    p_game->board[7 * 8 + 2] = BBishop;
    p_game->board[7 * 8 + 3] = BQueen;
    p_game->board[7 * 8 + 4] = BKing;
    p_game->board[7 * 8 + 5] = BBishop;
    p_game->board[7 * 8 + 6] = BKnight;
    p_game->board[7 * 8 + 7] = BRook;

    // Mask pieces
    for (uint8_t i = 0; i < 64; i++)
        if ((p_mask & (1uLL << i)) == 0)
            p_game->board[i] = Empty;

    // Game state
    p_game->state.status          = bits::White | bits::ToPlay;
    p_game->state.removed_1.index = NULL_INDEX;
    p_game->state.removed_1.piece = Empty;
    p_game->state.removed_2.index = NULL_INDEX;
    p_game->state.removed_2.piece = Empty;
    p_game->lastMoveB.piece       = Empty;
    p_game->lastMoveW.piece       = Empty;
    p_game->moves                 = 0;

    LOG("Game has been initialized");
}

//-----------------------------------------------------------------------------
const char* getPieceStr(EPiece p_piece)
//-----------------------------------------------------------------------------
{
    switch (p_piece) {
    case WPawn:
        return "wP";
    case WKnight:
        return "wN";
    case WBishop:
        return "wB";
    case WRook:
        return "wR";
    case WQueen:
        return "wQ";
    case WKing:
        return "wK";
    case BPawn:
        return "bP";
    case BKnight:
        return "bN";
    case BBishop:
        return "bB";
    case BRook:
        return "bR";
    case BQueen:
        return "bQ";
    case BKing:
        return "bK";
    default:
        return "  ";
    };
}

//-----------------------------------------------------------------------------
const char* getStatusStr(uint8_t p_status)
//-----------------------------------------------------------------------------
{
    // All strings are of equal width to clear LCD screen
    switch (p_status) {
    case bits::White | bits::ToPlay:
        return "White to play  ";
    case bits::White | bits::Playing:
        return "White playing  ";
    case bits::White | bits::Capturing:
        return "White capturing";
    case bits::White | bits::Castling:
        return "White castling ";
    case bits::Black | bits::ToPlay:
        return "Black to play  ";
    case bits::Black | bits::Playing:
        return "Black playing  ";
    case bits::Black | bits::Capturing:
        return "Black capturing";
    case bits::Black | bits::Castling:
        return "Black castling ";
    case bits::Draw | bits::Finished:
        return "Draw           ";
    case bits::White | bits::Finished:
        return "White won      ";
    case bits::Black | bits::Finished:
        return "Black won      ";

    default:
        return "Undefined      ";
    };
}

//-----------------------------------------------------------------------------
void printGame(Game* p_game)
//-----------------------------------------------------------------------------
{
    if (NULL == p_game) {
        LOG("Unable to print null game");
        return;
    }

    LOG("  +----+----+----+----+----+----+----+----+");
    for (uint8_t i = 0; i < 8; i++) {
#ifdef HAS_PRINTF
        printf("%d ", (8 - i));
        for (uint8_t j = 0; j < 8; j++) {
            printf("| %s ", getPieceStr(p_game->board[8 * (8 - i - 1) + j]));
        }
#else
        Serial.print(8 - i);
        for (uint8_t j = 0; j < 8; j++) {
            Serial.print(" | ");
            Serial.print(getPieceStr(p_game->board[8 * (8 - i - 1) + j]));
        }
        Serial.print(' ');
#endif

        LOG("|\n  +----+----+----+----+----+----+----+----+");
    }

    LOG("   a    b    c    d    e    f    g    h");
#ifdef HAS_PRINTF
    printf("[%s, %d move%s]\n", getStatusStr(p_game->state.status), p_game->moves, (p_game->moves > 1 ? "s" : ""));
#else
    Serial.print('[');
    Serial.print(getStatusStr(p_game->state.status));
    Serial.print(", ");
    Serial.print(p_game->moves);
    Serial.print("moves]\n");
#endif
}

//-----------------------------------------------------------------------------
bool isCheck(Game* p_game)
//-----------------------------------------------------------------------------
{
    if (NULL == p_game) {
        LOG("Unable to analyze check of null game");
        return false;
    }

    bool (*isCheckingColor)(EPiece);
    bool (*isCheckedColor)(EPiece);

    uint8_t nextPlayer = (p_game->state.status & bits::ColorMask);

    if (0 == (p_game->state.status & bits::ToPlay)) {
        LOG("Unable to analyze check when neither white nor black has to play");
        return false;
    } else if (bits::White == nextPlayer) {
        isCheckingColor = &isBlack;
        isCheckedColor  = &isWhite;
    } else {
        isCheckingColor = &isWhite;
        isCheckedColor  = &isBlack;
    }

    // Find King
    uint8_t checkedKingIndex = NULL_INDEX;
    for (uint8_t i = 0; i < 64; i++) {
        EPiece piece = p_game->board[i];
        if ((true == isKing(piece)) && (true == (*isCheckedColor)(piece))) {
            checkedKingIndex = i;
            break;
        }
    }

    if (NULL_INDEX == checkedKingIndex) {
        LOG("Unable to localize King");
        return false;
    }

    uint8_t checkedKingCol = checkedKingIndex % 8;
    uint8_t checkedKingRow = checkedKingIndex / 8;

    // 1. Check from Queen, Rooks, Bishops
    // Prepare directions: col-, col+, row-, row+, diagBL, diagTL, diagBR, diagTR
    const int8_t dirCol[8] = {-1, 1, 0, 0, -1, -1, 1, 1};
    const int8_t dirRow[8] = {0, 0, -1, 1, -1, 1, -1, 1};

    bool (*matchingFunc[2])(EPiece) = {&isThreateningOrthogonal, &isThreateningDiagonal};

    for (uint8_t i = 0; i < 8; i++) {
        uint8_t col = checkedKingCol + dirCol[i];
        uint8_t row = checkedKingRow + dirRow[i];

        bool pieceFound = false;
        EPiece piece    = Empty;
        while ((col < 8) && (row < 8)) {
            piece = p_game->board[8 * row + col];
            if (piece != EPiece::Empty) {
                // Found a piece aligned with the king (straight or diagonally)
                pieceFound = true;
                break;
            }

            col += dirCol[i];
            row += dirRow[i];
        }

        if (true == pieceFound) {
            if ((true == (*matchingFunc[i / 4])(piece)) && (true == (*isCheckingColor)(piece))) {
                return true;
            }
        }
    }

    // 2. Check from Knights
    const int8_t posKCol[8] = {1, 2, 2, 1, -1, -2, -2, -1};
    const int8_t posKRow[8] = {2, 1, -1, -2, -2, -1, 1, 2};

    for (uint8_t i = 0; i < 8; i++) {
        uint8_t col = checkedKingCol + posKCol[i];
        uint8_t row = checkedKingRow + posKRow[i];

        if ((col < 8) && (row < 8)) {
            EPiece piece = p_game->board[8 * row + col];
            if ((true == isKnight(piece)) && (true == (*isCheckingColor)(piece))) {
                return true;
            }
        }
    }

    // 3. Check from Pawns
    const int8_t posPCol[2] = {-1, 1};
    const uint8_t dirPRow   = (bits::White == nextPlayer) ? 1 : -1;

    for (uint8_t i = 0; i < 2; i++) {
        uint8_t col = checkedKingCol + posPCol[i];
        uint8_t row = checkedKingRow + dirPRow;

        if ((col < 8) && (row < 8)) {
            EPiece piece = p_game->board[8 * row + col];
            if ((true == isPawn(piece)) && (true == (*isCheckingColor)(piece))) {
                return true;
            }
        }
    }

    return false;
}

//-----------------------------------------------------------------------------
bool evolveGame(Game* p_game, uint64_t p_sensors)
//-----------------------------------------------------------------------------
{
    if (NULL == p_game) {
        LOG("Unable to evolve null game");
        return false;
    }

    uint8_t indexRemoved = NULL_INDEX;
    uint8_t indexPlaced  = NULL_INDEX;

    for (uint8_t i = 0; i < 64; i++) {
        bool sensor = (p_sensors >> i) & 0x01;
        if ((false == sensor) && (p_game->board[i] != Empty)) {
            indexRemoved = i;
        }

        if ((true == sensor) && (p_game->board[i] == Empty)) {
            indexPlaced = i;
        }
    }

    if ((NULL_INDEX == indexRemoved) && (NULL_INDEX == indexPlaced)) {
        // No change
#ifndef ARDUINO_ARCH_AVR
        LOG("-> no change");
#endif
        return false;
    }

    uint8_t player      = (p_game->state.status & bits::ColorMask);
    uint8_t otherPlayer = bits::White == player ? bits::Black : bits::White;
    uint8_t currentMove = (p_game->state.status & bits::MoveMask);
    Move* lastMovePtr;

    bool (*isPlayerColor)(EPiece);
    bool (*isOtherPlayerColor)(EPiece);

    if (bits::White == player) {
        lastMovePtr        = &(p_game->lastMoveW);
        isPlayerColor      = &isWhite;
        isOtherPlayerColor = &isBlack;
    } else {
        lastMovePtr        = &(p_game->lastMoveB);
        isPlayerColor      = &isBlack;
        isOtherPlayerColor = &isWhite;
    }

    // Decision flow
    switch (currentMove) {
    // ========================= GAME FINISHED
    case bits::Finished:
        if (bits::White == player)
        {
            LOG("-> Game is over: white won!");
        }
        else
        {
            LOG("-> Game is over: black won!");
        }
        return false;
    case bits::Draw:
        LOG("-> Game is over: draw!");
        return false;

    // ========================= TO PLAY
    case bits::ToPlay: {
        if (NULL_INDEX != indexRemoved) {
            // Piece has been removed, player is playing
            LOG_INDEX("-> Piece is removed", indexRemoved);
            p_game->state.removed_1.index                = indexRemoved;
            p_game->state.removed_1.piece                = p_game->board[p_game->state.removed_1.index];
            p_game->board[p_game->state.removed_1.index] = Empty;
            p_game->state.status                         = player | bits::Playing;
        }

        if (NULL_INDEX != indexPlaced) {
            LOG_INDEX("-> Additional piece placed during player turn!", indexPlaced);
        }
        break;
    }
    
    // ========================= IS PLAYING
    case bits::Playing: {
        if (NULL_INDEX != indexPlaced) {
            // Piece has been placed
            if (p_game->state.removed_1.index == indexPlaced) {
                // A piece has been removed and placed at the same location (undo), still same player to play
                LOG("-> Player canceled its move");
                p_game->board[indexPlaced]    = p_game->state.removed_1.piece;
                p_game->state.removed_1.index = NULL_INDEX;
                p_game->state.removed_1.piece = Empty;
                p_game->state.status          = player | bits::ToPlay;
            } else {
                // The piece moved to a different location
                LOG_INDEX("-> Piece is placed", indexPlaced);

                uint8_t diff = abs(p_game->state.removed_1.index - indexPlaced);
                if ((WKing == p_game->state.removed_1.piece) && (2 == diff)) {
                    // King replaced two cells away on the same row: player is castling (1/3)
                    *lastMovePtr                  = {p_game->state.removed_1.index, indexPlaced, p_game->state.removed_1.piece, false, false};
                    p_game->board[indexPlaced]    = p_game->state.removed_1.piece;
                    p_game->state.removed_1.index = NULL_INDEX;
                    p_game->state.removed_1.piece = Empty;
                    p_game->state.status          = player | bits::Castling;
                } else {
                    // Player has played
                    p_game->board[indexPlaced]    = p_game->state.removed_1.piece;
                    *lastMovePtr                  = {p_game->state.removed_1.index, indexPlaced, p_game->state.removed_1.piece, false, false};
                    p_game->state.removed_1.index = NULL_INDEX;
                    p_game->state.removed_1.piece = Empty;
                    p_game->state.status          = otherPlayer | bits::ToPlay;
                    lastMovePtr->check            = isCheck(p_game);
                    p_game->moves++;
                    return true;
                }
            }
        } else if (NULL_INDEX != indexRemoved) {
            // Second piece has been removed, player is capturing
            LOG_INDEX("-> Second piece is removed", indexRemoved);
            p_game->state.removed_2.index                = indexRemoved;
            p_game->state.removed_2.piece                = p_game->board[p_game->state.removed_2.index];
            p_game->board[p_game->state.removed_2.index] = Empty;
            p_game->state.status                         = player | bits::Capturing;
        }
        break;
    }

    // ========================= IS CAPTURING
    case bits::Capturing: {
        if (NULL_INDEX != indexPlaced) {
            // The piece has been placed
            if ((indexPlaced != p_game->state.removed_1.index) && (indexPlaced != p_game->state.removed_2.index)) {
                LOG_INDEX("Two pieces removed and one piece placed at a different location!", indexPlaced);
            } else {
                // The piece has been placed where one was removed, player has played
                LOG_INDEX("-> Player captured", indexPlaced);

                // Check which piece has been removed first (capturing piece or captured piece)
                if (true == isPlayerColor(p_game->state.removed_1.piece)) {
                    // The capturing piece was removed first
                    *lastMovePtr               = {p_game->state.removed_1.index, indexPlaced, p_game->state.removed_1.piece, true, false};
                    p_game->board[indexPlaced] = p_game->state.removed_1.piece;
                } else if (true == isOtherPlayerColor(p_game->state.removed_1.piece)) {
                    // The captured piece was removed first
                    p_game->board[indexPlaced] = p_game->state.removed_2.piece;
                    *lastMovePtr               = {p_game->state.removed_2.index, indexPlaced, p_game->state.removed_2.piece, false, false};
                }

                p_game->state.removed_1.index = NULL_INDEX;
                p_game->state.removed_1.piece = Empty;
                p_game->state.removed_2.index = NULL_INDEX;
                p_game->state.removed_2.piece = Empty;
                p_game->state.status          = otherPlayer | bits::ToPlay;
                lastMovePtr->check            = isCheck(p_game);
                p_game->moves++;
                return true;
            }
        }

        if (NULL_INDEX != indexRemoved) {
            LOG_INDEX("-> Additional piece removed during capture!", indexRemoved);
        }
        break;
    }

    // ========================= IS CAPTURING
    case bits::Castling: {
        if (NULL_INDEX != indexRemoved) {
            // Player is castling (2/3)
            LOG_INDEX("-> Piece is removed", indexRemoved);
            p_game->state.removed_1.index                = indexRemoved;
            p_game->state.removed_1.piece                = p_game->board[p_game->state.removed_1.index];
            p_game->board[p_game->state.removed_1.index] = Empty;

            if (WRook != p_game->state.removed_1.piece) {
                LOG("-> Second piece removed during castling is not a rook!");
            }
        }

        if (NULL_INDEX != indexPlaced) {
            if (NULL_INDEX == p_game->state.removed_1.index) {
                LOG_INDEX("-> Additional piece is placed during castling!", indexPlaced);
            } else {
                // Player is castling (3/3)
                LOG_INDEX("-> Piece is placed", indexPlaced);
                p_game->board[indexPlaced]    = p_game->state.removed_1.piece;
                p_game->state.removed_1.index = NULL_INDEX;
                p_game->state.removed_1.piece = Empty;
                p_game->state.status          = otherPlayer | bits::ToPlay;
                lastMovePtr->check            = isCheck(p_game); // Rarest move ever if true :)
                p_game->moves++;
                return true;
            }
        }
        break;
    }

        // ========================= DEFAULT
    default: {
#ifdef HAS_PRINTF
        printf("-> Unhandled game status: %d\n", p_game->state.status);
#else
        Serial.print("-> Unhandled game status: ");
        Serial.print(p_game->state.status);
        Serial.print('\n');
#endif
    }
    }

    return false;
}

//-----------------------------------------------------------------------------
const char* getMoveStr(Move p_move)
//-----------------------------------------------------------------------------
{
    if (p_move.piece == EPiece::Empty)
        return "-";
    static char msg[7];
    uint8_t i = 0;

    switch (p_move.piece & bits::TypeMask) {
    case bits::King:
        msg[i++] = 'K';
        break;
    case bits::Queen:
        msg[i++] = 'Q';
        break;
    case bits::Bishop:
        msg[i++] = 'B';
        break;
    case bits::Knight:
        msg[i++] = 'N';
        break;
    case bits::Rook:
        msg[i++] = 'R';
        break;
    case bits::Pawn:
        msg[i++] = 'a' + (p_move.start % 8);
        break;
    }

    if (p_move.captured)
        msg[i++] = 'x';

    msg[i++] = 'a' + (p_move.end % 8);
    msg[i++] = '1' + (p_move.end / 8);

    // Override message with castling
    static const char CastlingMsg[5] = {'O', '-', 'O', '-', 'O'};
    if (isKing(p_move.piece)) {
        if (p_move.start + 2 == p_move.end) {
            std::memcpy(msg, CastlingMsg, i = 3);
        }
        if (p_move.start == 2 + p_move.end) {
            std::memcpy(msg, CastlingMsg, i = 5);
        }
    }

    if (p_move.check)
        msg[i++] = '+';

    msg[i] = 0;
    return msg;
}
