#include "../include/chess.h"

#include <stdio.h>
#include <stdlib.h>

const uint8_t NULL_INDEX = 64;

//-----------------------------------------------------------------------------
void initializeGame(Game* p_game)
//-----------------------------------------------------------------------------
{
    if (NULL == p_game)
    {
        printf("Unable to initialize null game\n");
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

    // Game state
    p_game->state.status = WhiteToPlay;
    p_game->state.removed_1.index = NULL_INDEX;
    p_game->state.removed_1.piece = Empty;
    p_game->state.removed_2.index = NULL_INDEX;
    p_game->state.removed_2.piece = Empty;
    p_game->moves = 0;
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
    switch (p_status)
    {
        case WhiteToPlay:
            return "White to play";
        case WhiteIsPlaying:
            return "White is playing";
        case WhiteIsCapturing:
            return "White is capturing";
        case WhiteIsCastling:
            return "White is castling";
        case BlackToPlay:
            return "Black to play";
        case BlackIsPlaying:
            return "Black is playing";
        case BlackIsCapturing:
            return "Black is capturing";
        case BlackIsCastling:
            return "Black is castling";
        case Draw:
            return "Draw";
        case WhiteWon:
            return "White won";
        case BlackWon:
            return "Black won";

        default:
            return "Undefined status";
    };
}

//-----------------------------------------------------------------------------
void printGame(Game* p_game)
//-----------------------------------------------------------------------------
{
    if (NULL == p_game)
    {
        printf("Unable to print null game\n");
        return;
    }

    printf("  +----+----+----+----+----+----+----+----+\n");
    for(uint8_t i = 0; i < 8; i++)
    {
        printf("%d ", (8-i));
        for(uint8_t j = 0; j < 8; j++)
        {
            printf("| %s ", getPieceStr(p_game->board[8*(8-i-1)+j]));
        }

        printf("|\n  +----+----+----+----+----+----+----+----+\n");
    }

    printf("   a    b    c    d    e    f    g    h\n");
    printf("[%s, %d move%s]\n", getStatusStr(p_game->state.status), p_game->moves, (p_game->moves > 1 ? "s" : ""));
}

//-----------------------------------------------------------------------------
bool evolveGame(Game* p_game, uint64_t p_sensors)
//-----------------------------------------------------------------------------
{
    if (NULL == p_game)
    {
        printf("Unable to evolve null game\n");
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
        printf("-> no change\n");
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
                printf("-> Piece is removed (index %d)\n", indexRemoved);
                p_game->state.removed_1.index = indexRemoved;
                p_game->state.removed_1.piece = p_game->board[p_game->state.removed_1.index];
                p_game->board[p_game->state.removed_1.index] = Empty;
                p_game->state.status = WhiteIsPlaying;
            }

            if (NULL_INDEX != indexPlaced)
            {
                printf("-> Additional piece placed during white turn (index %d)!\n", indexPlaced);
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
                    printf("-> White canceled its move\n");
                    p_game->board[indexPlaced] = p_game->state.removed_1.piece;
                    p_game->state.removed_1.index = NULL_INDEX;
                    p_game->state.removed_1.piece = Empty;
                    p_game->state.status = WhiteToPlay;
                }
                else
                {
                    // The piece moved to a different location
                    printf("-> Piece is placed (index %d)\n", indexPlaced);

                    uint8_t diff = abs(p_game->state.removed_1.index - indexPlaced);
                    if ((WKing == p_game->state.removed_1.piece) && (2 == diff))
                    {
                        // White king replaced two cells away on the same row: white is castling (1/3)
                        p_game->board[indexPlaced] = p_game->state.removed_1.piece;
                        p_game->state.removed_1.index = NULL_INDEX;
                        p_game->state.removed_1.piece = Empty;
                        p_game->state.status = WhiteIsCastling;
                    }
                    else
                    {
                        // White has played
                        p_game->board[indexPlaced] = p_game->state.removed_1.piece;
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
                printf("-> Second piece is removed (index %d)\n", indexRemoved);
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
                    printf("Two pieces removed and one piece placed at a different location (index %d)!\n", indexPlaced);
                }
                else
                {
                    // The piece has been placed where one was removed, white has played
                    printf("-> White captured (index %d)\n", indexPlaced);

                    // Check which piece has been removed first (capturing piece or captured piece)
                    if (true == isWhite(p_game->state.removed_1.piece))
                    {
                        // The capturing piece was removed first
                        p_game->board[indexPlaced] = p_game->state.removed_1.piece;
                    }
                    else if (true == isBlack(p_game->state.removed_1.piece))
                    {
                        // The captured piece was removed first
                        p_game->board[indexPlaced] = p_game->state.removed_2.piece;
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
                printf("-> Additional piece removed during white capture (index %d)!\n", indexRemoved);
            }
            break;
        }
        case WhiteIsCastling:
        {
            if (NULL_INDEX != indexRemoved)
            {
                // White is castling (2/3)
                printf("-> Piece is removed (index %d)\n", indexRemoved);
                p_game->state.removed_1.index = indexRemoved;
                p_game->state.removed_1.piece = p_game->board[p_game->state.removed_1.index];
                p_game->board[p_game->state.removed_1.index] = Empty;

                if (WRook != p_game->state.removed_1.piece)
                {
                    printf("-> Second piece removed during castling is not a rook!\n");
                }
            }

            if (NULL_INDEX != indexPlaced)
            {
                if (NULL_INDEX == p_game->state.removed_1.index)
                {
                    printf("-> Additional piece is placed during white castling (index %d)!\n", indexPlaced);
                }
                else
                {
                    // White is castling (3/3)
                    printf("-> Piece is placed (index %d)\n", indexPlaced);
                    p_game->board[indexPlaced] = p_game->state.removed_1.piece;
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
                printf("-> Piece is removed (index %d)\n", indexRemoved);
                p_game->state.removed_1.index = indexRemoved;
                p_game->state.removed_1.piece = p_game->board[p_game->state.removed_1.index];
                p_game->board[p_game->state.removed_1.index] = Empty;
                p_game->state.status = BlackIsPlaying;
            }

            if (NULL_INDEX != indexPlaced)
            {
                printf("-> Additional piece placed during black turn (index %d)!\n", indexPlaced);
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
                    printf("-> Black canceled its move\n");
                    p_game->board[indexPlaced] = p_game->state.removed_1.piece;
                    p_game->state.removed_1.index = NULL_INDEX;
                    p_game->state.removed_1.piece = Empty;
                    p_game->state.status = BlackToPlay;
                }
                else
                {
                    // The piece moved to a different location
                    printf("-> Piece is placed (index %d)\n", indexPlaced);

                    uint8_t diff = abs(p_game->state.removed_1.index - indexPlaced);
                    if ((BKing == p_game->state.removed_1.piece) && (2 == diff))
                    {
                        // Black king replaced two cells away on the same row: black is castling (1/3)
                        p_game->board[indexPlaced] = p_game->state.removed_1.piece;
                        p_game->state.removed_1.index = NULL_INDEX;
                        p_game->state.removed_1.piece = Empty;
                        p_game->state.status = BlackIsCastling;
                    }
                    else
                    {
                        // Black has played
                        p_game->board[indexPlaced] = p_game->state.removed_1.piece;
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
                printf("-> Second piece is removed (index %d)\n", indexRemoved);
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
                    printf("Two pieces removed and one piece placed at a different location (index %d)!\n", indexPlaced);
                }
                else
                {
                    // The piece has been placed where one was removed, black has played
                    printf("-> Black captured (index %d)\n", indexPlaced);

                    // Check which piece has been removed first (capturing piece or captured piece)
                    if (true == isBlack(p_game->state.removed_1.piece))
                    {
                        // The capturing piece was removed first
                        p_game->board[indexPlaced] = p_game->state.removed_1.piece;
                    }
                    else if (true == isWhite(p_game->state.removed_1.piece))
                    {
                        // The captured piece was removed first
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
                printf("-> Additional piece removed during black capture (index %d)!\n", indexRemoved);
            }

            break;
        }
        case BlackIsCastling:
        {
            if (NULL_INDEX != indexRemoved)
            {
                // Black is castling (2/3)
                printf("-> Piece is removed (index %d)\n", indexRemoved);
                p_game->state.removed_1.index = indexRemoved;
                p_game->state.removed_1.piece = p_game->board[p_game->state.removed_1.index];
                p_game->board[p_game->state.removed_1.index] = Empty;

                if (BRook != p_game->state.removed_1.piece)
                {
                    printf("-> Second piece removed during castling is not a rook!\n");
                }
            }

            if (NULL_INDEX != indexPlaced)
            {
                if (NULL_INDEX == p_game->state.removed_1.index)
                {
                    printf("-> Additional piece is placed during black castling (index %d)!\n", indexPlaced);
                }
                else
                {
                    // Black is castling (3/3)
                    printf("-> Piece is placed (index %d)\n", indexPlaced);
                    p_game->board[indexPlaced] = p_game->state.removed_1.piece;
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
            printf("-> Unhandled game status: %d\n", p_game->state.status);
        }
    }

    return false;
}