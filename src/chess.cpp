#include "../include/chess.h"

#include <stdio.h>
#include <stdlib.h>

#ifdef ARDUINO_ARCH_AVR
#include <Arduino.h>
#define LOG(X) Serial.println(X)
#define LOG_INDEX(X, IDX) do { Serial.print(X); Serial.print(" (index "); Serial.print(IDX); Serial.println(')'); } while(false)
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
    if (NULL == p_game)
    {
        LOG("Unable to initialize null game");
        return;
    }

    // Empty everything
    for(uint8_t i = 0; i < 64; i++)
    {
        p_game->board[i] = Empty;
    }

    // Pawns
    for(uint8_t i = 0; i < 8; i++)
    {
        p_game->board[1*8+i] = WPawn;
        p_game->board[6*8+i] = BPawn;
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
    p_game->board[7*8+0] = BRook;
    p_game->board[7*8+1] = BKnight;
    p_game->board[7*8+2] = BBishop;
    p_game->board[7*8+3] = BQueen;
    p_game->board[7*8+4] = BKing;
    p_game->board[7*8+5] = BBishop;
    p_game->board[7*8+6] = BKnight;
    p_game->board[7*8+7] = BRook;

    // Mask pieces
    for (uint8_t i = 0; i < 64; i++)
        if ((p_mask & (1uLL << i)) == 0)
            p_game->board[i] = Empty;

    // Game state
    p_game->state.status = WhiteToPlay;
    p_game->state.removed_1.index = NULL_INDEX;
    p_game->state.removed_1.piece = Empty;
    p_game->state.removed_2.index = NULL_INDEX;
    p_game->state.removed_2.piece = Empty;
    p_game->lastMoveB.piece = Empty;
    p_game->lastMoveW.piece = Empty;
    p_game->moves = 0;

    LOG("Game has been initialized");
}

//-----------------------------------------------------------------------------
bool isWhite(EPiece p_piece)
//-----------------------------------------------------------------------------
{
    switch (p_piece)
    {
        case WPawn:
        case WKnight:
        case WBishop:
        case WRook:
        case WQueen:
        case WKing:
            return true;
        default:
            return false;
    };
}

//-----------------------------------------------------------------------------
bool isBlack(EPiece p_piece)
//-----------------------------------------------------------------------------
{
    switch (p_piece)
    {
        case BPawn:
        case BKnight:
        case BBishop:
        case BRook:
        case BQueen:
        case BKing:
            return true;
        default:
            return false;
    };
}

//-----------------------------------------------------------------------------
const char* getPieceStr(EPiece p_piece)
//-----------------------------------------------------------------------------
{
    switch (p_piece)
    {
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
const char* getStatusStr(EStatus p_status)
//-----------------------------------------------------------------------------
{
    // All strings are of equal width to clear LCD screen
    switch (p_status)
    {
        case WhiteToPlay:
        return "White to play  ";
        case WhiteIsPlaying:
        return "White playing  ";
        case WhiteIsCapturing:
        return "White capturing";
        case WhiteIsCastling:
        return "White castling ";
        case BlackToPlay:
        return "Black to play  ";
        case BlackIsPlaying:
        return "Black playing  ";
        case BlackIsCapturing:
        return "Black capturing";
        case BlackIsCastling:
        return "Black castling ";
        case Draw:
        return "Draw           ";
        case WhiteWon:
        return "White won      ";
        case BlackWon:
        return "Black won      ";

        default:
        return "Undefined      ";
    };
}

//-----------------------------------------------------------------------------
void printGame(Game* p_game)
//-----------------------------------------------------------------------------
{
    if (NULL == p_game)
    {
        LOG("Unable to print null game");
        return;
    }

    LOG("  +----+----+----+----+----+----+----+----+");
    for(uint8_t i = 0; i < 8; i++)
    {
#ifdef HAS_PRINTF
        printf("%d ", (8-i));
        for(uint8_t j = 0; j < 8; j++)
        {
            printf("| %s ", getPieceStr(p_game->board[8*(8-i-1)+j]));
        }
#else
        Serial.print(8-i);
        for(uint8_t j = 0; j < 8; j++)
        {
            Serial.print(" | ");
            Serial.print(getPieceStr(p_game->board[8*(8-i-1)+j]));
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
bool evolveGame(Game* p_game, uint64_t p_sensors)
//-----------------------------------------------------------------------------
{
    if (NULL == p_game)
    {
        LOG("Unable to evolve null game");
        return false;
    }

    uint8_t indexRemoved = NULL_INDEX;
    uint8_t indexPlaced = NULL_INDEX;

    for(uint8_t i = 0; i < 64; i++)
    {
        bool sensor = (p_sensors >> i) & 0x01;
        if ((false == sensor) && (p_game->board[i] != Empty))
        {
            indexRemoved = i;
        }

        if ((true == sensor) && (p_game->board[i] == Empty))
        {
            indexPlaced = i;
        }
    }

    if ((NULL_INDEX == indexRemoved) && (NULL_INDEX == indexPlaced))
    {
        // No change
#ifndef ARDUINO_ARCH_AVR
        LOG("-> no change");
#endif
        return false;
    }

    // Decision flow
    switch(p_game->state.status)
    {
        // ========================= WHITE TURN
        case WhiteToPlay:
        {
            if (NULL_INDEX != indexRemoved)
            {
                // Piece has been removed, white is playing
                LOG_INDEX("-> Piece is removed", indexRemoved);
                p_game->state.removed_1.index = indexRemoved;
                p_game->state.removed_1.piece = p_game->board[p_game->state.removed_1.index];
                p_game->board[p_game->state.removed_1.index] = Empty;
                p_game->state.status = WhiteIsPlaying;
            }

            if (NULL_INDEX != indexPlaced)
            {
                LOG_INDEX("-> Additional piece placed during white turn!", indexPlaced);
            }
            break;
        }
        case WhiteIsPlaying:
        {
            if (NULL_INDEX != indexPlaced)
            {
                // Piece has been placed
                if (p_game->state.removed_1.index == indexPlaced)
                {
                    // A piece has been removed and placed at the same location (undo), still white to play
                    LOG("-> White canceled its move");
                    p_game->board[indexPlaced] = p_game->state.removed_1.piece;
                    p_game->state.removed_1.index = NULL_INDEX;
                    p_game->state.removed_1.piece = Empty;
                    p_game->state.status = WhiteToPlay;
                }
                else
                {
                    // The piece moved to a different location
                    LOG_INDEX("-> Piece is placed", indexPlaced);

                    uint8_t diff = abs(p_game->state.removed_1.index - indexPlaced);
                    if ((WKing == p_game->state.removed_1.piece) && (2 == diff))
                    {
                        // White king replaced two cells away on the same row: white is castling (1/3)
                        p_game->lastMoveW = {p_game->state.removed_1.index, indexPlaced, p_game->state.removed_1.piece, false};
                        p_game->board[indexPlaced] = p_game->state.removed_1.piece;
                        p_game->state.removed_1.index = NULL_INDEX;
                        p_game->state.removed_1.piece = Empty;
                        p_game->state.status = WhiteIsCastling;
                    }
                    else
                    {
                        // White has played
                        p_game->board[indexPlaced] = p_game->state.removed_1.piece;
                        p_game->lastMoveW = {p_game->state.removed_1.index, indexPlaced, p_game->state.removed_1.piece, false};
                        p_game->state.removed_1.index = NULL_INDEX;
                        p_game->state.removed_1.piece = Empty;
                        p_game->state.status = BlackToPlay;
                        p_game->moves++;
                        return true;
                    }
                }
            }
            else if (NULL_INDEX != indexRemoved)
            {
                // Second piece has been removed, white is capturing
                LOG_INDEX("-> Second piece is removed", indexRemoved);
                p_game->state.removed_2.index = indexRemoved;
                p_game->state.removed_2.piece = p_game->board[p_game->state.removed_2.index];
                p_game->board[p_game->state.removed_2.index] = Empty;
                p_game->state.status = WhiteIsCapturing;
            }
            break;
        }
        case WhiteIsCapturing:
        {
            if (NULL_INDEX != indexPlaced)
            {
                // The piece has been placed
                if ((indexPlaced != p_game->state.removed_1.index) && (indexPlaced != p_game->state.removed_2.index))
                {
                    LOG_INDEX("Two pieces removed and one piece placed at a different location!", indexPlaced);
                }
                else
                {
                    // The piece has been placed where one was removed, white has played
                    LOG_INDEX("-> White captured", indexPlaced);

                    // Check which piece has been removed first (capturing piece or captured piece)
                    if (true == isWhite(p_game->state.removed_1.piece))
                    {
                        // The capturing piece was removed first
                        p_game->lastMoveW = {p_game->state.removed_1.index, indexPlaced, p_game->state.removed_1.piece, true};
                        p_game->board[indexPlaced] = p_game->state.removed_1.piece;
                    }
                    else if (true == isBlack(p_game->state.removed_1.piece))
                    {
                        // The captured piece was removed first
                        p_game->board[indexPlaced] = p_game->state.removed_2.piece;
                        p_game->lastMoveW = {p_game->state.removed_2.index, indexPlaced, p_game->state.removed_2.piece, false};
                    }

                    p_game->state.removed_1.index = NULL_INDEX;
                    p_game->state.removed_1.piece = Empty;
                    p_game->state.removed_2.index = NULL_INDEX;
                    p_game->state.removed_2.piece = Empty;
                    p_game->state.status = BlackToPlay;
                    p_game->moves++;
                    return true;
                }
            }

            if (NULL_INDEX != indexRemoved)
            {
                LOG_INDEX("-> Additional piece removed during white capture!", indexRemoved);
            }
            break;
        }
        case WhiteIsCastling:
        {
            if (NULL_INDEX != indexRemoved)
            {
                // White is castling (2/3)
                LOG_INDEX("-> Piece is removed", indexRemoved);
                p_game->state.removed_1.index = indexRemoved;
                p_game->state.removed_1.piece = p_game->board[p_game->state.removed_1.index];
                p_game->board[p_game->state.removed_1.index] = Empty;

                if (WRook != p_game->state.removed_1.piece)
                {
                    LOG("-> Second piece removed during castling is not a rook!");
                }
            }

            if (NULL_INDEX != indexPlaced)
            {
                if (NULL_INDEX == p_game->state.removed_1.index)
                {
                    LOG_INDEX("-> Additional piece is placed during white castling!", indexPlaced);
                }
                else
                {
                    // White is castling (3/3)
                    LOG_INDEX("-> Piece is placed", indexPlaced);
                    p_game->board[indexPlaced] = p_game->state.removed_1.piece;
                    // p_game->lastMoveW = {p_game->state.removed_1.index, indexPlaced, p_game->state.removed_1.piece, false};
                    p_game->state.removed_1.index = NULL_INDEX;
                    p_game->state.removed_1.piece = Empty;
                    p_game->state.status = BlackToPlay;
                    p_game->moves++;
                    return true;
                }
            }
            break;
        }

        // ========================= BLACK TURN
        case BlackToPlay:
        {
            if (NULL_INDEX != indexRemoved)
            {
                // Piece has been removed, black is playing
                LOG_INDEX("-> Piece is removed", indexRemoved);
                p_game->state.removed_1.index = indexRemoved;
                p_game->state.removed_1.piece = p_game->board[p_game->state.removed_1.index];
                p_game->board[p_game->state.removed_1.index] = Empty;
                p_game->state.status = BlackIsPlaying;
            }

            if (NULL_INDEX != indexPlaced)
            {
                LOG_INDEX("-> Additional piece placed during black turn!", indexPlaced);
            }
            break;
        }
        case BlackIsPlaying:
        {
            if (NULL_INDEX != indexPlaced)
            {
                // Piece has been placed
                if (p_game->state.removed_1.index == indexPlaced)
                {
                    // A piece has been removed and placed at the same location (undo), still black to play
                    LOG("-> Black canceled its move");
                    p_game->board[indexPlaced] = p_game->state.removed_1.piece;
                    p_game->state.removed_1.index = NULL_INDEX;
                    p_game->state.removed_1.piece = Empty;
                    p_game->state.status = BlackToPlay;
                }
                else
                {
                    // The piece moved to a different location
                    LOG_INDEX("-> Piece is placed", indexPlaced);

                    uint8_t diff = abs(p_game->state.removed_1.index - indexPlaced);
                    if ((BKing == p_game->state.removed_1.piece) && (2 == diff))
                    {
                        // Black king replaced two cells away on the same row: black is castling (1/3)
                        p_game->board[indexPlaced] = p_game->state.removed_1.piece;
                        p_game->lastMoveB = {p_game->state.removed_1.index, indexPlaced, p_game->state.removed_1.piece, false};
                        p_game->state.removed_1.index = NULL_INDEX;
                        p_game->state.removed_1.piece = Empty;
                        p_game->state.status = BlackIsCastling;
                    }
                    else
                    {
                        // Black has played
                        p_game->board[indexPlaced] = p_game->state.removed_1.piece;
                        p_game->lastMoveB = {p_game->state.removed_1.index, indexPlaced, p_game->state.removed_1.piece, false};
                        p_game->state.removed_1.index = NULL_INDEX;
                        p_game->state.removed_1.piece = Empty;
                        p_game->state.status = WhiteToPlay;
                        p_game->moves++;
                        return true;
                    }
                }
            }
            else if (NULL_INDEX != indexRemoved)
            {
                // Second piece has been removed, black is capturing
                LOG_INDEX("-> Second piece is removed", indexRemoved);
                p_game->state.removed_2.index = indexRemoved;
                p_game->state.removed_2.piece = p_game->board[p_game->state.removed_2.index];
                p_game->board[p_game->state.removed_2.index] = Empty;
                p_game->state.status = BlackIsCapturing;
            }
            break;
        }
        case BlackIsCapturing:
        {
            if (NULL_INDEX != indexPlaced)
            {
                // The piece has been placed
                if ((indexPlaced != p_game->state.removed_1.index) && (indexPlaced != p_game->state.removed_2.index))
                {
                    LOG_INDEX("Two pieces removed and one piece placed at a different location!", indexPlaced);
                }
                else
                {
                    // The piece has been placed where one was removed, black has played
                    LOG_INDEX("-> Black captured", indexPlaced);

                    // Check which piece has been removed first (capturing piece or captured piece)
                    if (true == isBlack(p_game->state.removed_1.piece))
                    {
                        // The capturing piece was removed first
                        p_game->lastMoveB = {p_game->state.removed_1.index, indexPlaced, p_game->state.removed_1.piece, true};
                        p_game->board[indexPlaced] = p_game->state.removed_1.piece;
                    }
                    else if (true == isWhite(p_game->state.removed_1.piece))
                    {
                        // The captured piece was removed first
                        p_game->lastMoveB = {p_game->state.removed_2.index, indexPlaced, p_game->state.removed_2.piece, true};
                        p_game->board[indexPlaced] = p_game->state.removed_2.piece;
                    }

                    p_game->state.removed_1.index = NULL_INDEX;
                    p_game->state.removed_1.piece = Empty;
                    p_game->state.removed_2.index = NULL_INDEX;
                    p_game->state.removed_2.piece = Empty;
                    p_game->state.status = WhiteToPlay;
                    p_game->moves++;
                    return true;
                }
            }

            if (NULL_INDEX != indexRemoved)
            {
                LOG_INDEX("-> Additional piece removed during black capture!", indexRemoved);
            }

            break;
        }
        case BlackIsCastling:
        {
            if (NULL_INDEX != indexRemoved)
            {
                // Black is castling (2/3)
                LOG_INDEX("-> Piece is removed", indexRemoved);
                p_game->state.removed_1.index = indexRemoved;
                p_game->state.removed_1.piece = p_game->board[p_game->state.removed_1.index];
                p_game->board[p_game->state.removed_1.index] = Empty;

                if (BRook != p_game->state.removed_1.piece)
                {
                    LOG("-> Second piece removed during castling is not a rook!");
                }
            }

            if (NULL_INDEX != indexPlaced)
            {
                if (NULL_INDEX == p_game->state.removed_1.index)
                {
                    LOG_INDEX("-> Additional piece is placed during black castling!", indexPlaced);
                }
                else
                {
                    // Black is castling (3/3)
                    LOG_INDEX("-> Piece is placed", indexPlaced);
                    p_game->board[indexPlaced] = p_game->state.removed_1.piece;
                    // p_game->lastMoveB = {p_game->state.removed_1.index, indexPlaced, p_game->state.removed_1.piece, false};
                    p_game->state.removed_1.index = NULL_INDEX;
                    p_game->state.removed_1.piece = Empty;
                    p_game->state.status = WhiteToPlay;
                    p_game->moves++;
                    return true;
                }
            }
            break;
        }

        // ========================= DEFAULT
        default:
        {
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

const char* getMoveStr(Move p_move) {
    if (p_move.piece == EPiece::Empty)
        return "-";
    if ((p_move.piece == EPiece::BKing) || (p_move.piece == EPiece::WKing)) {
        if (p_move.start + 2 == p_move.end)
            return "O-O";
        if (p_move.start == 2 + p_move.end)
            return "O-O-O";
    }

    static char msg[7];
    uint8_t i = 0;

    switch (p_move.piece) {
    case WKing:
    case BKing:
        msg[i++] = 'K';
        break;
    case WQueen:
    case BQueen:
        msg[i++] = 'Q';
        break;
    case WBishop:
    case BBishop:
        msg[i++] = 'B';
        break;
    case WKnight:
    case BKnight:
        msg[i++] = 'N';
        break;
    case WRook:
    case BRook:
        msg[i++] = 'R';
        break;
    case WPawn:
    case BPawn:
        msg[i++] = 'a' + (p_move.start % 8);
        break;
    }

    if (p_move.captured)
        msg[i++] = 'x';

    msg[i++] = 'a' + (p_move.end % 8);
    msg[i++] = '1' + (p_move.end / 8);
    msg[i] = 0;
    return msg;
}
