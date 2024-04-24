#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    WhiteToPlay = 0,
    WhiteIsPlaying,
    WhiteIsCapturing,
    WhiteIsCastling,
    BlackToPlay,
    BlackIsPlaying,
    BlackIsCapturing,
    BlackIsCastling,
    Draw,
    WhiteWon,
    BlackWon
} EStatus;

typedef enum {
    Empty = 0,
    WPawn,
    WKnight,
    WBishop,
    WRook,
    WQueen,
    WKing,
    BPawn,
    BKnight,
    BBishop,
    BRook,
    BQueen,
    BKing
} EPiece;

typedef struct {
    uint8_t index;
    EPiece piece;
} Removed;

typedef struct {
    Removed removed_1;
    Removed removed_2;
    EStatus status;
} State;

typedef struct {
    EPiece board[64]; // a1, b1, c1..., a2, b2, c2...
    State state;
    uint8_t moves;
} Game;

void initializeGame(Game *p_game);
bool isWhite(EPiece p_piece);
bool isBlack(EPiece p_piece);
const char* getPieceStr(EPiece p_piece);
const char* getStatusStr(EStatus p_status);
void printGame(Game* p_game);

// The sensors status are stored in a 64-bits variable: b63 = h8, b62 = g8..., b55 = h7, b54 = g7..., b1 = b1, b0 = a1
bool evolveGame(Game* p_game, uint64_t p_sensors);