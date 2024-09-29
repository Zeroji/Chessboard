#pragma once

#include <stdbool.h>
#include <stdint.h>

// Use bit masks to check attributes faster
namespace bits {
constexpr uint8_t ColorMask      = 0b10000000;
constexpr uint8_t TypeMask       = 0b00000111;
constexpr uint8_t LongRangeFlag  = 0b00000100;
constexpr uint8_t DiagonalFlag   = 0b00000010;
constexpr uint8_t OrthogonalFlag = 0b00000001;

constexpr uint8_t Pawn   = DiagonalFlag;
constexpr uint8_t King   = DiagonalFlag | OrthogonalFlag;
constexpr uint8_t Knight = LongRangeFlag;
constexpr uint8_t Rook   = LongRangeFlag | OrthogonalFlag;
constexpr uint8_t Bishop = LongRangeFlag | DiagonalFlag;
constexpr uint8_t Queen  = LongRangeFlag | DiagonalFlag | OrthogonalFlag;

constexpr uint8_t White = 0, Black = ColorMask;

constexpr uint8_t MoveMask  = 0b00111111;
constexpr uint8_t ToPlay    = 0b00000001;
constexpr uint8_t Playing   = 0b00000010;
constexpr uint8_t Capturing = 0b00000100;
constexpr uint8_t EnPassant = 0b00000101;
constexpr uint8_t Castling  = 0b00001000;
constexpr uint8_t Finished  = 0b00010000;
constexpr uint8_t Draw      = 0b00100000;
} // namespace bits

typedef enum {
    Empty   = 0,
    WPawn   = bits::White | bits::Pawn,
    WKnight = bits::White | bits::Knight,
    WBishop = bits::White | bits::Bishop,
    WRook   = bits::White | bits::Rook,
    WQueen  = bits::White | bits::Queen,
    WKing   = bits::White | bits::King,
    BPawn   = bits::Black | bits::Pawn,
    BKnight = bits::Black | bits::Knight,
    BBishop = bits::Black | bits::Bishop,
    BRook   = bits::Black | bits::Rook,
    BQueen  = bits::Black | bits::Queen,
    BKing   = bits::Black | bits::King,
} EPiece;

inline bool isWhite(EPiece p_piece) {
    return (p_piece != EPiece::Empty) && ((p_piece & bits::ColorMask) == bits::White);
}

inline bool isBlack(EPiece p_piece) {
    return (p_piece & bits::ColorMask) == bits::Black;
}

inline bool isThreateningOrthogonal(EPiece p_piece) {
    return (p_piece & (bits::LongRangeFlag | bits::OrthogonalFlag)) == (bits::LongRangeFlag | bits::OrthogonalFlag);
}

inline bool isThreateningDiagonal(EPiece p_piece) {
    return (p_piece & (bits::LongRangeFlag | bits::DiagonalFlag)) == (bits::LongRangeFlag | bits::DiagonalFlag);
}

inline bool isPawn(EPiece p_piece) {
    return (p_piece & bits::TypeMask) == bits::Pawn;
}

inline bool isKnight(EPiece p_piece) {
    return (p_piece & bits::TypeMask) == bits::Knight;
}

inline bool isBishop(EPiece p_piece) {
    return (p_piece & bits::TypeMask) == bits::Bishop;
}

inline bool isRook(EPiece p_piece) {
    return (p_piece & bits::TypeMask) == bits::Rook;
}

inline bool isQueen(EPiece p_piece) {
    return (p_piece & bits::TypeMask) == bits::Queen;
}

inline bool isKing(EPiece p_piece) {
    return (p_piece & bits::TypeMask) == bits::King;
}

typedef struct {
    uint8_t index;
    EPiece piece;
} Removed;

typedef struct {
    Removed removed_1;
    Removed removed_2;
    uint8_t en_passant;
    uint8_t status;
} State;

typedef struct {
    uint8_t start;
    uint8_t end;
    EPiece piece;
    bool captured;
    bool check;
    bool promotion;
} Move;

typedef struct {
    EPiece board[64]; // a1, b1, c1..., a2, b2, c2...
    State state;
    Move lastMoveW;
    Move lastMoveB;
    uint8_t moves;
} Game;

void initializeGame(Game* p_game, uint64_t p_mask /* = 0xffff00000000ffffuLL */);
bool isWhite(EPiece p_piece);
bool isBlack(EPiece p_piece);
const char* getPieceStr(EPiece p_piece);
const char* getStatusStr(uint8_t p_status);
const char* getMoveStr(Move p_move);
void printGame(Game* p_game);
bool isCheck(Game* p_game);

// The sensors status are stored in a 64-bits variable: b63 = h8, b62 = g8..., b55 = h7, b54 = g7..., b1 = b1, b0 = a1
bool evolveGame(Game* p_game, uint64_t p_sensors);
