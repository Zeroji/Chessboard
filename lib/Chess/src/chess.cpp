#include "chess.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    p_game->state.en_passant      = NULL_INDEX;
    p_game->lastMoveB.piece       = Empty;
    p_game->lastMoveW.piece       = Empty;
    p_game->moves                 = 0;

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
    p_game->state.status          = bits::Draw | bits::Finished;
    p_game->state.removed_1.index = NULL_INDEX;
    p_game->state.removed_1.piece = Empty;
    p_game->state.removed_2.index = NULL_INDEX;
    p_game->state.removed_2.piece = Empty;
    p_game->state.en_passant      = NULL_INDEX;
    p_game->lastMoveB.piece       = Empty;
    p_game->lastMoveW.piece       = Empty;
    p_game->moves                 = 0;

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
    char castlingRights[5]; // not supported
    char enPassantTarget[3];
    int halfmoveClock; // not supported
    int fullMoveNumber;

    if (5 != sscanf(&p_fen[index], " %c%s%s%d%d", &playerToMove, castlingRights, enPassantTarget, &halfmoveClock, &fullMoveNumber)) {
        LOG_INDEX("sscanf failed to parse FEN remainder starting at", index);
        return;
    }

    if (playerToMove == 'w') {
        p_game->state.status = bits::White | bits::ToPlay;
    } else if (playerToMove == 'b') {
        p_game->state.status = bits::Black | bits::ToPlay;
    }

    p_game->state.en_passant = getSquareFromStr(enPassantTarget);
    p_game->moves            = (fullMoveNumber - 1) * 2 + (playerToMove == 'b');
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

    char enPassantTarget[3] = "-\0"; // '-' 0 0
    writeSquareToStr(p_game->state.en_passant, enPassantTarget);

    const char playerToMove  = (p_game->state.status & bits::ColorMask) == bits::White ? 'w' : 'b';
    const int fullMoveNumber = (p_game->moves / 2) + 1;

    int ret = sprintf(&p_buffer[index], " %c KQkq %s 0 %d", playerToMove, /* castling not supported, */ enPassantTarget, /* half moves not supported, */ fullMoveNumber);
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
    printf("[%s, %d move%s]\n", getStatusStr(p_game->state.status), p_game->moves, (p_game->moves > 1 ? "s" : ""));
#else
    Serial.print('[');
    Serial.print(getStatusStr(p_game->state.status));
    Serial.print(", ");
    Serial.print(p_game->moves);
    Serial.print("moves]\n");
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
                    *lastMovePtr                  = {p_game->state.removed_1.index, indexPlaced, p_game->state.removed_1.piece, false, false, false};
                    p_game->board[indexPlaced]    = p_game->state.removed_1.piece;
                    p_game->state.removed_1.index = NULL_INDEX;
                    p_game->state.removed_1.piece = Empty;
                    p_game->state.status          = player | bits::Castling;
                } else {
                    // Player has played
                    p_game->board[indexPlaced]    = p_game->state.removed_1.piece;
                    *lastMovePtr                  = {p_game->state.removed_1.index, indexPlaced, p_game->state.removed_1.piece, false, false, false};
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

                    lastMovePtr->check = isCheck(p_game);
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
                    *lastMovePtr               = {p_game->state.removed_1.index, indexPlaced, p_game->state.removed_1.piece, true, false, false};
                    p_game->board[indexPlaced] = p_game->state.removed_1.piece;
                } else if (true == isOtherPlayerColor(p_game->state.removed_1.piece)) {
                    // The captured piece was removed first
                    p_game->board[indexPlaced] = p_game->state.removed_2.piece;
                    *lastMovePtr               = {p_game->state.removed_2.index, indexPlaced, p_game->state.removed_2.piece, true, false, false};
                }

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

                lastMovePtr->check = isCheck(p_game);
                p_game->moves++;
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
            lastMovePtr->check          = isCheck(p_game);
            p_game->state.en_passant    = NULL_INDEX;
            p_game->state.status        = otherPlayer | bits::ToPlay;
            p_game->moves++;
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
                p_game->state.en_passant      = NULL_INDEX;
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

    if (p_move.check)
        msg[i++] = '+';

    msg[i] = 0;
    return msg;
}
