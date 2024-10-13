#include "chess.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef ARDUINO_ARCH_AVR
#include <Arduino.h>
#define LOG(X) Serial.println(F(X))
#define LOG_INDEX(X, IDX)            \
    do {                             \
        Serial.print(F(X));          \
        Serial.print(F(" (index ")); \
        Serial.print(IDX);           \
        Serial.println(')');         \
    } while (false)
#else
#define HAS_PRINTF
#define LOG(X) printf(X "\n")
#define LOG_INDEX(X, IDX) printf(X " (index %d)\n", IDX)
#endif

const uint64_t DEFAULT_SENSORS_STATE = 0xFFFF00000000FFFFuLL; // (11111111 11111111 00000000 00000000 00000000 00000000 11111111 11111111)
const uint8_t NULL_INDEX             = 64;

typedef struct {
    uint8_t square;
    uint8_t color;
    uint8_t noColorPiece;
} CastlingUpdate;

static constexpr CastlingUpdate s_castlingUpdates[4] = {
    {0 * 8 + 0 /* A1 */, bits::White, bits::Queen},
    {0 * 8 + 7 /* A8 */, bits::White, bits::King },
    {7 * 8 + 0 /* H1 */, bits::Black, bits::Queen},
    {7 * 8 + 7 /* H8 */, bits::Black, bits::King }
};

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
    p_game->state.status                 = bits::White | bits::ToPlay;
    p_game->state.removed_1.index        = NULL_INDEX;
    p_game->state.removed_1.piece        = Empty;
    p_game->state.removed_2.index        = NULL_INDEX;
    p_game->state.removed_2.piece        = Empty;
    p_game->state.en_passant             = NULL_INDEX;
    p_game->state.castlingK[bits::White] = true;
    p_game->state.castlingQ[bits::White] = true;
    p_game->state.castlingK[bits::Black] = true;
    p_game->state.castlingQ[bits::Black] = true;
    p_game->lastMoveB.piece              = Empty;
    p_game->lastMoveW.piece              = Empty;
    p_game->fullmoveClock                = 1;
    p_game->halfmoveClock                = 0;

    LOG("Game has been initialized");
}

//-----------------------------------------------------------------------------
void initializeFromFEN(Game* p_game, const char* p_fen)
//-----------------------------------------------------------------------------
{
    if (NULL == p_game) {
        LOG("Unable to initialize null game");
        return;
    }

    // Game state
    p_game->state.status                 = bits::Draw | bits::Finished;
    p_game->state.removed_1.index        = NULL_INDEX;
    p_game->state.removed_1.piece        = Empty;
    p_game->state.removed_2.index        = NULL_INDEX;
    p_game->state.removed_2.piece        = Empty;
    p_game->state.en_passant             = NULL_INDEX;
    p_game->state.castlingK[bits::White] = true;
    p_game->state.castlingQ[bits::White] = true;
    p_game->state.castlingK[bits::Black] = true;
    p_game->state.castlingQ[bits::Black] = true;
    p_game->lastMoveB.piece              = Empty;
    p_game->lastMoveW.piece              = Empty;
    p_game->fullmoveClock                = 1;
    p_game->halfmoveClock                = 0;

    // Read board (first part of FEN)
    uint8_t rank = 7;
    uint8_t file = 0;

    int index = 0;
    while (rank > 0 || file < 8) {
        char c = p_fen[index++];
        if (c == 0) {
            LOG_INDEX("Unexpected end of FEN notation at", index);
            return;
        } else if (c == '/') {
            if (file < 8) {
                LOG_INDEX("Unexpected / in FEN notation at", index);
            }
            rank--;
            file = 0;
        } else if (c >= '1' && c <= '8') {
            const uint8_t emptySquareCount = (c - '0');
            for (uint8_t i = 0; i < emptySquareCount; i++) {
                p_game->board[rank * 8 + file] = EPiece::Empty;
                file++;
            }
        } else {
            const EPiece piece = charToPiece(c);
            if (piece == EPiece::Empty) {
                LOG_INDEX("Unexpected character in FEN notation at", index);
            } else {
                p_game->board[rank * 8 + file] = piece;
                file++;
            }
        }
    }

    // Read the rest of FEN with sscanf
    char playerToMove;
    char castlingRights[5];
    char enPassantTarget[3];
    int halfmoveClock;
    int fullmoveClock;

    if (5 != sscanf(&p_fen[index], " %c%s%s%d%d", &playerToMove, castlingRights, enPassantTarget, &halfmoveClock, &fullmoveClock)) {
        LOG_INDEX("sscanf failed to parse FEN remainder starting at", index);
        return;
    }

    size_t len                           = strlen(castlingRights);
    p_game->state.castlingK[bits::White] = (strcspn(castlingRights, "K") != len);
    p_game->state.castlingQ[bits::White] = (strcspn(castlingRights, "Q") != len);
    p_game->state.castlingK[bits::Black] = (strcspn(castlingRights, "k") != len);
    p_game->state.castlingQ[bits::Black] = (strcspn(castlingRights, "q") != len);

    if (playerToMove == 'w') {
        p_game->state.status = bits::White | bits::ToPlay;
    } else if (playerToMove == 'b') {
        p_game->state.status = bits::Black | bits::ToPlay;
    }

    p_game->state.en_passant = getSquareFromStr(enPassantTarget);
    p_game->fullmoveClock    = fullmoveClock;
    p_game->halfmoveClock    = halfmoveClock;
}

//-----------------------------------------------------------------------------
int writeToFEN(Game* p_game, char* p_buffer)
//-----------------------------------------------------------------------------
{
    if (p_game == nullptr || p_buffer == nullptr)
        return 0;

    int index = 0;
    for (int8_t rank = 7; rank >= 0; rank--) {
        bool wasEmpty = false;
        for (uint8_t file = 0; file < 8; file++) {
            const EPiece piece = p_game->board[rank * 8 + file];
            if (piece == EPiece::Empty) {
                if (wasEmpty) {
                    // Increment the previously written digit
                    p_buffer[index - 1]++;
                } else {
                    // Write a new digit
                    p_buffer[index++] = '1';
                    wasEmpty          = true;
                }
            } else {
                p_buffer[index++] = getPieceChar(piece);
                wasEmpty          = false;
            }
        }

        if (rank)
            p_buffer[index++] = '/';
    }

    char castlingStates[5] = "-\0"; // '-' 0 0 0 0
    uint8_t castlingIndex  = 0;
    if (p_game->state.castlingK[bits::White])
        castlingStates[castlingIndex++] = 'K';
    if (p_game->state.castlingQ[bits::White])
        castlingStates[castlingIndex++] = 'Q';
    if (p_game->state.castlingK[bits::Black])
        castlingStates[castlingIndex++] = 'k';
    if (p_game->state.castlingQ[bits::Black])
        castlingStates[castlingIndex++] = 'q';

    char enPassantTarget[3] = "-\0"; // '-' 0 0
    writeSquareToStr(p_game->state.en_passant, enPassantTarget);

    const char playerToMove = (p_game->state.status & bits::ColorMask) == bits::White ? 'w' : 'b';

    int ret = sprintf(&p_buffer[index], " %c %s %s %d %d", playerToMove, castlingStates, enPassantTarget, p_game->halfmoveClock, p_game->fullmoveClock);
    if (ret <= 0) {
        LOG_INDEX("Error while writing FEN with sprintf:", ret);
        return 0;
    }
    return index + ret; // chars written
}

//-----------------------------------------------------------------------------
char getPieceChar(EPiece p_piece)
//-----------------------------------------------------------------------------
{
    switch (p_piece) {
    case WPawn:
        return 'P';
    case WKnight:
        return 'N';
    case WBishop:
        return 'B';
    case WRook:
        return 'R';
    case WQueen:
        return 'Q';
    case WKing:
        return 'K';
    case BPawn:
        return 'p';
    case BKnight:
        return 'n';
    case BBishop:
        return 'b';
    case BRook:
        return 'r';
    case BQueen:
        return 'q';
    case BKing:
        return 'k';
    default:
        return ' ';
    };
}

//-----------------------------------------------------------------------------
EPiece charToPiece(char p_char)
//-----------------------------------------------------------------------------
{
    bool isBlack  = p_char & 0b00100000; // lowercase bit
    uint8_t color = isBlack ? bits::Black : bits::White;
    switch (p_char & 0b11011111) {
    case 'P':
        return (EPiece)(color | bits::Pawn);
    case 'N':
        return (EPiece)(color | bits::Knight);
    case 'B':
        return (EPiece)(color | bits::Bishop);
    case 'R':
        return (EPiece)(color | bits::Rook);
    case 'Q':
        return (EPiece)(color | bits::Queen);
    case 'K':
        return (EPiece)(color | bits::King);
    }
    return EPiece::Empty;
}

//-----------------------------------------------------------------------------
uint8_t getSquareFromStr(const char* p_square)
//-----------------------------------------------------------------------------
{
    if (p_square == nullptr)
        return NULL_INDEX;

    const uint8_t file = ((uint8_t)(p_square[0]) & 0b11011111) - (uint8_t)('A');
    const uint8_t rank = (uint8_t)(p_square[1]) - (uint8_t)('1');
    if (file < 8 && rank < 8)
        return rank * 8 + file;
    return NULL_INDEX;
}

//-----------------------------------------------------------------------------
void writeSquareToStr(uint8_t p_index, char* p_buffer)
//-----------------------------------------------------------------------------
{
    if (p_index >= NULL_INDEX || p_buffer == nullptr)
        return;
    const uint8_t rank = p_index / 8, file = p_index % 8;
    p_buffer[0] = file + 'a';
    p_buffer[1] = rank + '1';
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
    case bits::White | bits::EnPassant:
        return "White enpassant";
    case bits::White | bits::Castling:
        return "White castling ";
    case bits::Black | bits::ToPlay:
        return "Black to play  ";
    case bits::Black | bits::Playing:
        return "Black playing  ";
    case bits::Black | bits::Capturing:
        return "Black capturing";
    case bits::Black | bits::EnPassant:
        return "Black enpassant";
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

    LOG("  +---+---+---+---+---+---+---+---+");
    for (uint8_t i = 0; i < 8; i++) {
#ifdef HAS_PRINTF
        printf("%d ", (8 - i));
        for (uint8_t j = 0; j < 8; j++) {
            printf("| %c ", getPieceChar(p_game->board[8 * (8 - i - 1) + j]));
        }
#else
        Serial.print(8 - i);
        for (uint8_t j = 0; j < 8; j++) {
            Serial.print(" | ");
            Serial.print(getPieceChar(p_game->board[8 * (8 - i - 1) + j]));
        }
        Serial.print(' ');
#endif

        LOG("|\n  +---+---+---+---+---+---+---+---+");
    }

    LOG("    a   b   c   d   e   f   g   h");
#ifdef HAS_PRINTF
    printf("[%s]\n", getStatusStr(p_game->state.status));
#else
    Serial.print('[');
    Serial.print(getStatusStr(p_game->state.status));
    Serial.print("]\n");
#endif

    if ((p_game->state.status & bits::MoveMask) == bits::ToPlay) {
        Move* lastMove;
        if ((p_game->state.status & bits::ColorMask) == bits::White) {
            lastMove = &(p_game->lastMoveB);
        } else {
            lastMove = &(p_game->lastMoveW);
        }

#ifdef HAS_PRINTF
        printf("[Last move: %s]\n", getMoveStr(*lastMove));
#else
        Serial.print("[Last move: ");
        Serial.print(getMoveStr(*lastMove));
        Serial.print("]\n");
#endif
    }
}

//-----------------------------------------------------------------------------
bool isCheck(Game* p_game)
//-----------------------------------------------------------------------------
{
    if (NULL == p_game) {
        LOG("Unable to analyze check of null game");
        return false;
    }

    if (0 == (p_game->state.status & bits::ToPlay)) {
        LOG("Unable to analyze check when neither white nor black has to play");
        return false;
    }

    uint8_t nextPlayer     = (p_game->state.status & bits::ColorMask);
    uint8_t checkingPlayer = (nextPlayer == bits::White) ? bits::Black : bits::White;

    // Find King
    uint8_t checkedKingIndex = NULL_INDEX;
    for (uint8_t i = 0; i < 64; i++) {
        EPiece piece = p_game->board[i];
        if ((true == isKing(piece)) && (nextPlayer == (bits::ColorMask & piece))) {
            checkedKingIndex = i;
            break;
        }
    }

    if (NULL_INDEX == checkedKingIndex) {
        LOG("Unable to locate King");
        return false;
    }

    Move moves[1];
    return findMovesToSquare(p_game, checkedKingIndex, checkingPlayer, true /* p_returnOnFirst */, true /* p_includeThreats */, moves);
}

//-----------------------------------------------------------------------------
bool isCheckmate(Game* p_game)
//-----------------------------------------------------------------------------
{
    if (NULL == p_game) {
        LOG("Unable to analyze checkmate of null game");
        return false;
    }

    if (0 == (p_game->state.status & bits::ToPlay)) {
        LOG("Unable to analyze checkmate when neither white nor black has to play");
        return false;
    }

    uint8_t checkedPlayer  = (p_game->state.status & bits::ColorMask);
    uint8_t checkingPlayer = (checkedPlayer == bits::White) ? bits::Black : bits::White;

    // 0. Find King
    uint8_t checkedKingIndex = NULL_INDEX;
    for (uint8_t i = 0; i < 64; i++) {
        EPiece piece = p_game->board[i];
        if ((true == isKing(piece)) && (checkedPlayer == (bits::ColorMask & piece))) {
            checkedKingIndex = i;
            break;
        }
    }

    if (NULL_INDEX == checkedKingIndex) {
        LOG("Unable to locate King");
        return false;
    }

    // 1. Find all moves threatening the King
    Move threatenKing[16];
    uint8_t threatenSize = findMovesToSquare(p_game, checkedKingIndex, checkingPlayer, false /* p_returnOnFirst */, true /* p_includeThreats */, threatenKing);

    if (threatenSize == 0) {
        return false; // No opponent piece are threatening the King, not checkmate
    }

    // 2. Look for a square for the King to escape
    uint8_t kingCol         = checkedKingIndex % 8;
    uint8_t kingRow         = checkedKingIndex / 8;
    static int8_t dirCol[8] = {-1, 0, 1, 1, 1, 0, -1, -1};
    static int8_t dirRow[8] = {1, 1, 1, 0, -1, -1, -1, 0};
    for (uint8_t i = 0; i < 8; i++) {
        uint8_t col = kingCol + dirCol[i];
        uint8_t row = kingRow + dirRow[i];
        if (col >= 8 || row >= 8)
            continue; // Out of board

        uint8_t escapeSquare = 8 * row + col;
        uint8_t onSquare     = p_game->board[escapeSquare];
        if ((EPiece::Empty != onSquare) && (checkedPlayer == (onSquare & bits::ColorMask)))
            continue; // Checked player piece is occupying the square

        // Temporary remove King from the board
        p_game->board[checkedKingIndex] = EPiece::Empty;

        // Look for pieces threatening/defending the escape square
        Move moves[1];
        uint8_t size = findMovesToSquare(p_game, escapeSquare, checkingPlayer, true /* p_returnOnFirst */, true /* p_includeThreats */, moves);

        // Replace the King on the board
        p_game->board[checkedKingIndex] = static_cast<EPiece>(bits::King | checkedPlayer);

        if (size == 0) {
            return false; // Found an escape square not threaten by any opponent piece, no checkmate
        }
    }

    // 3. If checked by several pieces with no escape square: checkmate
    if (threatenSize >= 2) {
        return true;
    }

    // 4. If checked by a knight, try to capture it
    if (isKnight(threatenKing[0].piece)) {
        Move threatenKnight[16];
        uint8_t size = findMovesToSquare(p_game, threatenKing[0].start, checkedPlayer, false /* p_returnOnFirst */, false /* p_includeThreats */, threatenKnight);

        // Verify capturing piece is not pinned
        for (uint8_t i = 0; i < size; i++) {
            if (false == isPinned(p_game, threatenKnight[i].start, checkedKingIndex, checkingPlayer)) {
                return false; // Found a piece to capture the checking knight
            }
        }

        // 4.1 No escape square, no capture/intercept of the checking knight: checkmate
        return true;
    }

    // 5. Try to capture or intercept the threatening piece
    uint8_t threatenCol = threatenKing[0].start % 8;
    uint8_t threatenRow = threatenKing[0].start / 8;

    int diffCol = kingCol - threatenCol;
    int diffRow = kingRow - threatenRow;

    uint8_t col = threatenCol;
    uint8_t row = threatenRow;
    while (col != kingCol || row != kingRow) {
        uint8_t index = 8 * row + col;
        Move intercept[16];
        uint8_t size = findMovesToSquare(p_game, index, checkedPlayer, false /* p_returnOnFirst */, false /* p_includeThreats */, intercept);

        // Verify intercepting/capturing piece is not pinned
        for (uint8_t i = 0; i < size; i++) {
            if (false == isPinned(p_game, intercept[i].start, checkedKingIndex, checkingPlayer)) {
                return false; // Found a piece to capture/intercept the checking piece
            }
        }

        if (diffCol != 0)
            col += (diffCol < 0) ? -1 : 1;
        if (diffRow != 0)
            row += (diffRow < 0) ? -1 : 1;
    }

    // 6. If checked by a pawn, try to capture with en-passant
    if (isPawn(threatenKing[0].piece)) {
        const int8_t dirPRow = (bits::Black == checkingPlayer) ? 1 : -1;

        col = threatenKing[0].start % 8;
        row = threatenKing[0].start / 8;

        if (p_game->state.en_passant == (8 * (row + dirPRow) + col)) {
            const int8_t posPCol[2] = {-1, 1};
            for (uint8_t i = 0; i < 2; i++) {
                uint8_t pawnCol = col + posPCol[i];

                if (pawnCol >= 8)
                    continue;

                uint8_t index = 8 * row + pawnCol;
                EPiece piece  = p_game->board[index];
                if (isPawn(piece) && (checkedPlayer == (piece & bits::ColorMask)) && !isPinned(p_game, index, checkedKingIndex, checkingPlayer)) {
                    // En-passant is saving from checkmate!
                    return false;
                }
            }
        }
    }

    // 7. No escape square, no capture/intercept of the checking piece: checkmate
    return true;
}

//-----------------------------------------------------------------------------
uint8_t findMovesToSquare(Game* p_game, uint8_t p_targetSquare, uint8_t p_color, bool p_returnOnFirst, bool p_includeThreats, Move* p_moves)
//-----------------------------------------------------------------------------
{
    if (NULL == p_game) {
        LOG("Unable to analyze checkmate of null game");
        return 0;
    }

    uint8_t size      = 0;
    uint8_t targetCol = p_targetSquare % 8;
    uint8_t targetRow = p_targetSquare / 8;

    // 1. Moves with Queens, Rooks, Bishops
    // Prepare directions: col-, col+, row-, row+, diagBL, diagTL, diagBR, diagTR
    const int8_t dirCol[8] = {-1, 1, 0, 0, -1, -1, 1, 1};
    const int8_t dirRow[8] = {0, 0, -1, 1, -1, 1, -1, 1};

    bool (*matchingFunc[2])(EPiece) = {&isThreateningOrthogonal, &isThreateningDiagonal};

    for (uint8_t i = 0; i < 8; i++) {
        uint8_t col = targetCol + dirCol[i];
        uint8_t row = targetRow + dirRow[i];

        bool pieceFound = false;
        EPiece piece    = Empty;
        while ((col < 8) && (row < 8)) {
            piece = p_game->board[8 * row + col];
            if (piece != EPiece::Empty) {
                // Found a piece aligned with the target square (straight or diagonally)
                pieceFound = true;
                break;
            }

            col += dirCol[i];
            row += dirRow[i];
        }

        if (true == pieceFound) {
            if ((true == (*matchingFunc[i / 4])(piece)) && (p_color == (piece & bits::ColorMask))) {
                p_moves[size].start = 8 * row + col;
                p_moves[size].end   = p_targetSquare;
                p_moves[size].piece = piece;
                size++;

                if (p_returnOnFirst)
                    return size;
            }
        }
    }

    // 2. Moves with Knights
    const int8_t posNCol[8] = {1, 2, 2, 1, -1, -2, -2, -1};
    const int8_t posNRow[8] = {2, 1, -1, -2, -2, -1, 1, 2};

    for (uint8_t i = 0; i < 8; i++) {
        uint8_t col = targetCol + posNCol[i];
        uint8_t row = targetRow + posNRow[i];

        if ((col < 8) && (row < 8)) {
            EPiece piece = p_game->board[8 * row + col];
            if ((true == isKnight(piece)) && (p_color == (piece & bits::ColorMask))) {
                p_moves[size].start = 8 * row + col;
                p_moves[size].end   = p_targetSquare;
                p_moves[size].piece = piece;
                size++;

                if (p_returnOnFirst)
                    return size;
            }
        }
    }

    // 3. Move with Pawns (threaten / capture)
    const int8_t posPCol[2]  = {-1, 1};
    const int8_t dirPRow     = (bits::Black == p_color) ? 1 : -1;
    const EPiece targetPiece = p_game->board[p_targetSquare];

    for (uint8_t i = 0; i < 2; i++) {
        uint8_t col = targetCol + posPCol[i];
        uint8_t row = targetRow + dirPRow;

        if ((col < 8) && (row < 8)) {
            EPiece piece = p_game->board[8 * row + col];
            if ((true == isPawn(piece)) && (p_color == (piece & bits::ColorMask))) {

                if (!p_includeThreats && (EPiece::Empty == targetPiece || p_color == (targetPiece & bits::ColorMask)))
                    continue; // Threats not required; and target square is not a piece than can be captured (not real move): skip

                p_moves[size].start = 8 * row + col;
                p_moves[size].end   = p_targetSquare;
                p_moves[size].piece = piece;
                size++;

                if (p_returnOnFirst)
                    return size;
            }
        }
    }

    // 4. Move with Pawns (en-passant)
    if (p_targetSquare == p_game->state.en_passant) {
        for (uint8_t i = 0; i < 2; i++) {
            uint8_t col = targetCol + dirCol[i];
            uint8_t row = targetRow + dirPRow;

            if ((col < 8) && (row < 8)) {
                EPiece piece = p_game->board[8 * row + col];
                if ((true == isPawn(piece)) && (p_color == (piece & bits::ColorMask))) {
                    p_moves[size].start = 8 * row + col;
                    p_moves[size].end   = p_targetSquare;
                    p_moves[size].piece = piece;
                    size++;

                    if (p_returnOnFirst)
                        return size;
                }
            }
        }
    }

    // 5. Move with Pawns (forward)
    if (EPiece::Empty == p_game->board[p_targetSquare]) {
        uint8_t row = targetRow + dirPRow;
        if (row < 8) {
            EPiece piece = p_game->board[8 * row + targetCol];
            if ((true == isPawn(piece)) && (p_color == (piece & bits::ColorMask))) {
                p_moves[size].start = 8 * row + targetCol;
                p_moves[size].end   = p_targetSquare;
                p_moves[size].piece = piece;
                size++;

                if (p_returnOnFirst)
                    return size;
            } else if (EPiece::Empty == piece) {
                row += dirPRow;
                if (row == 1 || row == 6) {
                    piece = p_game->board[8 * row + targetCol];
                    if ((true == isPawn(piece)) && (p_color == (piece & bits::ColorMask))) {
                        p_moves[size].start = 8 * row + targetCol;
                        p_moves[size].end   = p_targetSquare;
                        p_moves[size].piece = piece;
                        size++;

                        if (p_returnOnFirst)
                            return size;
                    }
                }
            }
        }
    }

    // 6. Moves with King
    for (uint8_t i = 0; i < 8; i++) {
        uint8_t col = targetCol + dirCol[i];
        uint8_t row = targetRow + dirRow[i];

        if (col >= 8 || row >= 8)
            continue; // Out of board

        EPiece piece = p_game->board[8 * row + col];
        if (isKing(piece) && (p_color == (piece & bits::ColorMask))) {
            if (!p_includeThreats) {
                // Verify if the King can actually move to the target square
                uint8_t blockSize = 0;
                Move blockMoves[1];
                uint8_t otherColor = (bits::White == p_color) ? bits::Black : bits::White;
                blockSize          = findMovesToSquare(p_game, 8 * row + col, otherColor, true /* p_returnOnFirst */, true /* p_includeThreats */, blockMoves);

                if (blockSize > 0)
                    continue; // King can't actually move to the target square (defended)
            }

            p_moves[size].start = 8 * row + col;
            p_moves[size].end   = p_targetSquare;
            p_moves[size].piece = piece;
            size++;

            if (p_returnOnFirst)
                return size;
        }
    }

    return size;
}

//-----------------------------------------------------------------------------
bool isPinned(Game* p_game, uint8_t p_piece, uint8_t p_king, uint8_t p_pinningColor)
//-----------------------------------------------------------------------------
{
    if (NULL == p_game) {
        LOG("Unable to analyze pinned piece of null game");
        return false;
    }

    uint8_t pieceCol = p_piece % 8;
    uint8_t pieceRow = p_piece / 8;
    uint8_t kingCol  = p_king % 8;
    uint8_t kingRow  = p_king / 8;

    int diffCol = kingCol - pieceCol;
    int diffRow = kingRow - pieceRow;

    bool orthogonal = (diffCol == 0 || diffRow == 0) && (diffCol != diffRow);
    bool diagonal   = abs(diffCol) == abs(diffRow);

    if (!orthogonal && !diagonal)
        return false; // Not orthogonally neither diagonally placed

    uint8_t col = pieceCol;
    uint8_t row = pieceRow;
    while (col < 8 && row < 8) {
        EPiece piece = p_game->board[8 * row + col];
        if (EPiece::Empty != piece && (p_pinningColor == (piece & bits::ColorMask))) {
            if (orthogonal && 0 != (piece & (bits::LongRangeFlag | bits::OrthogonalFlag))) {
                return true;
            } else if (diagonal && 0 != (piece & (bits::LongRangeFlag | bits::DiagonalFlag))) {
                return true;
            }
        }

        if (diffCol != 0)
            col += (diffCol < 0) ? 1 : -1;
        if (diffRow != 0)
            row += (diffRow < 0) ? 1 : -1;
    }

    return false;
}

//-----------------------------------------------------------------------------
uint8_t findEnPassantSquare(Move* p_move)
//-----------------------------------------------------------------------------
{
    if (NULL == p_move) {
        LOG("Unable to analyze en passant of null move");
        return NULL_INDEX;
    }

    if (!isPawn(p_move->piece)) {
        return NULL_INDEX;
    }

    uint8_t startRow = (p_move->start / 8);
    uint8_t endRow   = (p_move->end / 8);
    uint8_t endCol   = (p_move->end % 8);

    if ((startRow == 1) && (endRow == 3)) {
        return 2 * 8 + endCol; // white pawn can be taken en passant
    }
    if ((startRow == 6) && (endRow == 4)) {
        return 5 * 8 + endCol; // black pawn can be taken en passant
    }
    return NULL_INDEX;
}

//-----------------------------------------------------------------------------
bool isPromotion(Move* p_move)
//-----------------------------------------------------------------------------
{
    if (NULL == p_move) {
        LOG("Unable to analyze promotion of null move");
        return false;
    }

    uint8_t startRow = (p_move->start / 8) + 1;
    uint8_t endRow   = (p_move->end / 8) + 1;

    if ((7 == startRow) && (8 == endRow) && isPawn(p_move->piece) && isWhite(p_move->piece)) {
        return true;
    }

    if ((2 == startRow) && (1 == endRow) && isPawn(p_move->piece) && isBlack(p_move->piece)) {
        return true;
    }

    return false;
}

//-----------------------------------------------------------------------------
void updateCastlingAvailability(Game* p_game)
//-----------------------------------------------------------------------------
{
    if (nullptr == p_game) {
        LOG("Unable to update castling availability for null game");
        return;
    }

    if (bits::ToPlay != (p_game->state.status & bits::MoveMask)) {
        // Nothing changed since last castling update because no move was played ;)
        return;
    }

    Move* lastMove;
    uint8_t lastMoveColor;
    if (bits::White == (p_game->state.status & bits::ColorMask)) {
        lastMove      = &p_game->lastMoveB;
        lastMoveColor = bits::Black;
    } else {
        lastMove      = &p_game->lastMoveW;
        lastMoveColor = bits::White;
    }

    if (isKing(lastMove->piece)) {
        p_game->state.castlingK[lastMoveColor] = false;
        p_game->state.castlingQ[lastMoveColor] = false;
        return;
    }

    for (uint8_t i = 0; i < 4; i++) {
        CastlingUpdate cu = s_castlingUpdates[i];
        // If any piece moved from or to an initial rook square, castling is not allowed anymore on this side
        if ((lastMove->start == cu.square) || (lastMove->end == cu.square)) {
            if (bits::King == cu.noColorPiece) {
                p_game->state.castlingK[cu.color] = false;
            } else {
                p_game->state.castlingQ[cu.color] = false;
            }
        }
    }
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
        if (bits::White == player) {
            LOG("-> Game is over: white won!");
        } else {
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
                if ((true == isKing(p_game->state.removed_1.piece)) && (2 == diff)) {
                    // King replaced two cells away on the same row: player is castling (1/3)
                    *lastMovePtr                  = BUILD_MOVE(p_game->state.removed_1.index, indexPlaced, p_game->state.removed_1.piece);
                    p_game->board[indexPlaced]    = p_game->state.removed_1.piece;
                    p_game->state.removed_1.index = NULL_INDEX;
                    p_game->state.removed_1.piece = Empty;
                    p_game->state.status          = player | bits::Castling;
                } else {
                    // Player has played
                    p_game->board[indexPlaced]    = p_game->state.removed_1.piece;
                    *lastMovePtr                  = BUILD_MOVE(p_game->state.removed_1.index, indexPlaced, p_game->state.removed_1.piece);
                    p_game->state.removed_1.index = NULL_INDEX;
                    p_game->state.removed_1.piece = Empty;
                    p_game->state.status          = otherPlayer | bits::ToPlay;

                    // Check for en passant capture
                    if (p_game->state.en_passant == indexPlaced && isPawn(lastMovePtr->piece)) {
                        uint8_t startRow         = (lastMovePtr->start / 8);
                        uint8_t endCol           = (indexPlaced % 8);
                        p_game->state.en_passant = startRow * 8 + endCol; // position of pawn taken en passant
                        p_game->state.status     = player | bits::EnPassant;
                        return true;
                    }

                    // Check for en passant possible next turn
                    p_game->state.en_passant = findEnPassantSquare(lastMovePtr);
                    if (p_game->state.en_passant != NULL_INDEX) {
                        LOG_INDEX("en passant possible at index", p_game->state.en_passant);
                    }

                    // Check for promotion
                    if (true == isPromotion(lastMovePtr)) {
                        lastMovePtr->promotion     = true;
                        p_game->board[indexPlaced] = static_cast<EPiece>(player | bits::Queen);
                    }

                    updateCheckState(p_game, lastMovePtr);
                    p_game->fullmoveClock += (player == bits::Black ? 1 : 0);

                    if (isPawn(lastMovePtr->piece)) {
                        p_game->halfmoveClock = 0;
                    } else {
                        p_game->halfmoveClock++;
                    }

                    updateCastlingAvailability(p_game);
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
                    *lastMovePtr               = BUILD_MOVE(p_game->state.removed_1.index, indexPlaced, p_game->state.removed_1.piece);
                    p_game->board[indexPlaced] = p_game->state.removed_1.piece;
                } else if (true == isOtherPlayerColor(p_game->state.removed_1.piece)) {
                    // The captured piece was removed first
                    p_game->board[indexPlaced] = p_game->state.removed_2.piece;
                    *lastMovePtr               = BUILD_MOVE(p_game->state.removed_2.index, indexPlaced, p_game->state.removed_2.piece);
                }
                lastMovePtr->captured = true;

                p_game->state.removed_1.index = NULL_INDEX;
                p_game->state.removed_1.piece = Empty;
                p_game->state.removed_2.index = NULL_INDEX;
                p_game->state.removed_2.piece = Empty;
                p_game->state.en_passant      = NULL_INDEX;
                p_game->state.status          = otherPlayer | bits::ToPlay;

                // Check for promotion
                if (true == isPromotion(lastMovePtr)) {
                    lastMovePtr->promotion     = true;
                    p_game->board[indexPlaced] = static_cast<EPiece>(player | bits::Queen);
                }

                updateCheckState(p_game, lastMovePtr);
                p_game->fullmoveClock += (player == bits::Black ? 1 : 0);
                p_game->halfmoveClock = 0;
                updateCastlingAvailability(p_game);
                return true;
            }
        }

        if (NULL_INDEX != indexRemoved) {
            LOG_INDEX("-> Additional piece removed during capture!", indexRemoved);
        }
        break;
    }

    // ========================= IS CAPTURING en passant
    case bits::EnPassant: {
        if (NULL_INDEX != indexPlaced) {
            LOG_INDEX("-> Additional piece placed during en passant!", indexPlaced);
        } else if (indexRemoved == p_game->state.en_passant) {
            p_game->board[indexRemoved] = Empty;
            lastMovePtr->captured       = true;
            p_game->state.en_passant    = NULL_INDEX;
            p_game->state.status        = otherPlayer | bits::ToPlay;
            p_game->fullmoveClock += (player == bits::Black ? 1 : 0);
            p_game->halfmoveClock = 0;
            updateCheckState(p_game, lastMovePtr);
            // updateCastlingAvailability(p_game); // -> en-passant can't change castling availability
            return true;
        } else {
            LOG_INDEX("-> Wrong piece removed during en passant!", indexRemoved);
        }
        break;
    }

    // ========================= IS CASTLING
    case bits::Castling: {
        if (NULL_INDEX != indexRemoved) {
            // Player is castling (2/3)
            LOG_INDEX("-> Piece is removed", indexRemoved);
            p_game->state.removed_1.index                = indexRemoved;
            p_game->state.removed_1.piece                = p_game->board[p_game->state.removed_1.index];
            p_game->board[p_game->state.removed_1.index] = Empty;

            if (false == isRook(p_game->state.removed_1.piece)) {
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
                p_game->state.en_passant      = NULL_INDEX;
                p_game->state.status          = otherPlayer | bits::ToPlay;
                p_game->fullmoveClock += (player == bits::Black ? 1 : 0);
                p_game->halfmoveClock++;
                updateCheckState(p_game, lastMovePtr);
                updateCastlingAvailability(p_game);
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
    case bits::Pawn: {
        if (p_move.captured)
            msg[i++] = 'a' + (p_move.start % 8);
        break;
    }
    }

    if (p_move.captured)
        msg[i++] = 'x';

    msg[i++] = 'a' + (p_move.end % 8);
    msg[i++] = '1' + (p_move.end / 8);

    // Override message with castling
    static const char CastlingMsg[5] = {'O', '-', 'O', '-', 'O'};
    if (isKing(p_move.piece)) {
        if (p_move.start + 2 == p_move.end) {
            memcpy(msg, CastlingMsg, i = 3);
        }
        if (p_move.start == 2 + p_move.end) {
            memcpy(msg, CastlingMsg, i = 5);
        }
    }

    if (p_move.promotion) {
        msg[i++] = '=';
        msg[i++] = 'Q';
    }

    if (p_move.checkmate)
        msg[i++] = '#';
    else if (p_move.check)
        msg[i++] = '+';

    msg[i] = 0;
    return msg;
}

//-----------------------------------------------------------------------------
void updateCheckState(Game* p_game, Move* p_move)
//-----------------------------------------------------------------------------
{
    if (NULL == p_game || NULL == p_move) {
        return;
    }

    p_move->check     = isCheck(p_game);
    p_move->checkmate = p_move->check ? isCheckmate(p_game) : false;
}