/******************************************************************************
 * This is a port of the chess engine "bistromath" to the Pebbles kernel.
 * Both the original and the port are copyright Ben Blum <bblum@andrew.cmu.edu>
 * 2013, all rights reserved.
 *
 * Readers interested in the inner workings of the chess engine will be much
 * happier navigating the original codebase, where the files are not all
 * concatenated together.
 *
 *              ====> http://bblum.net/bistromath.tar.bz2 <====
 *
 * The pebbles glue / console interface (which are obviously not present in the
 * original version) are at the bottom of this file, around line 7050.
 *
 * This program incidentally tests for the following Pebbles functionalities:
 * - Loading very large ELFs (.bss comes out to be about 192MB)
 * - Stack growth (each ply searched takes a little over 32KB of stack)
 * - Simple threading -- another thread is used for the search timer
 *
 * Please direct all questions/bugs to Ben Blum <bblum@andrew.cmu.edu>.
 *
 ******************************************************************************
 *
 * This code is licensed under the GNU Library GPL:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 ******************************************************************************/

#include <syscall.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

#include "rand.h"
#include "thread.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) ((sizeof(a))/(sizeof(a[0])))
#endif

/****************************************************************************
 * attacks.h - precomputed attack-/move-mask tables
 * copyright (C) 2008 Ben Blum
 ****************************************************************************/
#ifndef ATTACKS_H
#define ATTACKS_H

#define U64(x) ((uint64_t)(x ## ull))
#define U32(x) ((uint32_t)(x))

extern uint64_t knightattacks[64];
extern uint64_t kingattacks[64];
extern uint64_t pawnattacks[2][64];
extern uint64_t pawnmoves[2][64];

extern uint64_t rowattacks[256][8];
extern uint64_t colattacks[256][8];

extern int rot45diagindex[64];
extern int rot315diagindex[64];

extern int rot45index_shiftamountright[15];
extern int rot315index_shiftamountright[15];
extern int rot315index_shiftamountleft[15];

extern int rotresult_shiftamountleft[15];
extern int rotresult_shiftamountright[15];

uint64_t rot45attacks[256][8];
uint64_t rot315attacks[256][8];

#endif
/****************************************************************************
 * bitscan.h - useful for switching bitscan implementations
 * copyright (C) 2008 Ben Blum
 ****************************************************************************/
#ifndef BITSCAN_H
#define BITSCAN_H

#define BITSCAN(x) (ffsll(x)-1)

#endif

// Below are ffs.c and ffsll.c from a standard glibc distribution, since 410user
// doesn't have them.

/* Copyright (C) 1991, 1992, 1997, 1998, 2004 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Torbjorn Granlund (tege@sics.se).

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

#define ffs(x) __ffs(x)

/* Find the first bit set in I.  */
int
__ffs (i)
     int i;
{
  static const unsigned char table[] =
    {
      0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
      6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
      7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
      7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
      8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
      8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
      8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
      8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8
    };
  unsigned int a;
  unsigned int x = i & -i;

  a = x <= 0xffff ? (x <= 0xff ? 0 : 8) : (x <= 0xffffff ?  16 : 24);

  return table[x >> a] + a;
}
/* Copyright (C) 1991, 1992, 1997, 1998 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Torbjorn Granlund (tege@sics.se).

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

/* Find the first bit set in I.  */
int
ffsll (i)
     long long int i;
{
  unsigned long long int x = i & -i;

  if (x <= 0xffffffff)
    return ffs (i);
  else
    return 32 + ffs (i >> 32);
}

/****************************************************************************
 * popcnt.h - various population-count implementations
 * copyright (C) 2008 Ben Blum
 ****************************************************************************/
#ifndef POPCNT_H
#define POPCNT_H

/* Can be used to change which implementation we use */
//#define POPCOUNT(x) popcnt3(x)
#define POPCOUNT(x) popcnt2(x)

/**
 * Returns a count of how many bits are set in the argument.
 */
int popcnt(uint64_t);

/**
 * Alternative implementations
 */
int popcnt2(uint64_t);
int popcnt3(uint64_t);

#endif
/****************************************************************************
 * rand.h - wrappers for random number generation utilities
 * copyright (C) 2008 Ben Blum
 ****************************************************************************/

/* Set up the random number generator */
void rand_init();
/* A "random" 32-bit integer as provided by GSL's mersenne twister */
uint32_t rand32();
/* A "random" 64-bit integer as provided by GSL's mersenne twister */
uint64_t rand64();

/****************************************************************************
 * movelist.h - data structure for move sorting in move generation/iteration
 * copyright (C) 2008 Ben Blum
 * ****************************************************************************/
#ifndef MOVELIST_H
#define MOVELIST_H


#define MOVELIST_NUM_BUCKETS 64
#define MOVELIST_SUBLIST_LENGTH 128

/****************************************************************************
 * Our move ordering scheme will look something like this:
 *
 * 00-08: Moves that lose material
 * 	00: -9 (Q hang)
 * 	01: -8 (Qx protected P)
 * 	02: -7 (None)
 * 	03: -6 (Qx protected B/N)
 * 	04: -5 (R hang)
 * 	05: -4 (Qx protected R)
 * 	06: -3 (B/N hang)
 * 	07: -2 (Bx/Nx protected P, Rx protected B/N)
 * 	08: -1 (P hang)
 * 9-41: Regular moves, indexed by piecetable[dest] - piecetable[src] + 25
 * 42: Minor promotions
 * 43: O-O-O
 * 44: O-O
 * 45-49: Neutral captures
 * 	45: PxP
 * 	46: NxN
 * 	47: BxB
 * 	48: RxR
 * 	49: QxQ
 * 50-58: Moves that win material
 * 	50: +1 (P unhang, *x hung P)
 * 	51: +2 (Px protected B/N, Bx/Nx protected R)
 * 	52: +3 (B/N unhang, *x hung B/N)
 * 	53: +4 (Px protected R)
 * 	54: +5 (R unhang, *x hung B/N)
 * 	55: +6 (Bx/Nx protected Q)
 * 	56: +7 (None)
 * 	57: +8 (Px protected Q)
 * 	58: +9 (Q unhang, *x hung Q)
 * 59: Promotions to queen
 * 60: Promotions to queen with capture
 * 61: K unhang
 ****************************************************************************/

/* starting index for moves that lose material - subtract how much is lost */
#define MOVELIST_INDEX_MAT_LOSS    9
/* regular move - add the piece/square table difference */
#define MOVELIST_INDEX_REGULAR    25
/* minor promotion - the thought behind this is if the Q promotion is wrong,
 * probably this will be too, so try other moves first */
#define MOVELIST_INDEX_PROM_MINOR 42
/* castle - add 1 if kingside */
#define MOVELIST_INDEX_CASTLE     43
/* neutral capture - add the piece number (P=0 ... Q=4) */
#define MOVELIST_INDEX_NEUTRAL    45
/* moves that win material - add how much is gained */
#define MOVELIST_INDEX_MAT_GAIN   49
/* promotion to queen - add one if it's also a capture */
#define MOVELIST_INDEX_PROM_QUEEN 59

/**
 * The movelist_t data structure is a generalization of maintaining multiple
 * linkedlists and appending them at the end for O(n) pseudo-sorting - it
 * allows an arbitrary number of lists for sorting into that many buckets,
 * with the only constraints being 1) space and 2) time to move the "max"
 * index to point to the next bucket when one bucket is empty. The result is
 * O(n) sorting during creation (beating O(n^2 logn)), but an additional O(m)
 * coefficient for both space and time complexity, so choose a small m!
 */
typedef struct {
	uint32_t array[MOVELIST_NUM_BUCKETS][MOVELIST_SUBLIST_LENGTH];
	int sublist_count[MOVELIST_NUM_BUCKETS];
	/* highest index of a bucket which contains elements; note, if
	 * array[max] is empty, that means the whole movelist is empty */
	unsigned long max;
} movelist_inner_t;

typedef movelist_inner_t *movelist_t;

/* Movelist's max should be zero, each array should be zero, and each
* array should have a count of zero. */
static inline void movelist_init(movelist_t *ml)
{
	*ml = malloc(sizeof(movelist_inner_t));
	assert(*ml && "Couldn't allocate memory to generate moves");
	memset((*ml)->sublist_count, 0, MOVELIST_NUM_BUCKETS*sizeof(int));
	(*ml)->max = 0;
}

#define movelist_destroy(mlp) do { free(*mlp); } while (0)

void movelist_add(movelist_t *, uint64_t[2], uint32_t);
void movelist_addtohead(movelist_t *, uint32_t, unsigned long);
int movelist_isempty(movelist_t *);
uint32_t movelist_remove_max(movelist_t *);

#endif

/****************************************************************************
 * board.h - rotated bitboard library; move generation and board state
 * copyright (C) 2008 Ben Blum
 ****************************************************************************/
#ifndef BOARD_H
#define BOARD_H

/**
 * Piece/color constants. Used as indices to get the position bitboard of a
 * given piece of a given color, among other things.
 */
#define WHITE	0
#define BLACK	1

/* The color not thise one. Equivalent to "x == WHITE ? BLACK : WHITE" */
#define OTHERCOLOR(x) (1^(x))
/* This player's home-rank (0 for white, 7 for black) */
#define HOMEROW(x) ((x == WHITE) ? 0 : 7)

/* Piece types */
#define PAWN	0x0
#define KNIGHT	0x1
#define BISHOP	0x2
#define ROOK	0x3
#define QUEEN	0x4
#define KING	0x5
#define PIECE_SLIDES(p) (((p) > KNIGHT) && ((p) < KING))
typedef uint8_t piece_t;

extern char *piecename[2][6];

/**
 * Square constants. 6 lowest bits: |0|0|r|o|w|c|o|l|
 * These can serve as indexes to get bits out of a bitboard. Also used to get
 * masks for sliding pieces by row/etc
 */
#define A1 0x00
#define B1 0x01
#define C1 0x02
#define D1 0x03
#define E1 0x04
#define F1 0x05
#define G1 0x06
#define H1 0x07
#define A2 0x08
#define B2 0x09
#define C2 0x0a
#define D2 0x0b
#define E2 0x0c
#define F2 0x0d
#define G2 0x0e
#define H2 0x0f
#define A3 0x10
#define B3 0x11
#define C3 0x12
#define D3 0x13
#define E3 0x14
#define F3 0x15
#define G3 0x16
#define H3 0x17
#define A4 0x18
#define B4 0x19
#define C4 0x1a
#define D4 0x1b
#define E4 0x1c
#define F4 0x1d
#define G4 0x1e
#define H4 0x1f
#define A5 0x20
#define B5 0x21
#define C5 0x22
#define D5 0x23
#define E5 0x24
#define F5 0x25
#define G5 0x26
#define H5 0x27
#define A6 0x28
#define B6 0x29
#define C6 0x2a
#define D6 0x2b
#define E6 0x2c
#define F6 0x2d
#define G6 0x2e
#define H6 0x2f
#define A7 0x30
#define B7 0x31
#define C7 0x32
#define D7 0x33
#define E7 0x34
#define F7 0x35
#define G7 0x36
#define H7 0x37
#define A8 0x38
#define B8 0x39
#define C8 0x3a
#define D8 0x3b
#define E8 0x3c
#define F8 0x3d
#define G8 0x3e
#define H8 0x3f
typedef uint8_t square_t;

/* Converts int to string. Better than using malloc for 3-length strings. */
extern char *squarename[64];

#define RANK_1 0
#define RANK_2 1
#define RANK_3 2
#define RANK_4 3
#define RANK_5 4
#define RANK_6 5
#define RANK_7 6
#define RANK_8 7
#define COL_A 0
#define COL_B 1
#define COL_C 2
#define COL_D 3
#define COL_E 4
#define COL_F 5
#define COL_G 6
#define COL_H 7

/* converting square <--> (column,row) */
#define COL(s)      ((s) & 0x7)          /* 0<=s<64 */
#define ROW(s)      ((s) >> 3)           /* 0<=s<64 */
#define SQUARE(c,r) ((square_t)((c) | ((r) << 3)))   /* 0<=c,r<8 */
/* what color is the square - 1 or 0, not necessarily BLACK or WHITE */
#define PARITY(s)   ((COL(s) + ROW(s)) & 0x1)

/* Used mostly as array indices, especially for castling */
#define QUEENSIDE 0
#define KINGSIDE  1
/* Given the side of the board, which column will the king end up on */
#define CASTLE_DEST_COL(x) (((x) == QUEENSIDE) ? COL_C : COL_G)

/* Constants for board_mated return values */
#define BOARD_CHECKMATED 1
#define BOARD_STALEMATED 2

/**
 * move_t will contain a src square (lowest 6 bits), a dest square (next 6
 * bits), and several flags - is check, isenpassant, iscapture (and what
 * piece type is captured), ispromotion (and what is promoted to), and
 * also which color made the move.
 * 000000  JJJ  III  HHH  G  F  E  D  C  BBBBBB AAAAAA
 *      26   23   20   17 16 15 14 13 12      6      0
 * A: Source square
 * B: Dest square
 * C: Color (which side is making this move)
 * D: Castle
 * E: En passant
 * F: Capture
 * G: Promotion
 * H: Captured piece type (color will be the opposite of C)
 * I: Promotion result piece type (color indicated by C)
 * J: Which piece type is moving (color indicated by C)
 * 0: Unused bits
 * Note: Bits 25 and up are used for the repetition count in the transposition
 * table; see transposition.{h,c}
 */
typedef uint32_t move_t;
#define MOV_INDEX_SRC     0
#define MOV_INDEX_DEST    6
#define MOV_INDEX_COLOR  12
#define MOV_INDEX_CASTLE 13
#define MOV_INDEX_EP     14
#define MOV_INDEX_CAPT   15
#define MOV_INDEX_PROM   16
#define MOV_INDEX_CAPTPC 17
#define MOV_INDEX_PROMPC 20
#define MOV_INDEX_PIECE  23
#define MOV_INDEX_UNUSED 26

#define MOV_SRC(m)    ((square_t)(((m) >> MOV_INDEX_SRC) & 0x3f))
#define MOV_DEST(m)   ((square_t)(((m) >> MOV_INDEX_DEST) & 0x3f))
#define MOV_COLOR(m)  (((m) >> MOV_INDEX_COLOR) & 0x1)
#define MOV_CASTLE(m) (((m) >> MOV_INDEX_CASTLE) & 0x1)
#define MOV_EP(m)     (((m) >> MOV_INDEX_EP) & 0x1)
#define MOV_CAPT(m)   (((m) >> MOV_INDEX_CAPT) & 0x1)
#define MOV_PROM(m)   (((m) >> MOV_INDEX_PROM) & 0x1)
#define MOV_CAPTPC(m) ((piece_t)(((m) >> MOV_INDEX_CAPTPC) & 0x7))
#define MOV_PROMPC(m) ((piece_t)(((m) >> MOV_INDEX_PROMPC) & 0x7))
#define MOV_PIECE(m)  ((piece_t)(((m) >> MOV_INDEX_PIECE) & 0x7))

/**
 * 64-bit representation of a board's state for one set of piece, also used
 * for representing attacks, regions of the board, ...
 */
typedef uint64_t bitboard_t;

/**
 * Zobrist hash key
 */
typedef uint64_t zobrist_t;

/* Upper bound on how many halfmoves a game should take. Memory is cheap. */
#define HISTORY_STACK_SIZE 2048

/**
 * Nonrecomputable state stored on the history stack when a move is made
 */
typedef struct {
	zobrist_t hash;
	bitboard_t attackedby[2];
	unsigned char castle[2][2];
	square_t ep;
	unsigned char halfmoves;
	unsigned char reps;
	move_t move;
} history_t;

/****************************************************************************
 * Representation of an entire board state.
 * The pos[][] array represents standard normal-oriented setups, accessed by
 * piece and color. Normally oriented bitboards look like this:
 * 
 * (64-bit format by square)
 * H8 G8 F8 E8 D8 C8 B8 A8 H7 G7 ... A2 H1 G1 F1 E1 D1 C1 B1 A1
 * 63 62 61 60 59 58 57 56 55 54 ...  8  7  6  5  4  3  2  1  0
 *
 * (8x8 format by index (from the least significant bit))
 *    -------------------------
 * 8 | 56 57 58 59 60 61 62 63 |
 * 7 |               ... 54 55 |
 *   |           ...           |
 * 3 | 16 17 ...               |
 * 2 |  8  9 10 11 12 13 14 15 |
 * 1 |  0  1  2  3  4  5  6  7 |
 *    -------------------------
 *      a  b  c  d  e  f  g  h
 * 
 * The n-th index row (0 for 1, 7 for 8) can be extracted by
 * "(bitboard >> (8*n)) & 0xFF". The rotated boards are slightly different.
 * Rot90, used for rook attacks, looks like this:
 * 
 * H8 H7 H6 H5 H4 H3 H2 H1 G8 G7 ... B1 A8 A7 A6 A5 A4 A3 A2 A1
 * 63 62 61 60 59 58 57 56 55 54 ...  8  7  6  5  4  3  2  1  0
 * 
 *    -------------------------
 * h | 56 57 58 59 60 61 62 63 |
 * g |               ... 54 55 |
 *   |           ...           |
 * c | 16 17 ...               |
 * b |  8  9 10 11 12 13 14 15 |
 * a |  0  1  2  3  4  5  6  7 |
 *    -------------------------
 *      1  2  3  4  5  6  7  8
 * 
 * So the n-th index file (0 for a, 7 for h) can be obtained easily. Note that
 * "rot90" is a lie, it's actually flipped across the a1-h8 diagonal. For
 * bishops, we rotate so squares on a single diagonal are adjacent.
 *                rot45:                                rot315:
 *                 ,--,                                8 ,--,
 *               ,' 63 ',                            7 ,' 63 ',
 *             ,' 61  62 ',                        6 ,' 61  62 ',
 *           ,' 58  59  60 ',                    5 ,' 58  59  60 ',
 *         ,' 54  55  56  57 ',                4 ,' 54  55  56  57 ',
 *       ,' 49  50  51  52  53 ',            3 ,' 49  50  51  52  53 ',
 *     ,' 43  44  45  46  47  48 ',        2 ,' 43  44  45  46  47  48 ',
 *   ,' 36  37  38  39  40  41  42 ',    1 ,' 36  37  38  39  40  41  42 ',
 *  ( 28  29  30  31  32  33  34  35 )    ( 28  29  30  31  32  33  34  35 )
 * 8 ', 21  22  23  24  25  26  27 ,' h  a ', 21  22  23  24  25  26  27 ,'
 *   7 ', 15  16  17  18  19  20 ,' g      b ', 15  16  17  18  19  20 ,'
 *     6 ', 10  11  12  13  14 ,' f          c ', 10  11  12  13  14 ,'
 *       5 ', 06  07  08  09 ,' e              d ', 06  07  08  09 ,'
 *         4 ', 03  04  05 ,' d                  e ', 03  04  05 ,'
 *           3 ', 01  02 ,' c                      f ', 01  02 ,'
 *             2 ', 00 ,' b                          g ', 00 ,'
 *               1 '--' a                              h '--'
 * Diagonals are "numbered" 0 to 14 (15 in total); in each diagram the diag
 * on the bottom will be #0, the major diagonal will be #7, and the top #14.
 * See precomputed.h for values and macros relating to this implementation.
 ****************************************************************************/
typedef struct {
	/* These arrays contain basic information about where the pieces are
	 * on the board. Used generally for masking with attack bitboards. */
	bitboard_t pos[2][6];        /* Positions of each PIECE of COLOR */
	bitboard_t piecesofcolor[2]; /* All squares occupied by COLOR */
	bitboard_t attackedby[2];    /* All squares attacked by COLOR */
	/* Maintain the occupancy status of every square on the board in four
	 * rotated boards, for easy generation of sliding attacks. */
	bitboard_t occupied;
	bitboard_t occupied90;
	bitboard_t occupied45;
	bitboard_t occupied315;
	/* The zobrist hash key for the current position */
	zobrist_t hash;
	/* Stores nonrecomputable state for undo. Index into with ->moves. */
	history_t history[HISTORY_STACK_SIZE];
	/* Various flags relating to the current position. Note for the ep
	 * flag that the capture square will always be on the 3rd (2-index)
	 * row, or the 6th (5-index) row. A value of zero means no enpassant
	 * is possible in this position. */
	square_t ep;                 /* The square on which an ep capture
	                              * could occur */
	unsigned char castle[2][2];  /* Can {WHITE,BLACK} castle {KINGSIDE,
	                              * QUEENSIDE} */
	unsigned char hascastled[2]; /* has {WHITE,BLACK} castled yet */
	unsigned char tomove;        /* Who has the move (WHITE, BLACK) */
	unsigned char halfmoves;     /* The halfmove clock. A draw occurs when
	                              * this hits 100. */
	unsigned int moves;          /* The game move clock. This also counts
	                              * halfmoves, but is never reset. */
	unsigned char reps;          /* how many times has this position
	                              * appeared before? if 2, draw */
	int16_t material[2];         /* keeps track of material; see eval.c */
} board_t;

/****************************************************************************
 * Some special macros and precomputed bitboards
 ****************************************************************************/
#define BB(b) ((bitboard_t)(b ## ull))
#define BB_FILEA BB(0x0101010101010101)
#define BB_FILEB BB(0x0202020202020202)
#define BB_FILEC BB(0x0404040404040404)
#define BB_FILED BB(0x0808080808080808)
#define BB_FILEE BB(0x1010101010101010)
#define BB_FILEF BB(0x2020202020202020)
#define BB_FILEG BB(0x4040404040404040)
#define BB_FILEH BB(0x8080808080808080)
#define BB_FILE(c) (BB_FILEA << (c))
#define BB_RANK1 BB(0x00000000000000ff)
#define BB_RANK2 BB(0x000000000000ff00)
#define BB_RANK3 BB(0x0000000000ff0000)
#define BB_RANK4 BB(0x00000000ff000000)
#define BB_RANK5 BB(0x000000ff00000000)
#define BB_RANK6 BB(0x0000ff0000000000)
#define BB_RANK7 BB(0x00ff000000000000)
#define BB_RANK8 BB(0xff00000000000000)
#define BB_RANK(r) (BB_RANK1 << (8 * (r)))
#define BB_HALF_RANKS(s) (((s) == WHITE) ? BB(0x00000000ffffffff) : BB(0xffffffff00000000))
#define BB_HALF_FILES(s) (((s) == QUEENSIDE) ? BB(0x0f0f0f0f0f0f0f0f) : BB(0xf0f0f0f0f0f0f0f0))


/* pawnstructure needs these */
extern bitboard_t bb_adjacentcols[8];
extern bitboard_t bb_passedpawnmask[2][64];

/* BB_SQUARE: Get a bitboard with only one bit set, on a specified square
 * Bb_ALLEXCEPT: The bitwise negation of BB_SQUARE(x) */
//#define BB_SHIFTFLIP /* prefer shift/flip to cached memory lookups */
#ifdef BB_SHIFTFLIP
	#define BB_SQUARE(x)    BB((BB(0x1))<<(x))
	#define BB_ALLEXCEPT(x) BB(~(BB_SQUARE(x)))
#else
	#define BB_SQUARE(x)    bb_square[x]
	#define BB_ALLEXCEPT(x) bb_allexcept[x]
	extern bitboard_t bb_square[64];
	extern bitboard_t bb_allexcept[64];
#endif

/* If you are {white,black}, you ep-capture with a pawn on rank {5,4} */
#define BB_EP_FROMRANK(c) (((c) == WHITE) ? BB_RANK5 : BB_RANK4)
/* If you are {white,black}, your pawn will be ep-captured on {3,6} */
#define BB_EP_TORANK(c) (((c) == WHITE) ? BB_RANK3 : BB_RANK6)

/****************************************************************************
 * Function declarations
 ****************************************************************************/
void init_zobrist();
void zobrist_gen(board_t *);

board_t *board_init();
void board_destroy(board_t *);
char *board_fen(board_t *);
piece_t board_pieceatsquare(board_t *, square_t, unsigned char *);
int board_incheck(board_t *);
int board_colorincheck(board_t *, unsigned char);
int board_mated(board_t *);
int board_pawnpassed(board_t *, square_t, unsigned char);
int board_squareisattacked(board_t *, square_t, unsigned char);
int board_squaresareattacked(board_t *, bitboard_t, unsigned char);
bitboard_t board_attacksfrom(board_t *, square_t, piece_t, unsigned char);
bitboard_t board_pawnpushesfrom(board_t *, square_t, unsigned char);
void board_generatemoves(board_t *, movelist_t *);
void board_generatecaptures(board_t *, movelist_t *);
void board_applymove(board_t *, move_t);
void board_undomove(board_t *, move_t);
bitboard_t board_pawnattacks(bitboard_t pawns, unsigned char);
int board_threefold_draw(board_t *);

move_t move_fromstring(char *);
move_t move_islegal(board_t *, char *, char **);
char *move_tostring(move_t);

#endif
/****************************************************************************
 * eval.h - evaluation function to tell how good a position is
 * copyright (C) 2008 Ben Blum
 ****************************************************************************/
#ifndef EVAL_H
#define EVAL_H

#ifndef EVAL_LAZY_THRESHHOLD
#define EVAL_LAZY_THRESHHOLD 250
#endif

/**
 * Is this board an endgame position
 */
int eval_isendgame(board_t *);

/**
 * Evaluation function. Given a board, returns an int value representing how
 * good this position is for whoever has the move. A positive score means the
 * player to move has the advantage; a negative score means the player to move
 * has the disadvantage; a zero score means both sides have equal chances.
 */
int16_t eval(board_t *);

/**
 * Lazy eval - just the material difference
 */
int16_t eval_lazy(board_t *);

#endif
/****************************************************************************
 * pawnstructure.h - evaluation helpers to judge pawn structure quality
 * copyright (C) 2008 Ben Blum
 * ****************************************************************************/
#ifndef PAWNSTRUCTURE_H
#define PAWNSTRUCTURE_H

/**
 * Evaluate the pawn structure given the position bitboard for the pawns. We
 * don't want to consider anything besides the position of the pawns relative
 * to each other - this just assigns rewards and penalties for pawn-related
 * issues such as doubled pawns, isolated pawns, etcetera.
 * Pass in the pawn position bitboard and the color to whom these pwans belong
 */
int16_t eval_pawnstructure(board_t *, unsigned char, bitboard_t *);

#endif
/****************************************************************************
 * quiescent.h - captures-only search to avoid evaluating unstable position
 * copyright (C) 2008 Ben Blum
 ****************************************************************************/
#ifndef QUIESCENT_H
#define QUIESCENT_H

int16_t quiesce(board_t *board, int16_t alpha, int16_t beta);

#endif
/****************************************************************************
 * transposition.h - avoid re-searching already searched positions
 * copyright (C) 2008 Ben Blum
 ****************************************************************************/
#ifndef TRANSPOSITION_H
#define TRANSPOSITION_H

/**
 * Transposition table - keyed by zobrist hash, stores info about a node. We
 * keep track of the following data:
 * 1) The best move at this position (possibly 0, if a fail-low occurred)
 * 2) The evaluated (or approximate) value of the position
 * 3) The depth this node was searched to, or -1 if this was {check,stale}mate
 *    Note we use -1 (as an unsigned depth value) so that we will always use
 *    transposition information for terminal nodes rather than wasting time
 *    figuring out how big the movelist is.
 * 4) How far into the game (not search tree) this position is. This data lets
 *    us always evict nodes beyond which the state of the game has gone. Note,
 * 5) What type of values these are:
 *         Exact value
 *         Beta cutoff - failed high (storedval is lower bound)
 *         Alpha cutoff - failed low (storedval is upper bound)
 * 
 * Note: The "depth" should not be simply the depth to which this node was
 * searched. It should be instead that depth plus how far into the game we are
 * (i.e., the game clock). This will help us replace nodes that are behind our
 * search tree entirely - otherwise, the table will fill with high-depth nodes
 * that we'll never use because they were back in the beginning of the game.
 * When comparing for replacement, use the gamedepth of the root node (the
 * position currently on the board), but when adding, use the gamedepth of
 * the current node (the position a few moves in the future). Because of
 * this the searcher needs to pass both values to trans_add().
 * Definitions: "search depth" refers to how deep this node will be searched;
 * "game depth" refer's to THIS NODE's depth in the whole game tree; "root
 * depth" refers to how deep the position the searcher started at is.
 */
typedef struct trans_data_t {
	/* gcc -O3 optimizes all this into a register;
	 * note, the element ordering is critical */
	uint8_t  flags;
	uint8_t  gamedepth;
	int16_t value;
	uint32_t move;
} trans_data_t;
/* The 'flags' field stores the alpha/beta/exact flags in two bits and the
 * searched-depth of the node
 * BBBBBB AA
 *      2  0
 * A: Alpha/beta/exact
 * B: searchdepth
 * If we search deeper than 63 plies then we're in trouble.
 * */
#define TRANS_FLAG_EXACT 0x2
#define TRANS_FLAG_BETA  0x1
#define TRANS_FLAG_ALPHA 0x0
/* Conversion macros - see trans_data() in trans.c for the other direction */
#define TRANS_INDEX_FLAG        0
#define TRANS_INDEX_SEARCHDEPTH 2
#define TRANS_FLAG(t)        (((t).flags >> TRANS_INDEX_FLAG) & 0x3)
#define TRANS_SEARCHDEPTH(t) (((t).flags >> TRANS_INDEX_SEARCHDEPTH) & 0x3f)
#define TRANS_GAMEDEPTH(t)   ((t).gamedepth)
#define TRANS_VALUE(t)       ((t).value)
#define TRANS_MOVE(t)        ((t).move & ((1 << MOV_INDEX_UNUSED) - 1))
#define TRANS_REPS(t)        ((t).move >> MOV_INDEX_UNUSED)

/* Add an entry. On collision, will succeed only if it's better information */
void trans_add(zobrist_t, move_t, uint8_t, int16_t, uint8_t, uint8_t, uint8_t,
               unsigned char);
/* Find a value from the table. Returns -1 if none exists - you can trust this
 * value because a trans_data_t will never be -1 due to the blank bits */
trans_data_t trans_get(zobrist_t);
/* Check if the return value from get() was valid */
int trans_data_valid(trans_data_t);

#endif
/****************************************************************************
 * book.h - simple interface for retrieving moves from a plaintext book
 * copyright (C) 2008 Ben Blum
 * ****************************************************************************/
#ifndef BOOK_H
#define BOOK_H

#define BOOK_LINE_MAX_LENGTH 256

move_t book_move(char *line, board_t *board);

#endif
/****************************************************************************
 * search.h - traverse the game tree looking for the best position
 * copyright (C) 2008 Ben Blum
 ****************************************************************************/
#ifndef SEARCH_H
#define SEARCH_H

/**
 * A search function will require the following arguments:
 * 1) Board state
 * 2) How many seconds we should think before moving
 * 3) int * - if nonnull, lets you know how many nodes were searched
 * 4) int * - if nonnull, lets you know the alpha value of the position
 */
typedef move_t (*search_fn)(board_t *, unsigned int, int *, int16_t *);

move_t getbestmove(board_t *, unsigned int, int *, int16_t *);

#endif
/****************************************************************************
 * engine.c - wrapper functions for managing computer vs human play
 * copyright (C) 2008 Ben Blum
 ****************************************************************************/
#ifndef ENGINE_H
#define ENGINE_H


#define ENGINE_NAME "bistromath"

typedef struct engine_t {
	board_t *board;
	search_fn search;
	char line[BOOK_LINE_MAX_LENGTH];
	unsigned int line_moves;
	unsigned char inbook;
} engine_t;

engine_t *engine_init(search_fn);
char *engine_generatemove(engine_t *);
int engine_applymove(engine_t *, char *, char **);
void engine_destroy(engine_t *);

#endif
/****************************************************************************
 * attacks.c - precomputed attack-/move-mask tables
 * copyright (C) 2008 Ben Blum
 ****************************************************************************/

/* Bitmasks for attackable squares given a square a certain piece is on. Use
 * the predefined constants (A1 to H8, 0-63) from bitboard.h, or some other
 * way of getting the square in the same format, to index into these arrays */
uint64_t knightattacks[64] = {
	/* A1 to H1 */
	U64(0x0000000000020400), U64(0x0000000000050800), U64(0x00000000000a1100), U64(0x0000000000142200),
	U64(0x0000000000284400), U64(0x0000000000508800), U64(0x0000000000a01000), U64(0x0000000000402000),
	/* A2 to H2 */
	U64(0x0000000002040004), U64(0x0000000005080008), U64(0x000000000a110011), U64(0x0000000014220022),
	U64(0x0000000028440044), U64(0x0000000050880088), U64(0x00000000a0100010), U64(0x0000000040200020),
	/* A3 to H3 */
	U64(0x0000000204000402), U64(0x0000000508000805), U64(0x0000000a1100110a), U64(0x0000001422002214),
	U64(0x0000002844004428), U64(0x0000005088008850), U64(0x000000a0100010a0), U64(0x0000004020002040),
	/* A4 to H4 */
	U64(0x0000020400040200), U64(0x0000050800080500), U64(0x00000a1100110a00), U64(0x0000142200221400),
	U64(0x0000284400442800), U64(0x0000508800885000), U64(0x0000a0100010a000), U64(0x0000402000204000),
	/* A5 to H5 */
	U64(0x0002040004020000), U64(0x0005080008050000), U64(0x000a1100110a0000), U64(0x0014220022140000),
	U64(0x0028440044280000), U64(0x0050880088500000), U64(0x00a0100010a00000), U64(0x0040200020400000),
	/* A6 to H6 */
	U64(0x0204000402000000), U64(0x0508000805000000), U64(0x0a1100110a000000), U64(0x1422002214000000),
	U64(0x2844004428000000), U64(0x5088008850000000), U64(0xa0100010a0000000), U64(0x4020002040000000),
	/* A7 to H7 */
	U64(0x0400040200000000), U64(0x0800080500000000), U64(0x1100110a00000000), U64(0x2200221400000000),
	U64(0x4400442800000000), U64(0x8800885000000000), U64(0x100010a000000000), U64(0x2000204000000000),
	/* A8 to H8 */
	U64(0x0004020000000000), U64(0x0008050000000000), U64(0x00110a0000000000), U64(0x0022140000000000),
	U64(0x0044280000000000), U64(0x0088500000000000), U64(0x0010a00000000000), U64(0x0020400000000000)
};
uint64_t kingattacks[64] = {
	/* A1 to H1 */
	U64(0x0000000000000302), U64(0x0000000000000705), U64(0x0000000000000e0a), U64(0x0000000000001c14),
	U64(0x0000000000003828), U64(0x0000000000007050), U64(0x000000000000e0a0), U64(0x000000000000c040),
	/* A2 to H2 */
	U64(0x0000000000030203), U64(0x0000000000070507), U64(0x00000000000e0a0e), U64(0x00000000001c141c),
	U64(0x0000000000382838), U64(0x0000000000705070), U64(0x0000000000e0a0e0), U64(0x0000000000c040c0),
	/* A3 to H3 */
	U64(0x0000000003020300), U64(0x0000000007050700), U64(0x000000000e0a0e00), U64(0x000000001c141c00),
	U64(0x0000000038283800), U64(0x0000000070507000), U64(0x00000000e0a0e000), U64(0x00000000c040c000),
	/* A4 to H4 */
	U64(0x0000000302030000), U64(0x0000000705070000), U64(0x0000000e0a0e0000), U64(0x0000001c141c0000),
	U64(0x0000003828380000), U64(0x0000007050700000), U64(0x000000e0a0e00000), U64(0x000000c040c00000),
	/* A5 to H5 */
	U64(0x0000030203000000), U64(0x0000070507000000), U64(0x00000e0a0e000000), U64(0x00001c141c000000),
	U64(0x0000382838000000), U64(0x0000705070000000), U64(0x0000e0a0e0000000), U64(0x0000c040c0000000),
	/* A6 to H6 */
	U64(0x0003020300000000), U64(0x0007050700000000), U64(0x000e0a0e00000000), U64(0x001c141c00000000),
	U64(0x0038283800000000), U64(0x0070507000000000), U64(0x00e0a0e000000000), U64(0x00c040c000000000),
	/* A7 to H7 */
	U64(0x0302030000000000), U64(0x0705070000000000), U64(0x0e0a0e0000000000), U64(0x1c141c0000000000),
	U64(0x3828380000000000), U64(0x7050700000000000), U64(0xe0a0e00000000000), U64(0xc040c00000000000),
	/* A8 to H8 */
	U64(0x0203000000000000), U64(0x0507000000000000), U64(0x0a0e000000000000), U64(0x141c000000000000),
	U64(0x2838000000000000), U64(0x5070000000000000), U64(0xa0e0000000000000), U64(0x40c0000000000000)
};
uint64_t pawnattacks[2][64] = {
	{ /* WHITE */
	/* A1 to H1 */
	U64(0x0000000000000200), U64(0x0000000000000500), U64(0x0000000000000a00), U64(0x0000000000001400),
	U64(0x0000000000002800), U64(0x0000000000005000), U64(0x000000000000a000), U64(0x0000000000004000),
	/* A2 to H2 */
	U64(0x0000000000020000), U64(0x0000000000050000), U64(0x00000000000a0000), U64(0x0000000000140000),
	U64(0x0000000000280000), U64(0x0000000000500000), U64(0x0000000000a00000), U64(0x0000000000400000),
	/* A3 to H3 */
	U64(0x0000000002000000), U64(0x0000000005000000), U64(0x000000000a000000), U64(0x0000000014000000),
	U64(0x0000000028000000), U64(0x0000000050000000), U64(0x00000000a0000000), U64(0x0000000040000000),
	/* A4 to H4 */
	U64(0x0000000200000000), U64(0x0000000500000000), U64(0x0000000a00000000), U64(0x0000001400000000),
	U64(0x0000002800000000), U64(0x0000005000000000), U64(0x000000a000000000), U64(0x0000004000000000),
	/* A5 to H5 */
	U64(0x0000020000000000), U64(0x0000050000000000), U64(0x00000a0000000000), U64(0x0000140000000000),
	U64(0x0000280000000000), U64(0x0000500000000000), U64(0x0000a00000000000), U64(0x0000400000000000),
	/* A6 to H6 */
	U64(0x0002000000000000), U64(0x0005000000000000), U64(0x000a000000000000), U64(0x0014000000000000),
	U64(0x0028000000000000), U64(0x0050000000000000), U64(0x00a0000000000000), U64(0x0040000000000000),
	/* A7 to H7 */
	U64(0x0200000000000000), U64(0x0500000000000000), U64(0x0a00000000000000), U64(0x1400000000000000),
	U64(0x2800000000000000), U64(0x5000000000000000), U64(0xa000000000000000), U64(0x4000000000000000),
	/* A8 to H8 */
	U64(0x0000000000000000), U64(0x0000000000000000), U64(0x0000000000000000), U64(0x0000000000000000),
	U64(0x0000000000000000), U64(0x0000000000000000), U64(0x0000000000000000), U64(0x0000000000000000)
	},
	{ /* BLACK */
	/* A1 to H1 */
	U64(0x0000000000000000), U64(0x0000000000000000), U64(0x0000000000000000), U64(0x0000000000000000),
	U64(0x0000000000000000), U64(0x0000000000000000), U64(0x0000000000000000), U64(0x0000000000000000),
	/* A2 to H2 */
	U64(0x0000000000000002), U64(0x0000000000000005), U64(0x000000000000000a), U64(0x0000000000000014),
	U64(0x0000000000000028), U64(0x0000000000000050), U64(0x00000000000000a0), U64(0x0000000000000040),
	/* A3 to H3 */
	U64(0x0000000000000200), U64(0x0000000000000500), U64(0x0000000000000a00), U64(0x0000000000001400),
	U64(0x0000000000002800), U64(0x0000000000005000), U64(0x000000000000a000), U64(0x0000000000004000),
	/* A4 to H4 */
	U64(0x0000000000020000), U64(0x0000000000050000), U64(0x00000000000a0000), U64(0x0000000000140000),
	U64(0x0000000000280000), U64(0x0000000000500000), U64(0x0000000000a00000), U64(0x0000000000400000),
	/* A5 to H5 */
	U64(0x0000000002000000), U64(0x0000000005000000), U64(0x000000000a000000), U64(0x0000000014000000),
	U64(0x0000000028000000), U64(0x0000000050000000), U64(0x00000000a0000000), U64(0x0000000040000000),
	/* A6 to H6 */
	U64(0x0000000200000000), U64(0x0000000500000000), U64(0x0000000a00000000), U64(0x0000001400000000),
	U64(0x0000002800000000), U64(0x0000005000000000), U64(0x000000a000000000), U64(0x0000004000000000),
	/* A7 to H7 */
	U64(0x0000020000000000), U64(0x0000050000000000), U64(0x00000a0000000000), U64(0x0000140000000000),
	U64(0x0000280000000000), U64(0x0000500000000000), U64(0x0000a00000000000), U64(0x0000400000000000),
	/* A8 to H8 */
	U64(0x0002000000000000), U64(0x0005000000000000), U64(0x000a000000000000), U64(0x0014000000000000),
	U64(0x0028000000000000), U64(0x0050000000000000), U64(0x00a0000000000000), U64(0x0040000000000000)
	}
};
/* Don't account for the double-pawn-pushes; the pawn moves generator takes
 * care of that */
uint64_t pawnmoves[2][64] = {
	{ /* WHITE */
	/* A1 to H1 */
	U64(0x0000000000000000), U64(0x0000000000000000), U64(0x0000000000000000), U64(0x0000000000000000),
	U64(0x0000000000000000), U64(0x0000000000000000), U64(0x0000000000000000), U64(0x0000000000000000),
	/* A2 to H2 */
	U64(0x0000000000010000), U64(0x0000000000020000), U64(0x0000000000040000), U64(0x0000000000080000),
	U64(0x0000000000100000), U64(0x0000000000200000), U64(0x0000000000400000), U64(0x0000000000800000),
	/* A3 to H3 */
	U64(0x0000000001000000), U64(0x0000000002000000), U64(0x0000000004000000), U64(0x0000000008000000),
	U64(0x0000000010000000), U64(0x0000000020000000), U64(0x0000000040000000), U64(0x0000000080000000),
	/* A4 to H4 */
	U64(0x0000000100000000), U64(0x0000000200000000), U64(0x0000000400000000), U64(0x0000000800000000),
	U64(0x0000001000000000), U64(0x0000002000000000), U64(0x0000004000000000), U64(0x0000008000000000),
	/* A5 to H5 */
	U64(0x0000010000000000), U64(0x0000020000000000), U64(0x0000040000000000), U64(0x0000080000000000),
	U64(0x0000100000000000), U64(0x0000200000000000), U64(0x0000400000000000), U64(0x0000800000000000),
	/* A6 to H6 */
	U64(0x0001000000000000), U64(0x0002000000000000), U64(0x0004000000000000), U64(0x0008000000000000),
	U64(0x0010000000000000), U64(0x0020000000000000), U64(0x0040000000000000), U64(0x0080000000000000),
	/* A7 to H7 */
	U64(0x0100000000000000), U64(0x0200000000000000), U64(0x0400000000000000), U64(0x0800000000000000),
	U64(0x1000000000000000), U64(0x2000000000000000), U64(0x4000000000000000), U64(0x8000000000000000),
	/* A8 to H8 */
	U64(0x0000000000000000), U64(0x0000000000000000), U64(0x0000000000000000), U64(0x0000000000000000),
	U64(0x0000000000000000), U64(0x0000000000000000), U64(0x0000000000000000), U64(0x0000000000000000)
	},
	{ /* BLACK */
	/* A1 to H1 */
	U64(0x0000000000000000), U64(0x0000000000000000), U64(0x0000000000000000), U64(0x0000000000000000),
	U64(0x0000000000000000), U64(0x0000000000000000), U64(0x0000000000000000), U64(0x0000000000000000),
	/* A2 to H2 */
	U64(0x0000000000000001), U64(0x0000000000000002), U64(0x0000000000000004), U64(0x0000000000000008),
	U64(0x0000000000000010), U64(0x0000000000000020), U64(0x0000000000000040), U64(0x0000000000000080),
	/* A3 to H3 */
	U64(0x0000000000000100), U64(0x0000000000000200), U64(0x0000000000000400), U64(0x0000000000000800),
	U64(0x0000000000001000), U64(0x0000000000002000), U64(0x0000000000004000), U64(0x0000000000008000),
	/* A4 to H4 */
	U64(0x0000000000010000), U64(0x0000000000020000), U64(0x0000000000040000), U64(0x0000000000080000),
	U64(0x0000000000100000), U64(0x0000000000200000), U64(0x0000000000400000), U64(0x0000000000800000),
	/* A5 to H5 */
	U64(0x0000000001000000), U64(0x0000000002000000), U64(0x0000000004000000), U64(0x0000000008000000),
	U64(0x0000000010000000), U64(0x0000000020000000), U64(0x0000000040000000), U64(0x0000000080000000),
	/* A6 to H6 */
	U64(0x0000000100000000), U64(0x0000000200000000), U64(0x0000000400000000), U64(0x0000000800000000),
	U64(0x0000001000000000), U64(0x0000002000000000), U64(0x0000004000000000), U64(0x0000008000000000),
	/* A7 to H7 */
	U64(0x0000010000000000), U64(0x0000020000000000), U64(0x0000040000000000), U64(0x0000080000000000),
	U64(0x0000100000000000), U64(0x0000200000000000), U64(0x0000400000000000), U64(0x0000800000000000),
	/* A8 to H8 */
	U64(0x0000000000000000), U64(0x0000000000000000), U64(0x0000000000000000), U64(0x0000000000000000),
	U64(0x0000000000000000), U64(0x0000000000000000), U64(0x0000000000000000), U64(0x0000000000000000)
	}
};

/**
 * For rook attacks along a row. Uses the 'occupied' status of the row (e.g.
 * if there are pieces on the columns a, d, e, f, h , the bitmask will be
 * (binary) 00111001) as the first index, and the position of the rook (0-7
 * for columns 1-8 (right to left on the mask)) as the second index.
 * 
 * It returns a row mask appropriate for a rook on the 1st (0 index) row's
 * attacks; note that the bit at the end of the attack ray will overlap the
 * first blocking piece (a rook on d-column with rowblock mask 11010011 (note
 * the 1 in the d-col indicates the rook itself) will get move mask 01101110
 * (note again the 0 on the d-column; the rook cannot move to itself)).
 * 
 * Because the return is suitable for a rook on row-1 (0 index), it must be
 * shifted left by 8*ROW(square) - for example, on the 8th row, the shift by
 * 8*7 will put it in the MSBs of the uint64_t.
 */
uint64_t rowattacks[256][8] = {
	{ U64(0xfe), U64(0xfd), U64(0xfb), U64(0xf7), U64(0xef), U64(0xdf), U64(0xbf), U64(0x7f) }, /* 0x00 */
	{ U64(0xfe), U64(0xfd), U64(0xfb), U64(0xf7), U64(0xef), U64(0xdf), U64(0xbf), U64(0x7f) }, /* 0x01 */
	{ U64(0x02), U64(0xfd), U64(0xfa), U64(0xf6), U64(0xee), U64(0xde), U64(0xbe), U64(0x7e) }, /* 0x02 */
	{ U64(0x02), U64(0xfd), U64(0xfa), U64(0xf6), U64(0xee), U64(0xde), U64(0xbe), U64(0x7e) }, /* 0x03 */
	{ U64(0x06), U64(0x05), U64(0xfb), U64(0xf4), U64(0xec), U64(0xdc), U64(0xbc), U64(0x7c) }, /* 0x04 */
	{ U64(0x06), U64(0x05), U64(0xfb), U64(0xf4), U64(0xec), U64(0xdc), U64(0xbc), U64(0x7c) }, /* 0x05 */
	{ U64(0x02), U64(0x05), U64(0xfa), U64(0xf4), U64(0xec), U64(0xdc), U64(0xbc), U64(0x7c) }, /* 0x06 */
	{ U64(0x02), U64(0x05), U64(0xfa), U64(0xf4), U64(0xec), U64(0xdc), U64(0xbc), U64(0x7c) }, /* 0x07 */
	{ U64(0x0e), U64(0x0d), U64(0x0b), U64(0xf7), U64(0xe8), U64(0xd8), U64(0xb8), U64(0x78) }, /* 0x08 */
	{ U64(0x0e), U64(0x0d), U64(0x0b), U64(0xf7), U64(0xe8), U64(0xd8), U64(0xb8), U64(0x78) }, /* 0x09 */
	{ U64(0x02), U64(0x0d), U64(0x0a), U64(0xf6), U64(0xe8), U64(0xd8), U64(0xb8), U64(0x78) }, /* 0x0a */
	{ U64(0x02), U64(0x0d), U64(0x0a), U64(0xf6), U64(0xe8), U64(0xd8), U64(0xb8), U64(0x78) }, /* 0x0b */
	{ U64(0x06), U64(0x05), U64(0x0b), U64(0xf4), U64(0xe8), U64(0xd8), U64(0xb8), U64(0x78) }, /* 0x0c */
	{ U64(0x06), U64(0x05), U64(0x0b), U64(0xf4), U64(0xe8), U64(0xd8), U64(0xb8), U64(0x78) }, /* 0x0d */
	{ U64(0x02), U64(0x05), U64(0x0a), U64(0xf4), U64(0xe8), U64(0xd8), U64(0xb8), U64(0x78) }, /* 0x0e */
	{ U64(0x02), U64(0x05), U64(0x0a), U64(0xf4), U64(0xe8), U64(0xd8), U64(0xb8), U64(0x78) }, /* 0x0f */
	{ U64(0x1e), U64(0x1d), U64(0x1b), U64(0x17), U64(0xef), U64(0xd0), U64(0xb0), U64(0x70) }, /* 0x10 */
	{ U64(0x1e), U64(0x1d), U64(0x1b), U64(0x17), U64(0xef), U64(0xd0), U64(0xb0), U64(0x70) }, /* 0x11 */
	{ U64(0x02), U64(0x1d), U64(0x1a), U64(0x16), U64(0xee), U64(0xd0), U64(0xb0), U64(0x70) }, /* 0x12 */
	{ U64(0x02), U64(0x1d), U64(0x1a), U64(0x16), U64(0xee), U64(0xd0), U64(0xb0), U64(0x70) }, /* 0x13 */
	{ U64(0x06), U64(0x05), U64(0x1b), U64(0x14), U64(0xec), U64(0xd0), U64(0xb0), U64(0x70) }, /* 0x14 */
	{ U64(0x06), U64(0x05), U64(0x1b), U64(0x14), U64(0xec), U64(0xd0), U64(0xb0), U64(0x70) }, /* 0x15 */
	{ U64(0x02), U64(0x05), U64(0x1a), U64(0x14), U64(0xec), U64(0xd0), U64(0xb0), U64(0x70) }, /* 0x16 */
	{ U64(0x02), U64(0x05), U64(0x1a), U64(0x14), U64(0xec), U64(0xd0), U64(0xb0), U64(0x70) }, /* 0x17 */
	{ U64(0x0e), U64(0x0d), U64(0x0b), U64(0x17), U64(0xe8), U64(0xd0), U64(0xb0), U64(0x70) }, /* 0x18 */
	{ U64(0x0e), U64(0x0d), U64(0x0b), U64(0x17), U64(0xe8), U64(0xd0), U64(0xb0), U64(0x70) }, /* 0x19 */
	{ U64(0x02), U64(0x0d), U64(0x0a), U64(0x16), U64(0xe8), U64(0xd0), U64(0xb0), U64(0x70) }, /* 0x1a */
	{ U64(0x02), U64(0x0d), U64(0x0a), U64(0x16), U64(0xe8), U64(0xd0), U64(0xb0), U64(0x70) }, /* 0x1b */
	{ U64(0x06), U64(0x05), U64(0x0b), U64(0x14), U64(0xe8), U64(0xd0), U64(0xb0), U64(0x70) }, /* 0x1c */
	{ U64(0x06), U64(0x05), U64(0x0b), U64(0x14), U64(0xe8), U64(0xd0), U64(0xb0), U64(0x70) }, /* 0x1d */
	{ U64(0x02), U64(0x05), U64(0x0a), U64(0x14), U64(0xe8), U64(0xd0), U64(0xb0), U64(0x70) }, /* 0x1e */
	{ U64(0x02), U64(0x05), U64(0x0a), U64(0x14), U64(0xe8), U64(0xd0), U64(0xb0), U64(0x70) }, /* 0x1f */
	{ U64(0x3e), U64(0x3d), U64(0x3b), U64(0x37), U64(0x2f), U64(0xdf), U64(0xa0), U64(0x60) }, /* 0x20 */
	{ U64(0x3e), U64(0x3d), U64(0x3b), U64(0x37), U64(0x2f), U64(0xdf), U64(0xa0), U64(0x60) }, /* 0x21 */
	{ U64(0x02), U64(0x3d), U64(0x3a), U64(0x36), U64(0x2e), U64(0xde), U64(0xa0), U64(0x60) }, /* 0x22 */
	{ U64(0x02), U64(0x3d), U64(0x3a), U64(0x36), U64(0x2e), U64(0xde), U64(0xa0), U64(0x60) }, /* 0x23 */
	{ U64(0x06), U64(0x05), U64(0x3b), U64(0x34), U64(0x2c), U64(0xdc), U64(0xa0), U64(0x60) }, /* 0x24 */
	{ U64(0x06), U64(0x05), U64(0x3b), U64(0x34), U64(0x2c), U64(0xdc), U64(0xa0), U64(0x60) }, /* 0x25 */
	{ U64(0x02), U64(0x05), U64(0x3a), U64(0x34), U64(0x2c), U64(0xdc), U64(0xa0), U64(0x60) }, /* 0x26 */
	{ U64(0x02), U64(0x05), U64(0x3a), U64(0x34), U64(0x2c), U64(0xdc), U64(0xa0), U64(0x60) }, /* 0x27 */
	{ U64(0x0e), U64(0x0d), U64(0x0b), U64(0x37), U64(0x28), U64(0xd8), U64(0xa0), U64(0x60) }, /* 0x28 */
	{ U64(0x0e), U64(0x0d), U64(0x0b), U64(0x37), U64(0x28), U64(0xd8), U64(0xa0), U64(0x60) }, /* 0x29 */
	{ U64(0x02), U64(0x0d), U64(0x0a), U64(0x36), U64(0x28), U64(0xd8), U64(0xa0), U64(0x60) }, /* 0x2a */
	{ U64(0x02), U64(0x0d), U64(0x0a), U64(0x36), U64(0x28), U64(0xd8), U64(0xa0), U64(0x60) }, /* 0x2b */
	{ U64(0x06), U64(0x05), U64(0x0b), U64(0x34), U64(0x28), U64(0xd8), U64(0xa0), U64(0x60) }, /* 0x2c */
	{ U64(0x06), U64(0x05), U64(0x0b), U64(0x34), U64(0x28), U64(0xd8), U64(0xa0), U64(0x60) }, /* 0x2d */
	{ U64(0x02), U64(0x05), U64(0x0a), U64(0x34), U64(0x28), U64(0xd8), U64(0xa0), U64(0x60) }, /* 0x2e */
	{ U64(0x02), U64(0x05), U64(0x0a), U64(0x34), U64(0x28), U64(0xd8), U64(0xa0), U64(0x60) }, /* 0x2f */
	{ U64(0x1e), U64(0x1d), U64(0x1b), U64(0x17), U64(0x2f), U64(0xd0), U64(0xa0), U64(0x60) }, /* 0x30 */
	{ U64(0x1e), U64(0x1d), U64(0x1b), U64(0x17), U64(0x2f), U64(0xd0), U64(0xa0), U64(0x60) }, /* 0x31 */
	{ U64(0x02), U64(0x1d), U64(0x1a), U64(0x16), U64(0x2e), U64(0xd0), U64(0xa0), U64(0x60) }, /* 0x32 */
	{ U64(0x02), U64(0x1d), U64(0x1a), U64(0x16), U64(0x2e), U64(0xd0), U64(0xa0), U64(0x60) }, /* 0x33 */
	{ U64(0x06), U64(0x05), U64(0x1b), U64(0x14), U64(0x2c), U64(0xd0), U64(0xa0), U64(0x60) }, /* 0x34 */
	{ U64(0x06), U64(0x05), U64(0x1b), U64(0x14), U64(0x2c), U64(0xd0), U64(0xa0), U64(0x60) }, /* 0x35 */
	{ U64(0x02), U64(0x05), U64(0x1a), U64(0x14), U64(0x2c), U64(0xd0), U64(0xa0), U64(0x60) }, /* 0x36 */
	{ U64(0x02), U64(0x05), U64(0x1a), U64(0x14), U64(0x2c), U64(0xd0), U64(0xa0), U64(0x60) }, /* 0x37 */
	{ U64(0x0e), U64(0x0d), U64(0x0b), U64(0x17), U64(0x28), U64(0xd0), U64(0xa0), U64(0x60) }, /* 0x38 */
	{ U64(0x0e), U64(0x0d), U64(0x0b), U64(0x17), U64(0x28), U64(0xd0), U64(0xa0), U64(0x60) }, /* 0x39 */
	{ U64(0x02), U64(0x0d), U64(0x0a), U64(0x16), U64(0x28), U64(0xd0), U64(0xa0), U64(0x60) }, /* 0x3a */
	{ U64(0x02), U64(0x0d), U64(0x0a), U64(0x16), U64(0x28), U64(0xd0), U64(0xa0), U64(0x60) }, /* 0x3b */
	{ U64(0x06), U64(0x05), U64(0x0b), U64(0x14), U64(0x28), U64(0xd0), U64(0xa0), U64(0x60) }, /* 0x3c */
	{ U64(0x06), U64(0x05), U64(0x0b), U64(0x14), U64(0x28), U64(0xd0), U64(0xa0), U64(0x60) }, /* 0x3d */
	{ U64(0x02), U64(0x05), U64(0x0a), U64(0x14), U64(0x28), U64(0xd0), U64(0xa0), U64(0x60) }, /* 0x3e */
	{ U64(0x02), U64(0x05), U64(0x0a), U64(0x14), U64(0x28), U64(0xd0), U64(0xa0), U64(0x60) }, /* 0x3f */
	{ U64(0x7e), U64(0x7d), U64(0x7b), U64(0x77), U64(0x6f), U64(0x5f), U64(0xbf), U64(0x40) }, /* 0x40 */
	{ U64(0x7e), U64(0x7d), U64(0x7b), U64(0x77), U64(0x6f), U64(0x5f), U64(0xbf), U64(0x40) }, /* 0x41 */
	{ U64(0x02), U64(0x7d), U64(0x7a), U64(0x76), U64(0x6e), U64(0x5e), U64(0xbe), U64(0x40) }, /* 0x42 */
	{ U64(0x02), U64(0x7d), U64(0x7a), U64(0x76), U64(0x6e), U64(0x5e), U64(0xbe), U64(0x40) }, /* 0x43 */
	{ U64(0x06), U64(0x05), U64(0x7b), U64(0x74), U64(0x6c), U64(0x5c), U64(0xbc), U64(0x40) }, /* 0x44 */
	{ U64(0x06), U64(0x05), U64(0x7b), U64(0x74), U64(0x6c), U64(0x5c), U64(0xbc), U64(0x40) }, /* 0x45 */
	{ U64(0x02), U64(0x05), U64(0x7a), U64(0x74), U64(0x6c), U64(0x5c), U64(0xbc), U64(0x40) }, /* 0x46 */
	{ U64(0x02), U64(0x05), U64(0x7a), U64(0x74), U64(0x6c), U64(0x5c), U64(0xbc), U64(0x40) }, /* 0x47 */
	{ U64(0x0e), U64(0x0d), U64(0x0b), U64(0x77), U64(0x68), U64(0x58), U64(0xb8), U64(0x40) }, /* 0x48 */
	{ U64(0x0e), U64(0x0d), U64(0x0b), U64(0x77), U64(0x68), U64(0x58), U64(0xb8), U64(0x40) }, /* 0x49 */
	{ U64(0x02), U64(0x0d), U64(0x0a), U64(0x76), U64(0x68), U64(0x58), U64(0xb8), U64(0x40) }, /* 0x4a */
	{ U64(0x02), U64(0x0d), U64(0x0a), U64(0x76), U64(0x68), U64(0x58), U64(0xb8), U64(0x40) }, /* 0x4b */
	{ U64(0x06), U64(0x05), U64(0x0b), U64(0x74), U64(0x68), U64(0x58), U64(0xb8), U64(0x40) }, /* 0x4c */
	{ U64(0x06), U64(0x05), U64(0x0b), U64(0x74), U64(0x68), U64(0x58), U64(0xb8), U64(0x40) }, /* 0x4d */
	{ U64(0x02), U64(0x05), U64(0x0a), U64(0x74), U64(0x68), U64(0x58), U64(0xb8), U64(0x40) }, /* 0x4e */
	{ U64(0x02), U64(0x05), U64(0x0a), U64(0x74), U64(0x68), U64(0x58), U64(0xb8), U64(0x40) }, /* 0x4f */
	{ U64(0x1e), U64(0x1d), U64(0x1b), U64(0x17), U64(0x6f), U64(0x50), U64(0xb0), U64(0x40) }, /* 0x50 */
	{ U64(0x1e), U64(0x1d), U64(0x1b), U64(0x17), U64(0x6f), U64(0x50), U64(0xb0), U64(0x40) }, /* 0x51 */
	{ U64(0x02), U64(0x1d), U64(0x1a), U64(0x16), U64(0x6e), U64(0x50), U64(0xb0), U64(0x40) }, /* 0x52 */
	{ U64(0x02), U64(0x1d), U64(0x1a), U64(0x16), U64(0x6e), U64(0x50), U64(0xb0), U64(0x40) }, /* 0x53 */
	{ U64(0x06), U64(0x05), U64(0x1b), U64(0x14), U64(0x6c), U64(0x50), U64(0xb0), U64(0x40) }, /* 0x54 */
	{ U64(0x06), U64(0x05), U64(0x1b), U64(0x14), U64(0x6c), U64(0x50), U64(0xb0), U64(0x40) }, /* 0x55 */
	{ U64(0x02), U64(0x05), U64(0x1a), U64(0x14), U64(0x6c), U64(0x50), U64(0xb0), U64(0x40) }, /* 0x56 */
	{ U64(0x02), U64(0x05), U64(0x1a), U64(0x14), U64(0x6c), U64(0x50), U64(0xb0), U64(0x40) }, /* 0x57 */
	{ U64(0x0e), U64(0x0d), U64(0x0b), U64(0x17), U64(0x68), U64(0x50), U64(0xb0), U64(0x40) }, /* 0x58 */
	{ U64(0x0e), U64(0x0d), U64(0x0b), U64(0x17), U64(0x68), U64(0x50), U64(0xb0), U64(0x40) }, /* 0x59 */
	{ U64(0x02), U64(0x0d), U64(0x0a), U64(0x16), U64(0x68), U64(0x50), U64(0xb0), U64(0x40) }, /* 0x5a */
	{ U64(0x02), U64(0x0d), U64(0x0a), U64(0x16), U64(0x68), U64(0x50), U64(0xb0), U64(0x40) }, /* 0x5b */
	{ U64(0x06), U64(0x05), U64(0x0b), U64(0x14), U64(0x68), U64(0x50), U64(0xb0), U64(0x40) }, /* 0x5c */
	{ U64(0x06), U64(0x05), U64(0x0b), U64(0x14), U64(0x68), U64(0x50), U64(0xb0), U64(0x40) }, /* 0x5d */
	{ U64(0x02), U64(0x05), U64(0x0a), U64(0x14), U64(0x68), U64(0x50), U64(0xb0), U64(0x40) }, /* 0x5e */
	{ U64(0x02), U64(0x05), U64(0x0a), U64(0x14), U64(0x68), U64(0x50), U64(0xb0), U64(0x40) }, /* 0x5f */
	{ U64(0x3e), U64(0x3d), U64(0x3b), U64(0x37), U64(0x2f), U64(0x5f), U64(0xa0), U64(0x40) }, /* 0x60 */
	{ U64(0x3e), U64(0x3d), U64(0x3b), U64(0x37), U64(0x2f), U64(0x5f), U64(0xa0), U64(0x40) }, /* 0x61 */
	{ U64(0x02), U64(0x3d), U64(0x3a), U64(0x36), U64(0x2e), U64(0x5e), U64(0xa0), U64(0x40) }, /* 0x62 */
	{ U64(0x02), U64(0x3d), U64(0x3a), U64(0x36), U64(0x2e), U64(0x5e), U64(0xa0), U64(0x40) }, /* 0x63 */
	{ U64(0x06), U64(0x05), U64(0x3b), U64(0x34), U64(0x2c), U64(0x5c), U64(0xa0), U64(0x40) }, /* 0x64 */
	{ U64(0x06), U64(0x05), U64(0x3b), U64(0x34), U64(0x2c), U64(0x5c), U64(0xa0), U64(0x40) }, /* 0x65 */
	{ U64(0x02), U64(0x05), U64(0x3a), U64(0x34), U64(0x2c), U64(0x5c), U64(0xa0), U64(0x40) }, /* 0x66 */
	{ U64(0x02), U64(0x05), U64(0x3a), U64(0x34), U64(0x2c), U64(0x5c), U64(0xa0), U64(0x40) }, /* 0x67 */
	{ U64(0x0e), U64(0x0d), U64(0x0b), U64(0x37), U64(0x28), U64(0x58), U64(0xa0), U64(0x40) }, /* 0x68 */
	{ U64(0x0e), U64(0x0d), U64(0x0b), U64(0x37), U64(0x28), U64(0x58), U64(0xa0), U64(0x40) }, /* 0x69 */
	{ U64(0x02), U64(0x0d), U64(0x0a), U64(0x36), U64(0x28), U64(0x58), U64(0xa0), U64(0x40) }, /* 0x6a */
	{ U64(0x02), U64(0x0d), U64(0x0a), U64(0x36), U64(0x28), U64(0x58), U64(0xa0), U64(0x40) }, /* 0x6b */
	{ U64(0x06), U64(0x05), U64(0x0b), U64(0x34), U64(0x28), U64(0x58), U64(0xa0), U64(0x40) }, /* 0x6c */
	{ U64(0x06), U64(0x05), U64(0x0b), U64(0x34), U64(0x28), U64(0x58), U64(0xa0), U64(0x40) }, /* 0x6d */
	{ U64(0x02), U64(0x05), U64(0x0a), U64(0x34), U64(0x28), U64(0x58), U64(0xa0), U64(0x40) }, /* 0x6e */
	{ U64(0x02), U64(0x05), U64(0x0a), U64(0x34), U64(0x28), U64(0x58), U64(0xa0), U64(0x40) }, /* 0x6f */
	{ U64(0x1e), U64(0x1d), U64(0x1b), U64(0x17), U64(0x2f), U64(0x50), U64(0xa0), U64(0x40) }, /* 0x70 */
	{ U64(0x1e), U64(0x1d), U64(0x1b), U64(0x17), U64(0x2f), U64(0x50), U64(0xa0), U64(0x40) }, /* 0x71 */
	{ U64(0x02), U64(0x1d), U64(0x1a), U64(0x16), U64(0x2e), U64(0x50), U64(0xa0), U64(0x40) }, /* 0x72 */
	{ U64(0x02), U64(0x1d), U64(0x1a), U64(0x16), U64(0x2e), U64(0x50), U64(0xa0), U64(0x40) }, /* 0x73 */
	{ U64(0x06), U64(0x05), U64(0x1b), U64(0x14), U64(0x2c), U64(0x50), U64(0xa0), U64(0x40) }, /* 0x74 */
	{ U64(0x06), U64(0x05), U64(0x1b), U64(0x14), U64(0x2c), U64(0x50), U64(0xa0), U64(0x40) }, /* 0x75 */
	{ U64(0x02), U64(0x05), U64(0x1a), U64(0x14), U64(0x2c), U64(0x50), U64(0xa0), U64(0x40) }, /* 0x76 */
	{ U64(0x02), U64(0x05), U64(0x1a), U64(0x14), U64(0x2c), U64(0x50), U64(0xa0), U64(0x40) }, /* 0x77 */
	{ U64(0x0e), U64(0x0d), U64(0x0b), U64(0x17), U64(0x28), U64(0x50), U64(0xa0), U64(0x40) }, /* 0x78 */
	{ U64(0x0e), U64(0x0d), U64(0x0b), U64(0x17), U64(0x28), U64(0x50), U64(0xa0), U64(0x40) }, /* 0x79 */
	{ U64(0x02), U64(0x0d), U64(0x0a), U64(0x16), U64(0x28), U64(0x50), U64(0xa0), U64(0x40) }, /* 0x7a */
	{ U64(0x02), U64(0x0d), U64(0x0a), U64(0x16), U64(0x28), U64(0x50), U64(0xa0), U64(0x40) }, /* 0x7b */
	{ U64(0x06), U64(0x05), U64(0x0b), U64(0x14), U64(0x28), U64(0x50), U64(0xa0), U64(0x40) }, /* 0x7c */
	{ U64(0x06), U64(0x05), U64(0x0b), U64(0x14), U64(0x28), U64(0x50), U64(0xa0), U64(0x40) }, /* 0x7d */
	{ U64(0x02), U64(0x05), U64(0x0a), U64(0x14), U64(0x28), U64(0x50), U64(0xa0), U64(0x40) }, /* 0x7e */
	{ U64(0x02), U64(0x05), U64(0x0a), U64(0x14), U64(0x28), U64(0x50), U64(0xa0), U64(0x40) }, /* 0x7f */
	{ U64(0xfe), U64(0xfd), U64(0xfb), U64(0xf7), U64(0xef), U64(0xdf), U64(0xbf), U64(0x7f) }, /* 0x80 */
	{ U64(0xfe), U64(0xfd), U64(0xfb), U64(0xf7), U64(0xef), U64(0xdf), U64(0xbf), U64(0x7f) }, /* 0x81 */
	{ U64(0x02), U64(0xfd), U64(0xfa), U64(0xf6), U64(0xee), U64(0xde), U64(0xbe), U64(0x7e) }, /* 0x82 */
	{ U64(0x02), U64(0xfd), U64(0xfa), U64(0xf6), U64(0xee), U64(0xde), U64(0xbe), U64(0x7e) }, /* 0x83 */
	{ U64(0x06), U64(0x05), U64(0xfb), U64(0xf4), U64(0xec), U64(0xdc), U64(0xbc), U64(0x7c) }, /* 0x84 */
	{ U64(0x06), U64(0x05), U64(0xfb), U64(0xf4), U64(0xec), U64(0xdc), U64(0xbc), U64(0x7c) }, /* 0x85 */
	{ U64(0x02), U64(0x05), U64(0xfa), U64(0xf4), U64(0xec), U64(0xdc), U64(0xbc), U64(0x7c) }, /* 0x86 */
	{ U64(0x02), U64(0x05), U64(0xfa), U64(0xf4), U64(0xec), U64(0xdc), U64(0xbc), U64(0x7c) }, /* 0x87 */
	{ U64(0x0e), U64(0x0d), U64(0x0b), U64(0xf7), U64(0xe8), U64(0xd8), U64(0xb8), U64(0x78) }, /* 0x88 */
	{ U64(0x0e), U64(0x0d), U64(0x0b), U64(0xf7), U64(0xe8), U64(0xd8), U64(0xb8), U64(0x78) }, /* 0x89 */
	{ U64(0x02), U64(0x0d), U64(0x0a), U64(0xf6), U64(0xe8), U64(0xd8), U64(0xb8), U64(0x78) }, /* 0x8a */
	{ U64(0x02), U64(0x0d), U64(0x0a), U64(0xf6), U64(0xe8), U64(0xd8), U64(0xb8), U64(0x78) }, /* 0x8b */
	{ U64(0x06), U64(0x05), U64(0x0b), U64(0xf4), U64(0xe8), U64(0xd8), U64(0xb8), U64(0x78) }, /* 0x8c */
	{ U64(0x06), U64(0x05), U64(0x0b), U64(0xf4), U64(0xe8), U64(0xd8), U64(0xb8), U64(0x78) }, /* 0x8d */
	{ U64(0x02), U64(0x05), U64(0x0a), U64(0xf4), U64(0xe8), U64(0xd8), U64(0xb8), U64(0x78) }, /* 0x8e */
	{ U64(0x02), U64(0x05), U64(0x0a), U64(0xf4), U64(0xe8), U64(0xd8), U64(0xb8), U64(0x78) }, /* 0x8f */
	{ U64(0x1e), U64(0x1d), U64(0x1b), U64(0x17), U64(0xef), U64(0xd0), U64(0xb0), U64(0x70) }, /* 0x90 */
	{ U64(0x1e), U64(0x1d), U64(0x1b), U64(0x17), U64(0xef), U64(0xd0), U64(0xb0), U64(0x70) }, /* 0x91 */
	{ U64(0x02), U64(0x1d), U64(0x1a), U64(0x16), U64(0xee), U64(0xd0), U64(0xb0), U64(0x70) }, /* 0x92 */
	{ U64(0x02), U64(0x1d), U64(0x1a), U64(0x16), U64(0xee), U64(0xd0), U64(0xb0), U64(0x70) }, /* 0x93 */
	{ U64(0x06), U64(0x05), U64(0x1b), U64(0x14), U64(0xec), U64(0xd0), U64(0xb0), U64(0x70) }, /* 0x94 */
	{ U64(0x06), U64(0x05), U64(0x1b), U64(0x14), U64(0xec), U64(0xd0), U64(0xb0), U64(0x70) }, /* 0x95 */
	{ U64(0x02), U64(0x05), U64(0x1a), U64(0x14), U64(0xec), U64(0xd0), U64(0xb0), U64(0x70) }, /* 0x96 */
	{ U64(0x02), U64(0x05), U64(0x1a), U64(0x14), U64(0xec), U64(0xd0), U64(0xb0), U64(0x70) }, /* 0x97 */
	{ U64(0x0e), U64(0x0d), U64(0x0b), U64(0x17), U64(0xe8), U64(0xd0), U64(0xb0), U64(0x70) }, /* 0x98 */
	{ U64(0x0e), U64(0x0d), U64(0x0b), U64(0x17), U64(0xe8), U64(0xd0), U64(0xb0), U64(0x70) }, /* 0x99 */
	{ U64(0x02), U64(0x0d), U64(0x0a), U64(0x16), U64(0xe8), U64(0xd0), U64(0xb0), U64(0x70) }, /* 0x9a */
	{ U64(0x02), U64(0x0d), U64(0x0a), U64(0x16), U64(0xe8), U64(0xd0), U64(0xb0), U64(0x70) }, /* 0x9b */
	{ U64(0x06), U64(0x05), U64(0x0b), U64(0x14), U64(0xe8), U64(0xd0), U64(0xb0), U64(0x70) }, /* 0x9c */
	{ U64(0x06), U64(0x05), U64(0x0b), U64(0x14), U64(0xe8), U64(0xd0), U64(0xb0), U64(0x70) }, /* 0x9d */
	{ U64(0x02), U64(0x05), U64(0x0a), U64(0x14), U64(0xe8), U64(0xd0), U64(0xb0), U64(0x70) }, /* 0x9e */
	{ U64(0x02), U64(0x05), U64(0x0a), U64(0x14), U64(0xe8), U64(0xd0), U64(0xb0), U64(0x70) }, /* 0x9f */
	{ U64(0x3e), U64(0x3d), U64(0x3b), U64(0x37), U64(0x2f), U64(0xdf), U64(0xa0), U64(0x60) }, /* 0xa0 */
	{ U64(0x3e), U64(0x3d), U64(0x3b), U64(0x37), U64(0x2f), U64(0xdf), U64(0xa0), U64(0x60) }, /* 0xa1 */
	{ U64(0x02), U64(0x3d), U64(0x3a), U64(0x36), U64(0x2e), U64(0xde), U64(0xa0), U64(0x60) }, /* 0xa2 */
	{ U64(0x02), U64(0x3d), U64(0x3a), U64(0x36), U64(0x2e), U64(0xde), U64(0xa0), U64(0x60) }, /* 0xa3 */
	{ U64(0x06), U64(0x05), U64(0x3b), U64(0x34), U64(0x2c), U64(0xdc), U64(0xa0), U64(0x60) }, /* 0xa4 */
	{ U64(0x06), U64(0x05), U64(0x3b), U64(0x34), U64(0x2c), U64(0xdc), U64(0xa0), U64(0x60) }, /* 0xa5 */
	{ U64(0x02), U64(0x05), U64(0x3a), U64(0x34), U64(0x2c), U64(0xdc), U64(0xa0), U64(0x60) }, /* 0xa6 */
	{ U64(0x02), U64(0x05), U64(0x3a), U64(0x34), U64(0x2c), U64(0xdc), U64(0xa0), U64(0x60) }, /* 0xa7 */
	{ U64(0x0e), U64(0x0d), U64(0x0b), U64(0x37), U64(0x28), U64(0xd8), U64(0xa0), U64(0x60) }, /* 0xa8 */
	{ U64(0x0e), U64(0x0d), U64(0x0b), U64(0x37), U64(0x28), U64(0xd8), U64(0xa0), U64(0x60) }, /* 0xa9 */
	{ U64(0x02), U64(0x0d), U64(0x0a), U64(0x36), U64(0x28), U64(0xd8), U64(0xa0), U64(0x60) }, /* 0xaa */
	{ U64(0x02), U64(0x0d), U64(0x0a), U64(0x36), U64(0x28), U64(0xd8), U64(0xa0), U64(0x60) }, /* 0xab */
	{ U64(0x06), U64(0x05), U64(0x0b), U64(0x34), U64(0x28), U64(0xd8), U64(0xa0), U64(0x60) }, /* 0xac */
	{ U64(0x06), U64(0x05), U64(0x0b), U64(0x34), U64(0x28), U64(0xd8), U64(0xa0), U64(0x60) }, /* 0xad */
	{ U64(0x02), U64(0x05), U64(0x0a), U64(0x34), U64(0x28), U64(0xd8), U64(0xa0), U64(0x60) }, /* 0xae */
	{ U64(0x02), U64(0x05), U64(0x0a), U64(0x34), U64(0x28), U64(0xd8), U64(0xa0), U64(0x60) }, /* 0xaf */
	{ U64(0x1e), U64(0x1d), U64(0x1b), U64(0x17), U64(0x2f), U64(0xd0), U64(0xa0), U64(0x60) }, /* 0xb0 */
	{ U64(0x1e), U64(0x1d), U64(0x1b), U64(0x17), U64(0x2f), U64(0xd0), U64(0xa0), U64(0x60) }, /* 0xb1 */
	{ U64(0x02), U64(0x1d), U64(0x1a), U64(0x16), U64(0x2e), U64(0xd0), U64(0xa0), U64(0x60) }, /* 0xb2 */
	{ U64(0x02), U64(0x1d), U64(0x1a), U64(0x16), U64(0x2e), U64(0xd0), U64(0xa0), U64(0x60) }, /* 0xb3 */
	{ U64(0x06), U64(0x05), U64(0x1b), U64(0x14), U64(0x2c), U64(0xd0), U64(0xa0), U64(0x60) }, /* 0xb4 */
	{ U64(0x06), U64(0x05), U64(0x1b), U64(0x14), U64(0x2c), U64(0xd0), U64(0xa0), U64(0x60) }, /* 0xb5 */
	{ U64(0x02), U64(0x05), U64(0x1a), U64(0x14), U64(0x2c), U64(0xd0), U64(0xa0), U64(0x60) }, /* 0xb6 */
	{ U64(0x02), U64(0x05), U64(0x1a), U64(0x14), U64(0x2c), U64(0xd0), U64(0xa0), U64(0x60) }, /* 0xb7 */
	{ U64(0x0e), U64(0x0d), U64(0x0b), U64(0x17), U64(0x28), U64(0xd0), U64(0xa0), U64(0x60) }, /* 0xb8 */
	{ U64(0x0e), U64(0x0d), U64(0x0b), U64(0x17), U64(0x28), U64(0xd0), U64(0xa0), U64(0x60) }, /* 0xb9 */
	{ U64(0x02), U64(0x0d), U64(0x0a), U64(0x16), U64(0x28), U64(0xd0), U64(0xa0), U64(0x60) }, /* 0xba */
	{ U64(0x02), U64(0x0d), U64(0x0a), U64(0x16), U64(0x28), U64(0xd0), U64(0xa0), U64(0x60) }, /* 0xbb */
	{ U64(0x06), U64(0x05), U64(0x0b), U64(0x14), U64(0x28), U64(0xd0), U64(0xa0), U64(0x60) }, /* 0xbc */
	{ U64(0x06), U64(0x05), U64(0x0b), U64(0x14), U64(0x28), U64(0xd0), U64(0xa0), U64(0x60) }, /* 0xbd */
	{ U64(0x02), U64(0x05), U64(0x0a), U64(0x14), U64(0x28), U64(0xd0), U64(0xa0), U64(0x60) }, /* 0xbe */
	{ U64(0x02), U64(0x05), U64(0x0a), U64(0x14), U64(0x28), U64(0xd0), U64(0xa0), U64(0x60) }, /* 0xbf */
	{ U64(0x7e), U64(0x7d), U64(0x7b), U64(0x77), U64(0x6f), U64(0x5f), U64(0xbf), U64(0x40) }, /* 0xc0 */
	{ U64(0x7e), U64(0x7d), U64(0x7b), U64(0x77), U64(0x6f), U64(0x5f), U64(0xbf), U64(0x40) }, /* 0xc1 */
	{ U64(0x02), U64(0x7d), U64(0x7a), U64(0x76), U64(0x6e), U64(0x5e), U64(0xbe), U64(0x40) }, /* 0xc2 */
	{ U64(0x02), U64(0x7d), U64(0x7a), U64(0x76), U64(0x6e), U64(0x5e), U64(0xbe), U64(0x40) }, /* 0xc3 */
	{ U64(0x06), U64(0x05), U64(0x7b), U64(0x74), U64(0x6c), U64(0x5c), U64(0xbc), U64(0x40) }, /* 0xc4 */
	{ U64(0x06), U64(0x05), U64(0x7b), U64(0x74), U64(0x6c), U64(0x5c), U64(0xbc), U64(0x40) }, /* 0xc5 */
	{ U64(0x02), U64(0x05), U64(0x7a), U64(0x74), U64(0x6c), U64(0x5c), U64(0xbc), U64(0x40) }, /* 0xc6 */
	{ U64(0x02), U64(0x05), U64(0x7a), U64(0x74), U64(0x6c), U64(0x5c), U64(0xbc), U64(0x40) }, /* 0xc7 */
	{ U64(0x0e), U64(0x0d), U64(0x0b), U64(0x77), U64(0x68), U64(0x58), U64(0xb8), U64(0x40) }, /* 0xc8 */
	{ U64(0x0e), U64(0x0d), U64(0x0b), U64(0x77), U64(0x68), U64(0x58), U64(0xb8), U64(0x40) }, /* 0xc9 */
	{ U64(0x02), U64(0x0d), U64(0x0a), U64(0x76), U64(0x68), U64(0x58), U64(0xb8), U64(0x40) }, /* 0xca */
	{ U64(0x02), U64(0x0d), U64(0x0a), U64(0x76), U64(0x68), U64(0x58), U64(0xb8), U64(0x40) }, /* 0xcb */
	{ U64(0x06), U64(0x05), U64(0x0b), U64(0x74), U64(0x68), U64(0x58), U64(0xb8), U64(0x40) }, /* 0xcc */
	{ U64(0x06), U64(0x05), U64(0x0b), U64(0x74), U64(0x68), U64(0x58), U64(0xb8), U64(0x40) }, /* 0xcd */
	{ U64(0x02), U64(0x05), U64(0x0a), U64(0x74), U64(0x68), U64(0x58), U64(0xb8), U64(0x40) }, /* 0xce */
	{ U64(0x02), U64(0x05), U64(0x0a), U64(0x74), U64(0x68), U64(0x58), U64(0xb8), U64(0x40) }, /* 0xcf */
	{ U64(0x1e), U64(0x1d), U64(0x1b), U64(0x17), U64(0x6f), U64(0x50), U64(0xb0), U64(0x40) }, /* 0xd0 */
	{ U64(0x1e), U64(0x1d), U64(0x1b), U64(0x17), U64(0x6f), U64(0x50), U64(0xb0), U64(0x40) }, /* 0xd1 */
	{ U64(0x02), U64(0x1d), U64(0x1a), U64(0x16), U64(0x6e), U64(0x50), U64(0xb0), U64(0x40) }, /* 0xd2 */
	{ U64(0x02), U64(0x1d), U64(0x1a), U64(0x16), U64(0x6e), U64(0x50), U64(0xb0), U64(0x40) }, /* 0xd3 */
	{ U64(0x06), U64(0x05), U64(0x1b), U64(0x14), U64(0x6c), U64(0x50), U64(0xb0), U64(0x40) }, /* 0xd4 */
	{ U64(0x06), U64(0x05), U64(0x1b), U64(0x14), U64(0x6c), U64(0x50), U64(0xb0), U64(0x40) }, /* 0xd5 */
	{ U64(0x02), U64(0x05), U64(0x1a), U64(0x14), U64(0x6c), U64(0x50), U64(0xb0), U64(0x40) }, /* 0xd6 */
	{ U64(0x02), U64(0x05), U64(0x1a), U64(0x14), U64(0x6c), U64(0x50), U64(0xb0), U64(0x40) }, /* 0xd7 */
	{ U64(0x0e), U64(0x0d), U64(0x0b), U64(0x17), U64(0x68), U64(0x50), U64(0xb0), U64(0x40) }, /* 0xd8 */
	{ U64(0x0e), U64(0x0d), U64(0x0b), U64(0x17), U64(0x68), U64(0x50), U64(0xb0), U64(0x40) }, /* 0xd9 */
	{ U64(0x02), U64(0x0d), U64(0x0a), U64(0x16), U64(0x68), U64(0x50), U64(0xb0), U64(0x40) }, /* 0xda */
	{ U64(0x02), U64(0x0d), U64(0x0a), U64(0x16), U64(0x68), U64(0x50), U64(0xb0), U64(0x40) }, /* 0xdb */
	{ U64(0x06), U64(0x05), U64(0x0b), U64(0x14), U64(0x68), U64(0x50), U64(0xb0), U64(0x40) }, /* 0xdc */
	{ U64(0x06), U64(0x05), U64(0x0b), U64(0x14), U64(0x68), U64(0x50), U64(0xb0), U64(0x40) }, /* 0xdd */
	{ U64(0x02), U64(0x05), U64(0x0a), U64(0x14), U64(0x68), U64(0x50), U64(0xb0), U64(0x40) }, /* 0xde */
	{ U64(0x02), U64(0x05), U64(0x0a), U64(0x14), U64(0x68), U64(0x50), U64(0xb0), U64(0x40) }, /* 0xdf */
	{ U64(0x3e), U64(0x3d), U64(0x3b), U64(0x37), U64(0x2f), U64(0x5f), U64(0xa0), U64(0x40) }, /* 0xe0 */
	{ U64(0x3e), U64(0x3d), U64(0x3b), U64(0x37), U64(0x2f), U64(0x5f), U64(0xa0), U64(0x40) }, /* 0xe1 */
	{ U64(0x02), U64(0x3d), U64(0x3a), U64(0x36), U64(0x2e), U64(0x5e), U64(0xa0), U64(0x40) }, /* 0xe2 */
	{ U64(0x02), U64(0x3d), U64(0x3a), U64(0x36), U64(0x2e), U64(0x5e), U64(0xa0), U64(0x40) }, /* 0xe3 */
	{ U64(0x06), U64(0x05), U64(0x3b), U64(0x34), U64(0x2c), U64(0x5c), U64(0xa0), U64(0x40) }, /* 0xe4 */
	{ U64(0x06), U64(0x05), U64(0x3b), U64(0x34), U64(0x2c), U64(0x5c), U64(0xa0), U64(0x40) }, /* 0xe5 */
	{ U64(0x02), U64(0x05), U64(0x3a), U64(0x34), U64(0x2c), U64(0x5c), U64(0xa0), U64(0x40) }, /* 0xe6 */
	{ U64(0x02), U64(0x05), U64(0x3a), U64(0x34), U64(0x2c), U64(0x5c), U64(0xa0), U64(0x40) }, /* 0xe7 */
	{ U64(0x0e), U64(0x0d), U64(0x0b), U64(0x37), U64(0x28), U64(0x58), U64(0xa0), U64(0x40) }, /* 0xe8 */
	{ U64(0x0e), U64(0x0d), U64(0x0b), U64(0x37), U64(0x28), U64(0x58), U64(0xa0), U64(0x40) }, /* 0xe9 */
	{ U64(0x02), U64(0x0d), U64(0x0a), U64(0x36), U64(0x28), U64(0x58), U64(0xa0), U64(0x40) }, /* 0xea */
	{ U64(0x02), U64(0x0d), U64(0x0a), U64(0x36), U64(0x28), U64(0x58), U64(0xa0), U64(0x40) }, /* 0xeb */
	{ U64(0x06), U64(0x05), U64(0x0b), U64(0x34), U64(0x28), U64(0x58), U64(0xa0), U64(0x40) }, /* 0xec */
	{ U64(0x06), U64(0x05), U64(0x0b), U64(0x34), U64(0x28), U64(0x58), U64(0xa0), U64(0x40) }, /* 0xed */
	{ U64(0x02), U64(0x05), U64(0x0a), U64(0x34), U64(0x28), U64(0x58), U64(0xa0), U64(0x40) }, /* 0xee */
	{ U64(0x02), U64(0x05), U64(0x0a), U64(0x34), U64(0x28), U64(0x58), U64(0xa0), U64(0x40) }, /* 0xef */
	{ U64(0x1e), U64(0x1d), U64(0x1b), U64(0x17), U64(0x2f), U64(0x50), U64(0xa0), U64(0x40) }, /* 0xf0 */
	{ U64(0x1e), U64(0x1d), U64(0x1b), U64(0x17), U64(0x2f), U64(0x50), U64(0xa0), U64(0x40) }, /* 0xf1 */
	{ U64(0x02), U64(0x1d), U64(0x1a), U64(0x16), U64(0x2e), U64(0x50), U64(0xa0), U64(0x40) }, /* 0xf2 */
	{ U64(0x02), U64(0x1d), U64(0x1a), U64(0x16), U64(0x2e), U64(0x50), U64(0xa0), U64(0x40) }, /* 0xf3 */
	{ U64(0x06), U64(0x05), U64(0x1b), U64(0x14), U64(0x2c), U64(0x50), U64(0xa0), U64(0x40) }, /* 0xf4 */
	{ U64(0x06), U64(0x05), U64(0x1b), U64(0x14), U64(0x2c), U64(0x50), U64(0xa0), U64(0x40) }, /* 0xf5 */
	{ U64(0x02), U64(0x05), U64(0x1a), U64(0x14), U64(0x2c), U64(0x50), U64(0xa0), U64(0x40) }, /* 0xf6 */
	{ U64(0x02), U64(0x05), U64(0x1a), U64(0x14), U64(0x2c), U64(0x50), U64(0xa0), U64(0x40) }, /* 0xf7 */
	{ U64(0x0e), U64(0x0d), U64(0x0b), U64(0x17), U64(0x28), U64(0x50), U64(0xa0), U64(0x40) }, /* 0xf8 */
	{ U64(0x0e), U64(0x0d), U64(0x0b), U64(0x17), U64(0x28), U64(0x50), U64(0xa0), U64(0x40) }, /* 0xf9 */
	{ U64(0x02), U64(0x0d), U64(0x0a), U64(0x16), U64(0x28), U64(0x50), U64(0xa0), U64(0x40) }, /* 0xfa */
	{ U64(0x02), U64(0x0d), U64(0x0a), U64(0x16), U64(0x28), U64(0x50), U64(0xa0), U64(0x40) }, /* 0xfb */
	{ U64(0x06), U64(0x05), U64(0x0b), U64(0x14), U64(0x28), U64(0x50), U64(0xa0), U64(0x40) }, /* 0xfc */
	{ U64(0x06), U64(0x05), U64(0x0b), U64(0x14), U64(0x28), U64(0x50), U64(0xa0), U64(0x40) }, /* 0xfd */
	{ U64(0x02), U64(0x05), U64(0x0a), U64(0x14), U64(0x28), U64(0x50), U64(0xa0), U64(0x40) }, /* 0xfe */
	{ U64(0x02), U64(0x05), U64(0x0a), U64(0x14), U64(0x28), U64(0x50), U64(0xa0), U64(0x40) }  /* 0xff */
};


/****************************************************************************
 * For rook attacks along a column. Using the 'occupied' status of the column
 * passed in from the rot90 bitboard, shifted to the 8 least-significant bits
 * (a-column), returns a NON-ROTATED bitmask of attacks for the a-column. You
 * will want to shift this result over by COL(square) bits to get it in the
 * right place - think about the way the bitboard works; each row is a byte
 * (major index, times 8), and each column is a bit (minor index, times 1).
 * You will want to use whatever ROW the piece is on in a normal bitboard as
 * the second index.
 ****************************************************************************/

uint64_t colattacks[256][8] = {
	{ U64(0x0101010101010100), U64(0x0101010101010001), U64(0x0101010101000101), U64(0x0101010100010101), U64(0x0101010001010101), U64(0x0101000101010101), U64(0x0100010101010101), U64(0x0001010101010101) }, /* 0x00 */
	{ U64(0x0101010101010100), U64(0x0101010101010001), U64(0x0101010101000101), U64(0x0101010100010101), U64(0x0101010001010101), U64(0x0101000101010101), U64(0x0100010101010101), U64(0x0001010101010101) }, /* 0x01 */
	{ U64(0x0000000000000100), U64(0x0101010101010001), U64(0x0101010101000100), U64(0x0101010100010100), U64(0x0101010001010100), U64(0x0101000101010100), U64(0x0100010101010100), U64(0x0001010101010100) }, /* 0x02 */
	{ U64(0x0000000000000100), U64(0x0101010101010001), U64(0x0101010101000100), U64(0x0101010100010100), U64(0x0101010001010100), U64(0x0101000101010100), U64(0x0100010101010100), U64(0x0001010101010100) }, /* 0x03 */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0101010101000101), U64(0x0101010100010000), U64(0x0101010001010000), U64(0x0101000101010000), U64(0x0100010101010000), U64(0x0001010101010000) }, /* 0x04 */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0101010101000101), U64(0x0101010100010000), U64(0x0101010001010000), U64(0x0101000101010000), U64(0x0100010101010000), U64(0x0001010101010000) }, /* 0x05 */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0101010101000100), U64(0x0101010100010000), U64(0x0101010001010000), U64(0x0101000101010000), U64(0x0100010101010000), U64(0x0001010101010000) }, /* 0x06 */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0101010101000100), U64(0x0101010100010000), U64(0x0101010001010000), U64(0x0101000101010000), U64(0x0100010101010000), U64(0x0001010101010000) }, /* 0x07 */
	{ U64(0x0000000001010100), U64(0x0000000001010001), U64(0x0000000001000101), U64(0x0101010100010101), U64(0x0101010001000000), U64(0x0101000101000000), U64(0x0100010101000000), U64(0x0001010101000000) }, /* 0x08 */
	{ U64(0x0000000001010100), U64(0x0000000001010001), U64(0x0000000001000101), U64(0x0101010100010101), U64(0x0101010001000000), U64(0x0101000101000000), U64(0x0100010101000000), U64(0x0001010101000000) }, /* 0x09 */
	{ U64(0x0000000000000100), U64(0x0000000001010001), U64(0x0000000001000100), U64(0x0101010100010100), U64(0x0101010001000000), U64(0x0101000101000000), U64(0x0100010101000000), U64(0x0001010101000000) }, /* 0x0a */
	{ U64(0x0000000000000100), U64(0x0000000001010001), U64(0x0000000001000100), U64(0x0101010100010100), U64(0x0101010001000000), U64(0x0101000101000000), U64(0x0100010101000000), U64(0x0001010101000000) }, /* 0x0b */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0000000001000101), U64(0x0101010100010000), U64(0x0101010001000000), U64(0x0101000101000000), U64(0x0100010101000000), U64(0x0001010101000000) }, /* 0x0c */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0000000001000101), U64(0x0101010100010000), U64(0x0101010001000000), U64(0x0101000101000000), U64(0x0100010101000000), U64(0x0001010101000000) }, /* 0x0d */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0000000001000100), U64(0x0101010100010000), U64(0x0101010001000000), U64(0x0101000101000000), U64(0x0100010101000000), U64(0x0001010101000000) }, /* 0x0e */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0000000001000100), U64(0x0101010100010000), U64(0x0101010001000000), U64(0x0101000101000000), U64(0x0100010101000000), U64(0x0001010101000000) }, /* 0x0f */
	{ U64(0x0000000101010100), U64(0x0000000101010001), U64(0x0000000101000101), U64(0x0000000100010101), U64(0x0101010001010101), U64(0x0101000100000000), U64(0x0100010100000000), U64(0x0001010100000000) }, /* 0x10 */
	{ U64(0x0000000101010100), U64(0x0000000101010001), U64(0x0000000101000101), U64(0x0000000100010101), U64(0x0101010001010101), U64(0x0101000100000000), U64(0x0100010100000000), U64(0x0001010100000000) }, /* 0x11 */
	{ U64(0x0000000000000100), U64(0x0000000101010001), U64(0x0000000101000100), U64(0x0000000100010100), U64(0x0101010001010100), U64(0x0101000100000000), U64(0x0100010100000000), U64(0x0001010100000000) }, /* 0x12 */
	{ U64(0x0000000000000100), U64(0x0000000101010001), U64(0x0000000101000100), U64(0x0000000100010100), U64(0x0101010001010100), U64(0x0101000100000000), U64(0x0100010100000000), U64(0x0001010100000000) }, /* 0x13 */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0000000101000101), U64(0x0000000100010000), U64(0x0101010001010000), U64(0x0101000100000000), U64(0x0100010100000000), U64(0x0001010100000000) }, /* 0x14 */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0000000101000101), U64(0x0000000100010000), U64(0x0101010001010000), U64(0x0101000100000000), U64(0x0100010100000000), U64(0x0001010100000000) }, /* 0x15 */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0000000101000100), U64(0x0000000100010000), U64(0x0101010001010000), U64(0x0101000100000000), U64(0x0100010100000000), U64(0x0001010100000000) }, /* 0x16 */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0000000101000100), U64(0x0000000100010000), U64(0x0101010001010000), U64(0x0101000100000000), U64(0x0100010100000000), U64(0x0001010100000000) }, /* 0x17 */
	{ U64(0x0000000001010100), U64(0x0000000001010001), U64(0x0000000001000101), U64(0x0000000100010101), U64(0x0101010001000000), U64(0x0101000100000000), U64(0x0100010100000000), U64(0x0001010100000000) }, /* 0x18 */
	{ U64(0x0000000001010100), U64(0x0000000001010001), U64(0x0000000001000101), U64(0x0000000100010101), U64(0x0101010001000000), U64(0x0101000100000000), U64(0x0100010100000000), U64(0x0001010100000000) }, /* 0x19 */
	{ U64(0x0000000000000100), U64(0x0000000001010001), U64(0x0000000001000100), U64(0x0000000100010100), U64(0x0101010001000000), U64(0x0101000100000000), U64(0x0100010100000000), U64(0x0001010100000000) }, /* 0x1a */
	{ U64(0x0000000000000100), U64(0x0000000001010001), U64(0x0000000001000100), U64(0x0000000100010100), U64(0x0101010001000000), U64(0x0101000100000000), U64(0x0100010100000000), U64(0x0001010100000000) }, /* 0x1b */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0000000001000101), U64(0x0000000100010000), U64(0x0101010001000000), U64(0x0101000100000000), U64(0x0100010100000000), U64(0x0001010100000000) }, /* 0x1c */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0000000001000101), U64(0x0000000100010000), U64(0x0101010001000000), U64(0x0101000100000000), U64(0x0100010100000000), U64(0x0001010100000000) }, /* 0x1d */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0000000001000100), U64(0x0000000100010000), U64(0x0101010001000000), U64(0x0101000100000000), U64(0x0100010100000000), U64(0x0001010100000000) }, /* 0x1e */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0000000001000100), U64(0x0000000100010000), U64(0x0101010001000000), U64(0x0101000100000000), U64(0x0100010100000000), U64(0x0001010100000000) }, /* 0x1f */
	{ U64(0x0000010101010100), U64(0x0000010101010001), U64(0x0000010101000101), U64(0x0000010100010101), U64(0x0000010001010101), U64(0x0101000101010101), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0x20 */
	{ U64(0x0000010101010100), U64(0x0000010101010001), U64(0x0000010101000101), U64(0x0000010100010101), U64(0x0000010001010101), U64(0x0101000101010101), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0x21 */
	{ U64(0x0000000000000100), U64(0x0000010101010001), U64(0x0000010101000100), U64(0x0000010100010100), U64(0x0000010001010100), U64(0x0101000101010100), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0x22 */
	{ U64(0x0000000000000100), U64(0x0000010101010001), U64(0x0000010101000100), U64(0x0000010100010100), U64(0x0000010001010100), U64(0x0101000101010100), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0x23 */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0000010101000101), U64(0x0000010100010000), U64(0x0000010001010000), U64(0x0101000101010000), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0x24 */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0000010101000101), U64(0x0000010100010000), U64(0x0000010001010000), U64(0x0101000101010000), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0x25 */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0000010101000100), U64(0x0000010100010000), U64(0x0000010001010000), U64(0x0101000101010000), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0x26 */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0000010101000100), U64(0x0000010100010000), U64(0x0000010001010000), U64(0x0101000101010000), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0x27 */
	{ U64(0x0000000001010100), U64(0x0000000001010001), U64(0x0000000001000101), U64(0x0000010100010101), U64(0x0000010001000000), U64(0x0101000101000000), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0x28 */
	{ U64(0x0000000001010100), U64(0x0000000001010001), U64(0x0000000001000101), U64(0x0000010100010101), U64(0x0000010001000000), U64(0x0101000101000000), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0x29 */
	{ U64(0x0000000000000100), U64(0x0000000001010001), U64(0x0000000001000100), U64(0x0000010100010100), U64(0x0000010001000000), U64(0x0101000101000000), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0x2a */
	{ U64(0x0000000000000100), U64(0x0000000001010001), U64(0x0000000001000100), U64(0x0000010100010100), U64(0x0000010001000000), U64(0x0101000101000000), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0x2b */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0000000001000101), U64(0x0000010100010000), U64(0x0000010001000000), U64(0x0101000101000000), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0x2c */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0000000001000101), U64(0x0000010100010000), U64(0x0000010001000000), U64(0x0101000101000000), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0x2d */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0000000001000100), U64(0x0000010100010000), U64(0x0000010001000000), U64(0x0101000101000000), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0x2e */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0000000001000100), U64(0x0000010100010000), U64(0x0000010001000000), U64(0x0101000101000000), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0x2f */
	{ U64(0x0000000101010100), U64(0x0000000101010001), U64(0x0000000101000101), U64(0x0000000100010101), U64(0x0000010001010101), U64(0x0101000100000000), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0x30 */
	{ U64(0x0000000101010100), U64(0x0000000101010001), U64(0x0000000101000101), U64(0x0000000100010101), U64(0x0000010001010101), U64(0x0101000100000000), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0x31 */
	{ U64(0x0000000000000100), U64(0x0000000101010001), U64(0x0000000101000100), U64(0x0000000100010100), U64(0x0000010001010100), U64(0x0101000100000000), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0x32 */
	{ U64(0x0000000000000100), U64(0x0000000101010001), U64(0x0000000101000100), U64(0x0000000100010100), U64(0x0000010001010100), U64(0x0101000100000000), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0x33 */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0000000101000101), U64(0x0000000100010000), U64(0x0000010001010000), U64(0x0101000100000000), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0x34 */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0000000101000101), U64(0x0000000100010000), U64(0x0000010001010000), U64(0x0101000100000000), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0x35 */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0000000101000100), U64(0x0000000100010000), U64(0x0000010001010000), U64(0x0101000100000000), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0x36 */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0000000101000100), U64(0x0000000100010000), U64(0x0000010001010000), U64(0x0101000100000000), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0x37 */
	{ U64(0x0000000001010100), U64(0x0000000001010001), U64(0x0000000001000101), U64(0x0000000100010101), U64(0x0000010001000000), U64(0x0101000100000000), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0x38 */
	{ U64(0x0000000001010100), U64(0x0000000001010001), U64(0x0000000001000101), U64(0x0000000100010101), U64(0x0000010001000000), U64(0x0101000100000000), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0x39 */
	{ U64(0x0000000000000100), U64(0x0000000001010001), U64(0x0000000001000100), U64(0x0000000100010100), U64(0x0000010001000000), U64(0x0101000100000000), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0x3a */
	{ U64(0x0000000000000100), U64(0x0000000001010001), U64(0x0000000001000100), U64(0x0000000100010100), U64(0x0000010001000000), U64(0x0101000100000000), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0x3b */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0000000001000101), U64(0x0000000100010000), U64(0x0000010001000000), U64(0x0101000100000000), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0x3c */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0000000001000101), U64(0x0000000100010000), U64(0x0000010001000000), U64(0x0101000100000000), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0x3d */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0000000001000100), U64(0x0000000100010000), U64(0x0000010001000000), U64(0x0101000100000000), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0x3e */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0000000001000100), U64(0x0000000100010000), U64(0x0000010001000000), U64(0x0101000100000000), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0x3f */
	{ U64(0x0001010101010100), U64(0x0001010101010001), U64(0x0001010101000101), U64(0x0001010100010101), U64(0x0001010001010101), U64(0x0001000101010101), U64(0x0100010101010101), U64(0x0001000000000000) }, /* 0x40 */
	{ U64(0x0001010101010100), U64(0x0001010101010001), U64(0x0001010101000101), U64(0x0001010100010101), U64(0x0001010001010101), U64(0x0001000101010101), U64(0x0100010101010101), U64(0x0001000000000000) }, /* 0x41 */
	{ U64(0x0000000000000100), U64(0x0001010101010001), U64(0x0001010101000100), U64(0x0001010100010100), U64(0x0001010001010100), U64(0x0001000101010100), U64(0x0100010101010100), U64(0x0001000000000000) }, /* 0x42 */
	{ U64(0x0000000000000100), U64(0x0001010101010001), U64(0x0001010101000100), U64(0x0001010100010100), U64(0x0001010001010100), U64(0x0001000101010100), U64(0x0100010101010100), U64(0x0001000000000000) }, /* 0x43 */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0001010101000101), U64(0x0001010100010000), U64(0x0001010001010000), U64(0x0001000101010000), U64(0x0100010101010000), U64(0x0001000000000000) }, /* 0x44 */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0001010101000101), U64(0x0001010100010000), U64(0x0001010001010000), U64(0x0001000101010000), U64(0x0100010101010000), U64(0x0001000000000000) }, /* 0x45 */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0001010101000100), U64(0x0001010100010000), U64(0x0001010001010000), U64(0x0001000101010000), U64(0x0100010101010000), U64(0x0001000000000000) }, /* 0x46 */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0001010101000100), U64(0x0001010100010000), U64(0x0001010001010000), U64(0x0001000101010000), U64(0x0100010101010000), U64(0x0001000000000000) }, /* 0x47 */
	{ U64(0x0000000001010100), U64(0x0000000001010001), U64(0x0000000001000101), U64(0x0001010100010101), U64(0x0001010001000000), U64(0x0001000101000000), U64(0x0100010101000000), U64(0x0001000000000000) }, /* 0x48 */
	{ U64(0x0000000001010100), U64(0x0000000001010001), U64(0x0000000001000101), U64(0x0001010100010101), U64(0x0001010001000000), U64(0x0001000101000000), U64(0x0100010101000000), U64(0x0001000000000000) }, /* 0x49 */
	{ U64(0x0000000000000100), U64(0x0000000001010001), U64(0x0000000001000100), U64(0x0001010100010100), U64(0x0001010001000000), U64(0x0001000101000000), U64(0x0100010101000000), U64(0x0001000000000000) }, /* 0x4a */
	{ U64(0x0000000000000100), U64(0x0000000001010001), U64(0x0000000001000100), U64(0x0001010100010100), U64(0x0001010001000000), U64(0x0001000101000000), U64(0x0100010101000000), U64(0x0001000000000000) }, /* 0x4b */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0000000001000101), U64(0x0001010100010000), U64(0x0001010001000000), U64(0x0001000101000000), U64(0x0100010101000000), U64(0x0001000000000000) }, /* 0x4c */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0000000001000101), U64(0x0001010100010000), U64(0x0001010001000000), U64(0x0001000101000000), U64(0x0100010101000000), U64(0x0001000000000000) }, /* 0x4d */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0000000001000100), U64(0x0001010100010000), U64(0x0001010001000000), U64(0x0001000101000000), U64(0x0100010101000000), U64(0x0001000000000000) }, /* 0x4e */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0000000001000100), U64(0x0001010100010000), U64(0x0001010001000000), U64(0x0001000101000000), U64(0x0100010101000000), U64(0x0001000000000000) }, /* 0x4f */
	{ U64(0x0000000101010100), U64(0x0000000101010001), U64(0x0000000101000101), U64(0x0000000100010101), U64(0x0001010001010101), U64(0x0001000100000000), U64(0x0100010100000000), U64(0x0001000000000000) }, /* 0x50 */
	{ U64(0x0000000101010100), U64(0x0000000101010001), U64(0x0000000101000101), U64(0x0000000100010101), U64(0x0001010001010101), U64(0x0001000100000000), U64(0x0100010100000000), U64(0x0001000000000000) }, /* 0x51 */
	{ U64(0x0000000000000100), U64(0x0000000101010001), U64(0x0000000101000100), U64(0x0000000100010100), U64(0x0001010001010100), U64(0x0001000100000000), U64(0x0100010100000000), U64(0x0001000000000000) }, /* 0x52 */
	{ U64(0x0000000000000100), U64(0x0000000101010001), U64(0x0000000101000100), U64(0x0000000100010100), U64(0x0001010001010100), U64(0x0001000100000000), U64(0x0100010100000000), U64(0x0001000000000000) }, /* 0x53 */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0000000101000101), U64(0x0000000100010000), U64(0x0001010001010000), U64(0x0001000100000000), U64(0x0100010100000000), U64(0x0001000000000000) }, /* 0x54 */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0000000101000101), U64(0x0000000100010000), U64(0x0001010001010000), U64(0x0001000100000000), U64(0x0100010100000000), U64(0x0001000000000000) }, /* 0x55 */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0000000101000100), U64(0x0000000100010000), U64(0x0001010001010000), U64(0x0001000100000000), U64(0x0100010100000000), U64(0x0001000000000000) }, /* 0x56 */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0000000101000100), U64(0x0000000100010000), U64(0x0001010001010000), U64(0x0001000100000000), U64(0x0100010100000000), U64(0x0001000000000000) }, /* 0x57 */
	{ U64(0x0000000001010100), U64(0x0000000001010001), U64(0x0000000001000101), U64(0x0000000100010101), U64(0x0001010001000000), U64(0x0001000100000000), U64(0x0100010100000000), U64(0x0001000000000000) }, /* 0x58 */
	{ U64(0x0000000001010100), U64(0x0000000001010001), U64(0x0000000001000101), U64(0x0000000100010101), U64(0x0001010001000000), U64(0x0001000100000000), U64(0x0100010100000000), U64(0x0001000000000000) }, /* 0x59 */
	{ U64(0x0000000000000100), U64(0x0000000001010001), U64(0x0000000001000100), U64(0x0000000100010100), U64(0x0001010001000000), U64(0x0001000100000000), U64(0x0100010100000000), U64(0x0001000000000000) }, /* 0x5a */
	{ U64(0x0000000000000100), U64(0x0000000001010001), U64(0x0000000001000100), U64(0x0000000100010100), U64(0x0001010001000000), U64(0x0001000100000000), U64(0x0100010100000000), U64(0x0001000000000000) }, /* 0x5b */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0000000001000101), U64(0x0000000100010000), U64(0x0001010001000000), U64(0x0001000100000000), U64(0x0100010100000000), U64(0x0001000000000000) }, /* 0x5c */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0000000001000101), U64(0x0000000100010000), U64(0x0001010001000000), U64(0x0001000100000000), U64(0x0100010100000000), U64(0x0001000000000000) }, /* 0x5d */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0000000001000100), U64(0x0000000100010000), U64(0x0001010001000000), U64(0x0001000100000000), U64(0x0100010100000000), U64(0x0001000000000000) }, /* 0x5e */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0000000001000100), U64(0x0000000100010000), U64(0x0001010001000000), U64(0x0001000100000000), U64(0x0100010100000000), U64(0x0001000000000000) }, /* 0x5f */
	{ U64(0x0000010101010100), U64(0x0000010101010001), U64(0x0000010101000101), U64(0x0000010100010101), U64(0x0000010001010101), U64(0x0001000101010101), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0x60 */
	{ U64(0x0000010101010100), U64(0x0000010101010001), U64(0x0000010101000101), U64(0x0000010100010101), U64(0x0000010001010101), U64(0x0001000101010101), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0x61 */
	{ U64(0x0000000000000100), U64(0x0000010101010001), U64(0x0000010101000100), U64(0x0000010100010100), U64(0x0000010001010100), U64(0x0001000101010100), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0x62 */
	{ U64(0x0000000000000100), U64(0x0000010101010001), U64(0x0000010101000100), U64(0x0000010100010100), U64(0x0000010001010100), U64(0x0001000101010100), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0x63 */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0000010101000101), U64(0x0000010100010000), U64(0x0000010001010000), U64(0x0001000101010000), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0x64 */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0000010101000101), U64(0x0000010100010000), U64(0x0000010001010000), U64(0x0001000101010000), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0x65 */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0000010101000100), U64(0x0000010100010000), U64(0x0000010001010000), U64(0x0001000101010000), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0x66 */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0000010101000100), U64(0x0000010100010000), U64(0x0000010001010000), U64(0x0001000101010000), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0x67 */
	{ U64(0x0000000001010100), U64(0x0000000001010001), U64(0x0000000001000101), U64(0x0000010100010101), U64(0x0000010001000000), U64(0x0001000101000000), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0x68 */
	{ U64(0x0000000001010100), U64(0x0000000001010001), U64(0x0000000001000101), U64(0x0000010100010101), U64(0x0000010001000000), U64(0x0001000101000000), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0x69 */
	{ U64(0x0000000000000100), U64(0x0000000001010001), U64(0x0000000001000100), U64(0x0000010100010100), U64(0x0000010001000000), U64(0x0001000101000000), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0x6a */
	{ U64(0x0000000000000100), U64(0x0000000001010001), U64(0x0000000001000100), U64(0x0000010100010100), U64(0x0000010001000000), U64(0x0001000101000000), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0x6b */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0000000001000101), U64(0x0000010100010000), U64(0x0000010001000000), U64(0x0001000101000000), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0x6c */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0000000001000101), U64(0x0000010100010000), U64(0x0000010001000000), U64(0x0001000101000000), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0x6d */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0000000001000100), U64(0x0000010100010000), U64(0x0000010001000000), U64(0x0001000101000000), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0x6e */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0000000001000100), U64(0x0000010100010000), U64(0x0000010001000000), U64(0x0001000101000000), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0x6f */
	{ U64(0x0000000101010100), U64(0x0000000101010001), U64(0x0000000101000101), U64(0x0000000100010101), U64(0x0000010001010101), U64(0x0001000100000000), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0x70 */
	{ U64(0x0000000101010100), U64(0x0000000101010001), U64(0x0000000101000101), U64(0x0000000100010101), U64(0x0000010001010101), U64(0x0001000100000000), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0x71 */
	{ U64(0x0000000000000100), U64(0x0000000101010001), U64(0x0000000101000100), U64(0x0000000100010100), U64(0x0000010001010100), U64(0x0001000100000000), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0x72 */
	{ U64(0x0000000000000100), U64(0x0000000101010001), U64(0x0000000101000100), U64(0x0000000100010100), U64(0x0000010001010100), U64(0x0001000100000000), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0x73 */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0000000101000101), U64(0x0000000100010000), U64(0x0000010001010000), U64(0x0001000100000000), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0x74 */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0000000101000101), U64(0x0000000100010000), U64(0x0000010001010000), U64(0x0001000100000000), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0x75 */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0000000101000100), U64(0x0000000100010000), U64(0x0000010001010000), U64(0x0001000100000000), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0x76 */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0000000101000100), U64(0x0000000100010000), U64(0x0000010001010000), U64(0x0001000100000000), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0x77 */
	{ U64(0x0000000001010100), U64(0x0000000001010001), U64(0x0000000001000101), U64(0x0000000100010101), U64(0x0000010001000000), U64(0x0001000100000000), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0x78 */
	{ U64(0x0000000001010100), U64(0x0000000001010001), U64(0x0000000001000101), U64(0x0000000100010101), U64(0x0000010001000000), U64(0x0001000100000000), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0x79 */
	{ U64(0x0000000000000100), U64(0x0000000001010001), U64(0x0000000001000100), U64(0x0000000100010100), U64(0x0000010001000000), U64(0x0001000100000000), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0x7a */
	{ U64(0x0000000000000100), U64(0x0000000001010001), U64(0x0000000001000100), U64(0x0000000100010100), U64(0x0000010001000000), U64(0x0001000100000000), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0x7b */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0000000001000101), U64(0x0000000100010000), U64(0x0000010001000000), U64(0x0001000100000000), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0x7c */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0000000001000101), U64(0x0000000100010000), U64(0x0000010001000000), U64(0x0001000100000000), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0x7d */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0000000001000100), U64(0x0000000100010000), U64(0x0000010001000000), U64(0x0001000100000000), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0x7e */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0000000001000100), U64(0x0000000100010000), U64(0x0000010001000000), U64(0x0001000100000000), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0x7f */
	{ U64(0x0101010101010100), U64(0x0101010101010001), U64(0x0101010101000101), U64(0x0101010100010101), U64(0x0101010001010101), U64(0x0101000101010101), U64(0x0100010101010101), U64(0x0001010101010101) }, /* 0x80 */
	{ U64(0x0101010101010100), U64(0x0101010101010001), U64(0x0101010101000101), U64(0x0101010100010101), U64(0x0101010001010101), U64(0x0101000101010101), U64(0x0100010101010101), U64(0x0001010101010101) }, /* 0x81 */
	{ U64(0x0000000000000100), U64(0x0101010101010001), U64(0x0101010101000100), U64(0x0101010100010100), U64(0x0101010001010100), U64(0x0101000101010100), U64(0x0100010101010100), U64(0x0001010101010100) }, /* 0x82 */
	{ U64(0x0000000000000100), U64(0x0101010101010001), U64(0x0101010101000100), U64(0x0101010100010100), U64(0x0101010001010100), U64(0x0101000101010100), U64(0x0100010101010100), U64(0x0001010101010100) }, /* 0x83 */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0101010101000101), U64(0x0101010100010000), U64(0x0101010001010000), U64(0x0101000101010000), U64(0x0100010101010000), U64(0x0001010101010000) }, /* 0x84 */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0101010101000101), U64(0x0101010100010000), U64(0x0101010001010000), U64(0x0101000101010000), U64(0x0100010101010000), U64(0x0001010101010000) }, /* 0x85 */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0101010101000100), U64(0x0101010100010000), U64(0x0101010001010000), U64(0x0101000101010000), U64(0x0100010101010000), U64(0x0001010101010000) }, /* 0x86 */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0101010101000100), U64(0x0101010100010000), U64(0x0101010001010000), U64(0x0101000101010000), U64(0x0100010101010000), U64(0x0001010101010000) }, /* 0x87 */
	{ U64(0x0000000001010100), U64(0x0000000001010001), U64(0x0000000001000101), U64(0x0101010100010101), U64(0x0101010001000000), U64(0x0101000101000000), U64(0x0100010101000000), U64(0x0001010101000000) }, /* 0x88 */
	{ U64(0x0000000001010100), U64(0x0000000001010001), U64(0x0000000001000101), U64(0x0101010100010101), U64(0x0101010001000000), U64(0x0101000101000000), U64(0x0100010101000000), U64(0x0001010101000000) }, /* 0x89 */
	{ U64(0x0000000000000100), U64(0x0000000001010001), U64(0x0000000001000100), U64(0x0101010100010100), U64(0x0101010001000000), U64(0x0101000101000000), U64(0x0100010101000000), U64(0x0001010101000000) }, /* 0x8a */
	{ U64(0x0000000000000100), U64(0x0000000001010001), U64(0x0000000001000100), U64(0x0101010100010100), U64(0x0101010001000000), U64(0x0101000101000000), U64(0x0100010101000000), U64(0x0001010101000000) }, /* 0x8b */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0000000001000101), U64(0x0101010100010000), U64(0x0101010001000000), U64(0x0101000101000000), U64(0x0100010101000000), U64(0x0001010101000000) }, /* 0x8c */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0000000001000101), U64(0x0101010100010000), U64(0x0101010001000000), U64(0x0101000101000000), U64(0x0100010101000000), U64(0x0001010101000000) }, /* 0x8d */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0000000001000100), U64(0x0101010100010000), U64(0x0101010001000000), U64(0x0101000101000000), U64(0x0100010101000000), U64(0x0001010101000000) }, /* 0x8e */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0000000001000100), U64(0x0101010100010000), U64(0x0101010001000000), U64(0x0101000101000000), U64(0x0100010101000000), U64(0x0001010101000000) }, /* 0x8f */
	{ U64(0x0000000101010100), U64(0x0000000101010001), U64(0x0000000101000101), U64(0x0000000100010101), U64(0x0101010001010101), U64(0x0101000100000000), U64(0x0100010100000000), U64(0x0001010100000000) }, /* 0x90 */
	{ U64(0x0000000101010100), U64(0x0000000101010001), U64(0x0000000101000101), U64(0x0000000100010101), U64(0x0101010001010101), U64(0x0101000100000000), U64(0x0100010100000000), U64(0x0001010100000000) }, /* 0x91 */
	{ U64(0x0000000000000100), U64(0x0000000101010001), U64(0x0000000101000100), U64(0x0000000100010100), U64(0x0101010001010100), U64(0x0101000100000000), U64(0x0100010100000000), U64(0x0001010100000000) }, /* 0x92 */
	{ U64(0x0000000000000100), U64(0x0000000101010001), U64(0x0000000101000100), U64(0x0000000100010100), U64(0x0101010001010100), U64(0x0101000100000000), U64(0x0100010100000000), U64(0x0001010100000000) }, /* 0x93 */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0000000101000101), U64(0x0000000100010000), U64(0x0101010001010000), U64(0x0101000100000000), U64(0x0100010100000000), U64(0x0001010100000000) }, /* 0x94 */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0000000101000101), U64(0x0000000100010000), U64(0x0101010001010000), U64(0x0101000100000000), U64(0x0100010100000000), U64(0x0001010100000000) }, /* 0x95 */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0000000101000100), U64(0x0000000100010000), U64(0x0101010001010000), U64(0x0101000100000000), U64(0x0100010100000000), U64(0x0001010100000000) }, /* 0x96 */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0000000101000100), U64(0x0000000100010000), U64(0x0101010001010000), U64(0x0101000100000000), U64(0x0100010100000000), U64(0x0001010100000000) }, /* 0x97 */
	{ U64(0x0000000001010100), U64(0x0000000001010001), U64(0x0000000001000101), U64(0x0000000100010101), U64(0x0101010001000000), U64(0x0101000100000000), U64(0x0100010100000000), U64(0x0001010100000000) }, /* 0x98 */
	{ U64(0x0000000001010100), U64(0x0000000001010001), U64(0x0000000001000101), U64(0x0000000100010101), U64(0x0101010001000000), U64(0x0101000100000000), U64(0x0100010100000000), U64(0x0001010100000000) }, /* 0x99 */
	{ U64(0x0000000000000100), U64(0x0000000001010001), U64(0x0000000001000100), U64(0x0000000100010100), U64(0x0101010001000000), U64(0x0101000100000000), U64(0x0100010100000000), U64(0x0001010100000000) }, /* 0x9a */
	{ U64(0x0000000000000100), U64(0x0000000001010001), U64(0x0000000001000100), U64(0x0000000100010100), U64(0x0101010001000000), U64(0x0101000100000000), U64(0x0100010100000000), U64(0x0001010100000000) }, /* 0x9b */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0000000001000101), U64(0x0000000100010000), U64(0x0101010001000000), U64(0x0101000100000000), U64(0x0100010100000000), U64(0x0001010100000000) }, /* 0x9c */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0000000001000101), U64(0x0000000100010000), U64(0x0101010001000000), U64(0x0101000100000000), U64(0x0100010100000000), U64(0x0001010100000000) }, /* 0x9d */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0000000001000100), U64(0x0000000100010000), U64(0x0101010001000000), U64(0x0101000100000000), U64(0x0100010100000000), U64(0x0001010100000000) }, /* 0x9e */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0000000001000100), U64(0x0000000100010000), U64(0x0101010001000000), U64(0x0101000100000000), U64(0x0100010100000000), U64(0x0001010100000000) }, /* 0x9f */
	{ U64(0x0000010101010100), U64(0x0000010101010001), U64(0x0000010101000101), U64(0x0000010100010101), U64(0x0000010001010101), U64(0x0101000101010101), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0xa0 */
	{ U64(0x0000010101010100), U64(0x0000010101010001), U64(0x0000010101000101), U64(0x0000010100010101), U64(0x0000010001010101), U64(0x0101000101010101), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0xa1 */
	{ U64(0x0000000000000100), U64(0x0000010101010001), U64(0x0000010101000100), U64(0x0000010100010100), U64(0x0000010001010100), U64(0x0101000101010100), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0xa2 */
	{ U64(0x0000000000000100), U64(0x0000010101010001), U64(0x0000010101000100), U64(0x0000010100010100), U64(0x0000010001010100), U64(0x0101000101010100), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0xa3 */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0000010101000101), U64(0x0000010100010000), U64(0x0000010001010000), U64(0x0101000101010000), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0xa4 */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0000010101000101), U64(0x0000010100010000), U64(0x0000010001010000), U64(0x0101000101010000), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0xa5 */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0000010101000100), U64(0x0000010100010000), U64(0x0000010001010000), U64(0x0101000101010000), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0xa6 */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0000010101000100), U64(0x0000010100010000), U64(0x0000010001010000), U64(0x0101000101010000), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0xa7 */
	{ U64(0x0000000001010100), U64(0x0000000001010001), U64(0x0000000001000101), U64(0x0000010100010101), U64(0x0000010001000000), U64(0x0101000101000000), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0xa8 */
	{ U64(0x0000000001010100), U64(0x0000000001010001), U64(0x0000000001000101), U64(0x0000010100010101), U64(0x0000010001000000), U64(0x0101000101000000), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0xa9 */
	{ U64(0x0000000000000100), U64(0x0000000001010001), U64(0x0000000001000100), U64(0x0000010100010100), U64(0x0000010001000000), U64(0x0101000101000000), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0xaa */
	{ U64(0x0000000000000100), U64(0x0000000001010001), U64(0x0000000001000100), U64(0x0000010100010100), U64(0x0000010001000000), U64(0x0101000101000000), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0xab */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0000000001000101), U64(0x0000010100010000), U64(0x0000010001000000), U64(0x0101000101000000), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0xac */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0000000001000101), U64(0x0000010100010000), U64(0x0000010001000000), U64(0x0101000101000000), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0xad */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0000000001000100), U64(0x0000010100010000), U64(0x0000010001000000), U64(0x0101000101000000), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0xae */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0000000001000100), U64(0x0000010100010000), U64(0x0000010001000000), U64(0x0101000101000000), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0xaf */
	{ U64(0x0000000101010100), U64(0x0000000101010001), U64(0x0000000101000101), U64(0x0000000100010101), U64(0x0000010001010101), U64(0x0101000100000000), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0xb0 */
	{ U64(0x0000000101010100), U64(0x0000000101010001), U64(0x0000000101000101), U64(0x0000000100010101), U64(0x0000010001010101), U64(0x0101000100000000), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0xb1 */
	{ U64(0x0000000000000100), U64(0x0000000101010001), U64(0x0000000101000100), U64(0x0000000100010100), U64(0x0000010001010100), U64(0x0101000100000000), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0xb2 */
	{ U64(0x0000000000000100), U64(0x0000000101010001), U64(0x0000000101000100), U64(0x0000000100010100), U64(0x0000010001010100), U64(0x0101000100000000), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0xb3 */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0000000101000101), U64(0x0000000100010000), U64(0x0000010001010000), U64(0x0101000100000000), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0xb4 */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0000000101000101), U64(0x0000000100010000), U64(0x0000010001010000), U64(0x0101000100000000), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0xb5 */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0000000101000100), U64(0x0000000100010000), U64(0x0000010001010000), U64(0x0101000100000000), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0xb6 */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0000000101000100), U64(0x0000000100010000), U64(0x0000010001010000), U64(0x0101000100000000), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0xb7 */
	{ U64(0x0000000001010100), U64(0x0000000001010001), U64(0x0000000001000101), U64(0x0000000100010101), U64(0x0000010001000000), U64(0x0101000100000000), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0xb8 */
	{ U64(0x0000000001010100), U64(0x0000000001010001), U64(0x0000000001000101), U64(0x0000000100010101), U64(0x0000010001000000), U64(0x0101000100000000), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0xb9 */
	{ U64(0x0000000000000100), U64(0x0000000001010001), U64(0x0000000001000100), U64(0x0000000100010100), U64(0x0000010001000000), U64(0x0101000100000000), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0xba */
	{ U64(0x0000000000000100), U64(0x0000000001010001), U64(0x0000000001000100), U64(0x0000000100010100), U64(0x0000010001000000), U64(0x0101000100000000), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0xbb */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0000000001000101), U64(0x0000000100010000), U64(0x0000010001000000), U64(0x0101000100000000), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0xbc */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0000000001000101), U64(0x0000000100010000), U64(0x0000010001000000), U64(0x0101000100000000), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0xbd */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0000000001000100), U64(0x0000000100010000), U64(0x0000010001000000), U64(0x0101000100000000), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0xbe */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0000000001000100), U64(0x0000000100010000), U64(0x0000010001000000), U64(0x0101000100000000), U64(0x0100010000000000), U64(0x0001010000000000) }, /* 0xbf */
	{ U64(0x0001010101010100), U64(0x0001010101010001), U64(0x0001010101000101), U64(0x0001010100010101), U64(0x0001010001010101), U64(0x0001000101010101), U64(0x0100010101010101), U64(0x0001000000000000) }, /* 0xc0 */
	{ U64(0x0001010101010100), U64(0x0001010101010001), U64(0x0001010101000101), U64(0x0001010100010101), U64(0x0001010001010101), U64(0x0001000101010101), U64(0x0100010101010101), U64(0x0001000000000000) }, /* 0xc1 */
	{ U64(0x0000000000000100), U64(0x0001010101010001), U64(0x0001010101000100), U64(0x0001010100010100), U64(0x0001010001010100), U64(0x0001000101010100), U64(0x0100010101010100), U64(0x0001000000000000) }, /* 0xc2 */
	{ U64(0x0000000000000100), U64(0x0001010101010001), U64(0x0001010101000100), U64(0x0001010100010100), U64(0x0001010001010100), U64(0x0001000101010100), U64(0x0100010101010100), U64(0x0001000000000000) }, /* 0xc3 */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0001010101000101), U64(0x0001010100010000), U64(0x0001010001010000), U64(0x0001000101010000), U64(0x0100010101010000), U64(0x0001000000000000) }, /* 0xc4 */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0001010101000101), U64(0x0001010100010000), U64(0x0001010001010000), U64(0x0001000101010000), U64(0x0100010101010000), U64(0x0001000000000000) }, /* 0xc5 */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0001010101000100), U64(0x0001010100010000), U64(0x0001010001010000), U64(0x0001000101010000), U64(0x0100010101010000), U64(0x0001000000000000) }, /* 0xc6 */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0001010101000100), U64(0x0001010100010000), U64(0x0001010001010000), U64(0x0001000101010000), U64(0x0100010101010000), U64(0x0001000000000000) }, /* 0xc7 */
	{ U64(0x0000000001010100), U64(0x0000000001010001), U64(0x0000000001000101), U64(0x0001010100010101), U64(0x0001010001000000), U64(0x0001000101000000), U64(0x0100010101000000), U64(0x0001000000000000) }, /* 0xc8 */
	{ U64(0x0000000001010100), U64(0x0000000001010001), U64(0x0000000001000101), U64(0x0001010100010101), U64(0x0001010001000000), U64(0x0001000101000000), U64(0x0100010101000000), U64(0x0001000000000000) }, /* 0xc9 */
	{ U64(0x0000000000000100), U64(0x0000000001010001), U64(0x0000000001000100), U64(0x0001010100010100), U64(0x0001010001000000), U64(0x0001000101000000), U64(0x0100010101000000), U64(0x0001000000000000) }, /* 0xca */
	{ U64(0x0000000000000100), U64(0x0000000001010001), U64(0x0000000001000100), U64(0x0001010100010100), U64(0x0001010001000000), U64(0x0001000101000000), U64(0x0100010101000000), U64(0x0001000000000000) }, /* 0xcb */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0000000001000101), U64(0x0001010100010000), U64(0x0001010001000000), U64(0x0001000101000000), U64(0x0100010101000000), U64(0x0001000000000000) }, /* 0xcc */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0000000001000101), U64(0x0001010100010000), U64(0x0001010001000000), U64(0x0001000101000000), U64(0x0100010101000000), U64(0x0001000000000000) }, /* 0xcd */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0000000001000100), U64(0x0001010100010000), U64(0x0001010001000000), U64(0x0001000101000000), U64(0x0100010101000000), U64(0x0001000000000000) }, /* 0xce */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0000000001000100), U64(0x0001010100010000), U64(0x0001010001000000), U64(0x0001000101000000), U64(0x0100010101000000), U64(0x0001000000000000) }, /* 0xcf */
	{ U64(0x0000000101010100), U64(0x0000000101010001), U64(0x0000000101000101), U64(0x0000000100010101), U64(0x0001010001010101), U64(0x0001000100000000), U64(0x0100010100000000), U64(0x0001000000000000) }, /* 0xd0 */
	{ U64(0x0000000101010100), U64(0x0000000101010001), U64(0x0000000101000101), U64(0x0000000100010101), U64(0x0001010001010101), U64(0x0001000100000000), U64(0x0100010100000000), U64(0x0001000000000000) }, /* 0xd1 */
	{ U64(0x0000000000000100), U64(0x0000000101010001), U64(0x0000000101000100), U64(0x0000000100010100), U64(0x0001010001010100), U64(0x0001000100000000), U64(0x0100010100000000), U64(0x0001000000000000) }, /* 0xd2 */
	{ U64(0x0000000000000100), U64(0x0000000101010001), U64(0x0000000101000100), U64(0x0000000100010100), U64(0x0001010001010100), U64(0x0001000100000000), U64(0x0100010100000000), U64(0x0001000000000000) }, /* 0xd3 */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0000000101000101), U64(0x0000000100010000), U64(0x0001010001010000), U64(0x0001000100000000), U64(0x0100010100000000), U64(0x0001000000000000) }, /* 0xd4 */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0000000101000101), U64(0x0000000100010000), U64(0x0001010001010000), U64(0x0001000100000000), U64(0x0100010100000000), U64(0x0001000000000000) }, /* 0xd5 */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0000000101000100), U64(0x0000000100010000), U64(0x0001010001010000), U64(0x0001000100000000), U64(0x0100010100000000), U64(0x0001000000000000) }, /* 0xd6 */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0000000101000100), U64(0x0000000100010000), U64(0x0001010001010000), U64(0x0001000100000000), U64(0x0100010100000000), U64(0x0001000000000000) }, /* 0xd7 */
	{ U64(0x0000000001010100), U64(0x0000000001010001), U64(0x0000000001000101), U64(0x0000000100010101), U64(0x0001010001000000), U64(0x0001000100000000), U64(0x0100010100000000), U64(0x0001000000000000) }, /* 0xd8 */
	{ U64(0x0000000001010100), U64(0x0000000001010001), U64(0x0000000001000101), U64(0x0000000100010101), U64(0x0001010001000000), U64(0x0001000100000000), U64(0x0100010100000000), U64(0x0001000000000000) }, /* 0xd9 */
	{ U64(0x0000000000000100), U64(0x0000000001010001), U64(0x0000000001000100), U64(0x0000000100010100), U64(0x0001010001000000), U64(0x0001000100000000), U64(0x0100010100000000), U64(0x0001000000000000) }, /* 0xda */
	{ U64(0x0000000000000100), U64(0x0000000001010001), U64(0x0000000001000100), U64(0x0000000100010100), U64(0x0001010001000000), U64(0x0001000100000000), U64(0x0100010100000000), U64(0x0001000000000000) }, /* 0xdb */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0000000001000101), U64(0x0000000100010000), U64(0x0001010001000000), U64(0x0001000100000000), U64(0x0100010100000000), U64(0x0001000000000000) }, /* 0xdc */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0000000001000101), U64(0x0000000100010000), U64(0x0001010001000000), U64(0x0001000100000000), U64(0x0100010100000000), U64(0x0001000000000000) }, /* 0xdd */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0000000001000100), U64(0x0000000100010000), U64(0x0001010001000000), U64(0x0001000100000000), U64(0x0100010100000000), U64(0x0001000000000000) }, /* 0xde */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0000000001000100), U64(0x0000000100010000), U64(0x0001010001000000), U64(0x0001000100000000), U64(0x0100010100000000), U64(0x0001000000000000) }, /* 0xdf */
	{ U64(0x0000010101010100), U64(0x0000010101010001), U64(0x0000010101000101), U64(0x0000010100010101), U64(0x0000010001010101), U64(0x0001000101010101), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0xe0 */
	{ U64(0x0000010101010100), U64(0x0000010101010001), U64(0x0000010101000101), U64(0x0000010100010101), U64(0x0000010001010101), U64(0x0001000101010101), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0xe1 */
	{ U64(0x0000000000000100), U64(0x0000010101010001), U64(0x0000010101000100), U64(0x0000010100010100), U64(0x0000010001010100), U64(0x0001000101010100), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0xe2 */
	{ U64(0x0000000000000100), U64(0x0000010101010001), U64(0x0000010101000100), U64(0x0000010100010100), U64(0x0000010001010100), U64(0x0001000101010100), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0xe3 */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0000010101000101), U64(0x0000010100010000), U64(0x0000010001010000), U64(0x0001000101010000), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0xe4 */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0000010101000101), U64(0x0000010100010000), U64(0x0000010001010000), U64(0x0001000101010000), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0xe5 */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0000010101000100), U64(0x0000010100010000), U64(0x0000010001010000), U64(0x0001000101010000), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0xe6 */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0000010101000100), U64(0x0000010100010000), U64(0x0000010001010000), U64(0x0001000101010000), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0xe7 */
	{ U64(0x0000000001010100), U64(0x0000000001010001), U64(0x0000000001000101), U64(0x0000010100010101), U64(0x0000010001000000), U64(0x0001000101000000), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0xe8 */
	{ U64(0x0000000001010100), U64(0x0000000001010001), U64(0x0000000001000101), U64(0x0000010100010101), U64(0x0000010001000000), U64(0x0001000101000000), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0xe9 */
	{ U64(0x0000000000000100), U64(0x0000000001010001), U64(0x0000000001000100), U64(0x0000010100010100), U64(0x0000010001000000), U64(0x0001000101000000), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0xea */
	{ U64(0x0000000000000100), U64(0x0000000001010001), U64(0x0000000001000100), U64(0x0000010100010100), U64(0x0000010001000000), U64(0x0001000101000000), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0xeb */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0000000001000101), U64(0x0000010100010000), U64(0x0000010001000000), U64(0x0001000101000000), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0xec */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0000000001000101), U64(0x0000010100010000), U64(0x0000010001000000), U64(0x0001000101000000), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0xed */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0000000001000100), U64(0x0000010100010000), U64(0x0000010001000000), U64(0x0001000101000000), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0xee */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0000000001000100), U64(0x0000010100010000), U64(0x0000010001000000), U64(0x0001000101000000), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0xef */
	{ U64(0x0000000101010100), U64(0x0000000101010001), U64(0x0000000101000101), U64(0x0000000100010101), U64(0x0000010001010101), U64(0x0001000100000000), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0xf0 */
	{ U64(0x0000000101010100), U64(0x0000000101010001), U64(0x0000000101000101), U64(0x0000000100010101), U64(0x0000010001010101), U64(0x0001000100000000), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0xf1 */
	{ U64(0x0000000000000100), U64(0x0000000101010001), U64(0x0000000101000100), U64(0x0000000100010100), U64(0x0000010001010100), U64(0x0001000100000000), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0xf2 */
	{ U64(0x0000000000000100), U64(0x0000000101010001), U64(0x0000000101000100), U64(0x0000000100010100), U64(0x0000010001010100), U64(0x0001000100000000), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0xf3 */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0000000101000101), U64(0x0000000100010000), U64(0x0000010001010000), U64(0x0001000100000000), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0xf4 */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0000000101000101), U64(0x0000000100010000), U64(0x0000010001010000), U64(0x0001000100000000), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0xf5 */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0000000101000100), U64(0x0000000100010000), U64(0x0000010001010000), U64(0x0001000100000000), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0xf6 */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0000000101000100), U64(0x0000000100010000), U64(0x0000010001010000), U64(0x0001000100000000), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0xf7 */
	{ U64(0x0000000001010100), U64(0x0000000001010001), U64(0x0000000001000101), U64(0x0000000100010101), U64(0x0000010001000000), U64(0x0001000100000000), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0xf8 */
	{ U64(0x0000000001010100), U64(0x0000000001010001), U64(0x0000000001000101), U64(0x0000000100010101), U64(0x0000010001000000), U64(0x0001000100000000), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0xf9 */
	{ U64(0x0000000000000100), U64(0x0000000001010001), U64(0x0000000001000100), U64(0x0000000100010100), U64(0x0000010001000000), U64(0x0001000100000000), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0xfa */
	{ U64(0x0000000000000100), U64(0x0000000001010001), U64(0x0000000001000100), U64(0x0000000100010100), U64(0x0000010001000000), U64(0x0001000100000000), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0xfb */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0000000001000101), U64(0x0000000100010000), U64(0x0000010001000000), U64(0x0001000100000000), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0xfc */
	{ U64(0x0000000000010100), U64(0x0000000000010001), U64(0x0000000001000101), U64(0x0000000100010000), U64(0x0000010001000000), U64(0x0001000100000000), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0xfd */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0000000001000100), U64(0x0000000100010000), U64(0x0000010001000000), U64(0x0001000100000000), U64(0x0100010000000000), U64(0x0001000000000000) }, /* 0xfe */
	{ U64(0x0000000000000100), U64(0x0000000000010001), U64(0x0000000001000100), U64(0x0000000100010000), U64(0x0000010001000000), U64(0x0001000100000000), U64(0x0100010000000000), U64(0x0001000000000000) } /* 0xff */
};

/****************************************************************************
 * For bishop attacks along a diagonal parallel to a8-h1 (rot45). This is
 * kind of special, because each diagonal is a different length you need to
 * think carefully about how the shifting works. This will give the move mask
 * for a bishop on the a8-h1 diagonal (the longest one) in NON-ROTATED format.
 *
 * Think carefully about how you want to shift. When you get the result back,
 * you will want to shift it forward or backward exactly 8 times N, where N is
 * the number of diagonals the one your bishop is on is away from a8-h1 (a
 * bishop on a8-h1 will be shifted 0. For a bishop on a7-g1 you will shift
 * RIGHT by 8 bits; a6-f1 RIGHT by 16 bits, a1-a1 RIGHT by 56 bits. On the
 * other hand, for a bishop on b8-h2 you will shift LEFT 8 bits, c8-h3 LEFT by
 * 16 bits, h8-h8 LEFT by 56 bits.
 *
 * The other important thing to consider is how you go into the table. Because
 * the diagonals are not all the same length, you need to aligh one edge of
 * your diagonal to either the 0 index OR the other edge to the 7 index. The
 * way you want to do it is if your diagonal touches the left side of the
 * board (aX-x1), shift so the square on the a-file is in the 0th index; if
 * your diagonal touches the right side (x8-hX), shift so the square on the
 * h-file will be in the 7th index. Either way, this means there will be some
 * garbage not related to your diagonal in the remaining slots - it will
 * affect the resulting move mask, but when you bitshift it (according to the
 * previous paragraph) the nonrelevant squares in the mask will overflow out.
 *
 * To get a piece's position on the diagonal (the second index in the array),
 * we consider alignment described above, and realize that because essentially
 * we're projecting the diagonal down to the first (0-index) row, plus some
 * garbage, the position index is simply the column the piece is on!
 ****************************************************************************/

/* Use these to get the number used to index into the next few arrays. Indexed
 * by square (A1 0, H8 63) for both rotation directions. Use the result from
 * these arrays as an index into the "shiftamount" arrays following. */
int rot45diagindex[64] = {
	0,  1,  2,  3,  4,  5,  6,  7,
	1,  2,  3,  4,  5,  6,  7,  8,
	2,  3,  4,  5,  6,  7,  8,  9,
	3,  4,  5,  6,  7,  8,  9, 10,
	4,  5,  6,  7,  8,  9, 10, 11,
	5,  6,  7,  8,  9, 10, 11, 12,
	6,  7,  8,  9, 10, 11, 12, 13,
	7,  8,  9, 10, 11, 12, 13, 14
};
int rot315diagindex[64] = {
	 7,  6,  5,  4,  3,  2,  1,  0,
	 8,  7,  6,  5,  4,  3,  2,  1,
	 9,  8,  7,  6,  5,  4,  3,  2,
	10,  9,  8,  7,  6,  5,  4,  3,
	11, 10,  9,  8,  7,  6,  5,  4,
	12, 11, 10,  9,  8,  7,  6,  5,
	13, 12, 11, 10,  9,  8,  7,  6,
	14, 13, 12, 11, 10,  9,  8,  7
};

/* The amount you want to shift the rotated bitboard by (then mask with 0xFF)
 * to get the correct index. Index into this with the diagonal's number. For
 * rot45, you want to align >7 diagonals against the 7th index, with garbage
 * in the bottom, or <7 diagonals against the 0th index, with garbage in the
 * top, so that when shifting the garbage goes out of the top/bottom of the
 * board. For rot315 it's the opposite case - >7 diags get aligned to the 0th
 * index, and <7 diags aligned to the 7th index; this way, the garbage will
 * still go out the top/bottom of the board. For the lowest diagonals, this
 * means we might have to shift backwards. (note: The numbers for aligning to
 * the top bits are generated by "extending" the row backwards.) */
int rot45index_shiftamountright[15] = { 0, 1, 3, 6, 10, 15, 21, 28, 35, 41, 46, 50, 53, 55, 56 };
int rot315index_shiftamountright[15] = { 0, 0, 0, 2, 7, 13, 20, 28, 36, 43, 49, 54, 58, 61, 63 };
int rot315index_shiftamountleft[15]  = { 7, 5, 2, 0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 };

/* The amount you want to shift the result by. Use the number of the diagonal
 * (0 for a1-a1, 7 for a8-h1, 14 for h8-h8) to index into these. */
int rotresult_shiftamountleft[15]  = {  0,  0,  0,  0,  0,  0, 0, 0, 8, 16, 24, 32, 40, 48, 56 };
int rotresult_shiftamountright[15] = { 56, 48, 40, 32, 24, 16, 8, 0, 0,  0,  0,  0,  0,  0,  0 };

uint64_t rot45attacks[256][8] = {
	{ U64(0x0002040810204080), U64(0x0100040810204080), U64(0x0102000810204080), U64(0x0102040010204080), U64(0x0102040800204080), U64(0x0102040810004080), U64(0x0102040810200080), U64(0x0102040810204000) }, /* 0x00 */
	{ U64(0x0002040810204080), U64(0x0100040810204080), U64(0x0102000810204080), U64(0x0102040010204080), U64(0x0102040800204080), U64(0x0102040810004080), U64(0x0102040810200080), U64(0x0102040810204000) }, /* 0x01 */
	{ U64(0x0002000000000000), U64(0x0100040810204080), U64(0x0002000810204080), U64(0x0002040010204080), U64(0x0002040800204080), U64(0x0002040810004080), U64(0x0002040810200080), U64(0x0002040810204000) }, /* 0x02 */
	{ U64(0x0002000000000000), U64(0x0100040810204080), U64(0x0002000810204080), U64(0x0002040010204080), U64(0x0002040800204080), U64(0x0002040810004080), U64(0x0002040810200080), U64(0x0002040810204000) }, /* 0x03 */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000810204080), U64(0x0000040010204080), U64(0x0000040800204080), U64(0x0000040810004080), U64(0x0000040810200080), U64(0x0000040810204000) }, /* 0x04 */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000810204080), U64(0x0000040010204080), U64(0x0000040800204080), U64(0x0000040810004080), U64(0x0000040810200080), U64(0x0000040810204000) }, /* 0x05 */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000810204080), U64(0x0000040010204080), U64(0x0000040800204080), U64(0x0000040810004080), U64(0x0000040810200080), U64(0x0000040810204000) }, /* 0x06 */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000810204080), U64(0x0000040010204080), U64(0x0000040800204080), U64(0x0000040810004080), U64(0x0000040810200080), U64(0x0000040810204000) }, /* 0x07 */
	{ U64(0x0002040800000000), U64(0x0100040800000000), U64(0x0102000800000000), U64(0x0102040010204080), U64(0x0000000800204080), U64(0x0000000810004080), U64(0x0000000810200080), U64(0x0000000810204000) }, /* 0x08 */
	{ U64(0x0002040800000000), U64(0x0100040800000000), U64(0x0102000800000000), U64(0x0102040010204080), U64(0x0000000800204080), U64(0x0000000810004080), U64(0x0000000810200080), U64(0x0000000810204000) }, /* 0x09 */
	{ U64(0x0002000000000000), U64(0x0100040800000000), U64(0x0002000800000000), U64(0x0002040010204080), U64(0x0000000800204080), U64(0x0000000810004080), U64(0x0000000810200080), U64(0x0000000810204000) }, /* 0x0a */
	{ U64(0x0002000000000000), U64(0x0100040800000000), U64(0x0002000800000000), U64(0x0002040010204080), U64(0x0000000800204080), U64(0x0000000810004080), U64(0x0000000810200080), U64(0x0000000810204000) }, /* 0x0b */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000800000000), U64(0x0000040010204080), U64(0x0000000800204080), U64(0x0000000810004080), U64(0x0000000810200080), U64(0x0000000810204000) }, /* 0x0c */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000800000000), U64(0x0000040010204080), U64(0x0000000800204080), U64(0x0000000810004080), U64(0x0000000810200080), U64(0x0000000810204000) }, /* 0x0d */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000800000000), U64(0x0000040010204080), U64(0x0000000800204080), U64(0x0000000810004080), U64(0x0000000810200080), U64(0x0000000810204000) }, /* 0x0e */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000800000000), U64(0x0000040010204080), U64(0x0000000800204080), U64(0x0000000810004080), U64(0x0000000810200080), U64(0x0000000810204000) }, /* 0x0f */
	{ U64(0x0002040810000000), U64(0x0100040810000000), U64(0x0102000810000000), U64(0x0102040010000000), U64(0x0102040800204080), U64(0x0000000010004080), U64(0x0000000010200080), U64(0x0000000010204000) }, /* 0x10 */
	{ U64(0x0002040810000000), U64(0x0100040810000000), U64(0x0102000810000000), U64(0x0102040010000000), U64(0x0102040800204080), U64(0x0000000010004080), U64(0x0000000010200080), U64(0x0000000010204000) }, /* 0x11 */
	{ U64(0x0002000000000000), U64(0x0100040810000000), U64(0x0002000810000000), U64(0x0002040010000000), U64(0x0002040800204080), U64(0x0000000010004080), U64(0x0000000010200080), U64(0x0000000010204000) }, /* 0x12 */
	{ U64(0x0002000000000000), U64(0x0100040810000000), U64(0x0002000810000000), U64(0x0002040010000000), U64(0x0002040800204080), U64(0x0000000010004080), U64(0x0000000010200080), U64(0x0000000010204000) }, /* 0x13 */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000810000000), U64(0x0000040010000000), U64(0x0000040800204080), U64(0x0000000010004080), U64(0x0000000010200080), U64(0x0000000010204000) }, /* 0x14 */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000810000000), U64(0x0000040010000000), U64(0x0000040800204080), U64(0x0000000010004080), U64(0x0000000010200080), U64(0x0000000010204000) }, /* 0x15 */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000810000000), U64(0x0000040010000000), U64(0x0000040800204080), U64(0x0000000010004080), U64(0x0000000010200080), U64(0x0000000010204000) }, /* 0x16 */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000810000000), U64(0x0000040010000000), U64(0x0000040800204080), U64(0x0000000010004080), U64(0x0000000010200080), U64(0x0000000010204000) }, /* 0x17 */
	{ U64(0x0002040800000000), U64(0x0100040800000000), U64(0x0102000800000000), U64(0x0102040010000000), U64(0x0000000800204080), U64(0x0000000010004080), U64(0x0000000010200080), U64(0x0000000010204000) }, /* 0x18 */
	{ U64(0x0002040800000000), U64(0x0100040800000000), U64(0x0102000800000000), U64(0x0102040010000000), U64(0x0000000800204080), U64(0x0000000010004080), U64(0x0000000010200080), U64(0x0000000010204000) }, /* 0x19 */
	{ U64(0x0002000000000000), U64(0x0100040800000000), U64(0x0002000800000000), U64(0x0002040010000000), U64(0x0000000800204080), U64(0x0000000010004080), U64(0x0000000010200080), U64(0x0000000010204000) }, /* 0x1a */
	{ U64(0x0002000000000000), U64(0x0100040800000000), U64(0x0002000800000000), U64(0x0002040010000000), U64(0x0000000800204080), U64(0x0000000010004080), U64(0x0000000010200080), U64(0x0000000010204000) }, /* 0x1b */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000800000000), U64(0x0000040010000000), U64(0x0000000800204080), U64(0x0000000010004080), U64(0x0000000010200080), U64(0x0000000010204000) }, /* 0x1c */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000800000000), U64(0x0000040010000000), U64(0x0000000800204080), U64(0x0000000010004080), U64(0x0000000010200080), U64(0x0000000010204000) }, /* 0x1d */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000800000000), U64(0x0000040010000000), U64(0x0000000800204080), U64(0x0000000010004080), U64(0x0000000010200080), U64(0x0000000010204000) }, /* 0x1e */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000800000000), U64(0x0000040010000000), U64(0x0000000800204080), U64(0x0000000010004080), U64(0x0000000010200080), U64(0x0000000010204000) }, /* 0x1f */
	{ U64(0x0002040810200000), U64(0x0100040810200000), U64(0x0102000810200000), U64(0x0102040010200000), U64(0x0102040800200000), U64(0x0102040810004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0x20 */
	{ U64(0x0002040810200000), U64(0x0100040810200000), U64(0x0102000810200000), U64(0x0102040010200000), U64(0x0102040800200000), U64(0x0102040810004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0x21 */
	{ U64(0x0002000000000000), U64(0x0100040810200000), U64(0x0002000810200000), U64(0x0002040010200000), U64(0x0002040800200000), U64(0x0002040810004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0x22 */
	{ U64(0x0002000000000000), U64(0x0100040810200000), U64(0x0002000810200000), U64(0x0002040010200000), U64(0x0002040800200000), U64(0x0002040810004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0x23 */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000810200000), U64(0x0000040010200000), U64(0x0000040800200000), U64(0x0000040810004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0x24 */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000810200000), U64(0x0000040010200000), U64(0x0000040800200000), U64(0x0000040810004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0x25 */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000810200000), U64(0x0000040010200000), U64(0x0000040800200000), U64(0x0000040810004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0x26 */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000810200000), U64(0x0000040010200000), U64(0x0000040800200000), U64(0x0000040810004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0x27 */
	{ U64(0x0002040800000000), U64(0x0100040800000000), U64(0x0102000800000000), U64(0x0102040010200000), U64(0x0000000800200000), U64(0x0000000810004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0x28 */
	{ U64(0x0002040800000000), U64(0x0100040800000000), U64(0x0102000800000000), U64(0x0102040010200000), U64(0x0000000800200000), U64(0x0000000810004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0x29 */
	{ U64(0x0002000000000000), U64(0x0100040800000000), U64(0x0002000800000000), U64(0x0002040010200000), U64(0x0000000800200000), U64(0x0000000810004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0x2a */
	{ U64(0x0002000000000000), U64(0x0100040800000000), U64(0x0002000800000000), U64(0x0002040010200000), U64(0x0000000800200000), U64(0x0000000810004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0x2b */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000800000000), U64(0x0000040010200000), U64(0x0000000800200000), U64(0x0000000810004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0x2c */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000800000000), U64(0x0000040010200000), U64(0x0000000800200000), U64(0x0000000810004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0x2d */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000800000000), U64(0x0000040010200000), U64(0x0000000800200000), U64(0x0000000810004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0x2e */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000800000000), U64(0x0000040010200000), U64(0x0000000800200000), U64(0x0000000810004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0x2f */
	{ U64(0x0002040810000000), U64(0x0100040810000000), U64(0x0102000810000000), U64(0x0102040010000000), U64(0x0102040800200000), U64(0x0000000010004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0x30 */
	{ U64(0x0002040810000000), U64(0x0100040810000000), U64(0x0102000810000000), U64(0x0102040010000000), U64(0x0102040800200000), U64(0x0000000010004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0x31 */
	{ U64(0x0002000000000000), U64(0x0100040810000000), U64(0x0002000810000000), U64(0x0002040010000000), U64(0x0002040800200000), U64(0x0000000010004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0x32 */
	{ U64(0x0002000000000000), U64(0x0100040810000000), U64(0x0002000810000000), U64(0x0002040010000000), U64(0x0002040800200000), U64(0x0000000010004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0x33 */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000810000000), U64(0x0000040010000000), U64(0x0000040800200000), U64(0x0000000010004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0x34 */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000810000000), U64(0x0000040010000000), U64(0x0000040800200000), U64(0x0000000010004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0x35 */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000810000000), U64(0x0000040010000000), U64(0x0000040800200000), U64(0x0000000010004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0x36 */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000810000000), U64(0x0000040010000000), U64(0x0000040800200000), U64(0x0000000010004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0x37 */
	{ U64(0x0002040800000000), U64(0x0100040800000000), U64(0x0102000800000000), U64(0x0102040010000000), U64(0x0000000800200000), U64(0x0000000010004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0x38 */
	{ U64(0x0002040800000000), U64(0x0100040800000000), U64(0x0102000800000000), U64(0x0102040010000000), U64(0x0000000800200000), U64(0x0000000010004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0x39 */
	{ U64(0x0002000000000000), U64(0x0100040800000000), U64(0x0002000800000000), U64(0x0002040010000000), U64(0x0000000800200000), U64(0x0000000010004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0x3a */
	{ U64(0x0002000000000000), U64(0x0100040800000000), U64(0x0002000800000000), U64(0x0002040010000000), U64(0x0000000800200000), U64(0x0000000010004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0x3b */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000800000000), U64(0x0000040010000000), U64(0x0000000800200000), U64(0x0000000010004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0x3c */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000800000000), U64(0x0000040010000000), U64(0x0000000800200000), U64(0x0000000010004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0x3d */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000800000000), U64(0x0000040010000000), U64(0x0000000800200000), U64(0x0000000010004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0x3e */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000800000000), U64(0x0000040010000000), U64(0x0000000800200000), U64(0x0000000010004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0x3f */
	{ U64(0x0002040810204000), U64(0x0100040810204000), U64(0x0102000810204000), U64(0x0102040010204000), U64(0x0102040800204000), U64(0x0102040810004000), U64(0x0102040810200080), U64(0x0000000000004000) }, /* 0x40 */
	{ U64(0x0002040810204000), U64(0x0100040810204000), U64(0x0102000810204000), U64(0x0102040010204000), U64(0x0102040800204000), U64(0x0102040810004000), U64(0x0102040810200080), U64(0x0000000000004000) }, /* 0x41 */
	{ U64(0x0002000000000000), U64(0x0100040810204000), U64(0x0002000810204000), U64(0x0002040010204000), U64(0x0002040800204000), U64(0x0002040810004000), U64(0x0002040810200080), U64(0x0000000000004000) }, /* 0x42 */
	{ U64(0x0002000000000000), U64(0x0100040810204000), U64(0x0002000810204000), U64(0x0002040010204000), U64(0x0002040800204000), U64(0x0002040810004000), U64(0x0002040810200080), U64(0x0000000000004000) }, /* 0x43 */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000810204000), U64(0x0000040010204000), U64(0x0000040800204000), U64(0x0000040810004000), U64(0x0000040810200080), U64(0x0000000000004000) }, /* 0x44 */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000810204000), U64(0x0000040010204000), U64(0x0000040800204000), U64(0x0000040810004000), U64(0x0000040810200080), U64(0x0000000000004000) }, /* 0x45 */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000810204000), U64(0x0000040010204000), U64(0x0000040800204000), U64(0x0000040810004000), U64(0x0000040810200080), U64(0x0000000000004000) }, /* 0x46 */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000810204000), U64(0x0000040010204000), U64(0x0000040800204000), U64(0x0000040810004000), U64(0x0000040810200080), U64(0x0000000000004000) }, /* 0x47 */
	{ U64(0x0002040800000000), U64(0x0100040800000000), U64(0x0102000800000000), U64(0x0102040010204000), U64(0x0000000800204000), U64(0x0000000810004000), U64(0x0000000810200080), U64(0x0000000000004000) }, /* 0x48 */
	{ U64(0x0002040800000000), U64(0x0100040800000000), U64(0x0102000800000000), U64(0x0102040010204000), U64(0x0000000800204000), U64(0x0000000810004000), U64(0x0000000810200080), U64(0x0000000000004000) }, /* 0x49 */
	{ U64(0x0002000000000000), U64(0x0100040800000000), U64(0x0002000800000000), U64(0x0002040010204000), U64(0x0000000800204000), U64(0x0000000810004000), U64(0x0000000810200080), U64(0x0000000000004000) }, /* 0x4a */
	{ U64(0x0002000000000000), U64(0x0100040800000000), U64(0x0002000800000000), U64(0x0002040010204000), U64(0x0000000800204000), U64(0x0000000810004000), U64(0x0000000810200080), U64(0x0000000000004000) }, /* 0x4b */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000800000000), U64(0x0000040010204000), U64(0x0000000800204000), U64(0x0000000810004000), U64(0x0000000810200080), U64(0x0000000000004000) }, /* 0x4c */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000800000000), U64(0x0000040010204000), U64(0x0000000800204000), U64(0x0000000810004000), U64(0x0000000810200080), U64(0x0000000000004000) }, /* 0x4d */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000800000000), U64(0x0000040010204000), U64(0x0000000800204000), U64(0x0000000810004000), U64(0x0000000810200080), U64(0x0000000000004000) }, /* 0x4e */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000800000000), U64(0x0000040010204000), U64(0x0000000800204000), U64(0x0000000810004000), U64(0x0000000810200080), U64(0x0000000000004000) }, /* 0x4f */
	{ U64(0x0002040810000000), U64(0x0100040810000000), U64(0x0102000810000000), U64(0x0102040010000000), U64(0x0102040800204000), U64(0x0000000010004000), U64(0x0000000010200080), U64(0x0000000000004000) }, /* 0x50 */
	{ U64(0x0002040810000000), U64(0x0100040810000000), U64(0x0102000810000000), U64(0x0102040010000000), U64(0x0102040800204000), U64(0x0000000010004000), U64(0x0000000010200080), U64(0x0000000000004000) }, /* 0x51 */
	{ U64(0x0002000000000000), U64(0x0100040810000000), U64(0x0002000810000000), U64(0x0002040010000000), U64(0x0002040800204000), U64(0x0000000010004000), U64(0x0000000010200080), U64(0x0000000000004000) }, /* 0x52 */
	{ U64(0x0002000000000000), U64(0x0100040810000000), U64(0x0002000810000000), U64(0x0002040010000000), U64(0x0002040800204000), U64(0x0000000010004000), U64(0x0000000010200080), U64(0x0000000000004000) }, /* 0x53 */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000810000000), U64(0x0000040010000000), U64(0x0000040800204000), U64(0x0000000010004000), U64(0x0000000010200080), U64(0x0000000000004000) }, /* 0x54 */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000810000000), U64(0x0000040010000000), U64(0x0000040800204000), U64(0x0000000010004000), U64(0x0000000010200080), U64(0x0000000000004000) }, /* 0x55 */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000810000000), U64(0x0000040010000000), U64(0x0000040800204000), U64(0x0000000010004000), U64(0x0000000010200080), U64(0x0000000000004000) }, /* 0x56 */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000810000000), U64(0x0000040010000000), U64(0x0000040800204000), U64(0x0000000010004000), U64(0x0000000010200080), U64(0x0000000000004000) }, /* 0x57 */
	{ U64(0x0002040800000000), U64(0x0100040800000000), U64(0x0102000800000000), U64(0x0102040010000000), U64(0x0000000800204000), U64(0x0000000010004000), U64(0x0000000010200080), U64(0x0000000000004000) }, /* 0x58 */
	{ U64(0x0002040800000000), U64(0x0100040800000000), U64(0x0102000800000000), U64(0x0102040010000000), U64(0x0000000800204000), U64(0x0000000010004000), U64(0x0000000010200080), U64(0x0000000000004000) }, /* 0x59 */
	{ U64(0x0002000000000000), U64(0x0100040800000000), U64(0x0002000800000000), U64(0x0002040010000000), U64(0x0000000800204000), U64(0x0000000010004000), U64(0x0000000010200080), U64(0x0000000000004000) }, /* 0x5a */
	{ U64(0x0002000000000000), U64(0x0100040800000000), U64(0x0002000800000000), U64(0x0002040010000000), U64(0x0000000800204000), U64(0x0000000010004000), U64(0x0000000010200080), U64(0x0000000000004000) }, /* 0x5b */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000800000000), U64(0x0000040010000000), U64(0x0000000800204000), U64(0x0000000010004000), U64(0x0000000010200080), U64(0x0000000000004000) }, /* 0x5c */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000800000000), U64(0x0000040010000000), U64(0x0000000800204000), U64(0x0000000010004000), U64(0x0000000010200080), U64(0x0000000000004000) }, /* 0x5d */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000800000000), U64(0x0000040010000000), U64(0x0000000800204000), U64(0x0000000010004000), U64(0x0000000010200080), U64(0x0000000000004000) }, /* 0x5e */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000800000000), U64(0x0000040010000000), U64(0x0000000800204000), U64(0x0000000010004000), U64(0x0000000010200080), U64(0x0000000000004000) }, /* 0x5f */
	{ U64(0x0002040810200000), U64(0x0100040810200000), U64(0x0102000810200000), U64(0x0102040010200000), U64(0x0102040800200000), U64(0x0102040810004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0x60 */
	{ U64(0x0002040810200000), U64(0x0100040810200000), U64(0x0102000810200000), U64(0x0102040010200000), U64(0x0102040800200000), U64(0x0102040810004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0x61 */
	{ U64(0x0002000000000000), U64(0x0100040810200000), U64(0x0002000810200000), U64(0x0002040010200000), U64(0x0002040800200000), U64(0x0002040810004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0x62 */
	{ U64(0x0002000000000000), U64(0x0100040810200000), U64(0x0002000810200000), U64(0x0002040010200000), U64(0x0002040800200000), U64(0x0002040810004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0x63 */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000810200000), U64(0x0000040010200000), U64(0x0000040800200000), U64(0x0000040810004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0x64 */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000810200000), U64(0x0000040010200000), U64(0x0000040800200000), U64(0x0000040810004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0x65 */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000810200000), U64(0x0000040010200000), U64(0x0000040800200000), U64(0x0000040810004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0x66 */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000810200000), U64(0x0000040010200000), U64(0x0000040800200000), U64(0x0000040810004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0x67 */
	{ U64(0x0002040800000000), U64(0x0100040800000000), U64(0x0102000800000000), U64(0x0102040010200000), U64(0x0000000800200000), U64(0x0000000810004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0x68 */
	{ U64(0x0002040800000000), U64(0x0100040800000000), U64(0x0102000800000000), U64(0x0102040010200000), U64(0x0000000800200000), U64(0x0000000810004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0x69 */
	{ U64(0x0002000000000000), U64(0x0100040800000000), U64(0x0002000800000000), U64(0x0002040010200000), U64(0x0000000800200000), U64(0x0000000810004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0x6a */
	{ U64(0x0002000000000000), U64(0x0100040800000000), U64(0x0002000800000000), U64(0x0002040010200000), U64(0x0000000800200000), U64(0x0000000810004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0x6b */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000800000000), U64(0x0000040010200000), U64(0x0000000800200000), U64(0x0000000810004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0x6c */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000800000000), U64(0x0000040010200000), U64(0x0000000800200000), U64(0x0000000810004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0x6d */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000800000000), U64(0x0000040010200000), U64(0x0000000800200000), U64(0x0000000810004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0x6e */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000800000000), U64(0x0000040010200000), U64(0x0000000800200000), U64(0x0000000810004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0x6f */
	{ U64(0x0002040810000000), U64(0x0100040810000000), U64(0x0102000810000000), U64(0x0102040010000000), U64(0x0102040800200000), U64(0x0000000010004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0x70 */
	{ U64(0x0002040810000000), U64(0x0100040810000000), U64(0x0102000810000000), U64(0x0102040010000000), U64(0x0102040800200000), U64(0x0000000010004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0x71 */
	{ U64(0x0002000000000000), U64(0x0100040810000000), U64(0x0002000810000000), U64(0x0002040010000000), U64(0x0002040800200000), U64(0x0000000010004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0x72 */
	{ U64(0x0002000000000000), U64(0x0100040810000000), U64(0x0002000810000000), U64(0x0002040010000000), U64(0x0002040800200000), U64(0x0000000010004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0x73 */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000810000000), U64(0x0000040010000000), U64(0x0000040800200000), U64(0x0000000010004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0x74 */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000810000000), U64(0x0000040010000000), U64(0x0000040800200000), U64(0x0000000010004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0x75 */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000810000000), U64(0x0000040010000000), U64(0x0000040800200000), U64(0x0000000010004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0x76 */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000810000000), U64(0x0000040010000000), U64(0x0000040800200000), U64(0x0000000010004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0x77 */
	{ U64(0x0002040800000000), U64(0x0100040800000000), U64(0x0102000800000000), U64(0x0102040010000000), U64(0x0000000800200000), U64(0x0000000010004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0x78 */
	{ U64(0x0002040800000000), U64(0x0100040800000000), U64(0x0102000800000000), U64(0x0102040010000000), U64(0x0000000800200000), U64(0x0000000010004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0x79 */
	{ U64(0x0002000000000000), U64(0x0100040800000000), U64(0x0002000800000000), U64(0x0002040010000000), U64(0x0000000800200000), U64(0x0000000010004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0x7a */
	{ U64(0x0002000000000000), U64(0x0100040800000000), U64(0x0002000800000000), U64(0x0002040010000000), U64(0x0000000800200000), U64(0x0000000010004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0x7b */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000800000000), U64(0x0000040010000000), U64(0x0000000800200000), U64(0x0000000010004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0x7c */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000800000000), U64(0x0000040010000000), U64(0x0000000800200000), U64(0x0000000010004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0x7d */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000800000000), U64(0x0000040010000000), U64(0x0000000800200000), U64(0x0000000010004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0x7e */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000800000000), U64(0x0000040010000000), U64(0x0000000800200000), U64(0x0000000010004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0x7f */
	{ U64(0x0002040810204080), U64(0x0100040810204080), U64(0x0102000810204080), U64(0x0102040010204080), U64(0x0102040800204080), U64(0x0102040810004080), U64(0x0102040810200080), U64(0x0102040810204000) }, /* 0x80 */
	{ U64(0x0002040810204080), U64(0x0100040810204080), U64(0x0102000810204080), U64(0x0102040010204080), U64(0x0102040800204080), U64(0x0102040810004080), U64(0x0102040810200080), U64(0x0102040810204000) }, /* 0x81 */
	{ U64(0x0002000000000000), U64(0x0100040810204080), U64(0x0002000810204080), U64(0x0002040010204080), U64(0x0002040800204080), U64(0x0002040810004080), U64(0x0002040810200080), U64(0x0002040810204000) }, /* 0x82 */
	{ U64(0x0002000000000000), U64(0x0100040810204080), U64(0x0002000810204080), U64(0x0002040010204080), U64(0x0002040800204080), U64(0x0002040810004080), U64(0x0002040810200080), U64(0x0002040810204000) }, /* 0x83 */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000810204080), U64(0x0000040010204080), U64(0x0000040800204080), U64(0x0000040810004080), U64(0x0000040810200080), U64(0x0000040810204000) }, /* 0x84 */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000810204080), U64(0x0000040010204080), U64(0x0000040800204080), U64(0x0000040810004080), U64(0x0000040810200080), U64(0x0000040810204000) }, /* 0x85 */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000810204080), U64(0x0000040010204080), U64(0x0000040800204080), U64(0x0000040810004080), U64(0x0000040810200080), U64(0x0000040810204000) }, /* 0x86 */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000810204080), U64(0x0000040010204080), U64(0x0000040800204080), U64(0x0000040810004080), U64(0x0000040810200080), U64(0x0000040810204000) }, /* 0x87 */
	{ U64(0x0002040800000000), U64(0x0100040800000000), U64(0x0102000800000000), U64(0x0102040010204080), U64(0x0000000800204080), U64(0x0000000810004080), U64(0x0000000810200080), U64(0x0000000810204000) }, /* 0x88 */
	{ U64(0x0002040800000000), U64(0x0100040800000000), U64(0x0102000800000000), U64(0x0102040010204080), U64(0x0000000800204080), U64(0x0000000810004080), U64(0x0000000810200080), U64(0x0000000810204000) }, /* 0x89 */
	{ U64(0x0002000000000000), U64(0x0100040800000000), U64(0x0002000800000000), U64(0x0002040010204080), U64(0x0000000800204080), U64(0x0000000810004080), U64(0x0000000810200080), U64(0x0000000810204000) }, /* 0x8a */
	{ U64(0x0002000000000000), U64(0x0100040800000000), U64(0x0002000800000000), U64(0x0002040010204080), U64(0x0000000800204080), U64(0x0000000810004080), U64(0x0000000810200080), U64(0x0000000810204000) }, /* 0x8b */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000800000000), U64(0x0000040010204080), U64(0x0000000800204080), U64(0x0000000810004080), U64(0x0000000810200080), U64(0x0000000810204000) }, /* 0x8c */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000800000000), U64(0x0000040010204080), U64(0x0000000800204080), U64(0x0000000810004080), U64(0x0000000810200080), U64(0x0000000810204000) }, /* 0x8d */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000800000000), U64(0x0000040010204080), U64(0x0000000800204080), U64(0x0000000810004080), U64(0x0000000810200080), U64(0x0000000810204000) }, /* 0x8e */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000800000000), U64(0x0000040010204080), U64(0x0000000800204080), U64(0x0000000810004080), U64(0x0000000810200080), U64(0x0000000810204000) }, /* 0x8f */
	{ U64(0x0002040810000000), U64(0x0100040810000000), U64(0x0102000810000000), U64(0x0102040010000000), U64(0x0102040800204080), U64(0x0000000010004080), U64(0x0000000010200080), U64(0x0000000010204000) }, /* 0x90 */
	{ U64(0x0002040810000000), U64(0x0100040810000000), U64(0x0102000810000000), U64(0x0102040010000000), U64(0x0102040800204080), U64(0x0000000010004080), U64(0x0000000010200080), U64(0x0000000010204000) }, /* 0x91 */
	{ U64(0x0002000000000000), U64(0x0100040810000000), U64(0x0002000810000000), U64(0x0002040010000000), U64(0x0002040800204080), U64(0x0000000010004080), U64(0x0000000010200080), U64(0x0000000010204000) }, /* 0x92 */
	{ U64(0x0002000000000000), U64(0x0100040810000000), U64(0x0002000810000000), U64(0x0002040010000000), U64(0x0002040800204080), U64(0x0000000010004080), U64(0x0000000010200080), U64(0x0000000010204000) }, /* 0x93 */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000810000000), U64(0x0000040010000000), U64(0x0000040800204080), U64(0x0000000010004080), U64(0x0000000010200080), U64(0x0000000010204000) }, /* 0x94 */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000810000000), U64(0x0000040010000000), U64(0x0000040800204080), U64(0x0000000010004080), U64(0x0000000010200080), U64(0x0000000010204000) }, /* 0x95 */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000810000000), U64(0x0000040010000000), U64(0x0000040800204080), U64(0x0000000010004080), U64(0x0000000010200080), U64(0x0000000010204000) }, /* 0x96 */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000810000000), U64(0x0000040010000000), U64(0x0000040800204080), U64(0x0000000010004080), U64(0x0000000010200080), U64(0x0000000010204000) }, /* 0x97 */
	{ U64(0x0002040800000000), U64(0x0100040800000000), U64(0x0102000800000000), U64(0x0102040010000000), U64(0x0000000800204080), U64(0x0000000010004080), U64(0x0000000010200080), U64(0x0000000010204000) }, /* 0x98 */
	{ U64(0x0002040800000000), U64(0x0100040800000000), U64(0x0102000800000000), U64(0x0102040010000000), U64(0x0000000800204080), U64(0x0000000010004080), U64(0x0000000010200080), U64(0x0000000010204000) }, /* 0x99 */
	{ U64(0x0002000000000000), U64(0x0100040800000000), U64(0x0002000800000000), U64(0x0002040010000000), U64(0x0000000800204080), U64(0x0000000010004080), U64(0x0000000010200080), U64(0x0000000010204000) }, /* 0x9a */
	{ U64(0x0002000000000000), U64(0x0100040800000000), U64(0x0002000800000000), U64(0x0002040010000000), U64(0x0000000800204080), U64(0x0000000010004080), U64(0x0000000010200080), U64(0x0000000010204000) }, /* 0x9b */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000800000000), U64(0x0000040010000000), U64(0x0000000800204080), U64(0x0000000010004080), U64(0x0000000010200080), U64(0x0000000010204000) }, /* 0x9c */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000800000000), U64(0x0000040010000000), U64(0x0000000800204080), U64(0x0000000010004080), U64(0x0000000010200080), U64(0x0000000010204000) }, /* 0x9d */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000800000000), U64(0x0000040010000000), U64(0x0000000800204080), U64(0x0000000010004080), U64(0x0000000010200080), U64(0x0000000010204000) }, /* 0x9e */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000800000000), U64(0x0000040010000000), U64(0x0000000800204080), U64(0x0000000010004080), U64(0x0000000010200080), U64(0x0000000010204000) }, /* 0x9f */
	{ U64(0x0002040810200000), U64(0x0100040810200000), U64(0x0102000810200000), U64(0x0102040010200000), U64(0x0102040800200000), U64(0x0102040810004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0xa0 */
	{ U64(0x0002040810200000), U64(0x0100040810200000), U64(0x0102000810200000), U64(0x0102040010200000), U64(0x0102040800200000), U64(0x0102040810004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0xa1 */
	{ U64(0x0002000000000000), U64(0x0100040810200000), U64(0x0002000810200000), U64(0x0002040010200000), U64(0x0002040800200000), U64(0x0002040810004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0xa2 */
	{ U64(0x0002000000000000), U64(0x0100040810200000), U64(0x0002000810200000), U64(0x0002040010200000), U64(0x0002040800200000), U64(0x0002040810004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0xa3 */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000810200000), U64(0x0000040010200000), U64(0x0000040800200000), U64(0x0000040810004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0xa4 */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000810200000), U64(0x0000040010200000), U64(0x0000040800200000), U64(0x0000040810004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0xa5 */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000810200000), U64(0x0000040010200000), U64(0x0000040800200000), U64(0x0000040810004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0xa6 */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000810200000), U64(0x0000040010200000), U64(0x0000040800200000), U64(0x0000040810004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0xa7 */
	{ U64(0x0002040800000000), U64(0x0100040800000000), U64(0x0102000800000000), U64(0x0102040010200000), U64(0x0000000800200000), U64(0x0000000810004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0xa8 */
	{ U64(0x0002040800000000), U64(0x0100040800000000), U64(0x0102000800000000), U64(0x0102040010200000), U64(0x0000000800200000), U64(0x0000000810004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0xa9 */
	{ U64(0x0002000000000000), U64(0x0100040800000000), U64(0x0002000800000000), U64(0x0002040010200000), U64(0x0000000800200000), U64(0x0000000810004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0xaa */
	{ U64(0x0002000000000000), U64(0x0100040800000000), U64(0x0002000800000000), U64(0x0002040010200000), U64(0x0000000800200000), U64(0x0000000810004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0xab */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000800000000), U64(0x0000040010200000), U64(0x0000000800200000), U64(0x0000000810004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0xac */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000800000000), U64(0x0000040010200000), U64(0x0000000800200000), U64(0x0000000810004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0xad */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000800000000), U64(0x0000040010200000), U64(0x0000000800200000), U64(0x0000000810004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0xae */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000800000000), U64(0x0000040010200000), U64(0x0000000800200000), U64(0x0000000810004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0xaf */
	{ U64(0x0002040810000000), U64(0x0100040810000000), U64(0x0102000810000000), U64(0x0102040010000000), U64(0x0102040800200000), U64(0x0000000010004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0xb0 */
	{ U64(0x0002040810000000), U64(0x0100040810000000), U64(0x0102000810000000), U64(0x0102040010000000), U64(0x0102040800200000), U64(0x0000000010004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0xb1 */
	{ U64(0x0002000000000000), U64(0x0100040810000000), U64(0x0002000810000000), U64(0x0002040010000000), U64(0x0002040800200000), U64(0x0000000010004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0xb2 */
	{ U64(0x0002000000000000), U64(0x0100040810000000), U64(0x0002000810000000), U64(0x0002040010000000), U64(0x0002040800200000), U64(0x0000000010004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0xb3 */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000810000000), U64(0x0000040010000000), U64(0x0000040800200000), U64(0x0000000010004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0xb4 */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000810000000), U64(0x0000040010000000), U64(0x0000040800200000), U64(0x0000000010004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0xb5 */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000810000000), U64(0x0000040010000000), U64(0x0000040800200000), U64(0x0000000010004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0xb6 */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000810000000), U64(0x0000040010000000), U64(0x0000040800200000), U64(0x0000000010004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0xb7 */
	{ U64(0x0002040800000000), U64(0x0100040800000000), U64(0x0102000800000000), U64(0x0102040010000000), U64(0x0000000800200000), U64(0x0000000010004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0xb8 */
	{ U64(0x0002040800000000), U64(0x0100040800000000), U64(0x0102000800000000), U64(0x0102040010000000), U64(0x0000000800200000), U64(0x0000000010004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0xb9 */
	{ U64(0x0002000000000000), U64(0x0100040800000000), U64(0x0002000800000000), U64(0x0002040010000000), U64(0x0000000800200000), U64(0x0000000010004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0xba */
	{ U64(0x0002000000000000), U64(0x0100040800000000), U64(0x0002000800000000), U64(0x0002040010000000), U64(0x0000000800200000), U64(0x0000000010004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0xbb */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000800000000), U64(0x0000040010000000), U64(0x0000000800200000), U64(0x0000000010004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0xbc */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000800000000), U64(0x0000040010000000), U64(0x0000000800200000), U64(0x0000000010004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0xbd */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000800000000), U64(0x0000040010000000), U64(0x0000000800200000), U64(0x0000000010004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0xbe */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000800000000), U64(0x0000040010000000), U64(0x0000000800200000), U64(0x0000000010004080), U64(0x0000000000200080), U64(0x0000000000204000) }, /* 0xbf */
	{ U64(0x0002040810204000), U64(0x0100040810204000), U64(0x0102000810204000), U64(0x0102040010204000), U64(0x0102040800204000), U64(0x0102040810004000), U64(0x0102040810200080), U64(0x0000000000004000) }, /* 0xc0 */
	{ U64(0x0002040810204000), U64(0x0100040810204000), U64(0x0102000810204000), U64(0x0102040010204000), U64(0x0102040800204000), U64(0x0102040810004000), U64(0x0102040810200080), U64(0x0000000000004000) }, /* 0xc1 */
	{ U64(0x0002000000000000), U64(0x0100040810204000), U64(0x0002000810204000), U64(0x0002040010204000), U64(0x0002040800204000), U64(0x0002040810004000), U64(0x0002040810200080), U64(0x0000000000004000) }, /* 0xc2 */
	{ U64(0x0002000000000000), U64(0x0100040810204000), U64(0x0002000810204000), U64(0x0002040010204000), U64(0x0002040800204000), U64(0x0002040810004000), U64(0x0002040810200080), U64(0x0000000000004000) }, /* 0xc3 */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000810204000), U64(0x0000040010204000), U64(0x0000040800204000), U64(0x0000040810004000), U64(0x0000040810200080), U64(0x0000000000004000) }, /* 0xc4 */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000810204000), U64(0x0000040010204000), U64(0x0000040800204000), U64(0x0000040810004000), U64(0x0000040810200080), U64(0x0000000000004000) }, /* 0xc5 */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000810204000), U64(0x0000040010204000), U64(0x0000040800204000), U64(0x0000040810004000), U64(0x0000040810200080), U64(0x0000000000004000) }, /* 0xc6 */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000810204000), U64(0x0000040010204000), U64(0x0000040800204000), U64(0x0000040810004000), U64(0x0000040810200080), U64(0x0000000000004000) }, /* 0xc7 */
	{ U64(0x0002040800000000), U64(0x0100040800000000), U64(0x0102000800000000), U64(0x0102040010204000), U64(0x0000000800204000), U64(0x0000000810004000), U64(0x0000000810200080), U64(0x0000000000004000) }, /* 0xc8 */
	{ U64(0x0002040800000000), U64(0x0100040800000000), U64(0x0102000800000000), U64(0x0102040010204000), U64(0x0000000800204000), U64(0x0000000810004000), U64(0x0000000810200080), U64(0x0000000000004000) }, /* 0xc9 */
	{ U64(0x0002000000000000), U64(0x0100040800000000), U64(0x0002000800000000), U64(0x0002040010204000), U64(0x0000000800204000), U64(0x0000000810004000), U64(0x0000000810200080), U64(0x0000000000004000) }, /* 0xca */
	{ U64(0x0002000000000000), U64(0x0100040800000000), U64(0x0002000800000000), U64(0x0002040010204000), U64(0x0000000800204000), U64(0x0000000810004000), U64(0x0000000810200080), U64(0x0000000000004000) }, /* 0xcb */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000800000000), U64(0x0000040010204000), U64(0x0000000800204000), U64(0x0000000810004000), U64(0x0000000810200080), U64(0x0000000000004000) }, /* 0xcc */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000800000000), U64(0x0000040010204000), U64(0x0000000800204000), U64(0x0000000810004000), U64(0x0000000810200080), U64(0x0000000000004000) }, /* 0xcd */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000800000000), U64(0x0000040010204000), U64(0x0000000800204000), U64(0x0000000810004000), U64(0x0000000810200080), U64(0x0000000000004000) }, /* 0xce */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000800000000), U64(0x0000040010204000), U64(0x0000000800204000), U64(0x0000000810004000), U64(0x0000000810200080), U64(0x0000000000004000) }, /* 0xcf */
	{ U64(0x0002040810000000), U64(0x0100040810000000), U64(0x0102000810000000), U64(0x0102040010000000), U64(0x0102040800204000), U64(0x0000000010004000), U64(0x0000000010200080), U64(0x0000000000004000) }, /* 0xd0 */
	{ U64(0x0002040810000000), U64(0x0100040810000000), U64(0x0102000810000000), U64(0x0102040010000000), U64(0x0102040800204000), U64(0x0000000010004000), U64(0x0000000010200080), U64(0x0000000000004000) }, /* 0xd1 */
	{ U64(0x0002000000000000), U64(0x0100040810000000), U64(0x0002000810000000), U64(0x0002040010000000), U64(0x0002040800204000), U64(0x0000000010004000), U64(0x0000000010200080), U64(0x0000000000004000) }, /* 0xd2 */
	{ U64(0x0002000000000000), U64(0x0100040810000000), U64(0x0002000810000000), U64(0x0002040010000000), U64(0x0002040800204000), U64(0x0000000010004000), U64(0x0000000010200080), U64(0x0000000000004000) }, /* 0xd3 */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000810000000), U64(0x0000040010000000), U64(0x0000040800204000), U64(0x0000000010004000), U64(0x0000000010200080), U64(0x0000000000004000) }, /* 0xd4 */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000810000000), U64(0x0000040010000000), U64(0x0000040800204000), U64(0x0000000010004000), U64(0x0000000010200080), U64(0x0000000000004000) }, /* 0xd5 */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000810000000), U64(0x0000040010000000), U64(0x0000040800204000), U64(0x0000000010004000), U64(0x0000000010200080), U64(0x0000000000004000) }, /* 0xd6 */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000810000000), U64(0x0000040010000000), U64(0x0000040800204000), U64(0x0000000010004000), U64(0x0000000010200080), U64(0x0000000000004000) }, /* 0xd7 */
	{ U64(0x0002040800000000), U64(0x0100040800000000), U64(0x0102000800000000), U64(0x0102040010000000), U64(0x0000000800204000), U64(0x0000000010004000), U64(0x0000000010200080), U64(0x0000000000004000) }, /* 0xd8 */
	{ U64(0x0002040800000000), U64(0x0100040800000000), U64(0x0102000800000000), U64(0x0102040010000000), U64(0x0000000800204000), U64(0x0000000010004000), U64(0x0000000010200080), U64(0x0000000000004000) }, /* 0xd9 */
	{ U64(0x0002000000000000), U64(0x0100040800000000), U64(0x0002000800000000), U64(0x0002040010000000), U64(0x0000000800204000), U64(0x0000000010004000), U64(0x0000000010200080), U64(0x0000000000004000) }, /* 0xda */
	{ U64(0x0002000000000000), U64(0x0100040800000000), U64(0x0002000800000000), U64(0x0002040010000000), U64(0x0000000800204000), U64(0x0000000010004000), U64(0x0000000010200080), U64(0x0000000000004000) }, /* 0xdb */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000800000000), U64(0x0000040010000000), U64(0x0000000800204000), U64(0x0000000010004000), U64(0x0000000010200080), U64(0x0000000000004000) }, /* 0xdc */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000800000000), U64(0x0000040010000000), U64(0x0000000800204000), U64(0x0000000010004000), U64(0x0000000010200080), U64(0x0000000000004000) }, /* 0xdd */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000800000000), U64(0x0000040010000000), U64(0x0000000800204000), U64(0x0000000010004000), U64(0x0000000010200080), U64(0x0000000000004000) }, /* 0xde */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000800000000), U64(0x0000040010000000), U64(0x0000000800204000), U64(0x0000000010004000), U64(0x0000000010200080), U64(0x0000000000004000) }, /* 0xdf */
	{ U64(0x0002040810200000), U64(0x0100040810200000), U64(0x0102000810200000), U64(0x0102040010200000), U64(0x0102040800200000), U64(0x0102040810004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0xe0 */
	{ U64(0x0002040810200000), U64(0x0100040810200000), U64(0x0102000810200000), U64(0x0102040010200000), U64(0x0102040800200000), U64(0x0102040810004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0xe1 */
	{ U64(0x0002000000000000), U64(0x0100040810200000), U64(0x0002000810200000), U64(0x0002040010200000), U64(0x0002040800200000), U64(0x0002040810004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0xe2 */
	{ U64(0x0002000000000000), U64(0x0100040810200000), U64(0x0002000810200000), U64(0x0002040010200000), U64(0x0002040800200000), U64(0x0002040810004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0xe3 */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000810200000), U64(0x0000040010200000), U64(0x0000040800200000), U64(0x0000040810004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0xe4 */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000810200000), U64(0x0000040010200000), U64(0x0000040800200000), U64(0x0000040810004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0xe5 */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000810200000), U64(0x0000040010200000), U64(0x0000040800200000), U64(0x0000040810004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0xe6 */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000810200000), U64(0x0000040010200000), U64(0x0000040800200000), U64(0x0000040810004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0xe7 */
	{ U64(0x0002040800000000), U64(0x0100040800000000), U64(0x0102000800000000), U64(0x0102040010200000), U64(0x0000000800200000), U64(0x0000000810004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0xe8 */
	{ U64(0x0002040800000000), U64(0x0100040800000000), U64(0x0102000800000000), U64(0x0102040010200000), U64(0x0000000800200000), U64(0x0000000810004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0xe9 */
	{ U64(0x0002000000000000), U64(0x0100040800000000), U64(0x0002000800000000), U64(0x0002040010200000), U64(0x0000000800200000), U64(0x0000000810004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0xea */
	{ U64(0x0002000000000000), U64(0x0100040800000000), U64(0x0002000800000000), U64(0x0002040010200000), U64(0x0000000800200000), U64(0x0000000810004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0xeb */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000800000000), U64(0x0000040010200000), U64(0x0000000800200000), U64(0x0000000810004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0xec */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000800000000), U64(0x0000040010200000), U64(0x0000000800200000), U64(0x0000000810004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0xed */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000800000000), U64(0x0000040010200000), U64(0x0000000800200000), U64(0x0000000810004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0xee */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000800000000), U64(0x0000040010200000), U64(0x0000000800200000), U64(0x0000000810004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0xef */
	{ U64(0x0002040810000000), U64(0x0100040810000000), U64(0x0102000810000000), U64(0x0102040010000000), U64(0x0102040800200000), U64(0x0000000010004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0xf0 */
	{ U64(0x0002040810000000), U64(0x0100040810000000), U64(0x0102000810000000), U64(0x0102040010000000), U64(0x0102040800200000), U64(0x0000000010004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0xf1 */
	{ U64(0x0002000000000000), U64(0x0100040810000000), U64(0x0002000810000000), U64(0x0002040010000000), U64(0x0002040800200000), U64(0x0000000010004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0xf2 */
	{ U64(0x0002000000000000), U64(0x0100040810000000), U64(0x0002000810000000), U64(0x0002040010000000), U64(0x0002040800200000), U64(0x0000000010004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0xf3 */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000810000000), U64(0x0000040010000000), U64(0x0000040800200000), U64(0x0000000010004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0xf4 */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000810000000), U64(0x0000040010000000), U64(0x0000040800200000), U64(0x0000000010004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0xf5 */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000810000000), U64(0x0000040010000000), U64(0x0000040800200000), U64(0x0000000010004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0xf6 */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000810000000), U64(0x0000040010000000), U64(0x0000040800200000), U64(0x0000000010004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0xf7 */
	{ U64(0x0002040800000000), U64(0x0100040800000000), U64(0x0102000800000000), U64(0x0102040010000000), U64(0x0000000800200000), U64(0x0000000010004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0xf8 */
	{ U64(0x0002040800000000), U64(0x0100040800000000), U64(0x0102000800000000), U64(0x0102040010000000), U64(0x0000000800200000), U64(0x0000000010004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0xf9 */
	{ U64(0x0002000000000000), U64(0x0100040800000000), U64(0x0002000800000000), U64(0x0002040010000000), U64(0x0000000800200000), U64(0x0000000010004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0xfa */
	{ U64(0x0002000000000000), U64(0x0100040800000000), U64(0x0002000800000000), U64(0x0002040010000000), U64(0x0000000800200000), U64(0x0000000010004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0xfb */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000800000000), U64(0x0000040010000000), U64(0x0000000800200000), U64(0x0000000010004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0xfc */
	{ U64(0x0002040000000000), U64(0x0100040000000000), U64(0x0102000800000000), U64(0x0000040010000000), U64(0x0000000800200000), U64(0x0000000010004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0xfd */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000800000000), U64(0x0000040010000000), U64(0x0000000800200000), U64(0x0000000010004000), U64(0x0000000000200080), U64(0x0000000000004000) }, /* 0xfe */
	{ U64(0x0002000000000000), U64(0x0100040000000000), U64(0x0002000800000000), U64(0x0000040010000000), U64(0x0000000800200000), U64(0x0000000010004000), U64(0x0000000000200080), U64(0x0000000000004000) } /* 0xff */
};

uint64_t rot315attacks[256][8] = {
	{ U64(0x8040201008040200), U64(0x8040201008040001), U64(0x8040201008000201), U64(0x8040201000040201), U64(0x8040200008040201), U64(0x8040001008040201), U64(0x8000201008040201), U64(0x0040201008040201) }, /* 0x00 */
	{ U64(0x8040201008040200), U64(0x8040201008040001), U64(0x8040201008000201), U64(0x8040201000040201), U64(0x8040200008040201), U64(0x8040001008040201), U64(0x8000201008040201), U64(0x0040201008040201) }, /* 0x01 */
	{ U64(0x0000000000000200), U64(0x8040201008040001), U64(0x8040201008000200), U64(0x8040201000040200), U64(0x8040200008040200), U64(0x8040001008040200), U64(0x8000201008040200), U64(0x0040201008040200) }, /* 0x02 */
	{ U64(0x0000000000000200), U64(0x8040201008040001), U64(0x8040201008000200), U64(0x8040201000040200), U64(0x8040200008040200), U64(0x8040001008040200), U64(0x8000201008040200), U64(0x0040201008040200) }, /* 0x03 */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x8040201008000201), U64(0x8040201000040000), U64(0x8040200008040000), U64(0x8040001008040000), U64(0x8000201008040000), U64(0x0040201008040000) }, /* 0x04 */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x8040201008000201), U64(0x8040201000040000), U64(0x8040200008040000), U64(0x8040001008040000), U64(0x8000201008040000), U64(0x0040201008040000) }, /* 0x05 */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x8040201008000200), U64(0x8040201000040000), U64(0x8040200008040000), U64(0x8040001008040000), U64(0x8000201008040000), U64(0x0040201008040000) }, /* 0x06 */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x8040201008000200), U64(0x8040201000040000), U64(0x8040200008040000), U64(0x8040001008040000), U64(0x8000201008040000), U64(0x0040201008040000) }, /* 0x07 */
	{ U64(0x0000000008040200), U64(0x0000000008040001), U64(0x0000000008000201), U64(0x8040201000040201), U64(0x8040200008000000), U64(0x8040001008000000), U64(0x8000201008000000), U64(0x0040201008000000) }, /* 0x08 */
	{ U64(0x0000000008040200), U64(0x0000000008040001), U64(0x0000000008000201), U64(0x8040201000040201), U64(0x8040200008000000), U64(0x8040001008000000), U64(0x8000201008000000), U64(0x0040201008000000) }, /* 0x09 */
	{ U64(0x0000000000000200), U64(0x0000000008040001), U64(0x0000000008000200), U64(0x8040201000040200), U64(0x8040200008000000), U64(0x8040001008000000), U64(0x8000201008000000), U64(0x0040201008000000) }, /* 0x0a */
	{ U64(0x0000000000000200), U64(0x0000000008040001), U64(0x0000000008000200), U64(0x8040201000040200), U64(0x8040200008000000), U64(0x8040001008000000), U64(0x8000201008000000), U64(0x0040201008000000) }, /* 0x0b */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0000000008000201), U64(0x8040201000040000), U64(0x8040200008000000), U64(0x8040001008000000), U64(0x8000201008000000), U64(0x0040201008000000) }, /* 0x0c */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0000000008000201), U64(0x8040201000040000), U64(0x8040200008000000), U64(0x8040001008000000), U64(0x8000201008000000), U64(0x0040201008000000) }, /* 0x0d */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0000000008000200), U64(0x8040201000040000), U64(0x8040200008000000), U64(0x8040001008000000), U64(0x8000201008000000), U64(0x0040201008000000) }, /* 0x0e */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0000000008000200), U64(0x8040201000040000), U64(0x8040200008000000), U64(0x8040001008000000), U64(0x8000201008000000), U64(0x0040201008000000) }, /* 0x0f */
	{ U64(0x0000001008040200), U64(0x0000001008040001), U64(0x0000001008000201), U64(0x0000001000040201), U64(0x8040200008040201), U64(0x8040001000000000), U64(0x8000201000000000), U64(0x0040201000000000) }, /* 0x10 */
	{ U64(0x0000001008040200), U64(0x0000001008040001), U64(0x0000001008000201), U64(0x0000001000040201), U64(0x8040200008040201), U64(0x8040001000000000), U64(0x8000201000000000), U64(0x0040201000000000) }, /* 0x11 */
	{ U64(0x0000000000000200), U64(0x0000001008040001), U64(0x0000001008000200), U64(0x0000001000040200), U64(0x8040200008040200), U64(0x8040001000000000), U64(0x8000201000000000), U64(0x0040201000000000) }, /* 0x12 */
	{ U64(0x0000000000000200), U64(0x0000001008040001), U64(0x0000001008000200), U64(0x0000001000040200), U64(0x8040200008040200), U64(0x8040001000000000), U64(0x8000201000000000), U64(0x0040201000000000) }, /* 0x13 */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0000001008000201), U64(0x0000001000040000), U64(0x8040200008040000), U64(0x8040001000000000), U64(0x8000201000000000), U64(0x0040201000000000) }, /* 0x14 */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0000001008000201), U64(0x0000001000040000), U64(0x8040200008040000), U64(0x8040001000000000), U64(0x8000201000000000), U64(0x0040201000000000) }, /* 0x15 */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0000001008000200), U64(0x0000001000040000), U64(0x8040200008040000), U64(0x8040001000000000), U64(0x8000201000000000), U64(0x0040201000000000) }, /* 0x16 */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0000001008000200), U64(0x0000001000040000), U64(0x8040200008040000), U64(0x8040001000000000), U64(0x8000201000000000), U64(0x0040201000000000) }, /* 0x17 */
	{ U64(0x0000000008040200), U64(0x0000000008040001), U64(0x0000000008000201), U64(0x0000001000040201), U64(0x8040200008000000), U64(0x8040001000000000), U64(0x8000201000000000), U64(0x0040201000000000) }, /* 0x18 */
	{ U64(0x0000000008040200), U64(0x0000000008040001), U64(0x0000000008000201), U64(0x0000001000040201), U64(0x8040200008000000), U64(0x8040001000000000), U64(0x8000201000000000), U64(0x0040201000000000) }, /* 0x19 */
	{ U64(0x0000000000000200), U64(0x0000000008040001), U64(0x0000000008000200), U64(0x0000001000040200), U64(0x8040200008000000), U64(0x8040001000000000), U64(0x8000201000000000), U64(0x0040201000000000) }, /* 0x1a */
	{ U64(0x0000000000000200), U64(0x0000000008040001), U64(0x0000000008000200), U64(0x0000001000040200), U64(0x8040200008000000), U64(0x8040001000000000), U64(0x8000201000000000), U64(0x0040201000000000) }, /* 0x1b */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0000000008000201), U64(0x0000001000040000), U64(0x8040200008000000), U64(0x8040001000000000), U64(0x8000201000000000), U64(0x0040201000000000) }, /* 0x1c */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0000000008000201), U64(0x0000001000040000), U64(0x8040200008000000), U64(0x8040001000000000), U64(0x8000201000000000), U64(0x0040201000000000) }, /* 0x1d */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0000000008000200), U64(0x0000001000040000), U64(0x8040200008000000), U64(0x8040001000000000), U64(0x8000201000000000), U64(0x0040201000000000) }, /* 0x1e */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0000000008000200), U64(0x0000001000040000), U64(0x8040200008000000), U64(0x8040001000000000), U64(0x8000201000000000), U64(0x0040201000000000) }, /* 0x1f */
	{ U64(0x0000201008040200), U64(0x0000201008040001), U64(0x0000201008000201), U64(0x0000201000040201), U64(0x0000200008040201), U64(0x8040001008040201), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0x20 */
	{ U64(0x0000201008040200), U64(0x0000201008040001), U64(0x0000201008000201), U64(0x0000201000040201), U64(0x0000200008040201), U64(0x8040001008040201), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0x21 */
	{ U64(0x0000000000000200), U64(0x0000201008040001), U64(0x0000201008000200), U64(0x0000201000040200), U64(0x0000200008040200), U64(0x8040001008040200), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0x22 */
	{ U64(0x0000000000000200), U64(0x0000201008040001), U64(0x0000201008000200), U64(0x0000201000040200), U64(0x0000200008040200), U64(0x8040001008040200), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0x23 */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0000201008000201), U64(0x0000201000040000), U64(0x0000200008040000), U64(0x8040001008040000), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0x24 */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0000201008000201), U64(0x0000201000040000), U64(0x0000200008040000), U64(0x8040001008040000), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0x25 */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0000201008000200), U64(0x0000201000040000), U64(0x0000200008040000), U64(0x8040001008040000), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0x26 */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0000201008000200), U64(0x0000201000040000), U64(0x0000200008040000), U64(0x8040001008040000), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0x27 */
	{ U64(0x0000000008040200), U64(0x0000000008040001), U64(0x0000000008000201), U64(0x0000201000040201), U64(0x0000200008000000), U64(0x8040001008000000), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0x28 */
	{ U64(0x0000000008040200), U64(0x0000000008040001), U64(0x0000000008000201), U64(0x0000201000040201), U64(0x0000200008000000), U64(0x8040001008000000), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0x29 */
	{ U64(0x0000000000000200), U64(0x0000000008040001), U64(0x0000000008000200), U64(0x0000201000040200), U64(0x0000200008000000), U64(0x8040001008000000), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0x2a */
	{ U64(0x0000000000000200), U64(0x0000000008040001), U64(0x0000000008000200), U64(0x0000201000040200), U64(0x0000200008000000), U64(0x8040001008000000), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0x2b */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0000000008000201), U64(0x0000201000040000), U64(0x0000200008000000), U64(0x8040001008000000), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0x2c */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0000000008000201), U64(0x0000201000040000), U64(0x0000200008000000), U64(0x8040001008000000), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0x2d */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0000000008000200), U64(0x0000201000040000), U64(0x0000200008000000), U64(0x8040001008000000), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0x2e */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0000000008000200), U64(0x0000201000040000), U64(0x0000200008000000), U64(0x8040001008000000), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0x2f */
	{ U64(0x0000001008040200), U64(0x0000001008040001), U64(0x0000001008000201), U64(0x0000001000040201), U64(0x0000200008040201), U64(0x8040001000000000), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0x30 */
	{ U64(0x0000001008040200), U64(0x0000001008040001), U64(0x0000001008000201), U64(0x0000001000040201), U64(0x0000200008040201), U64(0x8040001000000000), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0x31 */
	{ U64(0x0000000000000200), U64(0x0000001008040001), U64(0x0000001008000200), U64(0x0000001000040200), U64(0x0000200008040200), U64(0x8040001000000000), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0x32 */
	{ U64(0x0000000000000200), U64(0x0000001008040001), U64(0x0000001008000200), U64(0x0000001000040200), U64(0x0000200008040200), U64(0x8040001000000000), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0x33 */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0000001008000201), U64(0x0000001000040000), U64(0x0000200008040000), U64(0x8040001000000000), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0x34 */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0000001008000201), U64(0x0000001000040000), U64(0x0000200008040000), U64(0x8040001000000000), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0x35 */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0000001008000200), U64(0x0000001000040000), U64(0x0000200008040000), U64(0x8040001000000000), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0x36 */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0000001008000200), U64(0x0000001000040000), U64(0x0000200008040000), U64(0x8040001000000000), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0x37 */
	{ U64(0x0000000008040200), U64(0x0000000008040001), U64(0x0000000008000201), U64(0x0000001000040201), U64(0x0000200008000000), U64(0x8040001000000000), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0x38 */
	{ U64(0x0000000008040200), U64(0x0000000008040001), U64(0x0000000008000201), U64(0x0000001000040201), U64(0x0000200008000000), U64(0x8040001000000000), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0x39 */
	{ U64(0x0000000000000200), U64(0x0000000008040001), U64(0x0000000008000200), U64(0x0000001000040200), U64(0x0000200008000000), U64(0x8040001000000000), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0x3a */
	{ U64(0x0000000000000200), U64(0x0000000008040001), U64(0x0000000008000200), U64(0x0000001000040200), U64(0x0000200008000000), U64(0x8040001000000000), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0x3b */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0000000008000201), U64(0x0000001000040000), U64(0x0000200008000000), U64(0x8040001000000000), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0x3c */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0000000008000201), U64(0x0000001000040000), U64(0x0000200008000000), U64(0x8040001000000000), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0x3d */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0000000008000200), U64(0x0000001000040000), U64(0x0000200008000000), U64(0x8040001000000000), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0x3e */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0000000008000200), U64(0x0000001000040000), U64(0x0000200008000000), U64(0x8040001000000000), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0x3f */
	{ U64(0x0040201008040200), U64(0x0040201008040001), U64(0x0040201008000201), U64(0x0040201000040201), U64(0x0040200008040201), U64(0x0040001008040201), U64(0x8000201008040201), U64(0x0040000000000000) }, /* 0x40 */
	{ U64(0x0040201008040200), U64(0x0040201008040001), U64(0x0040201008000201), U64(0x0040201000040201), U64(0x0040200008040201), U64(0x0040001008040201), U64(0x8000201008040201), U64(0x0040000000000000) }, /* 0x41 */
	{ U64(0x0000000000000200), U64(0x0040201008040001), U64(0x0040201008000200), U64(0x0040201000040200), U64(0x0040200008040200), U64(0x0040001008040200), U64(0x8000201008040200), U64(0x0040000000000000) }, /* 0x42 */
	{ U64(0x0000000000000200), U64(0x0040201008040001), U64(0x0040201008000200), U64(0x0040201000040200), U64(0x0040200008040200), U64(0x0040001008040200), U64(0x8000201008040200), U64(0x0040000000000000) }, /* 0x43 */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0040201008000201), U64(0x0040201000040000), U64(0x0040200008040000), U64(0x0040001008040000), U64(0x8000201008040000), U64(0x0040000000000000) }, /* 0x44 */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0040201008000201), U64(0x0040201000040000), U64(0x0040200008040000), U64(0x0040001008040000), U64(0x8000201008040000), U64(0x0040000000000000) }, /* 0x45 */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0040201008000200), U64(0x0040201000040000), U64(0x0040200008040000), U64(0x0040001008040000), U64(0x8000201008040000), U64(0x0040000000000000) }, /* 0x46 */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0040201008000200), U64(0x0040201000040000), U64(0x0040200008040000), U64(0x0040001008040000), U64(0x8000201008040000), U64(0x0040000000000000) }, /* 0x47 */
	{ U64(0x0000000008040200), U64(0x0000000008040001), U64(0x0000000008000201), U64(0x0040201000040201), U64(0x0040200008000000), U64(0x0040001008000000), U64(0x8000201008000000), U64(0x0040000000000000) }, /* 0x48 */
	{ U64(0x0000000008040200), U64(0x0000000008040001), U64(0x0000000008000201), U64(0x0040201000040201), U64(0x0040200008000000), U64(0x0040001008000000), U64(0x8000201008000000), U64(0x0040000000000000) }, /* 0x49 */
	{ U64(0x0000000000000200), U64(0x0000000008040001), U64(0x0000000008000200), U64(0x0040201000040200), U64(0x0040200008000000), U64(0x0040001008000000), U64(0x8000201008000000), U64(0x0040000000000000) }, /* 0x4a */
	{ U64(0x0000000000000200), U64(0x0000000008040001), U64(0x0000000008000200), U64(0x0040201000040200), U64(0x0040200008000000), U64(0x0040001008000000), U64(0x8000201008000000), U64(0x0040000000000000) }, /* 0x4b */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0000000008000201), U64(0x0040201000040000), U64(0x0040200008000000), U64(0x0040001008000000), U64(0x8000201008000000), U64(0x0040000000000000) }, /* 0x4c */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0000000008000201), U64(0x0040201000040000), U64(0x0040200008000000), U64(0x0040001008000000), U64(0x8000201008000000), U64(0x0040000000000000) }, /* 0x4d */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0000000008000200), U64(0x0040201000040000), U64(0x0040200008000000), U64(0x0040001008000000), U64(0x8000201008000000), U64(0x0040000000000000) }, /* 0x4e */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0000000008000200), U64(0x0040201000040000), U64(0x0040200008000000), U64(0x0040001008000000), U64(0x8000201008000000), U64(0x0040000000000000) }, /* 0x4f */
	{ U64(0x0000001008040200), U64(0x0000001008040001), U64(0x0000001008000201), U64(0x0000001000040201), U64(0x0040200008040201), U64(0x0040001000000000), U64(0x8000201000000000), U64(0x0040000000000000) }, /* 0x50 */
	{ U64(0x0000001008040200), U64(0x0000001008040001), U64(0x0000001008000201), U64(0x0000001000040201), U64(0x0040200008040201), U64(0x0040001000000000), U64(0x8000201000000000), U64(0x0040000000000000) }, /* 0x51 */
	{ U64(0x0000000000000200), U64(0x0000001008040001), U64(0x0000001008000200), U64(0x0000001000040200), U64(0x0040200008040200), U64(0x0040001000000000), U64(0x8000201000000000), U64(0x0040000000000000) }, /* 0x52 */
	{ U64(0x0000000000000200), U64(0x0000001008040001), U64(0x0000001008000200), U64(0x0000001000040200), U64(0x0040200008040200), U64(0x0040001000000000), U64(0x8000201000000000), U64(0x0040000000000000) }, /* 0x53 */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0000001008000201), U64(0x0000001000040000), U64(0x0040200008040000), U64(0x0040001000000000), U64(0x8000201000000000), U64(0x0040000000000000) }, /* 0x54 */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0000001008000201), U64(0x0000001000040000), U64(0x0040200008040000), U64(0x0040001000000000), U64(0x8000201000000000), U64(0x0040000000000000) }, /* 0x55 */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0000001008000200), U64(0x0000001000040000), U64(0x0040200008040000), U64(0x0040001000000000), U64(0x8000201000000000), U64(0x0040000000000000) }, /* 0x56 */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0000001008000200), U64(0x0000001000040000), U64(0x0040200008040000), U64(0x0040001000000000), U64(0x8000201000000000), U64(0x0040000000000000) }, /* 0x57 */
	{ U64(0x0000000008040200), U64(0x0000000008040001), U64(0x0000000008000201), U64(0x0000001000040201), U64(0x0040200008000000), U64(0x0040001000000000), U64(0x8000201000000000), U64(0x0040000000000000) }, /* 0x58 */
	{ U64(0x0000000008040200), U64(0x0000000008040001), U64(0x0000000008000201), U64(0x0000001000040201), U64(0x0040200008000000), U64(0x0040001000000000), U64(0x8000201000000000), U64(0x0040000000000000) }, /* 0x59 */
	{ U64(0x0000000000000200), U64(0x0000000008040001), U64(0x0000000008000200), U64(0x0000001000040200), U64(0x0040200008000000), U64(0x0040001000000000), U64(0x8000201000000000), U64(0x0040000000000000) }, /* 0x5a */
	{ U64(0x0000000000000200), U64(0x0000000008040001), U64(0x0000000008000200), U64(0x0000001000040200), U64(0x0040200008000000), U64(0x0040001000000000), U64(0x8000201000000000), U64(0x0040000000000000) }, /* 0x5b */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0000000008000201), U64(0x0000001000040000), U64(0x0040200008000000), U64(0x0040001000000000), U64(0x8000201000000000), U64(0x0040000000000000) }, /* 0x5c */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0000000008000201), U64(0x0000001000040000), U64(0x0040200008000000), U64(0x0040001000000000), U64(0x8000201000000000), U64(0x0040000000000000) }, /* 0x5d */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0000000008000200), U64(0x0000001000040000), U64(0x0040200008000000), U64(0x0040001000000000), U64(0x8000201000000000), U64(0x0040000000000000) }, /* 0x5e */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0000000008000200), U64(0x0000001000040000), U64(0x0040200008000000), U64(0x0040001000000000), U64(0x8000201000000000), U64(0x0040000000000000) }, /* 0x5f */
	{ U64(0x0000201008040200), U64(0x0000201008040001), U64(0x0000201008000201), U64(0x0000201000040201), U64(0x0000200008040201), U64(0x0040001008040201), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0x60 */
	{ U64(0x0000201008040200), U64(0x0000201008040001), U64(0x0000201008000201), U64(0x0000201000040201), U64(0x0000200008040201), U64(0x0040001008040201), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0x61 */
	{ U64(0x0000000000000200), U64(0x0000201008040001), U64(0x0000201008000200), U64(0x0000201000040200), U64(0x0000200008040200), U64(0x0040001008040200), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0x62 */
	{ U64(0x0000000000000200), U64(0x0000201008040001), U64(0x0000201008000200), U64(0x0000201000040200), U64(0x0000200008040200), U64(0x0040001008040200), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0x63 */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0000201008000201), U64(0x0000201000040000), U64(0x0000200008040000), U64(0x0040001008040000), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0x64 */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0000201008000201), U64(0x0000201000040000), U64(0x0000200008040000), U64(0x0040001008040000), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0x65 */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0000201008000200), U64(0x0000201000040000), U64(0x0000200008040000), U64(0x0040001008040000), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0x66 */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0000201008000200), U64(0x0000201000040000), U64(0x0000200008040000), U64(0x0040001008040000), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0x67 */
	{ U64(0x0000000008040200), U64(0x0000000008040001), U64(0x0000000008000201), U64(0x0000201000040201), U64(0x0000200008000000), U64(0x0040001008000000), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0x68 */
	{ U64(0x0000000008040200), U64(0x0000000008040001), U64(0x0000000008000201), U64(0x0000201000040201), U64(0x0000200008000000), U64(0x0040001008000000), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0x69 */
	{ U64(0x0000000000000200), U64(0x0000000008040001), U64(0x0000000008000200), U64(0x0000201000040200), U64(0x0000200008000000), U64(0x0040001008000000), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0x6a */
	{ U64(0x0000000000000200), U64(0x0000000008040001), U64(0x0000000008000200), U64(0x0000201000040200), U64(0x0000200008000000), U64(0x0040001008000000), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0x6b */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0000000008000201), U64(0x0000201000040000), U64(0x0000200008000000), U64(0x0040001008000000), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0x6c */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0000000008000201), U64(0x0000201000040000), U64(0x0000200008000000), U64(0x0040001008000000), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0x6d */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0000000008000200), U64(0x0000201000040000), U64(0x0000200008000000), U64(0x0040001008000000), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0x6e */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0000000008000200), U64(0x0000201000040000), U64(0x0000200008000000), U64(0x0040001008000000), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0x6f */
	{ U64(0x0000001008040200), U64(0x0000001008040001), U64(0x0000001008000201), U64(0x0000001000040201), U64(0x0000200008040201), U64(0x0040001000000000), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0x70 */
	{ U64(0x0000001008040200), U64(0x0000001008040001), U64(0x0000001008000201), U64(0x0000001000040201), U64(0x0000200008040201), U64(0x0040001000000000), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0x71 */
	{ U64(0x0000000000000200), U64(0x0000001008040001), U64(0x0000001008000200), U64(0x0000001000040200), U64(0x0000200008040200), U64(0x0040001000000000), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0x72 */
	{ U64(0x0000000000000200), U64(0x0000001008040001), U64(0x0000001008000200), U64(0x0000001000040200), U64(0x0000200008040200), U64(0x0040001000000000), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0x73 */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0000001008000201), U64(0x0000001000040000), U64(0x0000200008040000), U64(0x0040001000000000), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0x74 */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0000001008000201), U64(0x0000001000040000), U64(0x0000200008040000), U64(0x0040001000000000), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0x75 */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0000001008000200), U64(0x0000001000040000), U64(0x0000200008040000), U64(0x0040001000000000), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0x76 */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0000001008000200), U64(0x0000001000040000), U64(0x0000200008040000), U64(0x0040001000000000), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0x77 */
	{ U64(0x0000000008040200), U64(0x0000000008040001), U64(0x0000000008000201), U64(0x0000001000040201), U64(0x0000200008000000), U64(0x0040001000000000), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0x78 */
	{ U64(0x0000000008040200), U64(0x0000000008040001), U64(0x0000000008000201), U64(0x0000001000040201), U64(0x0000200008000000), U64(0x0040001000000000), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0x79 */
	{ U64(0x0000000000000200), U64(0x0000000008040001), U64(0x0000000008000200), U64(0x0000001000040200), U64(0x0000200008000000), U64(0x0040001000000000), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0x7a */
	{ U64(0x0000000000000200), U64(0x0000000008040001), U64(0x0000000008000200), U64(0x0000001000040200), U64(0x0000200008000000), U64(0x0040001000000000), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0x7b */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0000000008000201), U64(0x0000001000040000), U64(0x0000200008000000), U64(0x0040001000000000), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0x7c */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0000000008000201), U64(0x0000001000040000), U64(0x0000200008000000), U64(0x0040001000000000), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0x7d */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0000000008000200), U64(0x0000001000040000), U64(0x0000200008000000), U64(0x0040001000000000), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0x7e */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0000000008000200), U64(0x0000001000040000), U64(0x0000200008000000), U64(0x0040001000000000), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0x7f */
	{ U64(0x8040201008040200), U64(0x8040201008040001), U64(0x8040201008000201), U64(0x8040201000040201), U64(0x8040200008040201), U64(0x8040001008040201), U64(0x8000201008040201), U64(0x0040201008040201) }, /* 0x80 */
	{ U64(0x8040201008040200), U64(0x8040201008040001), U64(0x8040201008000201), U64(0x8040201000040201), U64(0x8040200008040201), U64(0x8040001008040201), U64(0x8000201008040201), U64(0x0040201008040201) }, /* 0x81 */
	{ U64(0x0000000000000200), U64(0x8040201008040001), U64(0x8040201008000200), U64(0x8040201000040200), U64(0x8040200008040200), U64(0x8040001008040200), U64(0x8000201008040200), U64(0x0040201008040200) }, /* 0x82 */
	{ U64(0x0000000000000200), U64(0x8040201008040001), U64(0x8040201008000200), U64(0x8040201000040200), U64(0x8040200008040200), U64(0x8040001008040200), U64(0x8000201008040200), U64(0x0040201008040200) }, /* 0x83 */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x8040201008000201), U64(0x8040201000040000), U64(0x8040200008040000), U64(0x8040001008040000), U64(0x8000201008040000), U64(0x0040201008040000) }, /* 0x84 */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x8040201008000201), U64(0x8040201000040000), U64(0x8040200008040000), U64(0x8040001008040000), U64(0x8000201008040000), U64(0x0040201008040000) }, /* 0x85 */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x8040201008000200), U64(0x8040201000040000), U64(0x8040200008040000), U64(0x8040001008040000), U64(0x8000201008040000), U64(0x0040201008040000) }, /* 0x86 */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x8040201008000200), U64(0x8040201000040000), U64(0x8040200008040000), U64(0x8040001008040000), U64(0x8000201008040000), U64(0x0040201008040000) }, /* 0x87 */
	{ U64(0x0000000008040200), U64(0x0000000008040001), U64(0x0000000008000201), U64(0x8040201000040201), U64(0x8040200008000000), U64(0x8040001008000000), U64(0x8000201008000000), U64(0x0040201008000000) }, /* 0x88 */
	{ U64(0x0000000008040200), U64(0x0000000008040001), U64(0x0000000008000201), U64(0x8040201000040201), U64(0x8040200008000000), U64(0x8040001008000000), U64(0x8000201008000000), U64(0x0040201008000000) }, /* 0x89 */
	{ U64(0x0000000000000200), U64(0x0000000008040001), U64(0x0000000008000200), U64(0x8040201000040200), U64(0x8040200008000000), U64(0x8040001008000000), U64(0x8000201008000000), U64(0x0040201008000000) }, /* 0x8a */
	{ U64(0x0000000000000200), U64(0x0000000008040001), U64(0x0000000008000200), U64(0x8040201000040200), U64(0x8040200008000000), U64(0x8040001008000000), U64(0x8000201008000000), U64(0x0040201008000000) }, /* 0x8b */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0000000008000201), U64(0x8040201000040000), U64(0x8040200008000000), U64(0x8040001008000000), U64(0x8000201008000000), U64(0x0040201008000000) }, /* 0x8c */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0000000008000201), U64(0x8040201000040000), U64(0x8040200008000000), U64(0x8040001008000000), U64(0x8000201008000000), U64(0x0040201008000000) }, /* 0x8d */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0000000008000200), U64(0x8040201000040000), U64(0x8040200008000000), U64(0x8040001008000000), U64(0x8000201008000000), U64(0x0040201008000000) }, /* 0x8e */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0000000008000200), U64(0x8040201000040000), U64(0x8040200008000000), U64(0x8040001008000000), U64(0x8000201008000000), U64(0x0040201008000000) }, /* 0x8f */
	{ U64(0x0000001008040200), U64(0x0000001008040001), U64(0x0000001008000201), U64(0x0000001000040201), U64(0x8040200008040201), U64(0x8040001000000000), U64(0x8000201000000000), U64(0x0040201000000000) }, /* 0x90 */
	{ U64(0x0000001008040200), U64(0x0000001008040001), U64(0x0000001008000201), U64(0x0000001000040201), U64(0x8040200008040201), U64(0x8040001000000000), U64(0x8000201000000000), U64(0x0040201000000000) }, /* 0x91 */
	{ U64(0x0000000000000200), U64(0x0000001008040001), U64(0x0000001008000200), U64(0x0000001000040200), U64(0x8040200008040200), U64(0x8040001000000000), U64(0x8000201000000000), U64(0x0040201000000000) }, /* 0x92 */
	{ U64(0x0000000000000200), U64(0x0000001008040001), U64(0x0000001008000200), U64(0x0000001000040200), U64(0x8040200008040200), U64(0x8040001000000000), U64(0x8000201000000000), U64(0x0040201000000000) }, /* 0x93 */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0000001008000201), U64(0x0000001000040000), U64(0x8040200008040000), U64(0x8040001000000000), U64(0x8000201000000000), U64(0x0040201000000000) }, /* 0x94 */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0000001008000201), U64(0x0000001000040000), U64(0x8040200008040000), U64(0x8040001000000000), U64(0x8000201000000000), U64(0x0040201000000000) }, /* 0x95 */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0000001008000200), U64(0x0000001000040000), U64(0x8040200008040000), U64(0x8040001000000000), U64(0x8000201000000000), U64(0x0040201000000000) }, /* 0x96 */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0000001008000200), U64(0x0000001000040000), U64(0x8040200008040000), U64(0x8040001000000000), U64(0x8000201000000000), U64(0x0040201000000000) }, /* 0x97 */
	{ U64(0x0000000008040200), U64(0x0000000008040001), U64(0x0000000008000201), U64(0x0000001000040201), U64(0x8040200008000000), U64(0x8040001000000000), U64(0x8000201000000000), U64(0x0040201000000000) }, /* 0x98 */
	{ U64(0x0000000008040200), U64(0x0000000008040001), U64(0x0000000008000201), U64(0x0000001000040201), U64(0x8040200008000000), U64(0x8040001000000000), U64(0x8000201000000000), U64(0x0040201000000000) }, /* 0x99 */
	{ U64(0x0000000000000200), U64(0x0000000008040001), U64(0x0000000008000200), U64(0x0000001000040200), U64(0x8040200008000000), U64(0x8040001000000000), U64(0x8000201000000000), U64(0x0040201000000000) }, /* 0x9a */
	{ U64(0x0000000000000200), U64(0x0000000008040001), U64(0x0000000008000200), U64(0x0000001000040200), U64(0x8040200008000000), U64(0x8040001000000000), U64(0x8000201000000000), U64(0x0040201000000000) }, /* 0x9b */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0000000008000201), U64(0x0000001000040000), U64(0x8040200008000000), U64(0x8040001000000000), U64(0x8000201000000000), U64(0x0040201000000000) }, /* 0x9c */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0000000008000201), U64(0x0000001000040000), U64(0x8040200008000000), U64(0x8040001000000000), U64(0x8000201000000000), U64(0x0040201000000000) }, /* 0x9d */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0000000008000200), U64(0x0000001000040000), U64(0x8040200008000000), U64(0x8040001000000000), U64(0x8000201000000000), U64(0x0040201000000000) }, /* 0x9e */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0000000008000200), U64(0x0000001000040000), U64(0x8040200008000000), U64(0x8040001000000000), U64(0x8000201000000000), U64(0x0040201000000000) }, /* 0x9f */
	{ U64(0x0000201008040200), U64(0x0000201008040001), U64(0x0000201008000201), U64(0x0000201000040201), U64(0x0000200008040201), U64(0x8040001008040201), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0xa0 */
	{ U64(0x0000201008040200), U64(0x0000201008040001), U64(0x0000201008000201), U64(0x0000201000040201), U64(0x0000200008040201), U64(0x8040001008040201), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0xa1 */
	{ U64(0x0000000000000200), U64(0x0000201008040001), U64(0x0000201008000200), U64(0x0000201000040200), U64(0x0000200008040200), U64(0x8040001008040200), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0xa2 */
	{ U64(0x0000000000000200), U64(0x0000201008040001), U64(0x0000201008000200), U64(0x0000201000040200), U64(0x0000200008040200), U64(0x8040001008040200), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0xa3 */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0000201008000201), U64(0x0000201000040000), U64(0x0000200008040000), U64(0x8040001008040000), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0xa4 */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0000201008000201), U64(0x0000201000040000), U64(0x0000200008040000), U64(0x8040001008040000), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0xa5 */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0000201008000200), U64(0x0000201000040000), U64(0x0000200008040000), U64(0x8040001008040000), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0xa6 */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0000201008000200), U64(0x0000201000040000), U64(0x0000200008040000), U64(0x8040001008040000), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0xa7 */
	{ U64(0x0000000008040200), U64(0x0000000008040001), U64(0x0000000008000201), U64(0x0000201000040201), U64(0x0000200008000000), U64(0x8040001008000000), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0xa8 */
	{ U64(0x0000000008040200), U64(0x0000000008040001), U64(0x0000000008000201), U64(0x0000201000040201), U64(0x0000200008000000), U64(0x8040001008000000), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0xa9 */
	{ U64(0x0000000000000200), U64(0x0000000008040001), U64(0x0000000008000200), U64(0x0000201000040200), U64(0x0000200008000000), U64(0x8040001008000000), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0xaa */
	{ U64(0x0000000000000200), U64(0x0000000008040001), U64(0x0000000008000200), U64(0x0000201000040200), U64(0x0000200008000000), U64(0x8040001008000000), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0xab */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0000000008000201), U64(0x0000201000040000), U64(0x0000200008000000), U64(0x8040001008000000), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0xac */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0000000008000201), U64(0x0000201000040000), U64(0x0000200008000000), U64(0x8040001008000000), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0xad */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0000000008000200), U64(0x0000201000040000), U64(0x0000200008000000), U64(0x8040001008000000), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0xae */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0000000008000200), U64(0x0000201000040000), U64(0x0000200008000000), U64(0x8040001008000000), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0xaf */
	{ U64(0x0000001008040200), U64(0x0000001008040001), U64(0x0000001008000201), U64(0x0000001000040201), U64(0x0000200008040201), U64(0x8040001000000000), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0xb0 */
	{ U64(0x0000001008040200), U64(0x0000001008040001), U64(0x0000001008000201), U64(0x0000001000040201), U64(0x0000200008040201), U64(0x8040001000000000), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0xb1 */
	{ U64(0x0000000000000200), U64(0x0000001008040001), U64(0x0000001008000200), U64(0x0000001000040200), U64(0x0000200008040200), U64(0x8040001000000000), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0xb2 */
	{ U64(0x0000000000000200), U64(0x0000001008040001), U64(0x0000001008000200), U64(0x0000001000040200), U64(0x0000200008040200), U64(0x8040001000000000), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0xb3 */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0000001008000201), U64(0x0000001000040000), U64(0x0000200008040000), U64(0x8040001000000000), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0xb4 */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0000001008000201), U64(0x0000001000040000), U64(0x0000200008040000), U64(0x8040001000000000), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0xb5 */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0000001008000200), U64(0x0000001000040000), U64(0x0000200008040000), U64(0x8040001000000000), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0xb6 */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0000001008000200), U64(0x0000001000040000), U64(0x0000200008040000), U64(0x8040001000000000), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0xb7 */
	{ U64(0x0000000008040200), U64(0x0000000008040001), U64(0x0000000008000201), U64(0x0000001000040201), U64(0x0000200008000000), U64(0x8040001000000000), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0xb8 */
	{ U64(0x0000000008040200), U64(0x0000000008040001), U64(0x0000000008000201), U64(0x0000001000040201), U64(0x0000200008000000), U64(0x8040001000000000), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0xb9 */
	{ U64(0x0000000000000200), U64(0x0000000008040001), U64(0x0000000008000200), U64(0x0000001000040200), U64(0x0000200008000000), U64(0x8040001000000000), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0xba */
	{ U64(0x0000000000000200), U64(0x0000000008040001), U64(0x0000000008000200), U64(0x0000001000040200), U64(0x0000200008000000), U64(0x8040001000000000), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0xbb */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0000000008000201), U64(0x0000001000040000), U64(0x0000200008000000), U64(0x8040001000000000), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0xbc */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0000000008000201), U64(0x0000001000040000), U64(0x0000200008000000), U64(0x8040001000000000), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0xbd */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0000000008000200), U64(0x0000001000040000), U64(0x0000200008000000), U64(0x8040001000000000), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0xbe */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0000000008000200), U64(0x0000001000040000), U64(0x0000200008000000), U64(0x8040001000000000), U64(0x8000200000000000), U64(0x0040200000000000) }, /* 0xbf */
	{ U64(0x0040201008040200), U64(0x0040201008040001), U64(0x0040201008000201), U64(0x0040201000040201), U64(0x0040200008040201), U64(0x0040001008040201), U64(0x8000201008040201), U64(0x0040000000000000) }, /* 0xc0 */
	{ U64(0x0040201008040200), U64(0x0040201008040001), U64(0x0040201008000201), U64(0x0040201000040201), U64(0x0040200008040201), U64(0x0040001008040201), U64(0x8000201008040201), U64(0x0040000000000000) }, /* 0xc1 */
	{ U64(0x0000000000000200), U64(0x0040201008040001), U64(0x0040201008000200), U64(0x0040201000040200), U64(0x0040200008040200), U64(0x0040001008040200), U64(0x8000201008040200), U64(0x0040000000000000) }, /* 0xc2 */
	{ U64(0x0000000000000200), U64(0x0040201008040001), U64(0x0040201008000200), U64(0x0040201000040200), U64(0x0040200008040200), U64(0x0040001008040200), U64(0x8000201008040200), U64(0x0040000000000000) }, /* 0xc3 */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0040201008000201), U64(0x0040201000040000), U64(0x0040200008040000), U64(0x0040001008040000), U64(0x8000201008040000), U64(0x0040000000000000) }, /* 0xc4 */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0040201008000201), U64(0x0040201000040000), U64(0x0040200008040000), U64(0x0040001008040000), U64(0x8000201008040000), U64(0x0040000000000000) }, /* 0xc5 */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0040201008000200), U64(0x0040201000040000), U64(0x0040200008040000), U64(0x0040001008040000), U64(0x8000201008040000), U64(0x0040000000000000) }, /* 0xc6 */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0040201008000200), U64(0x0040201000040000), U64(0x0040200008040000), U64(0x0040001008040000), U64(0x8000201008040000), U64(0x0040000000000000) }, /* 0xc7 */
	{ U64(0x0000000008040200), U64(0x0000000008040001), U64(0x0000000008000201), U64(0x0040201000040201), U64(0x0040200008000000), U64(0x0040001008000000), U64(0x8000201008000000), U64(0x0040000000000000) }, /* 0xc8 */
	{ U64(0x0000000008040200), U64(0x0000000008040001), U64(0x0000000008000201), U64(0x0040201000040201), U64(0x0040200008000000), U64(0x0040001008000000), U64(0x8000201008000000), U64(0x0040000000000000) }, /* 0xc9 */
	{ U64(0x0000000000000200), U64(0x0000000008040001), U64(0x0000000008000200), U64(0x0040201000040200), U64(0x0040200008000000), U64(0x0040001008000000), U64(0x8000201008000000), U64(0x0040000000000000) }, /* 0xca */
	{ U64(0x0000000000000200), U64(0x0000000008040001), U64(0x0000000008000200), U64(0x0040201000040200), U64(0x0040200008000000), U64(0x0040001008000000), U64(0x8000201008000000), U64(0x0040000000000000) }, /* 0xcb */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0000000008000201), U64(0x0040201000040000), U64(0x0040200008000000), U64(0x0040001008000000), U64(0x8000201008000000), U64(0x0040000000000000) }, /* 0xcc */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0000000008000201), U64(0x0040201000040000), U64(0x0040200008000000), U64(0x0040001008000000), U64(0x8000201008000000), U64(0x0040000000000000) }, /* 0xcd */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0000000008000200), U64(0x0040201000040000), U64(0x0040200008000000), U64(0x0040001008000000), U64(0x8000201008000000), U64(0x0040000000000000) }, /* 0xce */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0000000008000200), U64(0x0040201000040000), U64(0x0040200008000000), U64(0x0040001008000000), U64(0x8000201008000000), U64(0x0040000000000000) }, /* 0xcf */
	{ U64(0x0000001008040200), U64(0x0000001008040001), U64(0x0000001008000201), U64(0x0000001000040201), U64(0x0040200008040201), U64(0x0040001000000000), U64(0x8000201000000000), U64(0x0040000000000000) }, /* 0xd0 */
	{ U64(0x0000001008040200), U64(0x0000001008040001), U64(0x0000001008000201), U64(0x0000001000040201), U64(0x0040200008040201), U64(0x0040001000000000), U64(0x8000201000000000), U64(0x0040000000000000) }, /* 0xd1 */
	{ U64(0x0000000000000200), U64(0x0000001008040001), U64(0x0000001008000200), U64(0x0000001000040200), U64(0x0040200008040200), U64(0x0040001000000000), U64(0x8000201000000000), U64(0x0040000000000000) }, /* 0xd2 */
	{ U64(0x0000000000000200), U64(0x0000001008040001), U64(0x0000001008000200), U64(0x0000001000040200), U64(0x0040200008040200), U64(0x0040001000000000), U64(0x8000201000000000), U64(0x0040000000000000) }, /* 0xd3 */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0000001008000201), U64(0x0000001000040000), U64(0x0040200008040000), U64(0x0040001000000000), U64(0x8000201000000000), U64(0x0040000000000000) }, /* 0xd4 */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0000001008000201), U64(0x0000001000040000), U64(0x0040200008040000), U64(0x0040001000000000), U64(0x8000201000000000), U64(0x0040000000000000) }, /* 0xd5 */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0000001008000200), U64(0x0000001000040000), U64(0x0040200008040000), U64(0x0040001000000000), U64(0x8000201000000000), U64(0x0040000000000000) }, /* 0xd6 */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0000001008000200), U64(0x0000001000040000), U64(0x0040200008040000), U64(0x0040001000000000), U64(0x8000201000000000), U64(0x0040000000000000) }, /* 0xd7 */
	{ U64(0x0000000008040200), U64(0x0000000008040001), U64(0x0000000008000201), U64(0x0000001000040201), U64(0x0040200008000000), U64(0x0040001000000000), U64(0x8000201000000000), U64(0x0040000000000000) }, /* 0xd8 */
	{ U64(0x0000000008040200), U64(0x0000000008040001), U64(0x0000000008000201), U64(0x0000001000040201), U64(0x0040200008000000), U64(0x0040001000000000), U64(0x8000201000000000), U64(0x0040000000000000) }, /* 0xd9 */
	{ U64(0x0000000000000200), U64(0x0000000008040001), U64(0x0000000008000200), U64(0x0000001000040200), U64(0x0040200008000000), U64(0x0040001000000000), U64(0x8000201000000000), U64(0x0040000000000000) }, /* 0xda */
	{ U64(0x0000000000000200), U64(0x0000000008040001), U64(0x0000000008000200), U64(0x0000001000040200), U64(0x0040200008000000), U64(0x0040001000000000), U64(0x8000201000000000), U64(0x0040000000000000) }, /* 0xdb */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0000000008000201), U64(0x0000001000040000), U64(0x0040200008000000), U64(0x0040001000000000), U64(0x8000201000000000), U64(0x0040000000000000) }, /* 0xdc */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0000000008000201), U64(0x0000001000040000), U64(0x0040200008000000), U64(0x0040001000000000), U64(0x8000201000000000), U64(0x0040000000000000) }, /* 0xdd */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0000000008000200), U64(0x0000001000040000), U64(0x0040200008000000), U64(0x0040001000000000), U64(0x8000201000000000), U64(0x0040000000000000) }, /* 0xde */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0000000008000200), U64(0x0000001000040000), U64(0x0040200008000000), U64(0x0040001000000000), U64(0x8000201000000000), U64(0x0040000000000000) }, /* 0xdf */
	{ U64(0x0000201008040200), U64(0x0000201008040001), U64(0x0000201008000201), U64(0x0000201000040201), U64(0x0000200008040201), U64(0x0040001008040201), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0xe0 */
	{ U64(0x0000201008040200), U64(0x0000201008040001), U64(0x0000201008000201), U64(0x0000201000040201), U64(0x0000200008040201), U64(0x0040001008040201), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0xe1 */
	{ U64(0x0000000000000200), U64(0x0000201008040001), U64(0x0000201008000200), U64(0x0000201000040200), U64(0x0000200008040200), U64(0x0040001008040200), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0xe2 */
	{ U64(0x0000000000000200), U64(0x0000201008040001), U64(0x0000201008000200), U64(0x0000201000040200), U64(0x0000200008040200), U64(0x0040001008040200), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0xe3 */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0000201008000201), U64(0x0000201000040000), U64(0x0000200008040000), U64(0x0040001008040000), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0xe4 */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0000201008000201), U64(0x0000201000040000), U64(0x0000200008040000), U64(0x0040001008040000), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0xe5 */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0000201008000200), U64(0x0000201000040000), U64(0x0000200008040000), U64(0x0040001008040000), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0xe6 */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0000201008000200), U64(0x0000201000040000), U64(0x0000200008040000), U64(0x0040001008040000), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0xe7 */
	{ U64(0x0000000008040200), U64(0x0000000008040001), U64(0x0000000008000201), U64(0x0000201000040201), U64(0x0000200008000000), U64(0x0040001008000000), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0xe8 */
	{ U64(0x0000000008040200), U64(0x0000000008040001), U64(0x0000000008000201), U64(0x0000201000040201), U64(0x0000200008000000), U64(0x0040001008000000), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0xe9 */
	{ U64(0x0000000000000200), U64(0x0000000008040001), U64(0x0000000008000200), U64(0x0000201000040200), U64(0x0000200008000000), U64(0x0040001008000000), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0xea */
	{ U64(0x0000000000000200), U64(0x0000000008040001), U64(0x0000000008000200), U64(0x0000201000040200), U64(0x0000200008000000), U64(0x0040001008000000), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0xeb */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0000000008000201), U64(0x0000201000040000), U64(0x0000200008000000), U64(0x0040001008000000), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0xec */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0000000008000201), U64(0x0000201000040000), U64(0x0000200008000000), U64(0x0040001008000000), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0xed */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0000000008000200), U64(0x0000201000040000), U64(0x0000200008000000), U64(0x0040001008000000), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0xee */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0000000008000200), U64(0x0000201000040000), U64(0x0000200008000000), U64(0x0040001008000000), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0xef */
	{ U64(0x0000001008040200), U64(0x0000001008040001), U64(0x0000001008000201), U64(0x0000001000040201), U64(0x0000200008040201), U64(0x0040001000000000), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0xf0 */
	{ U64(0x0000001008040200), U64(0x0000001008040001), U64(0x0000001008000201), U64(0x0000001000040201), U64(0x0000200008040201), U64(0x0040001000000000), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0xf1 */
	{ U64(0x0000000000000200), U64(0x0000001008040001), U64(0x0000001008000200), U64(0x0000001000040200), U64(0x0000200008040200), U64(0x0040001000000000), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0xf2 */
	{ U64(0x0000000000000200), U64(0x0000001008040001), U64(0x0000001008000200), U64(0x0000001000040200), U64(0x0000200008040200), U64(0x0040001000000000), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0xf3 */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0000001008000201), U64(0x0000001000040000), U64(0x0000200008040000), U64(0x0040001000000000), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0xf4 */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0000001008000201), U64(0x0000001000040000), U64(0x0000200008040000), U64(0x0040001000000000), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0xf5 */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0000001008000200), U64(0x0000001000040000), U64(0x0000200008040000), U64(0x0040001000000000), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0xf6 */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0000001008000200), U64(0x0000001000040000), U64(0x0000200008040000), U64(0x0040001000000000), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0xf7 */
	{ U64(0x0000000008040200), U64(0x0000000008040001), U64(0x0000000008000201), U64(0x0000001000040201), U64(0x0000200008000000), U64(0x0040001000000000), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0xf8 */
	{ U64(0x0000000008040200), U64(0x0000000008040001), U64(0x0000000008000201), U64(0x0000001000040201), U64(0x0000200008000000), U64(0x0040001000000000), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0xf9 */
	{ U64(0x0000000000000200), U64(0x0000000008040001), U64(0x0000000008000200), U64(0x0000001000040200), U64(0x0000200008000000), U64(0x0040001000000000), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0xfa */
	{ U64(0x0000000000000200), U64(0x0000000008040001), U64(0x0000000008000200), U64(0x0000001000040200), U64(0x0000200008000000), U64(0x0040001000000000), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0xfb */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0000000008000201), U64(0x0000001000040000), U64(0x0000200008000000), U64(0x0040001000000000), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0xfc */
	{ U64(0x0000000000040200), U64(0x0000000000040001), U64(0x0000000008000201), U64(0x0000001000040000), U64(0x0000200008000000), U64(0x0040001000000000), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0xfd */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0000000008000200), U64(0x0000001000040000), U64(0x0000200008000000), U64(0x0040001000000000), U64(0x8000200000000000), U64(0x0040000000000000) }, /* 0xfe */
	{ U64(0x0000000000000200), U64(0x0000000000040001), U64(0x0000000008000200), U64(0x0000001000040000), U64(0x0000200008000000), U64(0x0040001000000000), U64(0x8000200000000000), U64(0x0040000000000000) } /* 0xff */
};
/****************************************************************************
 * board.c - rotated bitboard library; move generation and board state
 * copyright (C) 2008 Ben Blum
 ****************************************************************************/

/* For FEN conversion, and move->string conversion */
char *piecename[2][6] = {
	{ "P", "N", "B", "R", "Q", "K" },
	{ "p", "n", "b", "r", "q", "k" }
};
/* Converts int to string. Better than using malloc for 3-length strings. */
char *squarename[64] = {
	"a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",
	"a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
	"a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
	"a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
	"a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
	"a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
	"a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
	"a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8"
};
#ifndef BB_SHIFTFLIP /* see board.h */
/* Get a bitboard with only one bit set, on a specified square */
bitboard_t bb_square[64] = {
	BB(0x0000000000000001), BB(0x0000000000000002), BB(0x0000000000000004), BB(0x0000000000000008),
	BB(0x0000000000000010), BB(0x0000000000000020), BB(0x0000000000000040), BB(0x0000000000000080),
	BB(0x0000000000000100), BB(0x0000000000000200), BB(0x0000000000000400), BB(0x0000000000000800),
	BB(0x0000000000001000), BB(0x0000000000002000), BB(0x0000000000004000), BB(0x0000000000008000),
	BB(0x0000000000010000), BB(0x0000000000020000), BB(0x0000000000040000), BB(0x0000000000080000),
	BB(0x0000000000100000), BB(0x0000000000200000), BB(0x0000000000400000), BB(0x0000000000800000),
	BB(0x0000000001000000), BB(0x0000000002000000), BB(0x0000000004000000), BB(0x0000000008000000),
	BB(0x0000000010000000), BB(0x0000000020000000), BB(0x0000000040000000), BB(0x0000000080000000),
	BB(0x0000000100000000), BB(0x0000000200000000), BB(0x0000000400000000), BB(0x0000000800000000),
	BB(0x0000001000000000), BB(0x0000002000000000), BB(0x0000004000000000), BB(0x0000008000000000),
	BB(0x0000010000000000), BB(0x0000020000000000), BB(0x0000040000000000), BB(0x0000080000000000),
	BB(0x0000100000000000), BB(0x0000200000000000), BB(0x0000400000000000), BB(0x0000800000000000),
	BB(0x0001000000000000), BB(0x0002000000000000), BB(0x0004000000000000), BB(0x0008000000000000),
	BB(0x0010000000000000), BB(0x0020000000000000), BB(0x0040000000000000), BB(0x0080000000000000),
	BB(0x0100000000000000), BB(0x0200000000000000), BB(0x0400000000000000), BB(0x0800000000000000),
	BB(0x1000000000000000), BB(0x2000000000000000), BB(0x4000000000000000), BB(0x8000000000000000),
};
/* The bitwise negation of SINGLESQUARE(x) */
bitboard_t bb_allexcept[64] = {
	BB(0xfffffffffffffffe), BB(0xfffffffffffffffd), BB(0xfffffffffffffffb), BB(0xfffffffffffffff7),
	BB(0xffffffffffffffef), BB(0xffffffffffffffdf), BB(0xffffffffffffffbf), BB(0xffffffffffffff7f),
	BB(0xfffffffffffffeff), BB(0xfffffffffffffdff), BB(0xfffffffffffffbff), BB(0xfffffffffffff7ff),
	BB(0xffffffffffffefff), BB(0xffffffffffffdfff), BB(0xffffffffffffbfff), BB(0xffffffffffff7fff),
	BB(0xfffffffffffeffff), BB(0xfffffffffffdffff), BB(0xfffffffffffbffff), BB(0xfffffffffff7ffff),
	BB(0xffffffffffefffff), BB(0xffffffffffdfffff), BB(0xffffffffffbfffff), BB(0xffffffffff7fffff),
	BB(0xfffffffffeffffff), BB(0xfffffffffdffffff), BB(0xfffffffffbffffff), BB(0xfffffffff7ffffff),
	BB(0xffffffffefffffff), BB(0xffffffffdfffffff), BB(0xffffffffbfffffff), BB(0xffffffff7fffffff),
	BB(0xfffffffeffffffff), BB(0xfffffffdffffffff), BB(0xfffffffbffffffff), BB(0xfffffff7ffffffff),
	BB(0xffffffefffffffff), BB(0xffffffdfffffffff), BB(0xffffffbfffffffff), BB(0xffffff7fffffffff),
	BB(0xfffffeffffffffff), BB(0xfffffdffffffffff), BB(0xfffffbffffffffff), BB(0xfffff7ffffffffff),
	BB(0xffffefffffffffff), BB(0xffffdfffffffffff), BB(0xffffbfffffffffff), BB(0xffff7fffffffffff),
	BB(0xfffeffffffffffff), BB(0xfffdffffffffffff), BB(0xfffbffffffffffff), BB(0xfff7ffffffffffff),
	BB(0xffefffffffffffff), BB(0xffdfffffffffffff), BB(0xffbfffffffffffff), BB(0xff7fffffffffffff),
	BB(0xfeffffffffffffff), BB(0xfdffffffffffffff), BB(0xfbffffffffffffff), BB(0xf7ffffffffffffff),
	BB(0xefffffffffffffff), BB(0xdfffffffffffffff), BB(0xbfffffffffffffff), BB(0x7fffffffffffffff),
};
#endif

/* Which squares must be unoccupied for COLOR (1st) to castle on SIDE (2nd) */
static bitboard_t castle_clearsquares[2][2] = {
	/*   queenside               kingside */
	{ BB(0x000000000000000e), BB(0x0000000000000060) }, /* white */
	{ BB(0x0e00000000000000), BB(0x6000000000000000) }  /* black */
};
/* Which squares must be unthreatened for COLOR to castle on SIDE */
static bitboard_t castle_safesquares[2][2] = {
	/*   queenside               kingside */
	{ BB(0x000000000000001c), BB(0x0000000000000070) }, /* white */
	{ BB(0x1c00000000000000), BB(0x7000000000000000) }  /* black */
};
/* A mask for the cols adjacent to a given col - for pawn captures, AND with a
 * row-mask for the rank you need the pawns to be on */
bitboard_t bb_adjacentcols[8] = {
	           BB_FILEB,
	BB_FILEA | BB_FILEC,
	BB_FILEB | BB_FILED,
	BB_FILEC | BB_FILEE,
	BB_FILED | BB_FILEF,
	BB_FILEE | BB_FILEG,
	BB_FILEF | BB_FILEH,
	BB_FILEG
};
/* used for determining when a pawn is passed */
bitboard_t bb_passedpawnmask[2][64] = {
	/* WHITE */
	{
		BB(0x0000000000000000), BB(0x0000000000000000), BB(0x0000000000000000), BB(0x0000000000000000), BB(0x0000000000000000), BB(0x0000000000000000), BB(0x0000000000000000), BB(0x0000000000000000),
		BB(0x0303030303030000), BB(0x0707070707070000), BB(0x0e0e0e0e0e0e0000), BB(0x1c1c1c1c1c1c0000), BB(0x3838383838380000), BB(0x7070707070700000), BB(0xe0e0e0e0e0e00000), BB(0xc0c0c0c0c0c00000),
		BB(0x0303030303000000), BB(0x0707070707000000), BB(0x0e0e0e0e0e000000), BB(0x1c1c1c1c1c000000), BB(0x3838383838000000), BB(0x7070707070000000), BB(0xe0e0e0e0e0000000), BB(0xc0c0c0c0c0000000),
		BB(0x0303030300000000), BB(0x0707070700000000), BB(0x0e0e0e0e00000000), BB(0x1c1c1c1c00000000), BB(0x3838383800000000), BB(0x7070707000000000), BB(0xe0e0e0e000000000), BB(0xc0c0c0c000000000),
		BB(0x0303030000000000), BB(0x0707070000000000), BB(0x0e0e0e0000000000), BB(0x1c1c1c0000000000), BB(0x3838380000000000), BB(0x7070700000000000), BB(0xe0e0e00000000000), BB(0xc0c0c00000000000),
		BB(0x0303000000000000), BB(0x0707000000000000), BB(0x0e0e000000000000), BB(0x1c1c000000000000), BB(0x3838000000000000), BB(0x7070000000000000), BB(0xe0e0000000000000), BB(0xc0c0000000000000),
		BB(0x0000000000000000), BB(0x0000000000000000), BB(0x0000000000000000), BB(0x0000000000000000), BB(0x0000000000000000), BB(0x0000000000000000), BB(0x0000000000000000), BB(0x0000000000000000),
		BB(0x0000000000000000), BB(0x0000000000000000), BB(0x0000000000000000), BB(0x0000000000000000), BB(0x0000000000000000), BB(0x0000000000000000), BB(0x0000000000000000), BB(0x0000000000000000)
	},
	/* BLACK */
	{
		BB(0x0000000000000000), BB(0x0000000000000000), BB(0x0000000000000000), BB(0x0000000000000000), BB(0x0000000000000000), BB(0x0000000000000000), BB(0x0000000000000000), BB(0x0000000000000000),
		BB(0x0000000000000000), BB(0x0000000000000000), BB(0x0000000000000000), BB(0x0000000000000000), BB(0x0000000000000000), BB(0x0000000000000000), BB(0x0000000000000000), BB(0x0000000000000000),
		BB(0x0000000000000303), BB(0x0000000000000707), BB(0x0000000000000e0e), BB(0x0000000000001c1c), BB(0x0000000000003838), BB(0x0000000000007070), BB(0x000000000000e0e0), BB(0x000000000000c0c0),
		BB(0x0000000000030303), BB(0x0000000000070707), BB(0x00000000000e0e0e), BB(0x00000000001c1c1c), BB(0x0000000000383838), BB(0x0000000000707070), BB(0x0000000000e0e0e0), BB(0x0000000000c0c0c0),
		BB(0x0000000003030303), BB(0x0000000007070707), BB(0x000000000e0e0e0e), BB(0x000000001c1c1c1c), BB(0x0000000038383838), BB(0x0000000070707070), BB(0x00000000e0e0e0e0), BB(0x00000000c0c0c0c0),
		BB(0x0000000303030303), BB(0x0000000707070707), BB(0x0000000e0e0e0e0e), BB(0x0000001c1c1c1c1c), BB(0x0000003838383838), BB(0x0000007070707070), BB(0x000000e0e0e0e0e0), BB(0x000000c0c0c0c0c0),
		BB(0x0000030303030303), BB(0x0000070707070707), BB(0x00000e0e0e0e0e0e), BB(0x00001c1c1c1c1c1c), BB(0x0000383838383838), BB(0x0000707070707070), BB(0x0000e0e0e0e0e0e0), BB(0x0000c0c0c0c0c0c0),
		BB(0x0000000000000000), BB(0x0000000000000000), BB(0x0000000000000000), BB(0x0000000000000000), BB(0x0000000000000000), BB(0x0000000000000000), BB(0x0000000000000000), BB(0x0000000000000000)
	}
};

/* Rotation translation from normal-oriented square indices to square indices
 * suitable for setting bits on board_t->occupied{90,45,315}. Note that we
 * don't use these for move generation at all, just for setting/clearing bits
 * on the occupied boards. */
#define ROT90SQUAREINDEX(s) rot90squareindex[s] /*(SQUARE(ROW(s),COL(s))) - slower*/
static int rot90squareindex[64] = {
	0,  8, 16, 24, 32, 40, 48, 56,
	1,  9, 17, 25, 33, 41, 49, 57,
	2, 10, 18, 26, 34, 42, 50, 58,
	3, 11, 19, 27, 35, 43, 51, 59,
	4, 12, 20, 28, 36, 44, 52, 60,
	5, 13, 21, 29, 37, 45, 53, 61,
	6, 14, 22, 30, 38, 46, 54, 62,
	7, 15, 23, 31, 39, 47, 55, 63
};
#define ROT45SQUAREINDEX(s) rot45squareindex[s]
static int rot45squareindex[64] = {
	 0,  2,  5,  9, 14, 20, 27, 35,
	 1,  4,  8, 13, 19, 26, 34, 42,
	 3,  7, 12, 18, 25, 33, 41, 48,
	 6, 11, 17, 24, 32, 40, 47, 53,
	10, 16, 23, 31, 39, 46, 52, 57,
	15, 22, 30, 38, 45, 51, 56, 60,
	21, 29, 37, 44, 50, 55, 59, 62,
	28, 36, 43, 49, 54, 58, 61, 63
};
#define ROT315SQUAREINDEX(s) rot315squareindex[s]
static int rot315squareindex[64] = {
	28, 21, 15, 10,  6,  3,  1,  0,
	36, 29, 22, 16, 11,  7,  4,  2,
	43, 37, 30, 23, 17, 12,  8,  5,
	49, 44, 38, 31, 24, 18, 13,  9,
	54, 50, 45, 39, 32, 25, 19, 14,
	58, 55, 51, 46, 40, 33, 26, 20,
	61, 59, 56, 52, 47, 41, 34, 27,
	63, 62, 60, 57, 53, 48, 42, 35,
};

/* for material count */
extern int16_t eval_piecevalue[6];



#define ZOBRIST_DEFAULT_HASH 0
/* Table of random numbers used to generate the zobrist hash. The indices are
 * first the color (WHITE/BLACK), then the piece type (PAWN-KING), then the
 * square the piece is on (A1-H8) */
zobrist_t zobrist_piece[2][6][64];
/* Whose move is it (WHITE/BLACK)? The value present if white has the move. */
zobrist_t zobrist_tomove;
/* Which square (A1-H8) is the enpassant square? If enpassant is not possible,
 * use the key at index 0. */
zobrist_t zobrist_ep[64];
/* Can (WHITE/BLACK, first) castle on the (KINGSIDE/QUEENSIDE, second)? If so,
 * the value here is xored into the hash. */
zobrist_t zobrist_castle[2][2];
/* Have the arrays been initialized? If you reinitialize them in the middle of
 * play, the keys will get screwed up. */
char zobrist_initialized = 0;

/**
 * Initialize the zobrist tables.
 */
void init_zobrist()
{
	int i, j, k;
	
	/* just in case... */
	if (zobrist_initialized)
	{
		return;
	}
	
	/* Init the piece position array */
	for (i = 0; i < 2; i++)
	{
		for (j = 0; j < 6; j++)
		{
			for (k = 0; k < 64; k++)
			{
				zobrist_piece[i][j][k] = (zobrist_t)rand64();
			}
		}
	}
	
	/* Init the whose-move-is-it value */
	zobrist_tomove = (zobrist_t)rand64();
	
	/* Init the enpassant square array */
	for (i = 0; i < 64; i++)
	{
		zobrist_ep[i] = (zobrist_t)rand64();
	}
	
	/* Init the castling rights array */
	zobrist_castle[0][0] = (zobrist_t)rand64();
	zobrist_castle[0][1] = (zobrist_t)rand64();
	zobrist_castle[1][0] = (zobrist_t)rand64();
	zobrist_castle[1][1] = (zobrist_t)rand64();
	
	zobrist_initialized = 1;
	return;
}

/**
 * (Re)generate the zobrist hash for a board, store it in board->hash
 * This shouldn't be used every time you change the zobrist, only when
 * initializing the board.
 */
void zobrist_gen(board_t *board)
{
	unsigned char color;
	piece_t piece;
	square_t square;

	int i, j;
	bitboard_t pos;
	zobrist_t hash = ZOBRIST_DEFAULT_HASH;
	
	if (board == NULL)
	{
		return;
	}
	if (!zobrist_initialized)
	{
		init_zobrist();
	}
	
	/* XOR in the zobrist for each piece on the board */
	for (color = 0; color < 2; color++)
	{
		for (piece = 0; piece < 6; piece++)
		{
			pos = board->pos[color][piece];
			while (pos)
			{
				/* Find the first bit */
				square = BITSCAN(pos);
				/* throw in the hash */
				hash ^= zobrist_piece[color][piece][square];
				/* and clear the bit */
				pos &= BB_ALLEXCEPT(square);
			}
		}
	}
	/* Have the hash reflect special conditions on the board */
	/* En passant */
	hash ^= zobrist_ep[board->ep];
	/* Castling rights */
	for (i = 0; i < 2; i++)
	{
		for (j = 0; j < 2; j++)
		{
			if (board->castle[i][j])
			{
				hash ^= zobrist_castle[i][j];
			}
		}
	}
	/* Who has the move */
	if (board->tomove == WHITE)
	{
		hash ^= zobrist_tomove;
	}

	board->hash = hash;
	return;
}

/****************************************************************************
 * BITBOARD FUNCTIONS
 ****************************************************************************/
static void board_addmoves_pawn(board_t *, square_t, unsigned char, movelist_t *);
static void board_addmoves_ep(board_t *, unsigned char, movelist_t *);
static void board_addmoves_king(board_t *, square_t, unsigned char, movelist_t *);
static void board_addmoves(board_t *, square_t, piece_t, unsigned char, movelist_t *);

static void board_addcaptures_pawn(board_t *, square_t, unsigned char, movelist_t *);
static void board_addcaptures_king(board_t *, square_t, unsigned char, movelist_t *);
static void board_addcaptures(board_t *, square_t, piece_t, unsigned char, movelist_t *);
static void board_togglepiece(board_t *, square_t, unsigned char, piece_t);
static void board_regeneratethreatened(board_t *);

/**
 * Generate a fresh board with the default initial starting position.
 */
board_t *board_init()
{
	board_t *board = malloc(sizeof(board_t));
	
	board->pos[WHITE][PAWN]   = BB_RANK2;
	board->pos[WHITE][KNIGHT] = BB_SQUARE(B1) | BB_SQUARE(G1);
	board->pos[WHITE][BISHOP] = BB_SQUARE(C1) | BB_SQUARE(F1);
	board->pos[WHITE][ROOK]   = BB_SQUARE(A1) | BB_SQUARE(H1);
	board->pos[WHITE][QUEEN]  = BB_SQUARE(D1);
	board->pos[WHITE][KING]   = BB_SQUARE(E1);
	board->pos[BLACK][PAWN]   = BB_RANK7;
	board->pos[BLACK][KNIGHT] = BB_SQUARE(B8) | BB_SQUARE(G8);
	board->pos[BLACK][BISHOP] = BB_SQUARE(C8) | BB_SQUARE(F8);
	board->pos[BLACK][ROOK]   = BB_SQUARE(A8) | BB_SQUARE(H8);
	board->pos[BLACK][QUEEN]  = BB_SQUARE(D8);
	board->pos[BLACK][KING]   = BB_SQUARE(E8);

	board->piecesofcolor[WHITE] = BB_RANK1 | BB_RANK2;
	board->piecesofcolor[BLACK] = BB_RANK8 | BB_RANK7;
	
	board->attackedby[WHITE] = BB_RANK3 | BB_RANK2 | (BB_RANK1 ^ BB_SQUARE(A1) ^ BB_SQUARE(H1));
	board->attackedby[BLACK] = BB_RANK6 | BB_RANK7 | (BB_RANK8 ^ BB_SQUARE(A8) ^ BB_SQUARE(H8));
	
	board->occupied    = BB_RANK1 | BB_RANK2 | BB_RANK7 | BB_RANK8;
	board->occupied90  = BB_FILEA | BB_FILEB | BB_FILEG | BB_FILEH;
	/* These ugly numbers, in contrast to the beautifully simple code
	 * above, are in fact correct. Check with ./bbtools/rotvisualizer */
	board->occupied45  = BB(0xecc61c3c3c386337);
	board->occupied315 = BB(0xfb31861c38618cdf);
	
	board->ep = 0;
	board->castle[WHITE][QUEENSIDE] = 1;
	board->castle[WHITE][KINGSIDE] = 1;
	board->castle[BLACK][QUEENSIDE] = 1;
	board->castle[BLACK][KINGSIDE] = 1;
	board->hascastled[WHITE] = 0;
	board->hascastled[BLACK] = 0;
	board->tomove = WHITE;
	board->halfmoves = 0;
	board->moves = 0;
	board->reps = 0;

	board->material[WHITE] = (8 * eval_piecevalue[PAWN]) +
	                         (2 * eval_piecevalue[KNIGHT]) +
	                         (2 * eval_piecevalue[BISHOP]) +
				 (2 * eval_piecevalue[ROOK]) +
				 (eval_piecevalue[QUEEN]) +
				 (eval_piecevalue[KING]);
	board->material[BLACK] = board->material[WHITE];
	
	zobrist_gen(board);
	
	return board;
}

/**
 * Free all memory associated with a board_t.
 */
void board_destroy(board_t *board)
{
	if (board)
	{
		free(board);
	}
}

/**
 * Generates a FEN string for the given position. Free it yourself when you're
 * done with it. This function makes no effort to be fast; you shouldn't be
 * using it during the search process anyway.
 */
#define FEN_MAX_LENGTH 85
char *board_fen(board_t *board)
{
	char *fen;
	int row, col;
	char emptysquares[] = "0"; /* counts consecutive empty squares */
	char halfmove[8]; /* Move clock string - "8 45" maybe */
	piece_t piece;
	unsigned char color;
	unsigned char empty_end_of_line;
	
	if (board == NULL)
	{
		return NULL;
	}
	
	fen = malloc(FEN_MAX_LENGTH + 1);
	fen[0] = 0;
	
	for (row = 7; row >= 0; row--)
	{
		emptysquares[0] = '0';
		empty_end_of_line = 0;
		
		for (col = 0; col < 8; col++)
		{
			color = 0; /* really init'ed next line; this makes gcc happy */
			piece = board_pieceatsquare(board, SQUARE(col,row), &color);
			if (piece == (piece_t)-1)
			{
				emptysquares[0]++;
				if (col == 7)
				{
					empty_end_of_line = 1;
				}
			}
			else
			{
				/* print the emptysquare count if it exists */
				if (emptysquares[0] != '0')
				{
					strcat(fen, emptysquares);
					emptysquares[0] = '0';
				}
				strcat(fen, piecename[color][piece]);
			}
		}
		/* we finished printing this row, unless it was empty */
		if (empty_end_of_line)
		{
			strcat(fen, emptysquares);
		}
		/* print the row separator */
		strcat(fen, ((row > 0) ? "/" : " "));
	}
	/* now all that's left is the flags */
	strcat(fen, ((board->tomove == WHITE) ? "w " : "b "));
	/* castle privileges - this is kinda messy */
	if (!(board->castle[WHITE][QUEENSIDE] || board->castle[WHITE][KINGSIDE] ||
	      board->castle[BLACK][QUEENSIDE] || board->castle[BLACK][KINGSIDE]))
	{
		strcat(fen, "-");
	}
	else
	{
		if (board->castle[WHITE][KINGSIDE])  strcat(fen, "K");
		if (board->castle[WHITE][QUEENSIDE]) strcat(fen, "Q");
		if (board->castle[BLACK][KINGSIDE])  strcat(fen, "k");
		if (board->castle[BLACK][QUEENSIDE]) strcat(fen, "q");
	}
	strcat(fen, " ");
	/* enpassant square */
	strcat(fen, ((board->ep != 0) ? squarename[board->ep] : "-"));
	strcat(fen, " ");
	/* move clocks - 0 for absolute move */
	sprintf(halfmove, "%d %d", board->halfmoves, (board->moves + 1)/2);
	strcat(fen, halfmove);
	
	return fen;
}

/**
 * Gets the piece at the given square index. If the square is empty, -1.
 * You should already know that there's a piece there when you call this.
 * If the board is NULL or the square is invalid, -2.
 * If the third argument is non-null, stores the color of the piece in there
 * (unmodified if piece not present).
 */
piece_t board_pieceatsquare(board_t *board, square_t square, unsigned char *c)
{
	bitboard_t mask;
	piece_t piece;
	unsigned char color;
	
	if (board == NULL || square > 63)
	{
		return -2;
	}

	mask = BB_SQUARE(square);
	
	/* find what color the piece is */
	if (board->piecesofcolor[WHITE] & mask)
	{
		color = WHITE;
	}
	else if (board->piecesofcolor[BLACK] & mask)
	{
		color = BLACK;
	}
	else
	{
		return -1;
	}
	
	if (c != NULL)
	{
		*c = color;
	}
	
	/* find what piece type it is */
	piece = 0;
	while (!(board->pos[color][piece] & mask))
	{
		piece++;
	}
	return piece;
}

/**
 * Is the player whose move it is in check? (1 if so, 0 if not)
 */
int board_incheck(board_t *board)
{
	return board_colorincheck(board, board->tomove);
}

/**
 * Is the given player in check? (1 if so, 0 if not)
 */
int board_colorincheck(board_t *board, unsigned char color)
{
	return !!(board->pos[color][KING] & board->attackedby[OTHERCOLOR(color)]);
}

/**
 * Check if the player to move is checkmated or stalemated. Returns either
 * BOARD_CHECKMATED or BOARD_STALEMATED (from board.h), or 0 if neither.
 * This is a very expensive call. Only the engine, not the searcher, should
 * use this in-between moves to determine when the game ends.
 */
int board_mated(board_t *board)
{
	int result;
	movelist_t moves;
	move_t move;
	unsigned char color;
	
	color = board->tomove;
	result = board_colorincheck(board, color) ?
	               BOARD_CHECKMATED : BOARD_STALEMATED;
	board_generatemoves(board, &moves);


	while (!movelist_isempty(&moves))
	{
		move = movelist_remove_max(&moves);
		/* try the move, see if it puts us in check */
		board_applymove(board, move);
		if (!board_colorincheck(board, color))
		{
			/* found a legal move */
			board_undomove(board, move);
			result = 0;
			break;
		}
		board_undomove(board, move);
	}
	movelist_destroy(&moves);
	return result;
}

/**
 * Is the pawn of given color at given square passed?
 */
int board_pawnpassed(board_t *board, square_t square, unsigned char color)
{
	return !(bb_passedpawnmask[color][square] &
	         board->pos[OTHERCOLOR(color)][PAWN]);
}

/**
 * Is the given square attacked by any piece of the given color? 1 or 0
 * Presumably the square has the OTHERCOLOR(color)'s king on it
 * Note: This is "the" expensive call now that the board library doesn't use
 * attacked bitboards.
 */
int board_squareisattacked(board_t *board, square_t square, unsigned char color)
{
	return !!(BB_SQUARE(square) & board->attackedby[color]);
}

/**
 * Returns true if *any* of the given squares are attacked by color. Useful
 * for determining castling legality
 */
int board_squaresareattacked(board_t *board, bitboard_t squares, unsigned char color)
{
	return !!(squares & board->attackedby[color]);
}


/******************
 * Move generation 
 ******************/

/**
 * Given the current board state, returns a bitmask representing all possible
 * attacks from a given square assuming a piece of the specified type/color at
 * that square. This is not the same as all the moves the piece at that square
 * can make - see the special cases: pawns (capture/ep) and kings (castling).
 * For pawns, forward pushes are NOT reflected in this mask, but diagonally
 * threatened squares ARE, EVEN IF there is a friendly piece there.
 * 
 * Note: Considering how the attack bitboards work, realize that we want to
 * keep both attacks on other color's pieces and attacks on same color's
 * pieces in here. The board_addmoves functions take care of making sure we
 * don't capture our own pieces.
 * 
 * For a cache-friendly optimization, consider taking the slidingpiece's
 * index OUT of the occupancy mask before looking up - i.e. for 11101011 and
 * the rook is the 3rd index from the right, use 11100011 as a lookup key
 * instead, so subsequent accesses with that mask (if you move the rook along
 * the column/row, etc) stay in cache.
 * UPDATE: It seems to not help. I took it out.
 */
bitboard_t board_attacksfrom(board_t *board, square_t square, piece_t piece, unsigned char color)
{
	/* Used for rook/bishop attack generation */
	int shiftamount;
	int diag;
	bitboard_t movemask;
	bitboard_t rowatk, colatk;
	bitboard_t diag45attacks, diag315attacks;
	
	/* NOTE: We mask with ~piecesofcolor in the calling function. */
	switch (piece)
	{
	case PAWN:
		return pawnattacks[color][square];
	case KNIGHT:
		return knightattacks[square];
	case BISHOP:
		/* The first table lookup */
		diag = rot45diagindex[square];
		movemask = rot45attacks[(board->occupied45 >> rot45index_shiftamountright[diag]) & 0xFF]
		                       [COL(square)];
		diag45attacks = (movemask << rotresult_shiftamountleft[diag])
		                >> rotresult_shiftamountright[diag];
		/* The second table lookup */
		diag = rot315diagindex[square];
		movemask = rot315attacks[((board->occupied315 << rot315index_shiftamountleft[diag]) >> rot315index_shiftamountright[diag]) & 0xFF]
					[COL(square)];
		diag315attacks = (movemask << rotresult_shiftamountleft[diag])
		                 >> rotresult_shiftamountright[diag];
		/* Put them together */
		return (diag45attacks | diag315attacks);
	case ROOK:
		/* The first table lookup */
		shiftamount = 8 * ROW(square);
		movemask = rowattacks[(board->occupied  >> shiftamount) & 0xFF][COL(square)];
		rowatk = movemask << shiftamount;
		/* The second table lookup */
		shiftamount = COL(square);
		movemask = colattacks[(board->occupied90 >> (shiftamount * 8)) & 0xFF][ROW(square)];
		colatk = movemask << shiftamount;
		/* Put them together */
		return (rowatk | colatk);
	case QUEEN:
		return board_attacksfrom(board, square, BISHOP, color) |
		       board_attacksfrom(board, square, ROOK, color);
	case KING:
		return kingattacks[square];
	}
	/* If we got to here then piece was invalid */
	panic("Invalid piece!");
	return BB(0x0);
}

/**
 * A bitboard of all legal pawn pushes from a square (in the direction
 * determined by color) considering the occupied state of the board
 */
bitboard_t board_pawnpushesfrom(board_t *board, square_t square, unsigned char color)
{
	bitboard_t moves, emptysquares;
	
	emptysquares = ~(board->occupied);
	moves = pawnmoves[color][square] & emptysquares; /* can't push if blocked */
	/* handle double push only if a single push was available */
	if (moves)
	{
		if (color == WHITE)
		{
			if (ROW(square) == RANK_2)
			{
				moves |= pawnmoves[color][square + 8] & emptysquares;
			}
		}
		else if (ROW(square) == RANK_7)
		{
			moves |= pawnmoves[color][square - 8] & emptysquares;
		}
	}
	return moves;
}

/**
 * Generates a linkedlist of "legal" moves for the color to play at the given
 * position. The moves are guaranteed legal, with all appropriate flags set,
 * suitable for being given to makemove(), EXCEPT it is possible that the move
 * might leave the player in check. We leave this task to the searcher - since
 * it is necessary to call makemove() to see if a move leaves the player in
 * check, and because the searcher calls makemove() anyway; we don't want to
 * waste time calling it twice.
 * This list needs to be movelist_destroy()ed. If there are no legal moves,
 * returns an empty list; if the board is NULL, returns a null pointer.
 */
void board_generatemoves(board_t *board, movelist_t *ml)
{
	bitboard_t position;
	piece_t piece;
	square_t square;
	unsigned char color;
	
	assert(board);
	
	color = board->tomove;
	movelist_init(ml);
	/* we only need to do this part once, not once per pawn */
	if (board->ep)
	{
		board_addmoves_ep(board, color, ml);
	}
	/* now do the rest of the moves */
	for (piece = 0; piece < 6; piece++)
	{
		position = board->pos[color][piece];
		while (position)
		{
			/* find a piece */
			square = BITSCAN(position);
			/* clear the bit */
			position ^= BB_SQUARE(square);
			/* special functions for special cases */
			switch (piece)
			{
			case PAWN:
				board_addmoves_pawn(board, square, color, ml);
				break;
			case KING:
				board_addmoves_king(board, square, color, ml);
				break;
			/* for knights, rooks, bishops, queens,
			 * we can use attacksfrom() directly */
			default:
				board_addmoves(board, square, piece, color, ml);
			}
		}
	}
	return;
}

/**
 * The board_addmoves* series of functions take pointers to two linkedlists,
 * and add each legal move for the specified piece of the specified color at
 * the specified square to the lists, sorted by capture/noncapture.
 */
static void board_addmoves_pawn(board_t *board, square_t square, unsigned char color,
                                movelist_t *ml)
{
	bitboard_t moves;
	square_t destsquare;
	piece_t captpiece;
	move_t move;
	/* note enpassant is handled separately. first we do captures: pawns
	 * can't move to a square they threaten unless it's a capture */
	moves = pawnattacks[color][square] & /* inlined attacksfrom */
	        board->piecesofcolor[OTHERCOLOR(color)];
	while (moves)
	{
		destsquare = BITSCAN(moves);
		moves ^= BB_SQUARE(destsquare);
		captpiece = board_pieceatsquare(board, destsquare, NULL);
		move = (square << MOV_INDEX_SRC) |
		       (destsquare << MOV_INDEX_DEST) |
		       (color << MOV_INDEX_COLOR) |
		       (0x1 << MOV_INDEX_CAPT) |
		       (captpiece << MOV_INDEX_CAPTPC) |
		       (PAWN << MOV_INDEX_PIECE);
		/* handle promotions */
		if (ROW(destsquare) == HOMEROW(OTHERCOLOR(color)))
		{
			move |= (0x1 << MOV_INDEX_PROM);
			movelist_add(ml, board->attackedby, (move | (BISHOP << MOV_INDEX_PROMPC)));
			movelist_add(ml, board->attackedby, (move | (ROOK << MOV_INDEX_PROMPC)));
			movelist_add(ml, board->attackedby, (move | (KNIGHT << MOV_INDEX_PROMPC)));
			movelist_add(ml, board->attackedby, (move | (QUEEN << MOV_INDEX_PROMPC)));
		}
		else /* normal capture */
		{
			movelist_add(ml, board->attackedby, move);
		}
	}
	/* now we handle pushes */
	moves = board_pawnpushesfrom(board, square, color);
	/* and now we have the proper movemask generated */
	while (moves)
	{
		destsquare = BITSCAN(moves);
		moves ^= BB_SQUARE(destsquare);
		move = (square << MOV_INDEX_SRC) |
		       (destsquare << MOV_INDEX_DEST) |
		       (color << MOV_INDEX_COLOR) |
		       (PAWN << MOV_INDEX_PIECE);
		/* handle promotions - add to the captures list because pawn
		 * promotions, like captures, change the board's material */
		if (ROW(destsquare) == HOMEROW(OTHERCOLOR(color)))
		{
			move |= (0x1 << MOV_INDEX_PROM);
			movelist_add(ml, board->attackedby, (move | (BISHOP << MOV_INDEX_PROMPC)));
			movelist_add(ml, board->attackedby, (move | (ROOK << MOV_INDEX_PROMPC)));
			movelist_add(ml, board->attackedby, (move | (KNIGHT << MOV_INDEX_PROMPC)));
			movelist_add(ml, board->attackedby, (move | (QUEEN << MOV_INDEX_PROMPC)));
		}
		else /* normal push */
		{
			movelist_add(ml, board->attackedby, move);
		}
	}
	/* and we're done, phew */
	return;
}
/* don't call this if board->ep = 0 */
static void board_addmoves_ep(board_t *board, unsigned char color, movelist_t *ml)
{
	bitboard_t pawns;
	square_t fromsquare; /* we know where the dest is; we need this now */
	move_t move;

	assert(board->ep);
	
	/* find all pawns eligible to make the capture */
	pawns = bb_adjacentcols[COL(board->ep)] & /* the col the pawn is on */
	        BB_EP_FROMRANK(color) & /* the rank a pawn needs to be on */
		board->pos[color][PAWN] /* where your pawns actually are */;
	while (pawns)
	{
		fromsquare = BITSCAN(pawns);
		pawns ^= BB_SQUARE(fromsquare);
		move = (fromsquare << MOV_INDEX_SRC) |
		       (board->ep << MOV_INDEX_DEST) |
		       (0x1 << MOV_INDEX_EP) |
		       (0x1 << MOV_INDEX_CAPT) |
		       (PAWN << MOV_INDEX_CAPTPC) |
		       (PAWN << MOV_INDEX_PIECE);
		movelist_add(ml, board->attackedby, move);
	}
	return;
}
/* wrapper for board_addmoves() with a special case handler for castling */
static void board_addmoves_king(board_t *board, square_t square, unsigned char color,
                                movelist_t *ml)
{
	move_t move;
	square_t destsquare;
	int side;
	/* used in the second half */
	bitboard_t moves, capts;
	piece_t captpiece;
	
	/* deal with castling - iterate side={0,1} */
	for (side = 0; side < 2; side++)
	{
		if (board->castle[color][side])
		{
			/* If the squares are occupied (1st two lines) or
			 * threatened (2nd two) then we cannot castle */
			if (!((castle_clearsquares[color][side] &
			       board->occupied) ||
			      board_squaresareattacked(board, castle_safesquares[color][side],
			                               OTHERCOLOR(color))))
			{
				/* find where the king will end up */
				destsquare = SQUARE(CASTLE_DEST_COL(side),
				                    HOMEROW(color));
				/* generate and add the move */
				move = (square << MOV_INDEX_SRC) |
				       (destsquare << MOV_INDEX_DEST) |
				       (color << MOV_INDEX_COLOR) |
				       (0x1 << MOV_INDEX_CASTLE) |
				       (KING << MOV_INDEX_PIECE);
				movelist_add(ml, board->attackedby, move);
			}
		}
	}
	/* now the special case is out of the way, the rest is simple. this
	 * could be a call to board_addmoves, but for move ordering purposes
	 * we do it separately. also, if we filter out suicide moves here,
	 * we avoid wasting time with applymove() in the searcher before
	 * seeing if the move is legal. */
	/* inlined attacksfrom */
	moves = kingattacks[square] & (~(board->piecesofcolor[color]));
	/* here's the special part - we can't move the king anywhere where it
	 * will be in check - note, this still produces an illegal move if the
	 * king walks straight backward from a sliding piece, but we're only
	 * producing pseudo-legal moves, so this is merely an optimization.
	 * the movelist relies on not producing an obvious king-hang move. */
	moves &= (~(board->attackedby[OTHERCOLOR(color)]));
	/* the rest is the same as board_addmoves */
	capts = moves & board->piecesofcolor[OTHERCOLOR(color)];
	moves ^= capts;
	/* that's a lie - the other special thing we do here is ordering so
	 * the king moves/capts are grouped with the pawn moves/capts */
	while (capts)
	{
		/* find and clear a bit */
		destsquare = BITSCAN(capts);
		capts ^= BB_SQUARE(destsquare);
		/* find what piece was captured */
		captpiece = board_pieceatsquare(board, destsquare, NULL);
		/* generate the move_t */
		move = (square << MOV_INDEX_SRC) |       /* from square */
		       (destsquare << MOV_INDEX_DEST) |  /* to square */
		       (color << MOV_INDEX_COLOR) |      /* color flag */
		       (0x1 << MOV_INDEX_CAPT) |         /* capture flag */
		       (captpiece << MOV_INDEX_CAPTPC) | /* captured piece */
		       (KING << MOV_INDEX_PIECE);        /* our piece */
		/* and add it to the list */
		movelist_add(ml, board->attackedby, move);
	}
	/* then process noncapture moves */
	while (moves)
	{
		/* find and clear a bit */
		destsquare = BITSCAN(moves);
		moves ^= BB_SQUARE(destsquare);
		/* generate the move_t */
		move = (square << MOV_INDEX_SRC) |      /* from square */
		       (destsquare << MOV_INDEX_DEST) | /* to square */
		       (color << MOV_INDEX_COLOR) |     /* color flag */
		       (KING << MOV_INDEX_PIECE);       /* our piece */
		/* and add it to the list */
		movelist_add(ml, board->attackedby, move);
	}

	return;
}
/* for non-special-case pieces: knight bishop rook queen. see above comment */
static void board_addmoves(board_t *board, square_t square, piece_t piece, unsigned char color,
                           movelist_t *ml)
{
	bitboard_t moves, capts;
	square_t destsquare;
	piece_t captpiece;
	move_t move;
	
	/* find all destination squares -- all attacked squares, but can't
	 * capture our own pieces */
	moves = board_attacksfrom(board, square, piece, color) & (~(board->piecesofcolor[color]));
	/* separate captures from noncaptures */
	capts = moves & board->piecesofcolor[OTHERCOLOR(color)];
	moves ^= capts;
	/* process captures first */
	while (capts)
	{
		/* find and clear a bit */
		destsquare = BITSCAN(capts);
		capts ^= BB_SQUARE(destsquare);
		/* find what piece was captured */
		captpiece = board_pieceatsquare(board, destsquare, NULL);
		/* generate the move_t */
		move = (square << MOV_INDEX_SRC) |       /* from square */
		       (destsquare << MOV_INDEX_DEST) |  /* to square */
		       (color << MOV_INDEX_COLOR) |      /* color flag */
		       (0x1 << MOV_INDEX_CAPT) |         /* capture flag */
		       (captpiece << MOV_INDEX_CAPTPC) | /* captured piece */
		       (piece << MOV_INDEX_PIECE);       /* our piece */
		/* and add it to the list */
		movelist_add(ml, board->attackedby, move);
	}
	/* then process noncapture moves */
	while (moves)
	{
		/* find and clear a bit */
		destsquare = BITSCAN(moves);
		moves ^= BB_SQUARE(destsquare);
		/* generate the move_t */
		move = (square << MOV_INDEX_SRC) |      /* from square */
		       (destsquare << MOV_INDEX_DEST) | /* to square */
		       (color << MOV_INDEX_COLOR) |     /* color flag */
		       (piece << MOV_INDEX_PIECE);      /* our piece */
		/* and add it to the list */
		movelist_add(ml, board->attackedby, move);
	}
	return;
}

/**
 * The same as board_generatemoves, except we generate only captures. This is
 * optimized to not even think about noncapture moves to be especially fast
 * for quiescence searching.
 */
void board_generatecaptures(board_t *board, movelist_t *ml)
{
	bitboard_t position;
	piece_t piece;
	square_t square;
	unsigned char color;

	assert(board);
	
	color = board->tomove;
	movelist_init(ml);
	/* same call as in addmoves... ep is always a capture */
	if (board->ep)
	{
		//XXX: Take this out, as it won't help that much in quiescence? Depends on how fast it is
		board_addmoves_ep(board, color, ml);
	}
	for (piece = 0; piece < 6; piece++)
	{
		position = board->pos[color][piece];
		while (position)
		{
			/* find a piece */
			square = BITSCAN(position);
			/* clear the bit */
			position ^= BB_SQUARE(square);
			switch (piece)
			{
			case PAWN:
				board_addcaptures_pawn(board, square, color, ml);
				break;
			case KING:
				board_addcaptures_king(board, square, color, ml);
				break;
			default:
				board_addcaptures(board, square, piece, color, ml);
			}
		}
	}
	return;
}

static void board_addcaptures_pawn(board_t *board, square_t square, unsigned char color, movelist_t *ml)
{
	bitboard_t moves;
	square_t destsquare;
	piece_t captpiece;
	move_t move;
	/* captures */
	moves = pawnattacks[color][square] & /* inlined attacksfrom */
	        board->piecesofcolor[OTHERCOLOR(color)];
	while (moves)
	{
		destsquare = BITSCAN(moves);
		moves ^= BB_SQUARE(destsquare);
		captpiece = board_pieceatsquare(board, destsquare, NULL);
		move = (square << MOV_INDEX_SRC) |
		       (destsquare << MOV_INDEX_DEST) |
		       (color << MOV_INDEX_COLOR) |
		       (0x1 << MOV_INDEX_CAPT) |
		       (captpiece << MOV_INDEX_CAPTPC) |
		       (PAWN << MOV_INDEX_PIECE);
		/* handle promotions */
		if (ROW(destsquare) == HOMEROW(OTHERCOLOR(color)))
		{
			move |= (0x1 << MOV_INDEX_PROM);
			movelist_add(ml, board->attackedby, (move | (BISHOP << MOV_INDEX_PROMPC)));
			movelist_add(ml, board->attackedby, (move | (ROOK << MOV_INDEX_PROMPC)));
			movelist_add(ml, board->attackedby, (move | (KNIGHT << MOV_INDEX_PROMPC)));
			movelist_add(ml, board->attackedby, (move | (QUEEN << MOV_INDEX_PROMPC)));
		}
		else /* normal capture */
		{
			movelist_add(ml, board->attackedby, move);
		}
	}
	/* now we handle pushes only if it's a passed pawn */
	if (board_pawnpassed(board, square, color))
	{
		moves = board_pawnpushesfrom(board, square, color);
		/* and now we have the proper movemask generated */
		while (moves)
		{
			destsquare = BITSCAN(moves);
			moves ^= BB_SQUARE(destsquare);
			move = (square << MOV_INDEX_SRC) |
			       (destsquare << MOV_INDEX_DEST) |
			       (color << MOV_INDEX_COLOR) |
			       (PAWN << MOV_INDEX_PIECE);
			/* handle promotions */
			if (ROW(destsquare) == HOMEROW(OTHERCOLOR(color)))
			{
				move |= (0x1 << MOV_INDEX_PROM);
				movelist_add(ml, board->attackedby, (move | (BISHOP << MOV_INDEX_PROMPC)));
				movelist_add(ml, board->attackedby, (move | (ROOK << MOV_INDEX_PROMPC)));
				movelist_add(ml, board->attackedby, (move | (KNIGHT << MOV_INDEX_PROMPC)));
				movelist_add(ml, board->attackedby, (move | (QUEEN << MOV_INDEX_PROMPC)));
			}
			else /* normal push */
			{
				movelist_add(ml, board->attackedby, move);
			}
		}
	}
	return;
}

static void board_addcaptures_king(board_t *board, square_t square, unsigned char color, movelist_t *ml)
{
	move_t move;
	square_t destsquare;
	bitboard_t moves, capts;
	piece_t captpiece;
	
	/* No dealing with castling! :D this is the same as addcaptures() but
	 * again we do the special thing */
	/* inlined attacksfrom */
	moves = kingattacks[square] & (~(board->piecesofcolor[color]));
	moves &= (~(board->attackedby[OTHERCOLOR(color)]));
	capts = moves & board->piecesofcolor[OTHERCOLOR(color)];
	moves ^= capts;
	/* like in regular movegen, captures by king go at the beginning */
	while (capts)
	{
		/* find and clear a bit */
		destsquare = BITSCAN(capts);
		capts ^= BB_SQUARE(destsquare);
		/* find what piece was captured */
		captpiece = board_pieceatsquare(board, destsquare, NULL);
		/* generate the move_t */
		move = (square << MOV_INDEX_SRC) |       /* from square */
		       (destsquare << MOV_INDEX_DEST) |  /* to square */
		       (color << MOV_INDEX_COLOR) |      /* color flag */
		       (0x1 << MOV_INDEX_CAPT) |         /* capture flag */
		       (captpiece << MOV_INDEX_CAPTPC) | /* captured piece */
		       (KING << MOV_INDEX_PIECE);        /* our piece */
		/* and add it to the list */
		movelist_add(ml, board->attackedby, move);
	}
	return;
}

static void board_addcaptures(board_t *board, square_t square, piece_t piece, unsigned char color, movelist_t *ml)
{
	bitboard_t moves, capts;
	square_t destsquare;
	piece_t captpiece;
	move_t move;
	
	/* find all destination squares -- all attacked squares, but can't
	 * capture our own pieces */
	moves = board_attacksfrom(board, square, piece, color) &
	        (~(board->piecesofcolor[color]));
	/* separate captures from noncaptures */
	capts = moves & board->piecesofcolor[OTHERCOLOR(color)];
	moves ^= capts;
	/* process captures first */
	while (capts)
	{
		/* find and clear a bit */
		destsquare = BITSCAN(capts);
		capts ^= BB_SQUARE(destsquare);
		/* find what piece was captured */
		captpiece = board_pieceatsquare(board, destsquare, NULL);
		/* generate the move_t */
		move = (square << MOV_INDEX_SRC) |       /* from square */
		       (destsquare << MOV_INDEX_DEST) |  /* to square */
		       (color << MOV_INDEX_COLOR) |      /* color flag */
		       (0x1 << MOV_INDEX_CAPT) |         /* capture flag */
		       (captpiece << MOV_INDEX_CAPTPC) | /* captured piece */
		       (piece << MOV_INDEX_PIECE);       /* our piece */
		/* and add it to the list */
		movelist_add(ml, board->attackedby, move);
	}
	return;
}


/***********************
 * Changing board state
 ***********************/

/**
 * Given a move, which MUST be properly formatted AND legal (only use moves
 * returned by board_generatemoves() or move_islegal() here), applies the move
 * to change the board position. Updates the zobrist hash, undo stacks,
 * everything.
 */
void board_applymove(board_t *board, move_t move)
{
	square_t rooksrc, rookdest; /* how the rook moves in castling */
	square_t epcapture;         /* where a pawn will disappear from */
	int i;
	
	/* save history. */
	history_t *h = &board->history[board->moves];
	assert(board->moves < HISTORY_STACK_SIZE);
	h->hash                     = board->hash;
	h->attackedby[WHITE]        = board->attackedby[WHITE];
	h->attackedby[BLACK]        = board->attackedby[BLACK];
	h->castle[WHITE][QUEENSIDE] = board->castle[WHITE][QUEENSIDE];
	h->castle[WHITE][KINGSIDE]  = board->castle[WHITE][KINGSIDE];
	h->castle[BLACK][QUEENSIDE] = board->castle[BLACK][QUEENSIDE];
	h->castle[BLACK][KINGSIDE]  = board->castle[BLACK][KINGSIDE];
	h->ep                       = board->ep;
	h->halfmoves                = board->halfmoves;
	h->reps                     = board->reps;
	h->move                     = move;

	/* cleanly handle null-move */
	if (move == 0)
	{
		/* clear ep square */
		board->hash ^= zobrist_ep[board->ep];
		board->ep = 0;
		board->hash ^= zobrist_ep[board->ep];
		/* update the move clock */
		board->halfmoves++;
		board->moves++;
		/* null move is illegal, so it can't cause a threefold draw */
		board->reps = 0;
		/* switch who has the move */
		board->tomove = OTHERCOLOR(board->tomove);
		board->hash ^= zobrist_tomove;
		return;
	}
	
	assert(MOV_PIECE(move) >= 0 && MOV_PIECE(move) < 6);

	/* Take care of captures - we need to remove the captured piece. */
	if (MOV_CAPT(move))
	{
		/* special case - en passant - square behind the dest square */
		if (MOV_EP(move))
		{
			assert(ROW(MOV_SRC(move)) == 4 - board->tomove);
			/* find where the pawn disappears from */
			epcapture = (board->tomove == WHITE) ? 
			            (MOV_DEST(move) - 8) :
				    (MOV_DEST(move) + 8);
			/* kill the pawn */
			board_togglepiece(board, epcapture,
			                  OTHERCOLOR(board->tomove), PAWN);
			/* change the material count */
			board->material[OTHERCOLOR(board->tomove)] -=
				eval_piecevalue[PAWN];
		}
		/* standard capture - square is the dest square */
		else
		{
			board_togglepiece(board, MOV_DEST(move),
			                  OTHERCOLOR(board->tomove),
			                  MOV_CAPTPC(move));
			/* If the captured piece is a rook on its home square,
			 * and the player was still able to castle that side,
			 * we need to clear the castle privileges there */
			if (MOV_CAPTPC(move) == ROOK &&
			    ROW(MOV_DEST(move)) == HOMEROW(OTHERCOLOR(board->tomove)) &&
			    (COL(MOV_DEST(move)) == COL_H ||
			     COL(MOV_DEST(move)) == COL_A))
			{
				/* (COLUMN == COL_H) will evaluate to 1 if on
				 * KINGSIDE (this depends on KINGSIDE == 1) */
				unsigned char side = (COL(MOV_DEST(move)) == COL_H);
				/* if castle rights is already gone there, we
				 * don't do anything. this is important when
				 * toggling the zobrist */
				if (board->castle[OTHERCOLOR(board->tomove)][side])
				{
					board->castle[OTHERCOLOR(board->tomove)][side] = 0;
					board->hash ^= zobrist_castle[OTHERCOLOR(board->tomove)]
					                             [side];
				}

			}
			/* change the material count */
			board->material[OTHERCOLOR(board->tomove)] -=
				eval_piecevalue[MOV_CAPTPC(move)];
		}
		board->halfmoves = 0;
	}
	/* here we detect irreversible moves for the halfmove clock */
	else
	{
		if (MOV_PIECE(move) == PAWN)
		{
			board->halfmoves = 0;
		}
		else
		{
			board->halfmoves++;
		}
	}
	/* the game clock as well */
	board->moves++;
	
	/* castling, need to move the rook. castle privileges handled below */
	if (MOV_CASTLE(move))
	{
		/* detect kingside vs queenside */
		if (COL(MOV_DEST(move)) == COL_G)
		{
			rooksrc  = SQUARE(COL_H,HOMEROW(board->tomove));
			rookdest = SQUARE(COL_F,HOMEROW(board->tomove));
		}
		else
		{
			rooksrc  = SQUARE(COL_A,HOMEROW(board->tomove));
			rookdest = SQUARE(COL_D,HOMEROW(board->tomove));
		}
		/* and move the rook */
		board_togglepiece(board, rooksrc, board->tomove, ROOK);
		board_togglepiece(board, rookdest, board->tomove, ROOK);
		/* set the flag */
		board->hascastled[board->tomove] = 1;
	}
	
	/* here we move the moving piece itself; in the former case it turns
	 * into a different type of piece */
	board_togglepiece(board, MOV_SRC(move), board->tomove,
	                  MOV_PIECE(move));
	if (MOV_PROM(move))
	{
		assert(ROW(MOV_DEST(move)) ==
		       HOMEROW(OTHERCOLOR(board->tomove)));
		board_togglepiece(board, MOV_DEST(move), board->tomove,
		                 MOV_PROMPC(move));
		/* and of course account for this in the material count */
		board->material[board->tomove] +=
			(eval_piecevalue[MOV_PROMPC(move)] -
			 eval_piecevalue[PAWN]);
	}
	else
	{
		board_togglepiece(board, MOV_DEST(move), board->tomove,
		                 MOV_PIECE(move));
	}

	/* detect an enpassantable move */
	if (MOV_PIECE(move) == PAWN)
	{
		/* if the difference between the square values is 16 not 8
		 * then it was a double push */
		if ((board->tomove == WHITE) &&
		    ((MOV_DEST(move) - MOV_SRC(move)) == 16))
		{
			/* clear the old ep status in the hash */
			board->hash ^= zobrist_ep[board->ep];
			/* set the new value */
			board->ep = MOV_SRC(move) + 8;
			/* throw in the new key */
			board->hash ^= zobrist_ep[board->ep];
		}
		else if ((board->tomove == BLACK) &&
		         ((MOV_SRC(move) - MOV_DEST(move)) == 16))
		{
			board->hash ^= zobrist_ep[board->ep];
			board->ep = MOV_DEST(move) + 8;
			board->hash ^= zobrist_ep[board->ep];
		}
		else
		{
			board->hash ^= zobrist_ep[board->ep];
			board->ep = 0;
			board->hash ^= zobrist_ep[board->ep];
		}
	}
	/* not a pawn move. ep square goes away. */
	else
	{
		board->hash ^= zobrist_ep[board->ep];
		board->ep = 0;
		board->hash ^= zobrist_ep[board->ep];
		/* handle castling rights */
		if (MOV_PIECE(move) == KING)
		{
			if (board->castle[board->tomove][KINGSIDE])
			{
				board->castle[board->tomove][KINGSIDE] = 0;
				board->hash ^= zobrist_castle[board->tomove][KINGSIDE];
			}
			if (board->castle[board->tomove][QUEENSIDE])
			{
				board->castle[board->tomove][QUEENSIDE] = 0;
				board->hash ^= zobrist_castle[board->tomove][QUEENSIDE];
			}
		}
		else if (MOV_PIECE(move) == ROOK)
		{
			if ((board->castle[board->tomove][KINGSIDE]) &&
			    (COL(MOV_SRC(move)) == COL_H))
			{
				board->castle[board->tomove][KINGSIDE] = 0;
				board->hash ^= zobrist_castle[board->tomove][KINGSIDE];
			}
			else if ((board->castle[board->tomove][QUEENSIDE]) &&
			         (COL(MOV_SRC(move)) == COL_A))
			{
				board->castle[board->tomove][QUEENSIDE] = 0;
				board->hash ^= zobrist_castle[board->tomove][QUEENSIDE];
			}
		}
	}

	/* switch who has the move */
	board->tomove = OTHERCOLOR(board->tomove);
	board->hash ^= zobrist_tomove;

	/* calculate repetition count */
	board->reps = 0;
	for (i = 1; i <= board->halfmoves; i++)
	{
		if (board->history[board->moves - i].hash == board->hash)
		{
			board->reps = board->history[board->moves - i].reps+1;
			break;
		}
	}
	
	/* and finally */
	board_regeneratethreatened(board);
	return;
}

/**
 * Go back one move. The cleaner implementation is to keep a stack of the
 * moves, but it's faster to have the searcher/legalitychecker just pass in
 * the move again rather than mucking about with data structures.
 * Note if you pass in a move different from the one that was most recently
 * made, the board state will choke and die horribly.
 */
void board_undomove(board_t *board, move_t move)
{
	square_t rooksrc, rookdest, epcapture;
	
	/* switch who has the move */
	board->tomove = OTHERCOLOR(board->tomove);
	board->hash ^= zobrist_tomove;

	/* cleanly handle null-move */
	if (move == 0)
	{
		board->moves--;
		board->halfmoves--;
		board->hash ^= zobrist_ep[board->ep];
		board->ep = board->history[board->moves].ep;
		board->hash ^= zobrist_ep[board->ep];
		board->reps = board->history[board->moves].reps;
		return;
	}
	
	/* The non-special things (see bottom of function for special things)
	 * are basically applymove in reverse. First move the piece back. */
	board_togglepiece(board, MOV_SRC(move), board->tomove, MOV_PIECE(move));
	if (MOV_PROM(move))
	{
		board_togglepiece(board, MOV_DEST(move), board->tomove,
		                  MOV_PROMPC(move));
		/* material count */
		board->material[board->tomove] -=
			(eval_piecevalue[MOV_PROMPC(move)] -
			 eval_piecevalue[PAWN]);
	}
	else
	{
		board_togglepiece(board, MOV_DEST(move), board->tomove,
		                  MOV_PIECE(move));
	}
	/* castling, need to move the rook back. */
	if (MOV_CASTLE(move))
	{
		/* detect kingside vs queenside */
		if (COL(MOV_DEST(move)) == COL_G)
		{
			/* note src is where it needs to end up because we're
			 * moving backwards not forwards */
			rooksrc  = SQUARE(COL_H,HOMEROW(board->tomove));
			rookdest = SQUARE(COL_F,HOMEROW(board->tomove));
		}
		else
		{
			rooksrc  = SQUARE(COL_A,HOMEROW(board->tomove));
			rookdest = SQUARE(COL_D,HOMEROW(board->tomove));
		}
		/* and move the rook */
		board_togglepiece(board, rooksrc, board->tomove, ROOK);
		board_togglepiece(board, rookdest, board->tomove, ROOK);
		/* flag */
		board->hascastled[board->tomove] = 0;
	}
	/* for captures, put the captured piece back */
	if (MOV_CAPT(move))
	{
		/* special case - en passant - square behind the dest square */
		if (MOV_EP(move))
		{
			epcapture = (board->tomove == WHITE) ? 
			            (MOV_DEST(move) - 8) :
				    (MOV_DEST(move) + 8);
			board_togglepiece(board, epcapture,
			                  OTHERCOLOR(board->tomove), PAWN);
			board->material[OTHERCOLOR(board->tomove)] +=
				eval_piecevalue[PAWN];
		}
		/* standard capture - square is the dest square */
		else
		{
			board_togglepiece(board, MOV_DEST(move),
			                  OTHERCOLOR(board->tomove),
			                  MOV_CAPTPC(move));
			board->material[OTHERCOLOR(board->tomove)] +=
				eval_piecevalue[MOV_CAPTPC(move)];
		}
	}

	/* decrement the game clock */
	board->moves--;
	
	/* Get the previous special board state information from the stacks */
	board->attackedby[WHITE] = board->history[board->moves].attackedby[WHITE];
	board->attackedby[BLACK] = board->history[board->moves].attackedby[BLACK];
	
	/* clear the old castle keys */
	if (board->castle[WHITE][QUEENSIDE]) { board->hash ^= zobrist_castle[WHITE][QUEENSIDE]; }
	if (board->castle[WHITE][KINGSIDE])  { board->hash ^= zobrist_castle[WHITE][KINGSIDE]; }
	if (board->castle[BLACK][QUEENSIDE]) { board->hash ^= zobrist_castle[BLACK][QUEENSIDE]; }
	if (board->castle[BLACK][KINGSIDE])  { board->hash ^= zobrist_castle[BLACK][KINGSIDE]; }
	/* set the new castle rights */
	board->castle[WHITE][QUEENSIDE] = board->history[board->moves].castle[WHITE][QUEENSIDE];
	board->castle[WHITE][KINGSIDE]  = board->history[board->moves].castle[WHITE][KINGSIDE];
	board->castle[BLACK][QUEENSIDE] = board->history[board->moves].castle[BLACK][QUEENSIDE];
	board->castle[BLACK][KINGSIDE]  = board->history[board->moves].castle[BLACK][KINGSIDE];
	/* set the new castle keys */
	if (board->castle[WHITE][QUEENSIDE]) { board->hash ^= zobrist_castle[WHITE][QUEENSIDE]; }
	if (board->castle[WHITE][KINGSIDE])  { board->hash ^= zobrist_castle[WHITE][KINGSIDE]; }
	if (board->castle[BLACK][QUEENSIDE]) { board->hash ^= zobrist_castle[BLACK][QUEENSIDE]; }
	if (board->castle[BLACK][KINGSIDE])  { board->hash ^= zobrist_castle[BLACK][KINGSIDE]; }

	/* restore ep information */
	board->hash ^= zobrist_ep[board->ep];
	board->ep = board->history[board->moves].ep;
	board->hash ^= zobrist_ep[board->ep];

	/* set the halfmove clock */
	board->halfmoves = board->history[board->moves].halfmoves;

	/* and repetitions */
	board->reps = board->history[board->moves].reps;

	/* consistency check */
	assert(board->hash == board->history[board->moves].hash);
	assert(move == board->history[board->moves].move);
	return;
}

/**
 * The togglepiece function is a wrapper to simplify moving pieces around on
 * the board. They modify the bits in the piece-specific position masks, the
 * all-pieces-of-one-color mask, and in the four occupied boards, and adjust
 * the zobrist hash.
 */
static void board_togglepiece(board_t *board, square_t square,
                              unsigned char color, piece_t piece)
{
	/* no other piece ought be on this square */
	assert(!(board->occupied & BB_SQUARE(square) &
		 ~board->pos[color][piece]));
	/* piece-specific position mask */
	board->pos[color][piece] ^= BB_SQUARE(square);
	
	/* all the color's pieces */
	board->piecesofcolor[color] ^= BB_SQUARE(square);
	
	/* general occupied masks */
	board->occupied ^= BB_SQUARE(square);
	board->occupied90 ^= BB_SQUARE(ROT90SQUAREINDEX(square));
	board->occupied45 ^= BB_SQUARE(ROT45SQUAREINDEX(square));
	board->occupied315 ^= BB_SQUARE(ROT315SQUAREINDEX(square));
	
	/* adjust the zobrist */
	board->hash ^= zobrist_piece[color][piece][square];
}

/**
 * This hashtable stores threatened square masks so we don't have to always
 * regenerate them.
 */
typedef struct {
	bitboard_t key;
	bitboard_t white;
	bitboard_t black;
} regen_entry_t;

#ifndef REGEN_NUM_BUCKETS
#define REGEN_NUM_BUCKETS (32 * 1024 * 1024 / sizeof(regen_entry_t)) // 32 MB
#endif
static regen_entry_t regen_array[REGEN_NUM_BUCKETS];

static void regen_add(bitboard_t key, bitboard_t white, bitboard_t black)
{
	unsigned long bucket = key % REGEN_NUM_BUCKETS;
	regen_array[bucket].key = key;
	regen_array[bucket].white = white;
	regen_array[bucket].black = black;
}

static int regen_get(bitboard_t key, bitboard_t *white, bitboard_t *black)
{
	unsigned long bucket = key % REGEN_NUM_BUCKETS;
	if (regen_array[bucket].key == key)
	{
		*white = regen_array[bucket].white;
		*black = regen_array[bucket].black;
		return 1;
	}
	return 0;
}

int regen_hits = 0;
int regen_misses = 0;

/**
 * Regenerate the threatened squares masks for both sides. Should be used when
 * a move is made. Don't use in undomove, use the special stacks instead
 */
static void board_regeneratethreatened(board_t *board)
{
	bitboard_t mask;
	bitboard_t position;
	piece_t piece;
	square_t square;
	
	/* check hashtable */
	if (regen_get(board->hash,
		      &board->attackedby[WHITE], &board->attackedby[BLACK]))
	{
		regen_hits++;
		return;
	}

	/********
	 * White
	 ********/
	mask = BB(0x0);
	/* pawn threatened can be done for all pawns at once */
	mask |= board_pawnattacks(board->pos[WHITE][PAWN], WHITE);
	/* Knights and kings manually inlined */
	position = board->pos[WHITE][KNIGHT];
	while (position)
	{
		square = BITSCAN(position);
		position ^= BB_SQUARE(square);
		mask |= knightattacks[square];
	}
	position = board->pos[WHITE][KING];
	while(position)
	{
		square = BITSCAN(position);
		position ^= BB_SQUARE(square);
		mask |= kingattacks[square];
	}
	/* other piece threatened */
	for (piece = 2; piece < 5; piece++)
	{
		position = board->pos[WHITE][piece];
		/* keep going until we've found all the pieces */
		while (position)
		{
			/* Find one of the pieces, clear its bit */
			square = BITSCAN(position);
			position ^= BB_SQUARE(square);
			/* Put the attacks from this square in the mask */
			mask |= board_attacksfrom(board, square, piece, WHITE);
		}
	}
	board->attackedby[WHITE] = mask;
	/********
	 * Black
	 ********/
	mask = BB(0x0);
	/* pawn threatened can be done for all pawns at once */
	mask |= board_pawnattacks(board->pos[BLACK][PAWN], BLACK);
	/* Knights and kings manually inlined */
	position = board->pos[BLACK][KNIGHT];
	while (position)
	{
		square = BITSCAN(position);
		position ^= BB_SQUARE(square);
		mask |= knightattacks[square];
	}
	position = board->pos[BLACK][KING];
	while(position)
	{
		square = BITSCAN(position);
		position ^= BB_SQUARE(square);
		mask |= kingattacks[square];
	}
	/* other piece threatened */
	for (piece = 2; piece < 5; piece++)
	{
		position = board->pos[BLACK][piece];
		/* keep going until we've found all the pieces */
		while (position)
		{
			/* Find one of the pieces, clear its bit */
			square = BITSCAN(position);
			position ^= BB_SQUARE(square);
			/* Put the attacks from this square in the mask */
			mask |= board_attacksfrom(board, square, piece, BLACK);
		}
	}
	board->attackedby[BLACK] = mask;

	/* save to hashtable */
	regen_add(board->hash, board->attackedby[WHITE], board->attackedby[BLACK]);
	regen_misses++;
	
	return;
}

/**
 * Get a bitmap of all squares attacked by the pawns in the given position of
 * the given color. Used by regenthreatened, and also in pawnstructure eval
 */
bitboard_t board_pawnattacks(bitboard_t pawns, unsigned char color)
{
	/* a-pawns don't attack left; h-pawns don't attack right */
	if (color == WHITE)
	{
		return ((pawns & ~BB_FILEA) << 7) | ((pawns & ~BB_FILEH) << 9);
	}
	else
	{
		return ((pawns & ~BB_FILEA) >> 9) | ((pawns & ~BB_FILEH) >> 7);
	}
}

/**
 * Is the current position a threefold draw?
 */
int board_threefold_draw(board_t *board)
{
	return board->reps >= 2;
}

/****************************************************************************
 * END:   BITBOARD FUNCTIONS
 * BEGIN: MOVE FUNCTIONS
 ****************************************************************************/

static move_t move_checksuicide(board_t *, move_t, char **);

/**
 * Parses computer friendly ("d7c8N" or "e1g1") notation to generate a simple
 * move_t template. Will return 0 on a malformatted move (note 0 is never a
 * valid move_t). This will only set the prom flag, not castle/ep/capt. As a
 * result, the resultant move is not suitable for throwing directly into
 * applyMove.
 */
move_t move_fromstring(char *str)
{
	size_t length = strlen(str);
	move_t move = 0;
	
	/* check for malformatted move string */
	if (length < 4 || length > 5 || /* too short/long */
	    !(str[0] >= 'a' && str[0] <= 'h') || /* bad src/dest squares */
	    !(str[1] >= '1' && str[1] <= '8') ||
	    !(str[2] >= 'a' && str[2] <= 'h') ||
	    !(str[3] >= '1' && str[3] <= '8') ||
	    (length == 5 && /* bad promotion piece */
	     !(str[4] == 'N' || str[4] == 'B' ||
	       str[4] == 'R' || str[4] == 'Q' ||
	       str[4] == 'n' || str[4] == 'b' ||
	       str[4] == 'r' || str[4] == 'q')))
	{
		return (move_t)0;
	}
	
	/* set the src and dest */
	move |= SQUARE((str[0] - 'a'), (str[1] - '1')) << MOV_INDEX_SRC;
	move |= SQUARE((str[2] - 'a'), (str[3] - '1')) << MOV_INDEX_DEST;
	
	/* promotion */
	if (length == 5)
	{
		move |= 1 << MOV_INDEX_PROM; /* set the flag */
		switch(str[4])
		{
		case 'N':
		case 'n':
			move |= KNIGHT << MOV_INDEX_PROMPC;
			break;
		case 'B':
		case 'b':
			move |= BISHOP << MOV_INDEX_PROMPC;
			break;
		case 'R':
		case 'r':
			move |= ROOK << MOV_INDEX_PROMPC;
			break;
		default:
			move |= QUEEN << MOV_INDEX_PROMPC;
		}
	}
	
	return move;
}

/**
 * A more advanced form of move_fromstring. Given also a board_t, will return
 * a move_t for the move represented by str if it is 1) properly formatted,
 * 2) valid, 3) legal. Otherwise returns 0. All appropriate flags will be set,
 * so this is suitable for applymove.
 * This code is not fast, as it will apply/undo the move from the board. Use
 * it for checking the opponent's move, not the engine's move.
 */
move_t move_islegal(board_t *board, char *str, char **errmsg)
{
	move_t move;
	square_t src, dest;
	piece_t piece;
	bitboard_t moves, clearsquares;
	
	/* Parse the string */
	move = move_fromstring(str);
	src = MOV_SRC(move);
	dest = MOV_DEST(move);
	
	/* First check: sanity; this also handles if move_fromstring failed */
	if (src == dest)
	{
		*errmsg = "!!! Motion is required for a move.";
		return 0;
	}
	/* Does this piece belong to us */
	if (!(BB_SQUARE(src) & board->piecesofcolor[board->tomove]))
	{
		*errmsg = "!!! You don't have a piece there.";
		return 0;
	}
	
	/* Find our piece and where it can go */
	piece = board_pieceatsquare(board, src, NULL);
	moves = board_attacksfrom(board, src, piece, board->tomove) &
	        (~(board->piecesofcolor[board->tomove])); /* can't capture own piece */
	move |= piece << MOV_INDEX_PIECE; /* set the piece-type flag */
	
	/* of course, pawns and kings are special... */
	if (piece == PAWN) /* pushes and enpassants */
	{
		/* can't make an attacking move unless it's a capture */
		moves &= board->piecesofcolor[OTHERCOLOR(board->tomove)];
		/* Trying to make an illegal promotion, or not promoting on an
		 * advance to the promotion rank */
		if ((!!(MOV_PROM(move))) ^
		    (!!(ROW(dest) == HOMEROW(OTHERCOLOR(board->tomove)))))
		{
			if (!!(MOV_PROM(move))) {
				*errmsg = "!!! Pawns don't promote like that.";
			} else {
				*errmsg = "!!! Promote into what? (N/B/R/Q)";
			}
			return 0;
		}
		/* move is a regular capture */
		if (BB_SQUARE(dest) & moves)
		{
			move |= 1 << MOV_INDEX_CAPT;
			move |= board_pieceatsquare(board, dest, NULL) << MOV_INDEX_CAPTPC;
			return move_checksuicide(board, move, errmsg);
		}
		/* enpassant */
		if (board->ep && dest == board->ep)
		{
			move |= (0x1 << MOV_INDEX_EP) |
			        (0x1 << MOV_INDEX_CAPT) |
			        (PAWN << MOV_INDEX_CAPTPC);
			return move_checksuicide(board, move, errmsg);
		}
		/* check for push */
		clearsquares = ~board->occupied;
		/* is pawn free to move one square ahead? we don't need to set
		 * any special additional flags here */
		if (pawnmoves[board->tomove][src] & clearsquares)
		{
			/* Are we trying to move 1 square ahead? Note: relies
			 * on the fact that pawnmoves[][] has one bit set. */
			if (BB_SQUARE(dest) == pawnmoves[board->tomove][src])
			{
				return move_checksuicide(board, move, errmsg);
			}
			/* 2-square push from 2nd rank */
			if (((board->tomove == WHITE) &&
			     (pawnmoves[board->tomove][src+8] & clearsquares) &&
			     (BB_SQUARE(dest) == pawnmoves[board->tomove][src+8])) ||
			    ((board->tomove == BLACK) &&
			     (pawnmoves[board->tomove][src-8] & clearsquares) &&
			     (BB_SQUARE(dest) == pawnmoves[board->tomove][src-8])))
			{
				return move_checksuicide(board, move, errmsg);
			}
		}
		*errmsg = "!!! Pawns can't move like that.";
		return 0;
	}
	/* not a pawn, trying to promote a different piece */
	else if (MOV_PROM(move))
	{
		*errmsg = "!!! You can't promote non-pawns!";
		return 0;
	}
	/* note - for king moves, we don't need to apply the move to check if
	 * it puts the player in check; we use squareisattacked() directly */
	else if (piece == KING)
	{
		/* handle castling - queenside */
		if (board->castle[board->tomove][QUEENSIDE] &&
		    (src == SQUARE(COL_E,HOMEROW(board->tomove))) &&
		    (dest == SQUARE(COL_C,HOMEROW(board->tomove)))) /* "e1c1" or "e8c8" */
		{
			/* check if special squares are blocked or attacked */
			if (castle_clearsquares[board->tomove][QUEENSIDE] & board->occupied) {
				*errmsg = "!!! Castling squares are blocked.";
				return 0;
			} else if (board_squaresareattacked(board, castle_safesquares[board->tomove][QUEENSIDE], OTHERCOLOR(board->tomove))) {
				*errmsg = "!!! Can't castle through check.";
				return 0;
			}
			/* here, the castle is legal */
			move |= 0x1 << MOV_INDEX_CASTLE;
			return move;
		}
		/* kingside */
		if (board->castle[board->tomove][KINGSIDE] &&
		    (src == SQUARE(COL_E,HOMEROW(board->tomove))) &&
		    (dest == SQUARE(COL_G,HOMEROW(board->tomove)))) /* "e1g1" or "e8g8" */
		{
			/* check if special squares are blocked or attacked */
			if (castle_clearsquares[board->tomove][KINGSIDE] & board->occupied) {
				*errmsg = "!!! Castling squares are blocked.";
				return 0;
			} else if (board_squaresareattacked(board, castle_safesquares[board->tomove][KINGSIDE], OTHERCOLOR(board->tomove))) {
				*errmsg = "!!! Can't castle through check.";
				return 0;
			}
			/* here, the castle is legal */
			move |= 0x1 << MOV_INDEX_CASTLE;
			return move;
		}
		/* otherwise it must be a normal king move to be legal */
		/* we can make this move */
		if (BB_SQUARE(dest) & moves)
		{
			/* making a capture */
			if (BB_SQUARE(dest) & board->piecesofcolor[OTHERCOLOR(board->tomove)])
			{
				move |= 1 << MOV_INDEX_CAPT;
				move |= board_pieceatsquare(board, dest, NULL) << MOV_INDEX_CAPTPC;
			}
			if (board_squareisattacked(board, dest, OTHERCOLOR(board->tomove)))
			{
				*errmsg = "!!! Can't move king into check.";
				return 0;
			}
			return move;
		}
		*errmsg = "!!! Kings can't move like that.";
		return 0;
	}
	/* typical case piece - N B R Q */
	else
	{
		/* is this one of our legal moves? */
		if (BB_SQUARE(dest) & moves)
		{
			/* making a capture? */
			if (BB_SQUARE(dest) & board->piecesofcolor[OTHERCOLOR(board->tomove)])
			{
				move |= 1 << MOV_INDEX_CAPT;
				move |= board_pieceatsquare(board, dest, NULL) << MOV_INDEX_CAPTPC;
			}
			return move_checksuicide(board, move, errmsg);
		}
		*errmsg =
			piece == QUEEN  ? "!!! Queens can't move like that." :
			piece == ROOK   ? "!!! Rooks can't move like that." :
			piece == BISHOP ? "!!! Bishops can't move like that." :
			                  "!!! Knights can't move like that.";
		return 0;
	}
}

/**
 * Given a move on a given position (must be legal and suitably formatted for
 * applymove), check if making this move would leave the player in check.
 * Returns the move if it's good, 0 if not.
 */
static move_t move_checksuicide(board_t *board, move_t move, char **errmsg)
{
	board_applymove(board, move);
	if (board_colorincheck(board, OTHERCOLOR(board->tomove)))
	{
		board_undomove(board, move);
		*errmsg = "!!! That would put you in check.";
		return 0;
	}
	board_undomove(board, move);
	return move;
}

/**
 * Returns the string representation of a move in computer-friendly format
 * (that is, "squarename[src]squarename[dest]promotionpiece"). Free the string
 * when you're done with it.
 */
char *move_tostring(move_t m)
{
	char *string;
	/* longest move in this format is a promotion, "a7a8Q" */
	string = malloc(6);
	string[0] = 0;
	strcat(string, squarename[MOV_SRC(m)]);
	strcat(string, squarename[MOV_DEST(m)]);
	if (MOV_PROM(m))
	{
		/* use WHITE as an index to get the uppercase */
		strcat(string, piecename[WHITE][MOV_PROMPC(m)]);
	}
	return string;
}
/****************************************************************************
 * book.c - simple interface for retrieving moves from a plaintext book
 * copyright (C) 2008 Ben Blum
 * ****************************************************************************/

#define BOOK_MOVE_LENGTH 4

// Must be sorted.
const char *book[] = {
	"c2c4 c7c5 g1f3 b8c6",
	"c2c4 c7c5 g1f3 g8f6 b1c3 b8c6",
	"c2c4 c7c5 g1f3 g8f6 b1c3 e7e6 g2g3 b7b6 f1g2 c8b7 e1g1 f8e7",
	"c2c4 c7c5 g1f3 g8f6 g2g3",
	"c2c4 c7c6",
	"c2c4 e7e5 b1c3 b8c6",
	"c2c4 e7e5 b1c3 g8f6 g1f3 b8c6 g2g3",
	"c2c4 e7e5 g2g3",
	"c2c4 e7e6 b1c3 d7d5 d2d4 c7c6",
	"c2c4 e7e6 b1c3 d7d5 d2d4 f8e7 g1f3 g8f6 c1f4 e8g8 e2e3",
	"c2c4 e7e6 b1c3 d7d5 d2d4 f8e7 g1f3 g8f6 c1g5 e8g8 e2e3 h7h6",
	"c2c4 e7e6 b1c3 d7d5 d2d4 f8e7 g1f3 g8f6 c1g5 h7h6 g5h4 e8g8 e2e3 b7b6",
	"c2c4 e7e6 b1c3 d7d5 d2d4 g8f6 c1g5 f8e7 e2e3",
	"c2c4 e7e6 b1c3 d7d5 d2d4 g8f6 c4d5 e6d5 c1g5",
	"c2c4 e7e6 b1c3 d7d5 d2d4 g8f6 g1f3 b8d7",
	"c2c4 e7e6 b1c3 d7d5 d2d4 g8f6 g1f3 c7c5",
	"c2c4 e7e6 b1c3 d7d5 d2d4 g8f6 g1f3 c7c6 c1g5",
	"c2c4 e7e6 b1c3 d7d5 d2d4 g8f6 g1f3 c7c6 e2e3 b8d7 d1c2 f8d6",
	"c2c4 e7e6 b1c3 d7d5 d2d4 g8f6 g1f3 c7c6 e2e3 b8d7 f1d3 d5c4 d3c4 b7b5 c4d3",
	"c2c4 e7e6 b1c3 d7d5 d2d4 g8f6 g1f3 f8b4",
	"c2c4 e7e6 b1c3 d7d5 d2d4 g8f6 g1f3 f8e7 c1f4 e8g8 e2e3",
	"c2c4 e7e6 b1c3 d7d5 d2d4 g8f6 g1f3 f8e7 c1g5 e8g8 e2e3 h7h6",
	"c2c4 e7e6 b1c3 d7d5 d2d4 g8f6 g1f3 f8e7 c1g5 h7h6 g5h4 e8g8 e2e3 b7b6",
	"c2c4 e7e6 g1f3",
	"c2c4 g8f6 b1c3 c7c5",
	"c2c4 g8f6 b1c3 e7e5 g1f3 b8c6 g2g3",
	"c2c4 g8f6 b1c3 e7e6 g1f3 d7d5 d2d4 b8d7",
	"c2c4 g8f6 b1c3 e7e6 g1f3 d7d5 d2d4 c7c5",
	"c2c4 g8f6 b1c3 e7e6 g1f3 d7d5 d2d4 c7c6 c1g5",
	"c2c4 g8f6 b1c3 e7e6 g1f3 d7d5 d2d4 c7c6 e2e3 b8d7 d1c2 f8d6",
	"c2c4 g8f6 b1c3 e7e6 g1f3 d7d5 d2d4 c7c6 e2e3 b8d7 f1d3 d5c4 d3c4 b7b5 c4d3",
	"c2c4 g8f6 b1c3 e7e6 g1f3 d7d5 d2d4 f8b4",
	"c2c4 g8f6 b1c3 e7e6 g1f3 d7d5 d2d4 f8e7 c1f4 e8g8 e2e3",
	"c2c4 g8f6 b1c3 e7e6 g1f3 d7d5 d2d4 f8e7 c1g5 e8g8 e2e3 h7h6",
	"c2c4 g8f6 b1c3 e7e6 g1f3 d7d5 d2d4 f8e7 c1g5 h7h6 g5h4 e8g8 e2e3 b7b6",
	"c2c4 g8f6 b1c3 g7g6",
	"c2c4 g8f6 g1f3 b7b6 g2g3",
	"c2c4 g8f6 g1f3 c7c5 b1c3 b8c6",
	"c2c4 g8f6 g1f3 c7c5 b1c3 e7e6 g2g3 b7b6 f1g2 c8b7 e1g1 f8e7",
	"c2c4 g8f6 g1f3 c7c5 g2g3",
	"c2c4 g8f6 g1f3 e7e6 b1c3 d7d5 d2d4 b8d7",
	"c2c4 g8f6 g1f3 e7e6 b1c3 d7d5 d2d4 c7c5",
	"c2c4 g8f6 g1f3 e7e6 b1c3 d7d5 d2d4 c7c6 c1g5",
	"c2c4 g8f6 g1f3 e7e6 b1c3 d7d5 d2d4 c7c6 e2e3 b8d7 d1c2 f8d6",
	"c2c4 g8f6 g1f3 e7e6 b1c3 d7d5 d2d4 c7c6 e2e3 b8d7 f1d3 d5c4 d3c4 b7b5 c4d3",
	"c2c4 g8f6 g1f3 e7e6 b1c3 d7d5 d2d4 f8b4",
	"c2c4 g8f6 g1f3 e7e6 b1c3 d7d5 d2d4 f8e7 c1f4 e8g8 e2e3",
	"c2c4 g8f6 g1f3 e7e6 b1c3 d7d5 d2d4 f8e7 c1g5 e8g8 e2e3 h7h6",
	"c2c4 g8f6 g1f3 e7e6 b1c3 d7d5 d2d4 f8e7 c1g5 h7h6 g5h4 e8g8 e2e3 b7b6",
	"c2c4 g8f6 g1f3 e7e6 g2g3 d7d5 f1g2 f8e7",
	"c2c4 g8f6 g1f3 g7g6 b1c3 f8g7 e2e4",
	"c2c4 g8f6 g1f3 g7g6 g2g3 f8g7 f1g2 e8g8",
	"d2d4 d7d5 c2c4 c7c6 b1c3 g8f6 e2e3",
	"d2d4 d7d5 c2c4 c7c6 b1c3 g8f6 g1f3 d5c4 a2a4 c8f5 e2e3 e7e6 f1c4",
	"d2d4 d7d5 c2c4 c7c6 b1c3 g8f6 g1f3 e7e6 c1g5",
	"d2d4 d7d5 c2c4 c7c6 b1c3 g8f6 g1f3 e7e6 e2e3 b8d7 d1c2 f8d6",
	"d2d4 d7d5 c2c4 c7c6 b1c3 g8f6 g1f3 e7e6 e2e3 b8d7 f1d3 d5c4 d3c4 b7b5 c4d3",
	"d2d4 d7d5 c2c4 c7c6 g1f3 g8f6 b1c3 d5c4 a2a4 c8f5 e2e3 e7e6 f1c4",
	"d2d4 d7d5 c2c4 c7c6 g1f3 g8f6 b1c3 e7e6 c1g5",
	"d2d4 d7d5 c2c4 c7c6 g1f3 g8f6 b1c3 e7e6 e2e3 b8d7 d1c2 f8d6",
	"d2d4 d7d5 c2c4 c7c6 g1f3 g8f6 b1c3 e7e6 e2e3 b8d7 f1d3 d5c4 d3c4 b7b5 c4d3",
	"d2d4 d7d5 c2c4 c7c6 g1f3 g8f6 e2e3",
	"d2d4 d7d5 c2c4 d5c4 g1f3 e7e6 e2e3 g8f6",
	"d2d4 d7d5 c2c4 d5c4 g1f3 g8f6 e2e3 e7e6 f1c4 c7c5 e1g1 a7a6",
	"d2d4 d7d5 c2c4 e7e6 b1c3 c7c6",
	"d2d4 d7d5 c2c4 e7e6 b1c3 f8e7 g1f3 g8f6 c1f4 e8g8 e2e3",
	"d2d4 d7d5 c2c4 e7e6 b1c3 f8e7 g1f3 g8f6 c1g5 e8g8 e2e3 h7h6",
	"d2d4 d7d5 c2c4 e7e6 b1c3 f8e7 g1f3 g8f6 c1g5 h7h6 g5h4 e8g8 e2e3 b7b6",
	"d2d4 d7d5 c2c4 e7e6 b1c3 g8f6 c1g5 f8e7 e2e3",
	"d2d4 d7d5 c2c4 e7e6 b1c3 g8f6 c4d5 e6d5 c1g5",
	"d2d4 d7d5 c2c4 e7e6 b1c3 g8f6 g1f3 b8d7",
	"d2d4 d7d5 c2c4 e7e6 b1c3 g8f6 g1f3 c7c5",
	"d2d4 d7d5 c2c4 e7e6 b1c3 g8f6 g1f3 c7c6 c1g5",
	"d2d4 d7d5 c2c4 e7e6 b1c3 g8f6 g1f3 c7c6 e2e3 b8d7 d1c2 f8d6",
	"d2d4 d7d5 c2c4 e7e6 b1c3 g8f6 g1f3 c7c6 e2e3 b8d7 f1d3 d5c4 d3c4 b7b5 c4d3",
	"d2d4 d7d5 c2c4 e7e6 b1c3 g8f6 g1f3 f8b4",
	"d2d4 d7d5 c2c4 e7e6 b1c3 g8f6 g1f3 f8e7 c1f4 e8g8 e2e3",
	"d2d4 d7d5 c2c4 e7e6 b1c3 g8f6 g1f3 f8e7 c1g5 e8g8 e2e3 h7h6",
	"d2d4 d7d5 c2c4 e7e6 b1c3 g8f6 g1f3 f8e7 c1g5 h7h6 g5h4 e8g8 e2e3 b7b6",
	"d2d4 d7d5 c2c4 e7e6 g1f3 g8f6 b1c3 b8d7",
	"d2d4 d7d5 c2c4 e7e6 g1f3 g8f6 b1c3 c7c5",
	"d2d4 d7d5 c2c4 e7e6 g1f3 g8f6 b1c3 c7c6 c1g5",
	"d2d4 d7d5 c2c4 e7e6 g1f3 g8f6 b1c3 c7c6 e2e3 b8d7 d1c2 f8d6",
	"d2d4 d7d5 c2c4 e7e6 g1f3 g8f6 b1c3 c7c6 e2e3 b8d7 f1d3 d5c4 d3c4 b7b5 c4d3",
	"d2d4 d7d5 c2c4 e7e6 g1f3 g8f6 b1c3 f8b4",
	"d2d4 d7d5 c2c4 e7e6 g1f3 g8f6 b1c3 f8e7 c1f4 e8g8 e2e3",
	"d2d4 d7d5 c2c4 e7e6 g1f3 g8f6 b1c3 f8e7 c1g5 e8g8 e2e3 h7h6",
	"d2d4 d7d5 c2c4 e7e6 g1f3 g8f6 b1c3 f8e7 c1g5 h7h6 g5h4 e8g8 e2e3 b7b6",
	"d2d4 d7d5 c2c4 e7e6 g1f3 g8f6 c1g5",
	"d2d4 d7d5 c2c4 e7e6 g1f3 g8f6 g2g3",
	"d2d4 d7d5 g1f3 e7e6 c2c4 g8f6 b1c3 b8d7",
	"d2d4 d7d5 g1f3 e7e6 c2c4 g8f6 b1c3 c7c5",
	"d2d4 d7d5 g1f3 e7e6 c2c4 g8f6 b1c3 c7c6 c1g5",
	"d2d4 d7d5 g1f3 e7e6 c2c4 g8f6 b1c3 c7c6 e2e3 b8d7 d1c2 f8d6",
	"d2d4 d7d5 g1f3 e7e6 c2c4 g8f6 b1c3 c7c6 e2e3 b8d7 f1d3 d5c4 d3c4 b7b5 c4d3",
	"d2d4 d7d5 g1f3 e7e6 c2c4 g8f6 b1c3 f8b4",
	"d2d4 d7d5 g1f3 e7e6 c2c4 g8f6 b1c3 f8e7 c1f4 e8g8 e2e3",
	"d2d4 d7d5 g1f3 e7e6 c2c4 g8f6 b1c3 f8e7 c1g5 e8g8 e2e3 h7h6",
	"d2d4 d7d5 g1f3 e7e6 c2c4 g8f6 b1c3 f8e7 c1g5 h7h6 g5h4 e8g8 e2e3 b7b6",
	"d2d4 d7d5 g1f3 e7e6 c2c4 g8f6 c1g5",
	"d2d4 d7d5 g1f3 e7e6 c2c4 g8f6 g2g3",
	"d2d4 d7d5 g1f3 g8f6 c2c4 c7c6 b1c3 d5c4 a2a4 c8f5 e2e3 e7e6 f1c4",
	"d2d4 d7d5 g1f3 g8f6 c2c4 c7c6 b1c3 e7e6 c1g5",
	"d2d4 d7d5 g1f3 g8f6 c2c4 c7c6 b1c3 e7e6 e2e3 b8d7 d1c2 f8d6",
	"d2d4 d7d5 g1f3 g8f6 c2c4 c7c6 b1c3 e7e6 e2e3 b8d7 f1d3 d5c4 d3c4 b7b5 c4d3",
	"d2d4 d7d5 g1f3 g8f6 c2c4 c7c6 e2e3",
	"d2d4 d7d5 g1f3 g8f6 c2c4 d5c4 e2e3 e7e6 f1c4 c7c5 e1g1 a7a6",
	"d2d4 d7d5 g1f3 g8f6 c2c4 e7e6 b1c3 b8d7",
	"d2d4 d7d5 g1f3 g8f6 c2c4 e7e6 b1c3 c7c5",
	"d2d4 d7d5 g1f3 g8f6 c2c4 e7e6 b1c3 c7c6 c1g5",
	"d2d4 d7d5 g1f3 g8f6 c2c4 e7e6 b1c3 c7c6 e2e3 b8d7 d1c2 f8d6",
	"d2d4 d7d5 g1f3 g8f6 c2c4 e7e6 b1c3 c7c6 e2e3 b8d7 f1d3 d5c4 d3c4 b7b5 c4d3",
	"d2d4 d7d5 g1f3 g8f6 c2c4 e7e6 b1c3 f8b4",
	"d2d4 d7d5 g1f3 g8f6 c2c4 e7e6 b1c3 f8e7 c1f4 e8g8 e2e3",
	"d2d4 d7d5 g1f3 g8f6 c2c4 e7e6 b1c3 f8e7 c1g5 e8g8 e2e3 h7h6",
	"d2d4 d7d5 g1f3 g8f6 c2c4 e7e6 b1c3 f8e7 c1g5 h7h6 g5h4 e8g8 e2e3 b7b6",
	"d2d4 d7d5 g1f3 g8f6 c2c4 e7e6 c1g5",
	"d2d4 d7d5 g1f3 g8f6 c2c4 e7e6 g2g3",
	"d2d4 d7d6 e2e4 g7g6",
	"d2d4 d7d6 e2e4 g8f6 b1c3 g7g6 f2f4 f8g7 g1f3",
	"d2d4 e7e6 c2c4 b7b6 g1f3",
	"d2d4 e7e6 c2c4 g8f6 b1c3 d7d5 c1g5 f8e7 e2e3",
	"d2d4 e7e6 c2c4 g8f6 b1c3 d7d5 c4d5 e6d5 c1g5",
	"d2d4 e7e6 c2c4 g8f6 b1c3 d7d5 g1f3 b8d7",
	"d2d4 e7e6 c2c4 g8f6 b1c3 d7d5 g1f3 c7c5",
	"d2d4 e7e6 c2c4 g8f6 b1c3 d7d5 g1f3 c7c6 c1g5",
	"d2d4 e7e6 c2c4 g8f6 b1c3 d7d5 g1f3 c7c6 e2e3 b8d7 d1c2 f8d6",
	"d2d4 e7e6 c2c4 g8f6 b1c3 d7d5 g1f3 c7c6 e2e3 b8d7 f1d3 d5c4 d3c4 b7b5 c4d3",
	"d2d4 e7e6 c2c4 g8f6 b1c3 d7d5 g1f3 f8b4",
	"d2d4 e7e6 c2c4 g8f6 b1c3 d7d5 g1f3 f8e7 c1f4 e8g8 e2e3",
	"d2d4 e7e6 c2c4 g8f6 b1c3 d7d5 g1f3 f8e7 c1g5 e8g8 e2e3 h7h6",
	"d2d4 e7e6 c2c4 g8f6 b1c3 d7d5 g1f3 f8e7 c1g5 h7h6 g5h4 e8g8 e2e3 b7b6",
	"d2d4 e7e6 c2c4 g8f6 b1c3 f8b4 d1c2 e8g8",
	"d2d4 e7e6 c2c4 g8f6 b1c3 f8b4 e2e3 b7b6",
	"d2d4 e7e6 c2c4 g8f6 b1c3 f8b4 e2e3 c7c5 f1d3",
	"d2d4 e7e6 c2c4 g8f6 b1c3 f8b4 e2e3 e8g8 f1d3 d7d5 g1f3",
	"d2d4 e7e6 c2c4 g8f6 b1c3 f8b4 g1f3",
	"d2d4 e7e6 c2c4 g8f6 c1g5 f8b4",
	"d2d4 e7e6 c2c4 g8f6 c1g5 h7h6",
	"d2d4 e7e6 c2c4 g8f6 g1f3 b7b6 a2a3 c8b7 b1c3 d7d5 c4d5 f6d5",
	"d2d4 e7e6 c2c4 g8f6 g1f3 b7b6 b1c3 c8b7 a2a3 d7d5 c4d5 f6d5",
	"d2d4 e7e6 c2c4 g8f6 g1f3 b7b6 b1c3 f8b4",
	"d2d4 e7e6 c2c4 g8f6 g1f3 b7b6 g2g3 c8a6 b2b3 f8b4 c1d2 b4e7",
	"d2d4 e7e6 c2c4 g8f6 g1f3 b7b6 g2g3 c8b7 f1g2 f8e7 e1g1 e8g8 b1c3 f6e4 d1c2 e4c3 c2c3",
	"d2d4 e7e6 c2c4 g8f6 g1f3 c7c5 d4d5 e6d5 c4d5 d7d6 b1c3 g7g6",
	"d2d4 e7e6 c2c4 g8f6 g1f3 d7d5 b1c3 b8d7",
	"d2d4 e7e6 c2c4 g8f6 g1f3 d7d5 b1c3 c7c5",
	"d2d4 e7e6 c2c4 g8f6 g1f3 d7d5 b1c3 c7c6 c1g5",
	"d2d4 e7e6 c2c4 g8f6 g1f3 d7d5 b1c3 c7c6 e2e3 b8d7 d1c2 f8d6",
	"d2d4 e7e6 c2c4 g8f6 g1f3 d7d5 b1c3 c7c6 e2e3 b8d7 f1d3 d5c4 d3c4 b7b5 c4d3",
	"d2d4 e7e6 c2c4 g8f6 g1f3 d7d5 b1c3 f8b4",
	"d2d4 e7e6 c2c4 g8f6 g1f3 d7d5 b1c3 f8e7 c1f4 e8g8 e2e3",
	"d2d4 e7e6 c2c4 g8f6 g1f3 d7d5 b1c3 f8e7 c1g5 e8g8 e2e3 h7h6",
	"d2d4 e7e6 c2c4 g8f6 g1f3 d7d5 b1c3 f8e7 c1g5 h7h6 g5h4 e8g8 e2e3 b7b6",
	"d2d4 e7e6 c2c4 g8f6 g1f3 d7d5 c1g5",
	"d2d4 e7e6 c2c4 g8f6 g1f3 d7d5 g2g3",
	"d2d4 e7e6 c2c4 g8f6 g1f3 f8b4 b1d2",
	"d2d4 e7e6 c2c4 g8f6 g1f3 f8b4 c1d2",
	"d2d4 e7e6 c2c4 g8f6 g2g3 d7d5 f1g2",
	"d2d4 e7e6 e2e4 d7d5 b1c3 f8b4 e4e5 c7c5 a2a3 b4c3",
	"d2d4 e7e6 e2e4 d7d5 b1d2 c7c5 g1f3",
	"d2d4 e7e6 e2e4 d7d5 b1d2 g8f6 e4e5 f6d7 f1d3 c7c5 c2c3 b8c6 g1e2 ",
	"d2d4 g8f6 c1g5 f6e4 g5f4 c7c5 f2f3 d8a5 c2c3 e4f6 d4d5 a5b6",
	"d2d4 g8f6 c2c4 c7c5 d4d5 b7b5 c4b5 a7a6",
	"d2d4 g8f6 c2c4 c7c5 d4d5 e7e6 b1c3 e6d5 c4d5 d7d6",
	"d2d4 g8f6 c2c4 d7d6 b1c3",
	"d2d4 g8f6 c2c4 e7e6 b1c3 d7d5 c1g5 f8e7 e2e3",
	"d2d4 g8f6 c2c4 e7e6 b1c3 d7d5 c4d5 e6d5 c1g5",
	"d2d4 g8f6 c2c4 e7e6 b1c3 d7d5 g1f3 b8d7",
	"d2d4 g8f6 c2c4 e7e6 b1c3 d7d5 g1f3 c7c5",
	"d2d4 g8f6 c2c4 e7e6 b1c3 d7d5 g1f3 c7c6 c1g5",
	"d2d4 g8f6 c2c4 e7e6 b1c3 d7d5 g1f3 c7c6 e2e3 b8d7 d1c2 f8d6",
	"d2d4 g8f6 c2c4 e7e6 b1c3 d7d5 g1f3 c7c6 e2e3 b8d7 f1d3 d5c4 d3c4 b7b5 c4d3",
	"d2d4 g8f6 c2c4 e7e6 b1c3 d7d5 g1f3 f8b4",
	"d2d4 g8f6 c2c4 e7e6 b1c3 d7d5 g1f3 f8e7 c1f4 e8g8 e2e3",
	"d2d4 g8f6 c2c4 e7e6 b1c3 d7d5 g1f3 f8e7 c1g5 e8g8 e2e3 h7h6",
	"d2d4 g8f6 c2c4 e7e6 b1c3 d7d5 g1f3 f8e7 c1g5 h7h6 g5h4 e8g8 e2e3 b7b6",
	"d2d4 g8f6 c2c4 e7e6 b1c3 f8b4 d1c2 e8g8",
	"d2d4 g8f6 c2c4 e7e6 b1c3 f8b4 e2e3 b7b6",
	"d2d4 g8f6 c2c4 e7e6 b1c3 f8b4 e2e3 c7c5 f1d3",
	"d2d4 g8f6 c2c4 e7e6 b1c3 f8b4 e2e3 e8g8 f1d3 d7d5 g1f3",
	"d2d4 g8f6 c2c4 e7e6 b1c3 f8b4 g1f3",
	"d2d4 g8f6 c2c4 e7e6 g1f3 b7b6 a2a3 c8b7 b1c3 d7d5 c4d5 f6d5",
	"d2d4 g8f6 c2c4 e7e6 g1f3 b7b6 b1c3 c8b7 a2a3 d7d5 c4d5 f6d5",
	"d2d4 g8f6 c2c4 e7e6 g1f3 b7b6 b1c3 f8b4",
	"d2d4 g8f6 c2c4 e7e6 g1f3 b7b6 g2g3 c8a6 b2b3 f8b4 c1d2 b4e7",
	"d2d4 g8f6 c2c4 e7e6 g1f3 b7b6 g2g3 c8b7 f1g2 f8e7 e1g1 e8g8 b1c3 f6e4 d1c2 e4c3 c2c3",
	"d2d4 g8f6 c2c4 e7e6 g1f3 c7c5 d4d5 e6d5 c4d5 d7d6 b1c3 g7g6",
	"d2d4 g8f6 c2c4 e7e6 g1f3 d7d5 b1c3 b8d7",
	"d2d4 g8f6 c2c4 e7e6 g1f3 d7d5 b1c3 c7c5",
	"d2d4 g8f6 c2c4 e7e6 g1f3 d7d5 b1c3 c7c6 c1g5",
	"d2d4 g8f6 c2c4 e7e6 g1f3 d7d5 b1c3 c7c6 e2e3 b8d7 d1c2 f8d6",
	"d2d4 g8f6 c2c4 e7e6 g1f3 d7d5 b1c3 c7c6 e2e3 b8d7 f1d3 d5c4 d3c4 b7b5 c4d3",
	"d2d4 g8f6 c2c4 e7e6 g1f3 d7d5 b1c3 f8b4",
	"d2d4 g8f6 c2c4 e7e6 g1f3 d7d5 b1c3 f8e7 c1f4 e8g8 e2e3",
	"d2d4 g8f6 c2c4 e7e6 g1f3 d7d5 b1c3 f8e7 c1g5 e8g8 e2e3 h7h6",
	"d2d4 g8f6 c2c4 e7e6 g1f3 d7d5 b1c3 f8e7 c1g5 h7h6 g5h4 e8g8 e2e3 b7b6",
	"d2d4 g8f6 c2c4 e7e6 g1f3 d7d5 c1g5",
	"d2d4 g8f6 c2c4 e7e6 g1f3 d7d5 g2g3",
	"d2d4 g8f6 c2c4 e7e6 g1f3 f8b4 b1d2",
	"d2d4 g8f6 c2c4 e7e6 g1f3 f8b4 c1d2",
	"d2d4 g8f6 c2c4 e7e6 g2g3 d7d5 f1g2",
	"d2d4 g8f6 c2c4 g7g6 b1c3 d7d5 c4d5 f6d5 e2e4 d5c3 b2c3 f8g7 f1c4",
	"d2d4 g8f6 c2c4 g7g6 b1c3 d7d5 g1f3 f8g7 d1b3 d5c4 b3c4",
	"d2d4 g8f6 c2c4 g7g6 b1c3 f8g7 e2e4 d7d6 f1e2 e8g8 c1g5",
	"d2d4 g8f6 c2c4 g7g6 b1c3 f8g7 e2e4 d7d6 f1e2 e8g8 g1f3 e7e5 e1g1 b8c6 d4d5 c6e7 f3e1 f6d7",
	"d2d4 g8f6 c2c4 g7g6 b1c3 f8g7 e2e4 d7d6 f2f3 e8g8 c1e3",
	"d2d4 g8f6 c2c4 g7g6 b1c3 f8g7 e2e4 d7d6 g1f3 e8g8 f1e2 e7e5 e1g1 b8c6 d4d5 c6e7 f3e1 f6d7",
	"d2d4 g8f6 c2c4 g7g6 g1f3 f8g7 b1c3 e8g8 e2e4 d7d6 f1e2 e7e5 e1g1 b8c6 d4d5 c6e7 f3e1 f6d7",
	"d2d4 g8f6 c2c4 g7g6 g1f3 f8g7 g2g3 e8g8 f1g2 d7d6 e1g1",
	"d2d4 g8f6 c2c4 g7g6 g2g3 f8g7 f1g2 e8g8",
	"d2d4 g8f6 g1f3 c7c5",
	"d2d4 g8f6 g1f3 d7d5 c2c4 c7c6 b1c3 d5c4 a2a4 c8f5 e2e3 e7e6 f1c4",
	"d2d4 g8f6 g1f3 d7d5 c2c4 c7c6 b1c3 e7e6 c1g5",
	"d2d4 g8f6 g1f3 d7d5 c2c4 c7c6 b1c3 e7e6 e2e3 b8d7 d1c2 f8d6",
	"d2d4 g8f6 g1f3 d7d5 c2c4 c7c6 b1c3 e7e6 e2e3 b8d7 f1d3 d5c4 d3c4 b7b5 c4d3",
	"d2d4 g8f6 g1f3 d7d5 c2c4 c7c6 e2e3",
	"d2d4 g8f6 g1f3 d7d5 c2c4 d5c4 e2e3 e7e6 f1c4 c7c5 e1g1 a7a6",
	"d2d4 g8f6 g1f3 d7d5 c2c4 e7e6 b1c3 b8d7",
	"d2d4 g8f6 g1f3 d7d5 c2c4 e7e6 b1c3 c7c5",
	"d2d4 g8f6 g1f3 d7d5 c2c4 e7e6 b1c3 c7c6 c1g5",
	"d2d4 g8f6 g1f3 d7d5 c2c4 e7e6 b1c3 c7c6 e2e3 b8d7 d1c2 f8d6",
	"d2d4 g8f6 g1f3 d7d5 c2c4 e7e6 b1c3 c7c6 e2e3 b8d7 f1d3 d5c4 d3c4 b7b5 c4d3",
	"d2d4 g8f6 g1f3 d7d5 c2c4 e7e6 b1c3 f8b4",
	"d2d4 g8f6 g1f3 d7d5 c2c4 e7e6 b1c3 f8e7 c1f4 e8g8 e2e3",
	"d2d4 g8f6 g1f3 d7d5 c2c4 e7e6 b1c3 f8e7 c1g5 e8g8 e2e3 h7h6",
	"d2d4 g8f6 g1f3 d7d5 c2c4 e7e6 b1c3 f8e7 c1g5 h7h6 g5h4 e8g8 e2e3 b7b6",
	"d2d4 g8f6 g1f3 d7d5 c2c4 e7e6 c1g5",
	"d2d4 g8f6 g1f3 d7d5 c2c4 e7e6 g2g3",
	"d2d4 g8f6 g1f3 e7e6 c1g5",
	"d2d4 g8f6 g1f3 e7e6 c2c4 b7b6 a2a3 c8b7 b1c3 d7d5 c4d5 f6d5",
	"d2d4 g8f6 g1f3 e7e6 c2c4 b7b6 b1c3 c8b7 a2a3 d7d5 c4d5 f6d5",
	"d2d4 g8f6 g1f3 e7e6 c2c4 b7b6 b1c3 f8b4",
	"d2d4 g8f6 g1f3 e7e6 c2c4 b7b6 g2g3 c8a6 b2b3 f8b4 c1d2 b4e7",
	"d2d4 g8f6 g1f3 e7e6 c2c4 b7b6 g2g3 c8b7 f1g2 f8e7 e1g1 e8g8 b1c3 f6e4 d1c2 e4c3 c2c3",
	"d2d4 g8f6 g1f3 e7e6 c2c4 c7c5 d4d5 e6d5 c4d5 d7d6 b1c3 g7g6",
	"d2d4 g8f6 g1f3 e7e6 c2c4 d7d5 b1c3 b8d7",
	"d2d4 g8f6 g1f3 e7e6 c2c4 d7d5 b1c3 c7c5",
	"d2d4 g8f6 g1f3 e7e6 c2c4 d7d5 b1c3 c7c6 c1g5",
	"d2d4 g8f6 g1f3 e7e6 c2c4 d7d5 b1c3 c7c6 e2e3 b8d7 d1c2 f8d6",
	"d2d4 g8f6 g1f3 e7e6 c2c4 d7d5 b1c3 c7c6 e2e3 b8d7 f1d3 d5c4 d3c4 b7b5 c4d3",
	"d2d4 g8f6 g1f3 e7e6 c2c4 d7d5 b1c3 f8b4",
	"d2d4 g8f6 g1f3 e7e6 c2c4 d7d5 b1c3 f8e7 c1f4 e8g8 e2e3",
	"d2d4 g8f6 g1f3 e7e6 c2c4 d7d5 b1c3 f8e7 c1g5 e8g8 e2e3 h7h6",
	"d2d4 g8f6 g1f3 e7e6 c2c4 d7d5 b1c3 f8e7 c1g5 h7h6 g5h4 e8g8 e2e3 b7b6",
	"d2d4 g8f6 g1f3 e7e6 c2c4 d7d5 c1g5",
	"d2d4 g8f6 g1f3 e7e6 c2c4 d7d5 g2g3",
	"d2d4 g8f6 g1f3 e7e6 c2c4 f8b4 b1d2",
	"d2d4 g8f6 g1f3 e7e6 c2c4 f8b4 c1d2",
	"d2d4 g8f6 g1f3 e7e6 g2g3",
	"d2d4 g8f6 g1f3 g7g6 c1g5",
	"d2d4 g8f6 g1f3 g7g6 c2c4 f8g7 b1c3 e8g8 e2e4 d7d6 f1e2 e7e5 e1g1 b8c6 d4d5 c6e7 f3e1 f6d7",
	"d2d4 g8f6 g1f3 g7g6 c2c4 f8g7 g2g3 e8g8 f1g2 d7d6 e1g1",
	"d2d4 g8f6 g1f3 g7g6 g2g3 f8g7 f1g2 e8g8",
	"e2e4 c7c5 b1c3 b8c6 g2g3 g7g6 f1g2 f8g7",
	"e2e4 c7c5 c2c3 d7d6 g1f3 g8f6",
	"e2e4 c7c5 c2c3 g8f6 e4e5 f6d5 d2d4 c5d4 g1f3 ",
	"e2e4 c7c5 c2c3 g8f6 e4e5 f6d5 g1f3",
	"e2e4 c7c5 d2d3 g8f6",
	"e2e4 c7c5 g1f3 b8c6 d2d4 c5d4 f3d4 e7e6 b1c3 a7a6",
	"e2e4 c7c5 g1f3 b8c6 d2d4 c5d4 f3d4 e7e6 b1c3 d8c7",
	"e2e4 c7c5 g1f3 b8c6 d2d4 c5d4 f3d4 g7g6 b1c3 f8g7 c1e3 g8f6 f1c4",
	"e2e4 c7c5 g1f3 b8c6 d2d4 c5d4 f3d4 g7g6 c2c4 f8g7 f1d3 g8f6 b1c3 ",
	"e2e4 c7c5 g1f3 b8c6 d2d4 c5d4 f3d4 g8f6 b1c3 d7d6 c1g5 e7e6 d1d2 a7a6 e1c1 h7h6",
	"e2e4 c7c5 g1f3 b8c6 d2d4 c5d4 f3d4 g8f6 b1c3 d7d6 c1g5 e7e6 d1d2 f8e7 e1c1 e8g8",
	"e2e4 c7c5 g1f3 b8c6 d2d4 c5d4 f3d4 g8f6 b1c3 d7d6 f1c4",
	"e2e4 c7c5 g1f3 b8c6 d2d4 c5d4 f3d4 g8f6 b1c3 e7e5 d4b5 d7d6",
	"e2e4 c7c5 g1f3 b8c6 f1b5",
	"e2e4 c7c5 g1f3 d7d6 d2d4 c5d4 f3d4 g8f6 b1c3 a7a6 c1e3",
	"e2e4 c7c5 g1f3 d7d6 d2d4 c5d4 f3d4 g8f6 b1c3 a7a6 c1g5 e7e6 f2f4",
	"e2e4 c7c5 g1f3 d7d6 d2d4 c5d4 f3d4 g8f6 b1c3 a7a6 f1e2 e7e5 d4b3 f8e7",
	"e2e4 c7c5 g1f3 d7d6 d2d4 c5d4 f3d4 g8f6 b1c3 a7a6 f2f4",
	"e2e4 c7c5 g1f3 d7d6 d2d4 c5d4 f3d4 g8f6 b1c3 b8c6 c1g5 e7e6 d1d2 a7a6 e1c1 h7h6",
	"e2e4 c7c5 g1f3 d7d6 d2d4 c5d4 f3d4 g8f6 b1c3 b8c6 c1g5 e7e6 d1d2 f8e7 e1c1 e8g8",
	"e2e4 c7c5 g1f3 d7d6 d2d4 c5d4 f3d4 g8f6 b1c3 b8c6 f1c4",
	"e2e4 c7c5 g1f3 d7d6 d2d4 c5d4 f3d4 g8f6 b1c3 e7e6 f1e2",
	"e2e4 c7c5 g1f3 d7d6 d2d4 c5d4 f3d4 g8f6 b1c3 e7e6 g2g4",
	"e2e4 c7c5 g1f3 d7d6 d2d4 c5d4 f3d4 g8f6 b1c3 g7g6 c1e3 f8g7 f2f3",
	"e2e4 c7c5 g1f3 d7d6 f1b5 b8d7",
	"e2e4 c7c5 g1f3 d7d6 f1b5 c8d7 b5d7",
	"e2e4 c7c5 g1f3 e7e6 b1c3",
	"e2e4 c7c5 g1f3 e7e6 d2d4 c5d4 f3d4 a7a6 f1d3",
	"e2e4 c7c5 g1f3 e7e6 d2d4 c5d4 f3d4 b8c6 b1c3 a7a6",
	"e2e4 c7c5 g1f3 e7e6 d2d4 c5d4 f3d4 b8c6 b1c3 d8c7",
	"e2e4 c7c5 g1f3 e7e6 d2d4 c5d4 f3d4 g8f6 b1c3 d7d6 f1e2",
	"e2e4 c7c5 g1f3 e7e6 d2d4 c5d4 f3d4 g8f6 b1c3 d7d6 g2g4",
	"e2e4 c7c6 c2c4 d7d5",
	"e2e4 c7c6 d2d4 d7d5 b1c3 d5e4 c3e4 b8d7",
	"e2e4 c7c6 d2d4 d7d5 b1c3 d5e4 c3e4 c8f5 e4g3 f5g6 h2h4 h7h6",
	"e2e4 c7c6 d2d4 d7d5 b1d2 d5e4 d2e4 b8d7",
	"e2e4 c7c6 d2d4 d7d5 b1d2 d5e4 d2e4 c8f5 e4g3 f5g6 h2h4 h7h6",
	"e2e4 c7c6 d2d4 d7d5 e4d5 c6d5 c2c4 g8f6 b1c3 e7e6 g1f3",
	"e2e4 c7c6 d2d4 d7d5 e4e5 c8f5",
	"e2e4 c7c6 b1c3 d7d5 d2d4 d5e4 c3e4 c8f5 e4g3 f5g6 h2h4 h7h6",
	"e2e4 c7c6 b1c3 d7d5 g1f3 c8g4 h2h3 g4f3 d1f3 e7e6",
	"e2e4 c7c6 b1c3 d7d5 g1f3 d5e4 c3e4 g8f6",
	"e2e4 c7c6 g1f3 d7d5 b1c3",
	"e2e4 d7d6 d2d4 g7g6",
	"e2e4 d7d6 d2d4 g8f6 b1c3 g7g6 f2f4 f8g7 g1f3",
	"e2e4 e7e5 b1c3 g8f6 g1f3 b8c6",
	"e2e4 e7e5 g1f3 b8c6 b1c3 g8f6 f1b5",
	"e2e4 e7e5 g1f3 b8c6 d2d4 e5d4 f3d4",
	"e2e4 e7e5 g1f3 b8c6 f1b5 a7a6 b5a4 d7d6",
	"e2e4 e7e5 g1f3 b8c6 f1b5 a7a6 b5a4 g8f6 e1g1 f6e4 d2d4 b7b5 a4b3 d7d5 d4e5 c8e6 c2c3",
	"e2e4 e7e5 g1f3 b8c6 f1b5 a7a6 b5a4 g8f6 e1g1 f8e7 f1e1 b7b5 a4b3 d7d6 c2c3 e8g8 h2h3 c6a5 b3c2 c7c5 d2d4 d8c7 b1d2",
	"e2e4 e7e5 g1f3 b8c6 f1b5 a7a6 b5a4 g8f6 e1g1 f8e7 f1e1 b7b5 a4b3 d7d6 c2c3 e8g8 h2h3 c6b8 d2d4 b8d7",
	"e2e4 e7e5 g1f3 b8c6 f1b5 a7a6 b5a4 g8f6 e1g1 f8e7 f1e1 b7b5 a4b3 d7d6 c2c3 e8g8 h2h3 c8b7 d2d4 f8e8",
	"e2e4 e7e5 g1f3 b8c6 f1b5 a7a6 b5a4 g8f6 e1g1 f8e7 f1e1 b7b5 a4b3 e8g8 c2c3 d7d6 h2h3 c6a5 b3c2 c7c5 d2d4 d8c7 b1d2",
	"e2e4 e7e5 g1f3 b8c6 f1b5 a7a6 b5a4 g8f6 e1g1 f8e7 f1e1 b7b5 a4b3 e8g8 c2c3 d7d6 h2h3 c6b8 d2d4 b8d7",
	"e2e4 e7e5 g1f3 b8c6 f1b5 a7a6 b5a4 g8f6 e1g1 f8e7 f1e1 b7b5 a4b3 e8g8 c2c3 d7d6 h2h3 c8b7 d2d4 f8e8",
	"e2e4 e7e5 g1f3 b8c6 f1b5 a7a6 b5c6 d7c6 e1g1",
	"e2e4 e7e5 g1f3 b8c6 f1b5 g8f6 e1g1",
	"e2e4 e7e5 g1f3 b8c6 f1c4 f8c5",
	"e2e4 e7e5 g1f3 b8c6 f1c4 g8f6",
	"e2e4 e7e5 g1f3 g8f6 f3e5 d7d6 e5f3 f6e4 d2d4",
	"e2e4 e7e6 d2d4 d7d5 b1c3 f8b4 e4e5 c7c5 a2a3 b4c3 b2c3 g8e7",
	"e2e4 e7e6 d2d4 d7d5 b1c3 g8f6 c1g5",
	"e2e4 e7e6 d2d4 d7d5 b1d2 c7c5 e4d5 e6d5",
	"e2e4 e7e6 d2d4 d7d5 b1d2 c7c5 g1f3",
	"e2e4 e7e6 d2d4 d7d5 b1d2 g8f6 e4e5",
	"e2e4 e7e6 d2d4 d7d5 e4e5 c7c5 c2c3 b8c6 g1f3 d8b6 a2a3 ",
	"e2e4 e7e6 d2d4 d7d5 e4e5 c7c5 c2c3 d8b6 g1f3 b8c6 a2a3",
	"e2e4 g7g6 d2d4 f8g7 b1c3 c7c6 g1f3",
	"e2e4 g7g6 d2d4 f8g7 b1c3 d7d6 c1e3",
	"e2e4 g7g6 d2d4 f8g7 b1c3 d7d6 g1f3",
	"e2e4 g8f6 e4e5 f6d5 c2c4 d5b6 d2d4 d7d6 e5d6 e7d6",
	"e2e4 g8f6 e4e5 f6d5 d2d4 d7d6 g1f3",
	"g1f3 c7c5 c2c4 b8c6",
	"g1f3 c7c5 c2c4 g8f6 b1c3 b8c6",
	"g1f3 c7c5 c2c4 g8f6 b1c3 e7e6 g2g3 b7b6 f1g2 c8b7 e1g1 f8e7",
	"g1f3 c7c5 c2c4 g8f6 g2g3",
	"g1f3 d7d5 d2d4 g8f6 c2c4 c7c6 b1c3 d5c4 a2a4 c8f5 e2e3 e7e6 f1c4",
	"g1f3 d7d5 d2d4 g8f6 c2c4 c7c6 b1c3 e7e6 c1g5",
	"g1f3 d7d5 d2d4 g8f6 c2c4 c7c6 b1c3 e7e6 e2e3 b8d7 d1c2 f8d6",
	"g1f3 d7d5 d2d4 g8f6 c2c4 c7c6 b1c3 e7e6 e2e3 b8d7 f1d3 d5c4 d3c4 b7b5 c4d3",
	"g1f3 d7d5 d2d4 g8f6 c2c4 c7c6 e2e3",
	"g1f3 d7d5 d2d4 g8f6 c2c4 d5c4 e2e3 e7e6 f1c4 c7c5 e1g1 a7a6",
	"g1f3 d7d5 d2d4 g8f6 c2c4 e7e6 b1c3 b8d7",
	"g1f3 d7d5 d2d4 g8f6 c2c4 e7e6 b1c3 c7c5",
	"g1f3 d7d5 d2d4 g8f6 c2c4 e7e6 b1c3 c7c6 c1g5",
	"g1f3 d7d5 d2d4 g8f6 c2c4 e7e6 b1c3 c7c6 e2e3 b8d7 d1c2 f8d6",
	"g1f3 d7d5 d2d4 g8f6 c2c4 e7e6 b1c3 c7c6 e2e3 b8d7 f1d3 d5c4 d3c4 b7b5 c4d3",
	"g1f3 d7d5 d2d4 g8f6 c2c4 e7e6 b1c3 f8b4",
	"g1f3 d7d5 d2d4 g8f6 c2c4 e7e6 b1c3 f8e7 c1f4 e8g8 e2e3",
	"g1f3 d7d5 d2d4 g8f6 c2c4 e7e6 b1c3 f8e7 c1g5 e8g8 e2e3 h7h6",
	"g1f3 d7d5 d2d4 g8f6 c2c4 e7e6 b1c3 f8e7 c1g5 h7h6 g5h4 e8g8 e2e3 b7b6",
	"g1f3 d7d5 d2d4 g8f6 c2c4 e7e6 c1g5",
	"g1f3 d7d5 d2d4 g8f6 c2c4 e7e6 g2g3",
	"g1f3 d7d5 g2g3",
	"g1f3 g8f6 c2c4 b7b6 g2g3",
	"g1f3 g8f6 c2c4 c7c5 b1c3 b8c6",
	"g1f3 g8f6 c2c4 c7c5 b1c3 e7e6 g2g3 b7b6 f1g2 c8b7 e1g1 f8e7",
	"g1f3 g8f6 c2c4 c7c5 g2g3",
	"g1f3 g8f6 c2c4 e7e6 b1c3 d7d5 d2d4 b8d7",
	"g1f3 g8f6 c2c4 e7e6 b1c3 d7d5 d2d4 c7c5",
	"g1f3 g8f6 c2c4 e7e6 b1c3 d7d5 d2d4 c7c6 c1g5",
	"g1f3 g8f6 c2c4 e7e6 b1c3 d7d5 d2d4 c7c6 e2e3 b8d7 d1c2 f8d6",
	"g1f3 g8f6 c2c4 e7e6 b1c3 d7d5 d2d4 c7c6 e2e3 b8d7 f1d3 d5c4 d3c4 b7b5 c4d3",
	"g1f3 g8f6 c2c4 e7e6 b1c3 d7d5 d2d4 f8b4",
	"g1f3 g8f6 c2c4 e7e6 b1c3 d7d5 d2d4 f8e7 c1f4 e8g8 e2e3",
	"g1f3 g8f6 c2c4 e7e6 b1c3 d7d5 d2d4 f8e7 c1g5 e8g8 e2e3 h7h6",
	"g1f3 g8f6 c2c4 e7e6 b1c3 d7d5 d2d4 f8e7 c1g5 h7h6 g5h4 e8g8 e2e3 b7b6",
	"g1f3 g8f6 c2c4 e7e6 g2g3 d7d5 f1g2 f8e7",
	"g1f3 g8f6 c2c4 g7g6 b1c3 f8g7 e2e4",
	"g1f3 g8f6 c2c4 g7g6 g2g3 f8g7 f1g2 e8g8",
	"g1f3 g8f6 d2d4 c7c5",
	"g1f3 g8f6 d2d4 d7d5 c2c4 c7c6 b1c3 d5c4 a2a4 c8f5 e2e3 e7e6 f1c4",
	"g1f3 g8f6 d2d4 d7d5 c2c4 c7c6 b1c3 e7e6 c1g5",
	"g1f3 g8f6 d2d4 d7d5 c2c4 c7c6 b1c3 e7e6 e2e3 b8d7 d1c2 f8d6",
	"g1f3 g8f6 d2d4 d7d5 c2c4 c7c6 b1c3 e7e6 e2e3 b8d7 f1d3 d5c4 d3c4 b7b5 c4d3",
	"g1f3 g8f6 d2d4 d7d5 c2c4 c7c6 e2e3",
	"g1f3 g8f6 d2d4 d7d5 c2c4 d5c4 e2e3 e7e6 f1c4 c7c5 e1g1 a7a6",
	"g1f3 g8f6 d2d4 d7d5 c2c4 e7e6 b1c3 b8d7",
	"g1f3 g8f6 d2d4 d7d5 c2c4 e7e6 b1c3 c7c5",
	"g1f3 g8f6 d2d4 d7d5 c2c4 e7e6 b1c3 c7c6 c1g5",
	"g1f3 g8f6 d2d4 d7d5 c2c4 e7e6 b1c3 c7c6 e2e3 b8d7 d1c2 f8d6",
	"g1f3 g8f6 d2d4 d7d5 c2c4 e7e6 b1c3 c7c6 e2e3 b8d7 f1d3 d5c4 d3c4 b7b5 c4d3",
	"g1f3 g8f6 d2d4 d7d5 c2c4 e7e6 b1c3 f8b4",
	"g1f3 g8f6 d2d4 d7d5 c2c4 e7e6 b1c3 f8e7 c1f4 e8g8 e2e3",
	"g1f3 g8f6 d2d4 d7d5 c2c4 e7e6 b1c3 f8e7 c1g5 e8g8 e2e3 h7h6",
	"g1f3 g8f6 d2d4 d7d5 c2c4 e7e6 b1c3 f8e7 c1g5 h7h6 g5h4 e8g8 e2e3 b7b6",
	"g1f3 g8f6 d2d4 d7d5 c2c4 e7e6 c1g5",
	"g1f3 g8f6 d2d4 d7d5 c2c4 e7e6 g2g3",
	"g1f3 g8f6 d2d4 e7e6 c1g5",
	"g1f3 g8f6 d2d4 e7e6 c2c4 b7b6 a2a3 c8b7 b1c3 d7d5 c4d5 f6d5",
	"g1f3 g8f6 d2d4 e7e6 c2c4 b7b6 b1c3 c8b7 a2a3 d7d5 c4d5 f6d5",
	"g1f3 g8f6 d2d4 e7e6 c2c4 b7b6 b1c3 f8b4",
	"g1f3 g8f6 d2d4 e7e6 c2c4 b7b6 g2g3 c8a6 b2b3 f8b4 c1d2 b4e7",
	"g1f3 g8f6 d2d4 e7e6 c2c4 b7b6 g2g3 c8b7 f1g2 f8e7 e1g1 e8g8 b1c3 f6e4 d1c2 e4c3 c2c3",
	"g1f3 g8f6 d2d4 e7e6 c2c4 c7c5 d4d5 e6d5 c4d5 d7d6 b1c3 g7g6",
	"g1f3 g8f6 d2d4 e7e6 c2c4 d7d5 b1c3 b8d7",
	"g1f3 g8f6 d2d4 e7e6 c2c4 d7d5 b1c3 c7c5",
	"g1f3 g8f6 d2d4 e7e6 c2c4 d7d5 b1c3 c7c6 c1g5",
	"g1f3 g8f6 d2d4 e7e6 c2c4 d7d5 b1c3 c7c6 e2e3 b8d7 d1c2 f8d6",
	"g1f3 g8f6 d2d4 e7e6 c2c4 d7d5 b1c3 c7c6 e2e3 b8d7 f1d3 d5c4 d3c4 b7b5 c4d3",
	"g1f3 g8f6 d2d4 e7e6 c2c4 d7d5 b1c3 f8b4",
	"g1f3 g8f6 d2d4 e7e6 c2c4 d7d5 b1c3 f8e7 c1f4 e8g8 e2e3",
	"g1f3 g8f6 d2d4 e7e6 c2c4 d7d5 b1c3 f8e7 c1g5 e8g8 e2e3 h7h6",
	"g1f3 g8f6 d2d4 e7e6 c2c4 d7d5 b1c3 f8e7 c1g5 h7h6 g5h4 e8g8 e2e3 b7b6",
	"g1f3 g8f6 d2d4 e7e6 c2c4 d7d5 c1g5",
	"g1f3 g8f6 d2d4 e7e6 c2c4 d7d5 g2g3",
	"g1f3 g8f6 d2d4 e7e6 c2c4 f8b4 b1d2",
	"g1f3 g8f6 d2d4 e7e6 c2c4 f8b4 c1d2",
	"g1f3 g8f6 d2d4 e7e6 g2g3",
	"g1f3 g8f6 d2d4 g7g6 c1g5",
	"g1f3 g8f6 d2d4 g7g6 c2c4 f8g7 b1c3 e8g8 e2e4 d7d6 f1e2 e7e5 e1g1 b8c6 d4d5 c6e7 f3e1 f6d7",
	"g1f3 g8f6 d2d4 g7g6 c2c4 f8g7 g2g3 e8g8 f1g2 d7d6 e1g1",
	"g1f3 g8f6 d2d4 g7g6 g2g3 f8g7 f1g2 e8g8",
	"g1f3 g8f6 g2g3 g7g6",
};

// Returns bool. If true, then book[start] to book[end-1] inclusive will
// match line. So end-start is the number of matching lines.
int book_range(char *line, int *startp, int *endp)
{
	int start, end;
	if (ARRAY_SIZE(book) == 0) return 0; // no book??
	/* Find start. */
	for (start = 0; start < ARRAY_SIZE(book); start++) {
		int result = strncmp(book[start], line, strlen(line));
		if (result < 0)
			continue; // not yet
		else if (result > 0)
			break; // line not in book at all
		// OK, found start.
		for (end = start + 1; end < ARRAY_SIZE(book); end++) {
			int result = strncmp(book[end], line, strlen(line));
			if (result > 0)
				break;
			else
				assert(result == 0 && "opening book not sorted :(");
		}
		// Found end.
		*startp = start;
		*endp = end;
		return 1;
	}
	return 0; // line not found
}

int book_enabled = 1;

/**
 * Get a move from the book. Line is a string like "e2e4 e7e5 g1f3 " - note
 * the space at the end! and the board is for passing to move_islegal.
 */
move_t book_move(char *line, board_t *board)
{
	int start, end;
	if (!book_enabled) return 0;
	if (book_range(line, &start, &end)) {
		char book_move_str[BOOK_MOVE_LENGTH+1];
		move_t book_move;
		unsigned long move_index;
		static int rand_inited = 0;
		char *errmsg;
		if (!rand_inited) {
			sgenrand((unsigned long)get_ticks());
			rand_inited = 1;
		}
		assert(end - start > 0);
		move_index = (genrand() % (end-start)) + start;
		strncpy(book_move_str, &book[move_index][strlen(line)],
			BOOK_MOVE_LENGTH);
		book_move_str[BOOK_MOVE_LENGTH] = 0;
		book_move = move_islegal(board, book_move_str, &errmsg);
		if (!book_move) {
			set_cursor_pos(0,0);
			printf("BUG: please email bblum@cs.cmu.edu the following: \n");
			printf("Line: \"%s\"\n", line);
			printf("Attempted book line: \"%s\"\n", book[move_index]);
			printf("Attempted move: \"%s\"\n", book_move_str);
			assert(0 && "Illegal line in opening book?");
		}
		return book_move;
	} else {
		return 0;
	}
}
/****************************************************************************
 * engine.c - wrapper functions for managing computer vs human play
 * copyright (C) 2008 Ben Blum
 ****************************************************************************/

#define ENGINE_REPEATED_BUCKETS 127

/* from xboard.c, for printing */
// void output(char *);

/**
 * Initialize a game engine given the searching function, with a new board
 */
engine_t *engine_init(search_fn search)
{
	engine_t *e = malloc(sizeof(engine_t));
	e->board = board_init();
	e->search = search;
	e->line[0] = '\0';
	e->line_moves = 0;
	e->inbook = 1;
	return e;
}

#define SEARCHTIME_DEFAULT 5
#define SEARCHTIME_MAX 60
unsigned int searchtime = SEARCHTIME_DEFAULT;

extern int outbuf_size;
extern int text_loud;
extern char outbuf[];
void output();

/**
 * Have the engine generate a move for the side to play. String must be freed.
 */
char *engine_generatemove(engine_t *e)
{
	if (e->inbook)
	{
		move_t move = book_move(e->line, e->board);
		if (move)
		{
			/* book lookup was successful */
			// output("ENGINE: Book lookup successful");
			char *str = move_tostring(move);
			if (text_loud) {
				snprintf(outbuf, outbuf_size,
					 "My move: %s (from opening book)", str);
				output();
			}
			return str;
		}
		else
		{
			/* we've left the book lines */
			// output("ENGINE: Leaving opening book lines");
			e->inbook = 0;
		}
	}
	/* we get here if already out of book or if we just left book */
	return move_tostring(e->search(e->board, searchtime, NULL, NULL));
}

/**
 * Change the board to reflect a move being made. Returns 1 if the move is
 * illegal, 0 if it's good
 */
int engine_applymove(engine_t *e, char *str, char **errmsg)
{
	/* we need to make the move before adding, else we won't detect draws
	 * before they actually are called */
	move_t move = move_islegal(e->board, str, errmsg);
	if (move == 0) {
		return 0;
	}
	/* keep track of the game's line, but only if not too long */
	if (strlen(e->line) < BOOK_LINE_MAX_LENGTH-6) {
		strcat(e->line, str);
		strcat(e->line, " ");
		assert(e->line_moves == e->board->moves);
		e->line_moves++;
	}

	board_applymove(e->board, move);
	
	return 1;
}

void engine_undomove(engine_t *e)
{
	assert(e->board->moves > 0);
	if (e->line_moves == e->board->moves) {
		int i = strlen(e->line)-1;
		assert(i > 0);
		assert(e->line[i] == ' ');
		if (e->line_moves == 1) {
			e->line[0] = 0;
		} else {
			for (i--; i > 0 && e->line[i] != ' '; i--) { continue; }
			assert(i > 0);
			e->line[i+1] = 0;
		}
		e->line_moves--;
	}
	e->inbook = 1; // Not necessarily true. But it can't hurt.
	board_undomove(e->board, e->board->history[e->board->moves-1].move);
}

void engine_destroy(engine_t *e)
{
	if (e == NULL)
	{
		return;
	}
	board_destroy(e->board);
	free(e);
	return;
}
/****************************************************************************
 * eval.c - evaluation function to tell how good a position is
 * copyright (C) 2008 Ben Blum
 ****************************************************************************/

/* endgame starts when both sides have <= LIM_ENDGAME */
#define EVAL_LIM_ENDGAME  1600
/* use this limit when no queens on the board */
#define EVAL_LIM_ENDGAME2 2000

#define EVAL_CASTLE_BONUS 20

int16_t eval_piecevalue[6] = { 100, 300, 300, 500, 900, 0 };
int16_t eval_piecevalue_endgame[6] = { 125, 300, 300, 550, 1200, 0 };

/* Players get small point bonuses if their pieces are on good squares. If
 * white has a rook on d1, squarevalue[WHITE][ROOK][D1] will be added to
 * white's score. */
/* Differences between src/dest squares are considered when ordering moves, so
 * it is important that for any legal (src,dest) pair, abs(sqval(dest) -
 * sqval(src)) <= SOME_CONSTANT - that constant is 16 for now. (only in the
 * regular tables, not the endgame tables) */
int16_t eval_squarevalue[2][6][64] = {
	/********************************************************************
	 * WHITE
	 ********************************************************************/
	{
	/* PAWN */
	{
		 0,  0,  0,  0,  0,  0,  0,  0,
		 0,  0,  0, -4, -4,  4,  0,  0,
		 6,  2,  3,  4,  4,  3,  2,  8,
		 3,  4, 12, 12, 12,  8,  4,  3,
		 5,  8, 16, 20, 20, 16,  8,  5,
		20, 24, 24, 32, 32, 24, 24, 20,
		36, 36, 40, 40, 40, 40, 36, 36,
		 0,  0,  0,  0,  0,  0,  0,  0
	},
	/* KNIGHT */
	{
		-10, -6, -6, -6, -6, -6, -6,-10,
		 -6,  0,  0,  3,  3,  0,  0, -6,
		 -6,  0,  8,  4,  4, 10,  0, -6,
		 -6,  0,  8, 10, 10,  8,  0, -6,
		 -4,  0,  8, 10, 10,  8,  0, -4,
		 -4,  5, 12, 12, 12, 12,  5, -4,
		 -4,  0,  5,  3,  3,  5,  0, -4,
		-10, -4, -4, -4, -4, -4, -4,-10
	},
	/* BISHOP */
	{
		-6, -5, -5, -5, -5, -5, -5, -6,
		-5, 10,  5,  8,  8,  5, 10, -5,
		-5,  5,  3,  5,  5,  3,  5, -5,
		-5,  3, 10,  3,  3, 10,  3, -5,
		-5,  5, 10,  3,  3, 10,  5, -5,
		-5,  3,  8,  8,  8,  8,  3, -5,
		-5,  5,  5,  8,  8,  5,  5, -5,
		-6, -5, -5, -5, -5, -5, -5, -6
	},
	/* ROOK */
	{
		0,  3,  3,  3,  3,  3,  3,  0,
		0,  1,  2,  3,  3,  2,  1,  0,
		0,  1,  2,  3,  3,  2,  1,  0,
		0,  1,  2,  3,  3,  2,  1,  0,
		0,  1,  2,  3,  3,  2,  1,  0,
		0,  1,  2,  2,  2,  2,  1,  0,
		3,  5,  8,  8,  8,  8,  5,  3,
		0,  0,  0,  0,  0,  0,  0,  0
	},
	/* QUEEN */
	{
		0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  5,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0
	},
	/* KING */
	{
		  5,  5,  8,  0,  0, -5, 10, 10,
		  0,  0,  0,  0,  0,  0,  5,  5,
		  0,  0,  0, -5, -5,  0,  0,  0,
		  0,  0, -5,-10,-10, -5,  0,  0,
		  0, -5,-10,-10,-10,-10, -5,  0,
		 -5,-10,-10,-15,-15,-10,-10, -5,
		-20,-20,-20,-20,-20,-20,-20,-20,
		-20,-20,-20,-20,-20,-20,-20,-20
	}
	},
	/********************************************************************
	 * BLACK
	 ********************************************************************/
	{
	/* PAWN */
	{
		 0,  0,  0,  0,  0,  0,  0,  0,
		36, 36, 40, 40, 40, 40, 36, 36,
		20, 24, 24, 32, 32, 24, 24, 20,
		 5,  8, 16, 20, 20, 16,  8,  5,
		 3,  4, 12, 12, 12,  8,  4,  3,
		 6,  2,  3,  4,  4,  3,  2,  8,
		 0,  0,  0, -4, -4,  4,  0,  0,
		 0,  0,  0,  0,  0,  0,  0,  0
	},
	/* KNIGHT */
	{
		-10, -4, -4, -4, -4, -4, -4,-10,
		 -4,  0,  5,  3,  3,  5,  0, -4,
		 -4,  5, 12, 12, 12, 12,  5, -4,
		 -4,  0,  8, 10, 10,  8,  0, -4,
		 -6,  0,  8, 10, 10,  8,  0, -6,
		 -6,  0,  8,  4,  4, 10,  0, -6,
		 -6,  0,  0,  3,  3,  0,  0, -6,
		-10, -6, -6, -6, -6, -6, -6,-10
	},
	/* BISHOP */
	{
		-6, -5, -5, -5, -5, -5, -5, -6,
		-5,  5,  5,  8,  8,  5,  5, -5,
		-5,  5,  8,  8,  8,  8,  5, -5,
		-5,  3, 10,  5,  5, 10,  3, -5,
		-5,  5, 10,  5,  5, 10,  5, -5,
		-5,  3,  3,  5,  5,  3,  3, -5,
		-5, 10,  5,  8,  8,  5, 10, -5,
		-6, -5, -5, -5, -5, -5, -5, -6
	},
	/* ROOK */
	{
		0,  0,  0,  0,  0,  0,  0,  0,
		3,  5,  8,  8,  8,  8,  5,  3,
		0,  1,  2,  2,  2,  2,  1,  0,
		0,  1,  2,  3,  3,  2,  1,  0,
		0,  1,  2,  3,  3,  2,  1,  0,
		0,  1,  2,  3,  3,  2,  1,  0,
		0,  1,  2,  3,  3,  2,  1,  0,
		0,  3,  3,  3,  3,  3,  3,  0
	},
	/* QUEEN */
	{
		0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  5,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0
	},
	/* KING */
	{
		-20,-20,-20,-20,-20,-20,-20,-20,
		-20,-20,-20,-20,-20,-20,-20,-20,
		 -5,-10,-10,-15,-15,-10,-10, -5,
		  0, -5,-10,-10,-10,-10, -5,  0,
		  0,  0, -5,-10,-10, -5,  0,  0,
		  0,  0,  0, -5, -5,  0,  0,  0,
		  0,  0,  0,  0,  0,  0,  5,  5,
		  5,  5,  8,  0,  0, -5, 10, 10
	}
	}
};
int16_t eval_squarevalue_endgame[2][6][64] = {
	/********************************************************************
	 * WHITE
	 ********************************************************************/
	{
	/* PAWN */
	{
		  0,  0,  0,  0,  0,  0,  0,  0,
		-10,-10,-10,-10,-10,-10,-10,-10,
		  0,  0,  0,  0,  0,  0,  0,  0,
		 10, 10, 10, 10, 10, 10, 10, 10,
		 20, 20, 20, 20, 20, 20, 20, 20,
		 40, 40, 40, 40, 40, 40, 40, 40,
		 80, 80, 80, 80, 80, 80, 80, 80,
		  0,  0,  0,  0,  0,  0,  0,  0
	},
	/* KNIGHT */
	{
		-10, -5, -5, -5, -5, -5, -5,-10,
		 -8,  0,  0,  3,  3,  0,  0, -8,
		 -8,  0, 10,  8,  8, 10,  0, -8,
		 -8,  0,  8, 10, 10,  8,  0, -8,
		 -8,  0,  8, 10, 10,  8,  0, -8,
		 -8,  0, 12, 12, 12, 12,  0, -8,
		 -8,  0,  9,  3,  3,  9,  0, -8,
		-10, -5, -5, -5, -5, -5, -5,-10
	},
	/* BISHOP */
	{
		-8, -5, -5, -5, -5, -5, -5, -8,
		-5,  3,  5,  5,  5,  5,  3, -5,
		-5,  5,  5,  8,  8,  5,  5, -5,
		-5,  5, 10, 10, 10, 10,  5, -5,
		-5,  5, 10, 10, 10, 10,  5, -5,
		-5,  3,  8,  8,  8,  8,  3, -5,
		-5,  3,  5,  8,  8,  5,  3, -5,
		-8, -5, -5, -5, -5, -5, -5, -8
	},
	/* ROOK */
	{
		0,  3,  3,  5,  5,  3,  3,  0,
		0,  1,  2,  3,  3,  2,  1,  0,
		0,  1,  2,  3,  3,  2,  1,  0,
		0,  1,  2,  3,  3,  2,  1,  0,
		0,  1,  2,  3,  3,  2,  1,  0,
		0,  1,  2,  2,  2,  2,  1,  0,
		1,  3,  5,  5,  5,  5,  3,  1,
		0,  0,  0,  0,  0,  0,  0,  0
	},
	/* QUEEN */
	{
		0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0
	},
	/* KING */
	{
		-25,-15,-10,-10,-10,-10,-15,-25,
		-15, -5,  0,  0,  0,  0, -5,-15,
		-10,  0,  5, 10, 10,  5,  0,-10,
		-10,  0, 10, 15, 15, 10,  0,-10,
		 -5,  5, 15, 20, 20, 15,  5, -5,
		  0, 10, 20, 20, 20, 15, 10,  0,
		-15,  0,  5,  5,  5,  5,  0,-15,
		-25,-15,-10,-10,-10,-10,-15,-25
	}
	},
	/********************************************************************
	 * BLACK
	 ********************************************************************/
	{
	/* PAWN */
	{
		  0,  0,  0,  0,  0,  0,  0,  0,
		 80, 80, 80, 80, 80, 80, 80, 80,
		 40, 40, 40, 40, 40, 40, 40, 40,
		 20, 20, 20, 20, 20, 20, 20, 20,
		 10, 10, 10, 10, 10, 10, 10, 10,
		  0,  0,  0,  0,  0,  0,  0,  0,
		-10,-10,-10,-10,-10,-10,-10,-10,
		  0,  0,  0,  0,  0,  0,  0,  0
	},
	/* KNIGHT */
	{
		-10, -5, -5, -5, -5, -5, -5,-10,
		 -8,  0,  9,  3,  3,  9,  0, -8,
		 -8,  0, 12, 12, 12, 12,  0, -8,
		 -8,  0,  8, 10, 10,  8,  0, -8,
		 -8,  0,  8, 10, 10,  8,  0, -8,
		 -8,  0, 10,  8,  8, 10,  0, -8,
		 -8,  0,  0,  3,  3,  0,  0, -8,
		-10, -5, -5, -5, -5, -5, -5,-10
	},
	/* BISHOP */
	{
		-8, -5, -5, -5, -5, -5, -5, -8,
		-5,  3,  5,  8,  8,  5,  3, -5,
		-5,  3,  8,  8,  8,  8,  3, -5,
		-5,  5, 10, 10, 10, 10,  5, -5,
		-5,  5, 10, 10, 10, 10,  5, -5,
		-5,  5,  5,  8,  8,  5,  5, -5,
		-5,  3,  5,  5,  5,  5,  3, -5,
		-8, -5, -5, -5, -5, -5, -5, -8
	},
	/* ROOK */
	{
		0,  0,  0,  0,  0,  0,  0,  0,
		1,  3,  5,  5,  5,  5,  3,  1,
		0,  1,  2,  2,  2,  2,  1,  0,
		0,  1,  2,  3,  3,  2,  1,  0,
		0,  1,  2,  3,  3,  2,  1,  0,
		0,  1,  2,  3,  3,  2,  1,  0,
		0,  1,  2,  3,  3,  2,  1,  0,
		0,  3,  3,  5,  5,  3,  3,  0
	},
	/* QUEEN */
	{
		0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0
	},
	/* KING */
	{
		-25,-15,-10,-10,-10,-10,-15,-25
		-15,  0,  5,  5,  5,  5,  0,-15,
		  0, 10, 20, 20, 20, 15, 10,  0,
		 -5,  5, 15, 20, 20, 15,  5, -5,
		-10,  0, 10, 15, 15, 10,  0,-10,
		-10,  0,  5, 10, 10,  5,  0,-10,
		-15, -5,  0,  0,  0,  0, -5,-15,
		-25,-15,-10,-10,-10,-10,-15,-25,
	}
	}
};

static int16_t eval_endgame(board_t *);

/* Penalty for trapping the d- e- pawns on 2nd rank, or c-pawn with the knight
 * if there's a pawn on d4 and no pawn on e4 */
#define EVAL_BLOCKED_PAWN -25
/* used for pawn-block checking N on c3 with pawns c2 d4 !e4 */
#define BB_C2D4   (BB_SQUARE(C2) | BB_SQUARE(D4))
#define BB_C2D4E4 (BB_C2D4 | BB_SQUARE(E4))
#define BB_C7D5   (BB_SQUARE(C7) | BB_SQUARE(D5))
#define BB_C7D5E5 (BB_C7D5 | BB_SQUARE(E5))
/* reward the bishop pair and penalize the knight pair */
#define EVAL_BISHOP_PAIR 20
#define EVAL_KNIGHT_PAIR -20
/* Rooks like to see towards the other end of the board */
#define EVAL_ROOK_OPENFILE 10
#define EVAL_ROOK_OPENFILE_MULTIPLIER 3
/* bonus for having multiple rooks on the 7th - take the number of heavies
 * on the 7th, leftshift this constant by that number */
#define EVAL_ROOK_RANK7_MULTIPLIER 12
/* bonus for having a minor piece on an "outpost" - a hole in the opp's pawn
 * structure protected by one of own pawns. */
#define EVAL_OUTPOST_BONUS 32

/**************
 * King safety
 **************/
/* Points for having N pawns on rank 2/3 in front of a castled king*/
static int16_t pawncover_rank2[4] = { -80, -40, 0,   5 };
static int16_t pawncover_rank3[4] = {   0,   5, 20, 40 };
/* penalty for having no pawns on king's file */
#define EVAL_KINGFILEOPEN  -35
/* penalty for having no pawns on file adjacent to king's file */
#define EVAL_ADJACENTFILEOPEN -15

/* tropism bonus from one square to another (one square has a piece of given
 * type on it, the other has the enemy king) */
#define EVAL_TROPISM_MAX 192
/* this lookup array gives shiftamounts for distance so we don't have to idiv
 *                              0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 */
static int tropism_rice[15] = { 0, 0, 0, 1, 1, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5};
/* this lookup array gives shiftamounts for scaling by piece value
 *                                   P  N  B  R  Q  K */
static int tropism_piecescale[6] = { 3, 2, 2, 1, 0, 0 };

static int16_t tropism(square_t a, square_t b, piece_t piece)
{
	int16_t rowdist, coldist;
	/* absolute values of the difference between the two squares */
	rowdist = ROW(a) - ROW(b);
	if (rowdist < 0) { rowdist = -rowdist; }
	coldist = COL(a) - COL(b);
	if (coldist < 0) { coldist = -coldist; }
	/* score inversely proportional to distance, also scaled by how
	 * heavy the piece is - queen right up against the opp king will
	 * get like 100 points */
	return (EVAL_TROPISM_MAX >> tropism_rice[rowdist + coldist]) >>
	       tropism_piecescale[piece];
}

int eval_isendgame(board_t *board)
{
	if (board->pos[WHITE][QUEEN] | board->pos[BLACK][QUEEN])
	{
		return (board->material[WHITE] <= EVAL_LIM_ENDGAME) ||
		       (board->material[BLACK] <= EVAL_LIM_ENDGAME);
	}
	return (board->material[WHITE] <= EVAL_LIM_ENDGAME2) ||
	       (board->material[BLACK] <= EVAL_LIM_ENDGAME2);
}

/**
 * Evaluate - return a score for the given position for who's to move
 */
int16_t eval(board_t *board)
{
	int piece, square;
	bitboard_t piecepos;
	/* keeps track of {WHITE,BLACK}s score */
	int16_t score_white, score_black;
	/* king safety scores - scaled by the material on the board to
	 * encourage trading when under attack */
	int16_t ksafety_white, ksafety_black;
	square_t kingsq_white, kingsq_black; /* where's each king */
	/* counting pieces/pawns for king safety */
	int num_pieces;
	/* holes in <color>'s pawnstructure */
	bitboard_t holes_white, holes_black;

	/* use the special endgame evaluator if in the endgame */
	if (eval_isendgame(board))
	{
		return eval_endgame(board);
	}

	/* begin evaluating */
	score_white = 0; score_black = 0;
	ksafety_white = 0; ksafety_black = 0;
	kingsq_white = BITSCAN(board->pos[WHITE][KING]);
	kingsq_black = BITSCAN(board->pos[BLACK][KING]);
	
	/********************************************************************
	 * Piece/square tables
	 ********************************************************************/
	/********
	 * White
	 ********/
	/* regular pieces - pawns taken care of in pawnstructure eval */
	for (piece = 1; piece < 5; piece++)
	{
		piecepos = board->pos[WHITE][piece];
		while (piecepos)
		{
			square = BITSCAN(piecepos);
			piecepos ^= BB_SQUARE(square);
			score_white += eval_squarevalue[WHITE][piece][square];
			/* tropism scores - how close is to our/opp king? */
			//ksafety_white += tropism(square, kingsq_white, piece);
			ksafety_black -= tropism(square, kingsq_black, piece);
			/* Rooks like open files */
			if (piece == ROOK)
			{
				/* no pawns on this file */
				if (!(board->pos[WHITE][PAWN] & BB_FILE(COL(square))))
				{
					score_white += EVAL_ROOK_OPENFILE;
				}
				/* how far can we see? */
				score_white += EVAL_ROOK_OPENFILE_MULTIPLIER * POPCOUNT(colattacks[(board->occupied90 >> (8 * COL(square))) & 0xFF][ROW(square)]);
			}
		}
	}
	/* king */
	score_white += eval_squarevalue[WHITE][KING][kingsq_white];
	
	/********
	 * Black
	 ********/
	for (piece = 1; piece < 5; piece++)
	{
		piecepos = board->pos[BLACK][piece];
		while (piecepos)
		{
			square = BITSCAN(piecepos);
			piecepos ^= BB_SQUARE(square);
			score_black += eval_squarevalue[BLACK][piece][square];
			/* tropism scores - how close is to our/opp king? */
			ksafety_white -= tropism(square, kingsq_white, piece);
			//ksafety_black += tropism(square, kingsq_black, piece);
			/* Rooks like open files */
			if (piece == ROOK)
			{
				/* no pawns on this file */
				if (!(board->pos[BLACK][PAWN] & BB_FILE(COL(square))))
				{
					score_black += EVAL_ROOK_OPENFILE;
				}
				/* how far can we see? */
				score_black += EVAL_ROOK_OPENFILE_MULTIPLIER * POPCOUNT(colattacks[(board->occupied90 >> (8 * COL(square))) & 0xFF][ROW(square)]);
			}
		}
	}
	/* king */
	score_black += eval_squarevalue[BLACK][KING][kingsq_white];
	
	/********************************************************************
	 * Misc. bonuses
	 ********************************************************************/
	/* bishop/knight pair bonus/penalties */
	if (POPCOUNT(board->pos[WHITE][BISHOP]) > 1) { score_white += EVAL_BISHOP_PAIR; }
	if (POPCOUNT(board->pos[WHITE][KNIGHT]) > 1) { score_white += EVAL_KNIGHT_PAIR; }
	if (POPCOUNT(board->pos[BLACK][BISHOP]) > 1) { score_black += EVAL_BISHOP_PAIR; }
	if (POPCOUNT(board->pos[BLACK][KNIGHT]) > 1) { score_black += EVAL_KNIGHT_PAIR; }
	/* pawnstructure bonus */
	score_white += eval_pawnstructure(board, WHITE, &holes_white);
	score_black += eval_pawnstructure(board, BLACK, &holes_black);
	/* analysis of pawnstructure holes - all holes that our pawns attack
	 * and have a minor piece on them get an "outpost" bonus - don't allow
	 * bonus for outposts on the A and H files */
	/* find white's outposts */
	holes_black &= board_pawnattacks(board->pos[WHITE][PAWN], WHITE) &
	               (board->pos[WHITE][KNIGHT] | board->pos[WHITE][BISHOP]) &
	               ~(BB_FILEA | BB_FILEH);
	score_white += EVAL_OUTPOST_BONUS * POPCOUNT(holes_black);
	/* find black's outposts */
	holes_white &= board_pawnattacks(board->pos[BLACK][PAWN], BLACK) &
	               (board->pos[BLACK][KNIGHT] | board->pos[BLACK][BISHOP]) &
	               ~(BB_FILEA | BB_FILEH);
	score_black += EVAL_OUTPOST_BONUS * POPCOUNT(holes_white);
	/* pawn block penalties */
	if (((board->piecesofcolor[WHITE] ^ board->pos[WHITE][PAWN]) & BB_SQUARE(D3)) &&
	    (board->pos[WHITE][PAWN] & BB_SQUARE(D2))) /* blocked on D2 */
	{
		score_white += EVAL_BLOCKED_PAWN;
	}
	if (((board->piecesofcolor[WHITE] ^ board->pos[WHITE][PAWN]) & BB_SQUARE(E3)) &&
	    (board->pos[WHITE][PAWN] & BB_SQUARE(E2))) /* blocked on E2 */
	{
		score_white += EVAL_BLOCKED_PAWN;
	}
	if ((board->pos[WHITE][KNIGHT] & BB_SQUARE(C3)) && /* knight on C3 */
	    !((board->pos[WHITE][PAWN] ^ BB_C2D4) & BB_C2D4E4)) /* "closed" opening */
	{
		score_white += EVAL_BLOCKED_PAWN;
	}
	/* black */
	if (((board->piecesofcolor[BLACK] ^ board->pos[BLACK][PAWN]) & BB_SQUARE(D6)) &&
	    (board->pos[BLACK][PAWN] & BB_SQUARE(D7))) /* blocked on D7 */
	{
		score_black += EVAL_BLOCKED_PAWN;
	}
	if (((board->piecesofcolor[BLACK] ^ board->pos[BLACK][PAWN]) & BB_SQUARE(E6)) &&
	    (board->pos[BLACK][PAWN] & BB_SQUARE(E7))) /* blocked on E7 */
	{
		score_black += EVAL_BLOCKED_PAWN;
	}
	if ((board->pos[BLACK][KNIGHT] & BB_SQUARE(C6)) && /* knight on C6 */
	    !((board->pos[BLACK][PAWN] ^ BB_C7D5) & BB_C7D5E5)) /* "closed" defense */
	{
		score_black += EVAL_BLOCKED_PAWN;
	}
	/* rook/queen on the 7th bonus */
	score_white += EVAL_ROOK_RANK7_MULTIPLIER <<
	               POPCOUNT((board->pos[WHITE][ROOK] | board->pos[WHITE][QUEEN]) &
	                        BB_RANK7);
	score_black += EVAL_ROOK_RANK7_MULTIPLIER <<
	               POPCOUNT((board->pos[BLACK][ROOK] | board->pos[BLACK][QUEEN]) &
	                        BB_RANK2);
	
	/********************************************************************
	 * King safety - white
	 ********************************************************************/
	/* pawn shield */
	if (board->hascastled[WHITE])
	{
		ksafety_white += EVAL_CASTLE_BONUS;
		/* pawn shield one row in front of the king */
		num_pieces = POPCOUNT(kingattacks[kingsq_white] &
		                    board->pos[WHITE][PAWN] & BB_RANK2);
		ksafety_white += pawncover_rank2[num_pieces];
		/* pawn shield two rows in front of the king */
		num_pieces = POPCOUNT(kingattacks[kingsq_white + 8] &
		                    board->pos[WHITE][PAWN] & BB_RANK3);
		ksafety_white += pawncover_rank3[num_pieces];
	}
	/* open files near the king */
	if (!(board->pos[WHITE][PAWN] & BB_FILE(COL(kingsq_white))))
	{
		ksafety_white += EVAL_KINGFILEOPEN;
	}
	switch(COL(kingsq_white)) /* this idea taken from gnuchess */
	{
		case COL_A:
		case COL_E:
		case COL_F:
		case COL_G:
			if (!(board->pos[WHITE][PAWN] & BB_FILE(COL(kingsq_white) + 1)))
			{
				ksafety_white += EVAL_ADJACENTFILEOPEN;
			}
			break;
		case COL_H:
		case COL_D:
		case COL_C:
		case COL_B:
			if (!(board->pos[WHITE][PAWN] & BB_FILE(COL(kingsq_white) - 1)))
			{
				ksafety_white += EVAL_ADJACENTFILEOPEN;
			}
			break;
		default:
			break;
	}
	score_white += ksafety_white * board->material[BLACK] / 3100;
	/********************************************************************
	 * King safety - black
	 ********************************************************************/
	/* pawn shield */
	if (board->hascastled[BLACK])
	{
		ksafety_black += EVAL_CASTLE_BONUS;
		/* pawn shield one row in front of the king */
		num_pieces = POPCOUNT(kingattacks[kingsq_black] &
		                    board->pos[BLACK][PAWN] & BB_RANK7);
		ksafety_black += pawncover_rank2[num_pieces];
		/* pawn shield two rows in front of the king */
		num_pieces = POPCOUNT(kingattacks[kingsq_black - 8] &
		                    board->pos[BLACK][PAWN] & BB_RANK6);
		ksafety_black += pawncover_rank3[num_pieces];
	}
	/* open files near the king */
	if (!(board->pos[BLACK][PAWN] & BB_FILE(COL(kingsq_black))))
	{
		ksafety_black += EVAL_KINGFILEOPEN;
	}
	switch(COL(kingsq_black)) /* this idea taken from gnuchess */
	{
		case COL_A:
		case COL_E:
		case COL_F:
		case COL_G:
			if (!(board->pos[BLACK][PAWN] & BB_FILE(COL(kingsq_black) + 1)))
			{
				ksafety_black += EVAL_ADJACENTFILEOPEN;
			}
			break;
		case COL_H:
		case COL_D:
		case COL_C:
		case COL_B:
			if (!(board->pos[BLACK][PAWN] & BB_FILE(COL(kingsq_black) - 1)))
			{
				ksafety_black += EVAL_ADJACENTFILEOPEN;
			}
			break;
		default:
			break;
	}
	score_black += ksafety_black * board->material[WHITE] / 3100;
	/********************************************************************
	 * return the values
	 ********************************************************************/
	if (board->tomove == WHITE)
	{
		return (score_white + board->material[WHITE]) -
		       (score_black + board->material[BLACK]);
	}
	else
	{
		return (score_black + board->material[BLACK]) -
		       (score_white + board->material[WHITE]);
	}
}

/**
 * Special-case endgame evaluator
 */
static int16_t eval_endgame(board_t *board)
{
	int piece, square;
	bitboard_t piecepos;
	/* keeps track of {WHITE,BLACK}s score */
	int16_t score_white = 0, score_black = 0;
	int16_t difference = board->material[WHITE] -
	                 board->material[BLACK];
	
	/* draw evaluation - if no pawns are left, many possibilities
	 * note, our goal in this if block is to put the conditionals most
	 * likely to fail first, so we don't waste time saying "is the pawn's
	 * promotion square the opposite color of the bishop" when there's a
	 * queen on the board. */
	if (!(board->pos[WHITE][PAWN] | board->pos[BLACK][PAWN]))
	{
		/* absolute value */
		if (difference < 0) { difference = -difference; }
		
		/* a good heuristic to use is if the material difference is
		 * less than 400, the position is drawn */
		if (difference < 400)
		{
			return 0;
		}

		/* KNN vs K */
		else if (difference == 600)
		{
			/* check if white has the two knights */
			if ((board->material[BLACK] == 0) &&
			    (board->piecesofcolor[WHITE] ==
			     (board->pos[WHITE][KING] |
			      board->pos[WHITE][KNIGHT])))
			{
				return 0;
			}
			/* or black has the two knights */
			if ((board->material[WHITE] == 0) &&
			    (board->piecesofcolor[BLACK] ==
			     (board->pos[BLACK][KING] |
			      board->pos[BLACK][KNIGHT])))
			{
				return 0;
			}
			/* here it must be a different piece configuration */
		}
	}
	/* KB + wrong color rook pawn vs K */
	else if (difference == 400)
	{
		/* first make sure it's KB rookpawn v K */
		if ((board->material[BLACK] == 0) &&
		    (board->pos[WHITE][BISHOP] != BB(0x0)) &&
		    (board->pos[WHITE][PAWN] & (COL_A | COL_H)))
		{
			square_t pawnsquare = BITSCAN(board->pos[WHITE][PAWN]);
			square_t bishopsquare = BITSCAN(board->pos[WHITE][BISHOP]);
			square_t promotionsquare = SQUARE(COL(pawnsquare),RANK_8);
			/* check the square parities and also where's
			 * the enemy king */
			if ((PARITY(bishopsquare) != PARITY(promotionsquare)) &&
			    (kingattacks[BITSCAN(board->pos[BLACK][KING])] &
			     BB_SQUARE(promotionsquare)))
			{
				return 0;
			}
		}
		/* same thing for the other color */
		else if ((board->material[WHITE] == 0) &&
		         (board->pos[BLACK][BISHOP] != BB(0x0)) &&
		         (board->pos[BLACK][PAWN] & (COL_A | COL_H)))
		{
			square_t pawnsquare = BITSCAN(board->pos[BLACK][PAWN]);
			square_t bishopsquare = BITSCAN(board->pos[BLACK][BISHOP]);
			square_t promotionsquare = SQUARE(COL(pawnsquare),RANK_1);
			/* check the square parities and also where's
			 * the enemy king */
			if ((PARITY(bishopsquare) != PARITY(promotionsquare)) &&
			    (kingattacks[BITSCAN(board->pos[WHITE][KING])] &
			     BB_SQUARE(promotionsquare)))
			{
				return 0;
			}
		}
	}
	/* KP vs K - here we code just two cases, to facilitate the searcher;
	 * once we hit the back rank the searcher will see the draw itself.
	 * the first position is with the Ks in opposition with the pawn
	 * in between (either side to move, it's a draw); the second is
	 * with the pawn behind the winning king, in opposition (zugzwang) */
	else if (difference == 100)
	{
		/* white has the pawn */
		if (board->material[BLACK] == 0)
		{
			square_t whiteking = BITSCAN(board->pos[WHITE][KING]);
			square_t blackking = BITSCAN(board->pos[BLACK][KING]);
			square_t whitepawn = BITSCAN(board->pos[WHITE][PAWN]);
			unsigned char pawncol = COL(whitepawn);
			/* test for rook pawn draws */
			if (pawncol == COL_A || pawncol == COL_H)
			{
				/* if black king gets in front of the pawn,
				 * it's a draw no matter what */
				if ((COL(blackking) == pawncol) &&
				    (blackking > whitepawn))
				{
					return 0;
				}
				/* if the white king is in front of his pawn,
				 * but trapped by the black king, it'll end in
				 * stalemate */
				if (COL(whiteking) == pawncol)
				{
					if (pawncol == COL_A)
					{
						if (blackking - whiteking == 2)
						{
							return 0;
						}
					}
					else /* pawncol == COL_H */
					{
						if (whiteking - blackking == 2)
						{
							return 0;
						}
					}
				}
			}
			if ((blackking - whiteking == 16) &&
			    (ROW(blackking) != RANK_8))
			{
				if (whitepawn - whiteking == 8)
				{
					return 0;
				}
				else if ((whiteking - whitepawn == 8) &&
					 (board->tomove == WHITE))
				{
					return 0;
				}
			}	
		}
		/* black has the pawn */
		else if (board->material[WHITE] == 0)
		{
			square_t whiteking = BITSCAN(board->pos[WHITE][KING]);
			square_t blackking = BITSCAN(board->pos[BLACK][KING]);
			square_t blackpawn = BITSCAN(board->pos[BLACK][PAWN]);
			unsigned char pawncol = COL(blackpawn);
			/* test for rook pawn draws */
			if (pawncol == COL_A || pawncol == COL_H)
			{
				/* if black king gets in front of the pawn,
				 * it's a draw no matter what */
				if ((COL(whiteking) == pawncol) &&
				    (whiteking < blackpawn))
				{
					return 0;
				}
				/* if the white king is in front of his pawn,
				 * but trapped by the black king, it'll end in
				 * stalemate */
				if (COL(blackking) == pawncol)
				{
					if (pawncol == COL_A)
					{
						if (whiteking - blackking == 2)
						{
							return 0;
						}
					}
					else /* pawncol == COL_H */
					{
						if (blackking - whiteking== 2)
						{
							return 0;
						}
					}
				}
			}
			if ((blackking - whiteking == 16) &&
			    (ROW(whiteking) != RANK_1))
			{
				if (blackking - blackpawn == 8)
				{
					return 0;
				}
				else if ((blackpawn - blackking == 8) &&
					 (board->tomove == BLACK))
				{
					return 0;
				}
			}	
		}
	}
	
	/* calculate piece/square totals */
	for (piece = 0; piece < 6; piece++)
	{
		piecepos = board->pos[WHITE][piece];
		while (piecepos)
		{
			square = BITSCAN(piecepos);
			piecepos ^= BB_SQUARE(square);
			score_white += eval_piecevalue_endgame[piece] +
			               eval_squarevalue_endgame[WHITE][piece][square];
		}
	}
	for (piece = 0; piece < 6; piece++)
	{
		piecepos = board->pos[BLACK][piece];
		while (piecepos)
		{
			square = BITSCAN(piecepos);
			piecepos ^= BB_SQUARE(square);
			score_black += eval_piecevalue_endgame[piece] +
			               eval_squarevalue_endgame[BLACK][piece][square];
		}
	}
	
	/* return the values - we revalued the pieces to make pawns more
	 * important so board->material is inaccurate here */
	if (board->tomove == WHITE)
	{
		return score_white - score_black;
	}
	else
	{
		return score_black - score_white;
	}
}

/**
 * Lazy evaluator
 */
int16_t eval_lazy(board_t *board)
{
	return board->material[board->tomove] -
	       board->material[OTHERCOLOR(board->tomove)];
}
/****************************************************************************
 * movelist.c - data structure for move sorting in move generation/iteration
 * copyright (C) 2008 Ben Blum
 * ****************************************************************************/

extern int16_t eval_squarevalue[2][6][64];

/* Material values, add/subtract to find indices for moves which change the
 * material count */
static uint8_t movelist_material[] = { 1, 3, 3, 5, 9, 12 };

/* arg 1 is the attacked boards, arg 2 is the move */
static unsigned long movelist_findindex(uint64_t[2], uint32_t);

/**
 * Add a move. Determines where to add it using findindex
 */
void movelist_add(movelist_t *ml, uint64_t attackedby[2], uint32_t m)
{
	movelist_addtohead(ml, m, movelist_findindex(attackedby, m));
}

/**
 * Determine at which index in the movelist a move should be added
 */
static unsigned long movelist_findindex(uint64_t attackedby[2], uint32_t m)
{
	int16_t materialgain;
	/* which side has the move? this is a substitute for board->tomove */
	unsigned char tomove = MOV_COLOR(m);

	if (MOV_PROM(m))
	{
		if (MOV_PROMPC(m) == QUEEN)
		{
			/* MOV_CAPT is 1 or 0 */
			return MOVELIST_INDEX_PROM_QUEEN + MOV_CAPT(m);
		}
		return MOVELIST_INDEX_PROM_MINOR;
	}
	if (MOV_CASTLE(m))
	{
		/* The second term will be 1 if O-O, 0 if O-O-O */
		return MOVELIST_INDEX_CASTLE + (COL(MOV_DEST(m)) == COL_G);
	}
	if (MOV_CAPT(m))
	{
		materialgain = movelist_material[MOV_CAPTPC(m)];
		/* see if the capturing piece will be recaptured - assume it
		 * will be if the opponent attacks that square at all */
		if (attackedby[OTHERCOLOR(tomove)] & BB_SQUARE(MOV_DEST(m)))
		{
			/* this will index past the end of the array if the
			 * moving piece is a king - we rely on the board
			 * library to filter out suicide king captures */
			materialgain -= movelist_material[MOV_PIECE(m)];
			/* neutral capture */
			if (materialgain == 0)
			{
				return MOVELIST_INDEX_NEUTRAL + MOV_PIECE(m);
			}
			//real SEE could be done here
		}
		/* losing material */
		else if (materialgain < 0)
		{
			return MOVELIST_INDEX_MAT_LOSS + materialgain;
		}
		/* winning capture */
		else
		{
			return MOVELIST_INDEX_MAT_GAIN + materialgain;
		}
	}
	/* here it's a regular move */
	/* moving the piece where it can be captured */
	if (attackedby[OTHERCOLOR(tomove)] & BB_SQUARE(MOV_DEST(m)))
	{
		/* Moves that put the king in check are only generated if the
		 * attackedby bitmasks don't see it, so we won't fall apart on
		 * one either - they just get poor ordering, which is fine. */
		assert(MOV_PIECE(m) != KING);
		/* if we can recapture after being captured */
		if (attackedby[tomove] & BB_SQUARE(MOV_DEST(m)))
		{
			/* if the moving piece is better than a pawn, call it
			 * a loss. otherwise, assume the pawn won't be taken
			 * (and fall through to the next phase). */
			if (MOV_PIECE(m) != PAWN)
			{
				/* give an "approximate" SEE - guessing, when
				 * we recapture we'll get between 1 and 3 */
				return MOVELIST_INDEX_MAT_LOSS + 2 -
				       movelist_material[MOV_PIECE(m)];
			}
			//real SEE could be done here
		}
		/* if we move to a hanging square, we consider it complete
		 * loss of that piece */
		else
		{
			return MOVELIST_INDEX_MAT_LOSS -
			       movelist_material[MOV_PIECE(m)];
		}
	}
	/* here our piece is safe on its destination - let's see if we were
	 * saving it from being captured otherwise ("unhang"ing the piece); if
	 * so, it goes in the material gain category */
	if ((attackedby[OTHERCOLOR(tomove)] & ~(attackedby[tomove])) &
	    BB_SQUARE(MOV_SRC(m)))
	{
		return MOVELIST_INDEX_MAT_GAIN +
		       movelist_material[MOV_PIECE(m)];
	}
	/* regular move */
	return MOVELIST_INDEX_REGULAR +
	       eval_squarevalue[tomove][MOV_PIECE(m)][MOV_DEST(m)] -
	       eval_squarevalue[tomove][MOV_PIECE(m)][MOV_SRC(m)];
}

void movelist_addtohead(movelist_t *mlp, uint32_t m, unsigned long index)
{
	movelist_inner_t *ml = *mlp;
	/* sanity check */
	assert(index < MOVELIST_NUM_BUCKETS);
	/* add to list */
	assert(ml->sublist_count[index] < MOVELIST_SUBLIST_LENGTH);
	ml->array[index][ml->sublist_count[index]] = m;
	ml->sublist_count[index]++;

	/* adjust max pointer */
	if (index > ml->max)
	{
		ml->max = index;
	}
	return;
}

int movelist_isempty(movelist_t *mlp)
{
	movelist_inner_t *ml = *mlp;
	return (ml->sublist_count[ml->max] == 0);
}

/* Don't call this if movelist_isempty! */
uint32_t movelist_remove_max(movelist_t *mlp)
{
	movelist_inner_t *ml = *mlp;
	uint32_t returnval = ml->array[ml->max][--ml->sublist_count[ml->max]];
	/* check if we need to adjust the max pointer */
	while (movelist_isempty(mlp))
	{
		/* we hit the bottom of the array; doesn't matter if empty */
		if (ml->max == 0)
		{
			break;
		}
		ml->max--;
	}
	return returnval;
}
/****************************************************************************
 * pawnstructure.c - evaluation helpers to judge pawn structure quality
 * copyright (C) 2008 Ben Blum
 * ****************************************************************************/

/* We do pawn square scoring here rather than in eval.c to save time */
extern int16_t eval_squarevalue[2][6][64];

typedef struct ps_entry_t {
	bitboard_t key;
	bitboard_t holes;
	int16_t value;
} ps_entry_t;

/* this is a lot like the transposition table in case you haven't noticed... */
#ifndef PS_NUM_BUCKETS
#define PS_NUM_BUCKETS (32 * 1024 * 1024 / sizeof(ps_entry_t)) // 32MB
#endif
static ps_entry_t ps_array[PS_NUM_BUCKETS];

static void ps_add(bitboard_t key, bitboard_t holes, int16_t value)
{
	unsigned long bucket;
	
	/* find the bucket */
	bucket = key % PS_NUM_BUCKETS;
	/* always evict */
	ps_array[bucket].key = key;
	ps_array[bucket].holes = holes;
	ps_array[bucket].value = value;
	
	return;
}

/**
 * Fetch data from the table.
 */
#ifndef INT16_MIN
#define INT16_MIN (-(32767)-1)
#endif
#define PAWNSTRUCTURE_LOOKUP_FAIL INT16_MIN
static int16_t ps_get(bitboard_t key, bitboard_t *holes)
{
	unsigned long bucket = key % PS_NUM_BUCKETS;
	if (ps_array[bucket].key == key)
	{
		*holes = ps_array[bucket].holes;
		return ps_array[bucket].value;
	}
	return PAWNSTRUCTURE_LOOKUP_FAIL;
}

/* Bonus for pawn chains - for each pawn protected by another, add bonus */
#ifndef PS_CHAIN_BONUS
#define PS_CHAIN_BONUS      2
#endif
/* Penalty for doubling pawns - for each pawn with another pawn on its file,
 * subtract the penalty (note, tripled pawns net loss 6 times this!) */
#ifndef PS_DOUBLED_PENALTY
#define PS_DOUBLED_PENALTY  8
#endif
/* Isolated penalty - for each pawn with no brothers on adjacent files
 * note, if a pawn gets this penalty it will also get the backwards penalty */
#ifndef PS_ISOLATED_PENALTY
#define PS_ISOLATED_PENALTY 16
#endif
/* Backward pawns - nothing on same rank or behind on adjacent columns */
#ifndef PS_BACKWARD_PENALTY
#define PS_BACKWARD_PENALTY 8
#endif

/* Bonuses for having a passed pawn of <color> on the given <rank> */
int16_t pawnstructure_passed_bonus[2][8] = {
	{ 0,  3,  6, 12, 24, 48, 96, 0 },
	{ 0, 96, 48, 24, 12,  6,  3, 0 }
};

/**
 * Pawnstructure evaluator - color being whose pawns these are, and holes a
 * pass-by-reference second return value - a chart of all "holes" in the pawn
 * structure; that is, all squares which can never be attacked by these pawns.
 */
int16_t eval_pawnstructure(board_t *board, unsigned char color, bitboard_t *h)
{
	/* the value we calculate and store in the hashtable */
	int16_t value;
	/* value dependent on factors outside of just the pawn pos, so we
	 * cannot hash this and must calculate even if the cache hits */
	int16_t unhashable_bonus = 0;
	
	square_t square;
	bitboard_t pawns, pos, friends, everybodyelse, behindmask, holes;

	pawns = board->pos[color][PAWN];
	
	/* attempt to lookup - if we succeed, only need to calculate the
	 * unhashable bonus. pass in the h ptr as well; if the lookup
	 * fails it'll be untouched */
	if ((value = ps_get(pawns, h)) != PAWNSTRUCTURE_LOOKUP_FAIL)
	{
		while (pawns)
		{
			square = BITSCAN(pawns);
			pawns ^= BB_SQUARE(square);
			if (board_pawnpassed(board, square, color))
			{
				unhashable_bonus += pawnstructure_passed_bonus[color][ROW(square)];
			}
		}
		return value + unhashable_bonus;
	}
	
	/* holes starts with everything set, and we take off bits that our
	 * pawns could attack (using passedpawn masks for this) */
	holes = ~BB_SQUARE(0x0);
	/* for each pawn, consider its value with relation to the structure */
	pos = pawns;
	while (pos)
	{
		/* find and clear a bit */
		square = BITSCAN(pos);
		pos ^= BB_SQUARE(square);
		/* piece/square table */
		value += eval_squarevalue[color][PAWN][square];

		/* check for passed pawn */
		if (board_pawnpassed(board, square, color))
		{
			unhashable_bonus += pawnstructure_passed_bonus[color][ROW(square)];
		}
		
		everybodyelse = pawns ^ BB_SQUARE(square);
		
		/* first evaluate chaining */
		friends = everybodyelse & pawnattacks[color][square];
		value += PS_CHAIN_BONUS * POPCOUNT(friends);

		/* next, doubled pawns */
		friends = everybodyelse & BB_FILE(COL(square));
		value -= PS_DOUBLED_PENALTY * POPCOUNT(friends);
		
		/* backwards */
		friends = everybodyelse & bb_adjacentcols[COL(square)];
		/* get a mask of all squares behind/same rank as this square */
		if (color == WHITE)
		{
			behindmask = (BB_SQUARE(square) ^ (BB_SQUARE(square) - 1)) |
			             BB_RANK(ROW(square));
		}
		else
		{
			behindmask = (BB_SQUARE(square) ^ (-BB_SQUARE(square))) |
			             BB_RANK(ROW(square));
		}
		if (!(friends & behindmask))
		{
			value -= PS_BACKWARD_PENALTY;
			/* see if isolated entirely */
			if (!friends) /* SO RONERY */
			{
				value -= PS_ISOLATED_PENALTY;
			}
		}
		/* calculate holes in pawnstructure - any squares that this
		 * pawn may be able to attack, take out of the bitmap */
		holes ^= (bb_passedpawnmask[color][square] & ~BB_FILE(COL(square)));
	}
	ps_add(pawns, holes, value);
	*h = holes;
	return value + unhashable_bonus;
}
/****************************************************************************
 * popcnt.c - various population-count implementations
 * copyright (C) 2008 Ben Blum
 ****************************************************************************/

/**
 * Kernighan's LSB-resetting popcount implementation
 */
int popcnt(uint64_t x)
{
	int count = 0;
	while (x)
	{
		count++;
		x &= x - 1; // reset LS1B
	}
	return count;
}

/**
 * Good on procs with slow multiplication
 */
int popcnt2(uint64_t x)
{
	x -= (x >> 1) & 0x5555555555555555ull;
	x = (x & 0x3333333333333333ull) + ((x >> 2) & 0x3333333333333333ull);
	x = (x + (x >> 4)) & 0x0f0f0f0f0f0f0f0full;
	x += x >>  8;
	x += x >> 16;
	x += x >> 32;
	return x & 0x7f;
}

/**
 * Good on procs with fast multiplication
 */
int popcnt3(uint64_t x) {
	x -= (x >> 1) & 0x5555555555555555ull;
	x = (x & 0x3333333333333333ull) + ((x >> 2) & 0x3333333333333333ull);
	x = (x + (x >> 4)) & 0x0f0f0f0f0f0f0f0full;
	return (x * 0x0101010101010101ull)>>56;
}
/****************************************************************************
 * quiescent.c - captures-only search to avoid evaluating unstable positions
 * copyright (C) 2008 Ben Blum
 ****************************************************************************/

extern volatile unsigned char timeup;
extern int lazy, nonlazy;

#ifndef QUIESCENT_MAX_DEPTH
/* Warning: NEVER put this at 4 or below - it causes the bot to be too weak */
#define QUIESCENT_MAX_DEPTH 8
#endif

static int16_t qalphabeta(board_t *, int16_t, int16_t, uint8_t);

/**
 * Play out capture chains in a position until it's quiet (or until a certain
 * depth is reached, for speed) so we can accurately use the evaluator
 */
int16_t quiesce(board_t *board, int16_t alpha, int16_t beta)
{
	return qalphabeta(board, alpha, beta, QUIESCENT_MAX_DEPTH);
}

/**
 * Alpha beta searching over captures. Don't need to return a move, only the
 * value. No repetition/50-move checking (obvious reasons), no transposition
 * table (simplicity) - we get pretty good move ordering for captures already
 * from the board library.
 */
static int16_t qalphabeta(board_t *board, int16_t alpha, int16_t beta,
                         uint8_t depth)
{
	movelist_t moves;
	move_t curmove;
	/* who has the move */
	int color;
	//XXX: due to we're only generating captures, we can't check if we get
	//XXX: mated here... this seems bad, but should turn out ok (i.e., a
	//XXX: quiescence that doesn't see mates is better than no quiescence
	//XXX: at all.
	
	/* used in place of lastval */
	int16_t a;

	/* what happens if the player to move declines to make any captures */
	int16_t stand_pat;

	if (timeup)
	{
		return 0;
	}
	
	/* we'll be using stand_pat in a lot of places*/
	/* first do a lazy evaluation; if it's too far from the window we will
	 * not need the precision of eval() */
	stand_pat = eval_lazy(board);
	/* check if we're not allowed to be lazy */
	if ((stand_pat > (alpha - EVAL_LAZY_THRESHHOLD)) &&
	    (stand_pat < (beta + EVAL_LAZY_THRESHHOLD)))
	{
		stand_pat = eval(board);
		nonlazy++;
	}
	else { lazy++; }

	/********************************************************************
	 * terminal condition - search depth ran out
	 ********************************************************************/
	if (depth == 0)
	{
		return stand_pat;
	}
	
	/********************************************************************
	 * Main iteration over all the moves
	 ********************************************************************/
	/* set some preliminary values */
	if (stand_pat > alpha)
	{
		if (stand_pat >= beta)
		{
			return stand_pat;
		}
		alpha = stand_pat;
	}
	color = board->tomove;
	
	/* and we're ready to go */
	board_generatecaptures(board, &moves);
	while (!movelist_isempty(&moves))
	{
		if (timeup)
		{
			break;
		}
		curmove = movelist_remove_max(&moves);
		
		board_applymove(board, curmove);
		/* see if this move puts us in check */
		if (board_colorincheck(board, color))
		{
			board_undomove(board, curmove);
			continue;
		}
		a = -qalphabeta(board, -beta, -alpha, depth-1);
		board_undomove(board, curmove);

		if (a > alpha)
		{
			alpha = a;
		}
		if (beta <= alpha)
		{
			break;
		}
	}
	movelist_destroy(&moves);
	
	return alpha; /* will work even if no captures were available */
}
/****************************************************************************
 * rand.c - wrappers for GSL random number generation utilities
 * copyright (C) 2008 Ben Blum
 ****************************************************************************/

uint32_t
rand32(void)
{
	rand_init();
	return genrand();
}

static unsigned char rand_initted = 0;

void rand_init()
{
	if (rand_initted)
	{
		return;
	}
	sgenrand(get_ticks());
	rand_initted = 1;
}

uint64_t rand64()
{
	return ((uint64_t)rand32()) | (((uint64_t)rand32()) << 32);
}

/****************************************************************************
 * search.c - traverse the game tree looking for the best position
 * copyright (C) 2008 Ben Blum
 ****************************************************************************/

/* Constants for the search algorithm */
#define SEARCHER_INFINITY 32767
#define SEARCHER_MATE 16383

/* could be changed if you wanted to {dis,en}courage draws
 * TODO: make the evaluator also use this */
#define SEARCHER_DRAW_SCORE 0
#define VALUE_ISDRAW(v) ((v) == SEARCHER_DRAW_SCORE)

static move_t alphabeta(board_t *, int16_t, int16_t, uint8_t, uint8_t, move_t, uint8_t, uint8_t, unsigned char);

int transposition_hits, transposition_misses;
extern int regen_hits, regen_misses;

static int nodes;
static int16_t lastval;

/* used for printing stuff */
extern int outbuf_size;
extern int text_loud;
extern char outbuf[];
void output();

/* Used for iterative deepening and replacement policy in the trans table */
static uint8_t cur_searching_depth;

/* used for iterative deepening */
volatile unsigned char timeup;

#define SEARCHER_MIN_DEPTH 2
#define SEARCHER_MAX_DEPTH 64
// configurable; must never get bigger than MAX_DEPTH-1
unsigned  int searcher_max_depth = SEARCHER_MAX_DEPTH-1;

#define VALUE_ISMATE(v) (((v) >= SEARCHER_MATE - SEARCHER_MAX_DEPTH) || ((v) <= -(SEARCHER_MATE - SEARCHER_MAX_DEPTH)))

/* aspiration windows - first try a window of aspir_1 around prevalpha; if
 * that fails go to aspir_2; if that fails use the full -inf,+inf search */
#define SEARCHER_ASPIRATION_1  50
#define SEARCHER_ASPIRATION_2 200

/* killer moves
 *
 * Tried at each move after the hashed bestmove, before generatemoves(). A
 * killer move is basically any move by one player that produces a beta cutoff
 * when the pther player makes a move from X position; the hope here is that
 * the same refutation move will produce a cutoff in the next children of X.
 * This is likely to happen if the first player is threatened somehow, and the
 * killer will be the followthrough on that threat if player one doesn't
 * respond to it.
 *
 * note - you will want to clear all the killers in the following cases:
 * 	1) Beginning/end of getbestmove - clear all depths
 * 	2) After a node finishes its children, clear the nodes at that
 * 	   child's ply
 * If the killers are not adequately cleared, you may end up applying some
 * illegal moves at other nodes, resulting in inconsistent board state.
 * Another thing to watch out for is some killers can still be illegal - most
 * cases are eliminated by preventing captures and castles, the rest need
 * checking against board_attacksfrom() (or pushes, in the case of pawns).
 */
#define SEARCHER_USE_KILLERS
#ifdef SEARCHER_USE_KILLERS
#define SEARCHER_NUM_KILLERS 3
static move_t killers[SEARCHER_MAX_DEPTH][SEARCHER_NUM_KILLERS];
#endif

/* margins for futility pruning - how far away from the window do we need
 * to be in order to consider doing futility pruning. note that the value at
 * 0 will never be used */
#ifdef SEARCHER_FUTILITY_PRUNING
static int16_t futility_margin[3] = { 0xbeef, 250, 450 };
#endif

int lazy, nonlazy;

/* determining the type of a child from a parent (first index - parent's type)
 * given whether the first child (0 or 1, second index) has been searched */
#define PV  0
#define CUT 1
#define ALL 2
static unsigned char childnodetype[3][2] = {
	/* first child already searched:
	 * NO  YES */
	{ PV,  CUT }, /* parent was PV  */
	{ ALL, CUT }, /* parent was CUT */
	{ CUT, CUT }  /* parent was ALL */
};

/* null-move depth reduction - dependent on depth */
#define NULLMOVE_R(d) (((d) > 6) ? 3 : 2)

/* Late move reduction - we reduce after searching this many nodes given the
 * current node type (allows us to be more conservative at PV nodes) */
static uint8_t lmr_movecount[3] = { 16, 8, 8 };

#define GUESS_KERNEL_HZ 100 // I really wish the syscall interface exposed this.

void *sigalrm_thread(void *arg)
{
	sleep((int)arg * GUESS_KERNEL_HZ);
	timeup = 1;
	return (void *)0x15410FA1L;
}

/**
 * Stores the nodecount in the given int pointer if it's nonnull and the alpha
 * value in the second pointer if that's nonnull.
 */
move_t getbestmove(board_t *board, unsigned int time, int *nodecnt, int16_t *alphaval)
{
	move_t result, prevresult;
	int16_t prevalpha;
	int16_t window_low, window_high;
	char *movestr;
	int ret;
	void *statusp;

	/********************************************************************
	 * Setup
	 ********************************************************************/
	nodes = 0;
	timeup = 0;
	
	transposition_hits = 0; transposition_misses = 0;
	regen_hits = 0; regen_misses = 0;

	lazy = 0; nonlazy = 0;
	
	#ifdef SEARCHER_USE_KILLERS
	/* clear all killers */
	memset(killers, 0, sizeof(killers));
	#endif
	/********************************************************************
	 * Searching
	 ********************************************************************/
	snprintf(outbuf, outbuf_size, "Thinking (for %d %s)...",
		 time, time == 1 ? "sec" : "secs");
	output();
	/* preliminary search */
	cur_searching_depth = SEARCHER_MIN_DEPTH;
	result = alphabeta(board, -SEARCHER_INFINITY, SEARCHER_INFINITY,
	                          cur_searching_depth, 0, 0, 0, 0, PV);
	prevresult = result;
	prevalpha = lastval;
	
	/* start the iterative timer */
	ret = thr_create(sigalrm_thread, (void *)time);
	assert(ret > 0 && "Failed to create timer thread");
	
	while (!timeup && cur_searching_depth < searcher_max_depth)
	{
		/* These guys store the result of the previous depth in case
 		 * our next search times up */
		prevresult = result;
		prevalpha = lastval;

		// Don't waste time if we have a mate
		/*if (lastval >= SEARCHER_MATE)
		{
			break;
		}*/
		
		//TODO: Check if we've used up enough time to not even bother with the next search

		/* advance to the next depth */
		movestr = move_tostring(result);
		if (text_loud) {
			snprintf(outbuf, outbuf_size, "[depth %d] best %s score %d",
				 cur_searching_depth, movestr, lastval);
			output();
		}
		if (VALUE_ISMATE(lastval) && text_loud)
		{
			snprintf(outbuf, outbuf_size, "%s mates in %d",
			         ((lastval > 0) ?
			         	((board->tomove == WHITE) ? "White" : "Black")
			         :
			         	((board->tomove == WHITE) ? "Black" : "White")),
			         ((lastval > 0) ?
			         	((SEARCHER_MATE - lastval + 1) / 2)
			         :
			         	((SEARCHER_MATE + lastval + 1) / 2)));
			output();
		}
		free(movestr);
		cur_searching_depth++;
		nodes = 0;
		
		window_low  = prevalpha - SEARCHER_ASPIRATION_1;
		window_high = prevalpha + SEARCHER_ASPIRATION_1;
		result = alphabeta(board, window_low, window_high,
		                   cur_searching_depth, 0, 0, 0, 0, PV);
		if (timeup) break;
		/* test if aspir_1 window failed */
		if (lastval <= window_low || lastval >= window_high)
		{
			/* try wider window */
			window_low  = prevalpha - SEARCHER_ASPIRATION_2;
			window_high = prevalpha + SEARCHER_ASPIRATION_2;
			/* re-search */
			result = alphabeta(board, window_low, window_high,
			                   cur_searching_depth, 0, 0, 0, 0, PV);
		}
		if (timeup) break;
		/* test if aspir_2 window failed */
		if (lastval <= window_low || lastval >= window_high)
		{
			/* use full window */
			window_low  = -SEARCHER_INFINITY;
			window_high =  SEARCHER_INFINITY;
			/* re-search */
			result = alphabeta(board, window_low, window_high,
			                   cur_searching_depth, 0, 0, 0, 0, PV);
		}
	}
	
	/********************************************************************
	 * Teardown
	 ********************************************************************/
	movestr = move_tostring(prevresult);
	if (text_loud) {
		if (timeup) {
			snprintf(outbuf, outbuf_size, "[depth %d] (timeout, %d nodes)",
				 cur_searching_depth, nodes);
		} else {
			snprintf(outbuf, outbuf_size, "[depth %d] best %s score %d",
				 cur_searching_depth, movestr, lastval);
		}
		output();
	}
	snprintf(outbuf, outbuf_size, "My move: %s", movestr);
	output();
	free(movestr);
	#ifdef SEARCHER_USE_KILLERS
	/* clear killers, just in case */
	memset(killers, 0, sizeof(killers));
	#endif
	if (nodecnt != NULL)
	{
		*nodecnt = nodes;
	}
	if (alphaval != NULL)
	{
		*alphaval = prevalpha;
	}
	// TODO: Do this in a non-blocking way all the time.
	thr_join(ret, &statusp);
	assert(statusp == (void*)0x15410FA1L);
	return prevresult;
}

/**
 * Alpha-beta search, with trans table, check extension, futility pruning
 * board      - the current node's position
 * alpha      - lower bound
 * beta       - upper bound
 * depth      - how deep we will search this node
 * ply        - how far this position is from the board's real position (used
 *              because depth can change based on extensions/reductions)
 * prevmove   - the move that was just made (used in futility pruning)
 * num_checks - how many checks have occurred so far in the search (used for
 *              check extension)
 * nodetype   - what type of node we're searching - PV, CUT, or ALL
 */
static move_t alphabeta(board_t *board, int16_t alpha, int16_t beta,
                        uint8_t depth, uint8_t ply, move_t prevmove,
                        uint8_t num_checks, uint8_t null_extended,
                        unsigned char nodetype)
{
	movelist_t moves;
	move_t curmove, returnmove;
	/* who has the move */
	int color;
	/* used for telling if the movelist we got had no non-suicide moves,
	 * also for telling what the child's node type will be */
	unsigned char children_searched;
	
	/* transposition table */
	trans_data_t trans_data;
	int trans_flag;
	/* for move ordering */
	move_t bestmove;
	
	int killer_index;
	
	nodes++;
	if (timeup)
	{
		return 0;
	}
	/********************************************************************
	 * threefold repetition/50 move - always check first
	 ********************************************************************/
	/* draw score if only even -one- prior repetition, not two. this is
	 * more efficient (taken from crafty). note that it obsoletes storing
	 * the repetition count in the trans table, but if it gets put back,
	 * transing the reps will still be essential. */
	if (board->halfmoves >= 100 || board_threefold_draw(board))
	{
		//fprintf(ttyout, "%sSEARCHER: Detected threefold repetition at depth %d%s\n",
		//        TTYOUT_COLOR, cur_searching_depth - depth, DEFAULT_COLOR);
		lastval = SEARCHER_DRAW_SCORE;
		return 0;
	}
	/********************************************************************
	 * check transposition table
	 ********************************************************************/
	trans_data = trans_get(board->hash);
	if (trans_data_valid(trans_data))
	{
		if (TRANS_SEARCHDEPTH(trans_data) >= depth &&
		    TRANS_REPS(trans_data) >= board->reps)
		{
			int16_t storedval = TRANS_VALUE(trans_data);
			
			/* direct hit! */
			if (TRANS_FLAG(trans_data) == TRANS_FLAG_EXACT)
			{
				transposition_hits++;
				lastval = storedval;
				return TRANS_MOVE(trans_data);
			}
			/* if we do this on the root node, we run the risk of
			 * getting a1a1 as the returnmove, so prevent it */
			else if (depth != cur_searching_depth)
			{
				/* we have a lower bound for the true value */
				if (TRANS_FLAG(trans_data) == TRANS_FLAG_BETA)
				{
					/* above our current window? */
					if (storedval >= beta)
					{
						transposition_hits++;
						lastval = storedval;
						return 0;
					}
					/* not above the window but inside? */
					if (storedval > alpha)
					{
						alpha = storedval;
					}
				}
				/* this means we'll have an upper bound */
				else if (TRANS_FLAG(trans_data) ==
					 TRANS_FLAG_ALPHA)
				{
					/* below current window entirely? */
					if (storedval <= alpha)
					{
						transposition_hits++;
						lastval = storedval;
						return 0;
					}
					/* inside window - cut the top off */
					if (storedval < beta)
					{
						beta = storedval;
					}
				}
			}
		}
		/* well, we can still use move-ordering information. note:
		 * we'll want to grab it regardless of the flag. if it's beta
		 * we'll get a move; if it's alpha we'll get 0; oh well */
		bestmove = TRANS_MOVE(trans_data);
	}
	else
	{
		bestmove = (move_t)0;
		transposition_misses++;
	}
	/********************************************************************
	 * terminal condition - search depth ran out
	 * futility pruning done here
	 ********************************************************************/
	if (depth == 0)
	{
		lastval = quiesce(board, alpha, beta);
		/* we time up in the middle of quiescence and get zero, and go
		 * to add it in the trans table, OH SHI-- */
		if (timeup)
		{
			return 0;
		}
		/* if we already have an entry, and this is an end condition,
		 * we don't want to replace our better entry */
		if (!trans_data_valid(trans_data))
		{
			trans_add(board->hash, (move_t)0, board->reps, lastval,
				  (board->moves - cur_searching_depth),
			          board->moves, 0, TRANS_FLAG_EXACT);
		}
		return 0;
	}
	#ifdef SEARCHER_FUTILITY_PRUNING
	/* futility pruning - prune only 1 and 2 plies from the horizon */
	else if (depth < 3)
	{
		/* if we're in check or possibly in a capture sequence, trying
		 * to prune will be too dangerous */
		if (!(board_incheck(board) || MOV_CAPT(prevmove)))
		{
			int16_t score = eval_lazy(board);
			/* check how far we are from the window */
			if ((score + futility_margin[depth] < alpha) ||
			    (score - futility_margin[depth] > beta))
			{
				lastval = quiesce(board, alpha, beta);
				return 0;
			}
		}
	}
	#endif
	/********************************************************************
	 * Null-move pruning
	 ********************************************************************/
	/* only do it if all of the following:
	 * 	we're a fail-high (CUT) node
	 * 	depth is not three or less
	 * 	we're not in the endgame
	 * 	a lazy eval fails high
	 *	we're not in check
	 */
	if ((nodetype != PV) && (depth > 3) && !eval_isendgame(board) &&
	    (eval_lazy(board) >= beta) && !board_incheck(board))
	{
		/* make null move */
		board_applymove(board, 0);
		alphabeta(board, -beta, -beta+1, depth - NULLMOVE_R(depth) - 1,
			  ply+1, 0, num_checks, null_extended, ALL);
		board_undomove(board, 0);
		
		/* failed high, we can prune */
		if (-lastval >= beta)
		{
			lastval = -lastval;
			return 0;
		}
		/* mate threat - if we do nothing here, we get mated. better
		 * search deeper. only extend this way once per line. */
		if (VALUE_ISMATE(lastval) && !null_extended)
		{
			depth++;
			null_extended = 1;
		}
	}
	/********************************************************************
	 * Main iteration over all the moves
	 ********************************************************************/
	/* set some preliminary values */
	color = board->tomove;
	children_searched = 0;
	returnmove = 0;
	trans_flag = TRANS_FLAG_ALPHA;
	/* and we're ready to go - first do hash move and killers */
	if (bestmove)
	{
		board_applymove(board, bestmove);
		/* if this puts us in check something's really wrong... */
		assert(!board_colorincheck(board, color));
		/* check extension - if this checks the opp king */
		if (board_incheck(board))
		{
			/* only extend search if >1 checks in tree */
			if (num_checks)
			{
				alphabeta(board, -beta, -alpha, depth, ply+1,
				          bestmove, num_checks+1, null_extended,
				          childnodetype[nodetype][!!children_searched]);
			}
			else
			{
				alphabeta(board, -beta, -alpha, depth-1, ply+1,
				          bestmove, num_checks+1, null_extended,
				          childnodetype[nodetype][!!children_searched]);
			}
		}
		else /* no extensions, regular search */
		{
			alphabeta(board, -beta, -alpha, depth-1, ply+1,
			          bestmove, num_checks, null_extended,
			          childnodetype[nodetype][!!children_searched]);
		}
		board_undomove(board, bestmove);
		children_searched++;

		if (-lastval > alpha)
		{
			alpha = -lastval;
			returnmove = bestmove;
			trans_flag = TRANS_FLAG_EXACT;
		}
		if (alpha >= beta)
		{
			trans_flag = TRANS_FLAG_BETA;
			goto alphabeta_after_iteration;
		}
	}
	#ifdef SEARCHER_USE_KILLERS
	/* no cutoff from the bestmove (or no bestmove); let's try killers */
	for (killer_index = 0; killer_index < SEARCHER_NUM_KILLERS; killer_index++)
	{
		move_t killer = killers[ply][killer_index];
		piece_t killerpiece;
		square_t killersrc, killerdest;
		/* check if a killer is available */
		if (killer == 0 || timeup)
		{
			break;
		}
		killerpiece = MOV_PIECE(killer);
		killersrc = MOV_SRC(killer);
		killerdest = MOV_DEST(killer);
		/* need to check if the killer is legal in this position; it's
		 * guaranteed to not be a capture or castle, so the following
		 * legality checks are made:
		 * 1) We have the given piece on that square
		 * 2) The destination square is not occupied (by either color)
		 * 3) If a sliding piece, there must be nothing in the way */
		if ((!(BB_SQUARE(killersrc) & board->pos[color][killerpiece])) ||
		    (BB_SQUARE(killerdest) & board->occupied) || /* #2 */
		    (PIECE_SLIDES(killerpiece) && /* #3 */
		     !(BB_SQUARE(killerdest) &
		       board_attacksfrom(board, killersrc, killerpiece, color))))
		{
			continue;
		}
		/* next we check if our piece can move to the destsquare */
		if (killerpiece == PAWN)
		{
			/* destination square not among the legal pushes? */
			if (!(BB_SQUARE(killerdest) &
			      board_pawnpushesfrom(board, killersrc, color)))
			{
				continue;
			}
		}
		else
		{
			/* destination square not among the legal moves? */
			if (!(BB_SQUARE(killerdest) &
			      board_attacksfrom(board, killersrc, killerpiece,
			                        color)))
			{
				continue;
			}
		}
		/* we have a pseudolegal killer move to try, good */
		board_applymove(board, killer);
		/* this might put us in check, unlike the bestmove */
		if (board_colorincheck(board, color))
		{
			board_undomove(board, killer);
			continue;
		}
		/* check extension - if this checks the opp king */
		if (board_incheck(board))
		{
			/* only extend search if >1 checks in tree */
			if (num_checks)
			{
				alphabeta(board, -beta, -alpha, depth, ply+1,
				          killer, num_checks+1, null_extended,
					  childnodetype[nodetype][!!children_searched]);
			}
			else
			{
				alphabeta(board, -beta, -alpha, depth-1, ply+1,
				          killer, num_checks+1, null_extended,
					  childnodetype[nodetype][!!children_searched]);
			}
		}
		else /* no extensions, regular search */
		{
			alphabeta(board, -beta, -alpha, depth-1, ply+1,
			          killer, num_checks, null_extended,
			          childnodetype[nodetype][!!children_searched]);
		}
		board_undomove(board, killer);
		children_searched++;
		
		if (-lastval > alpha)
		{
			alpha = -lastval;
			returnmove = killer;
			trans_flag = TRANS_FLAG_EXACT;
		}
		if (alpha >= beta)
		{
			trans_flag = TRANS_FLAG_BETA;
			goto alphabeta_after_iteration;
		}
	}
	#endif
	/* okay, no beta cutoff; let's continue... */
	board_generatemoves(board, &moves);
	while (!movelist_isempty(&moves))
	{
		if (timeup)
		{
			break;
		}
		curmove = movelist_remove_max(&moves);
		
		board_applymove(board, curmove);
		/* see if this move puts us in check */
		if (board_colorincheck(board, color))
		{
			board_undomove(board, curmove);
			continue;
		}
		/* check extension - if this checks the opp king */
		if (board_incheck(board))
		{
			/* only extend search if >1 checks in tree */
			if (num_checks)
			{
				alphabeta(board, -beta, -alpha, depth, ply+1,
				          curmove, num_checks+1, null_extended,
				          childnodetype[nodetype][!!children_searched]);
			}
			else
			{
				alphabeta(board, -beta, -alpha, depth-1, ply+1,
				          curmove, num_checks+1, null_extended,
				          childnodetype[nodetype][!!children_searched]);
			}
		}
		/* Late move reductions (LMR)
		 * The idea here is if a move occurs late in the movelist,
		 * it's probably not worth searching to a full depth (if it
		 * happens to return a value within the window, re-search with
		 * full depth). This works recursively, so strings of moves
		 * that are all Late get searched to (optimal case) 1/2 depth
		 * overall, while lines with just one Late move are searched
		 * with depth reduced by just one overall. What happens if we
		 * encounter a seemingly bad move that only pays off when
		 * searched to the full depth (i.e. sacrifice -> attack)?
		 * Typically, such lines will only have one or two Late moves
		 * in them, and we rely on the more extreme reductions on the
		 * completely nonsense lines (1/2-depth mentioned before) to
		 * possibly give us another ply so we find the sacrifice. */
		else if (children_searched > lmr_movecount[nodetype] && depth > 3 &&
		         !MOV_CAPT(curmove) && !MOV_CAPT(prevmove))
		{
			alphabeta(board, -beta, -alpha, depth-2, ply+1,
			          curmove, num_checks, null_extended,
			          childnodetype[nodetype][1]);
			/* oops, it fell within our window. full re-search */
			if (-lastval > alpha)
			{
				alphabeta(board, -beta, -alpha, depth-1, ply+1,
				          curmove, num_checks, null_extended,
				          childnodetype[nodetype][1]);
			}
		}
		else /* no extensions/reductions, regular search */
		{
			alphabeta(board, -beta, -alpha, depth-1, ply+1,
			          curmove, num_checks, null_extended,
			          childnodetype[nodetype][!!children_searched]);
		}
		board_undomove(board, curmove);
		children_searched++;
		
		if (-lastval > alpha)
		{
			alpha = -lastval;
			returnmove = curmove;
			/* we've gotten above the bottom of the window */
			trans_flag = TRANS_FLAG_EXACT;
		}
		if (beta <= alpha)
		{
			/* oops, above the top of the window */
			trans_flag = TRANS_FLAG_BETA;
			#ifdef SEARCHER_USE_KILLERS
			/* Not a killer if capture or castle */
			if (!(MOV_CAPT(curmove)) && !(MOV_CASTLE(curmove)))
			{
				/* We found a killer move! Add it in... note
				 * we will never here cause duplicate killers
				 * because we would have tested this killer
				 * earlier and skipped this */
				killer_index = 0;
				while ((killers[ply][killer_index] != 0) &&
				       (killer_index < SEARCHER_NUM_KILLERS-1))
				{
					/* move to first empty killer slot,
					 * or if full replace at the end */
					killer_index++;
				}
				killers[ply][killer_index] = curmove;
			}
			#endif
			break;
		}
	}
	movelist_destroy(&moves);
	/* now we are done iterating; cleanup and exit this node. note, this
	 * label is here so we can skip over iteration if the bestmove/killers
	 * produce a cutoff */
alphabeta_after_iteration:
	/* The transposition table will get really sad if we choke all over it
 	 * with bogus results from after we get cut off */
	if (timeup)
	{
		return 0;
	}
	#ifdef SEARCHER_USE_KILLERS
	/* okay we need to clear the killers for the children here */
	memset(killers[ply+1], 0, (SEARCHER_NUM_KILLERS * sizeof(move_t)));
	#endif
	/* check if there were no legal moves */
	if (children_searched)
	{
		/* this is the "typical case" function ending
		 * set the value so the calling function sees it properly */
		lastval = alpha;
		/* Don't add mates because they are depth-dependent. Don't add
		 * draws because they are state-dependent. */
		if (!VALUE_ISMATE(alpha) && !VALUE_ISDRAW(alpha))
		{
			/* update the trans table - here we always replace */
			trans_add(board->hash, returnmove, board->reps, alpha,
			          (board->moves + depth - cur_searching_depth),
			          board->moves, depth, trans_flag);
		}
	}
	else /* no children were searched - must be an end condition */
	{
		/* checkmate vs stalemate */
		if (board_incheck(board))
		{
			lastval = -1 * (SEARCHER_MATE - ply);
		}
		else
		{
			lastval = 0;
		}
		/* DON'T add to the transposition table here... this can screw
		 * up which mates are sooner and cause 3-rep draws - besides,
		 * if a mate is coming up and we've seen it there's no use
		 * trying to search deeper/faster */
		return 0;
	}
	/* and we're done */
	return returnmove;
}
/****************************************************************************
 * transposition.c - avoid re-searching already searched positions
 * copyright (C) 2008 Ben Blum
 ****************************************************************************/

typedef struct trans_entry_t {
	uint64_t key;
	trans_data_t value;
} trans_entry_t;

/**
 * Statically initting the array is good because it avoids committing memory
 * we don't use, but depends, for efficiency not correctness, on the kernel
 * harbl-filling our memory with zeros. When we go to kick out an entry, if
 * we haven't put one there yet, we'll see that the depth is 0 and the flag
 * is ALPHA - the two worst possible values, so this type will always be
 * replaced.
 */
#ifndef TRANS_NUM_BUCKETS
#define TRANS_NUM_BUCKETS ((128 * 1024 * 1024)/sizeof(trans_entry_t)) // 128 MB
#endif
static trans_entry_t array[TRANS_NUM_BUCKETS];

/**
 * Convert separate data values -> condensed struct
 */
static trans_data_t trans_data(move_t move, uint8_t reps, int16_t value,
			       uint8_t gamedepth, uint8_t searchdepth,
			       uint8_t flag)
{
	trans_data_t foo;
	assert(searchdepth < 64);
	foo.move = move | (reps << MOV_INDEX_UNUSED);
	foo.value = value;
	foo.gamedepth = gamedepth;
	foo.flags = ((searchdepth & 0x3f) << TRANS_INDEX_SEARCHDEPTH) |
	           ((flag & 0x3) << TRANS_INDEX_FLAG);
	return foo;
}

/**
 * Add transposition information to the table. Note, it will only kick the old
 * entry out if:
 * 	1) The flag is higher (exact kicks out beta, beta kicks out alpha)
 * 	2) The flag is equal and the depth is higher
 * 	3) The old node was from ancient history
 * Note the different depth arguments:
 * 	rootdepth   - how far down the game tree the real board state is.
 * 	              used in comparison for evicting the old node.
 * 	gamedepth   - how far down this current node is in the game tree. we
 * 	              store this for future nodes to use to evict us.
 * 	searchdepth - how much deeper still we've searched this node. this
 * 	              tells how trustworthy the alpha value is.
 * At present we don't discriminate based on repetition depth.
 */
void trans_add(zobrist_t key, move_t move, uint8_t reps, int16_t value,
               uint8_t rootdepth, uint8_t gamedepth,
               uint8_t searchdepth, uint8_t flag)
{
	unsigned long bucket;
	trans_data_t old;
	
	/* find the bucket */
	bucket = key % TRANS_NUM_BUCKETS;
	
	/* find what used to be there */
	old = array[bucket].value;

	/* see if we should evict */
	if ((rootdepth >= TRANS_GAMEDEPTH(old)) || /* old node too old */
	    (flag > TRANS_FLAG(old)) || /* more accurate data */
	    ((flag == TRANS_FLAG(old)) && /* same type of data but... */
	     (searchdepth > TRANS_SEARCHDEPTH(old)))) /* ...deeper search */
	{
		/* so we set the stuff to our new node */
		array[bucket].key = key;
		array[bucket].value = trans_data(move, reps, value, gamedepth,
		                                 searchdepth, flag);
	}
	return;
}

/**
 * Fetch data from the table. If there is no entry for this key the flag in
 * the return value will be set to -1.
 */
static trans_data_t foo = { (uint8_t)(-1), (uint8_t)(-1),
                            (int16_t)(-1), (uint32_t)(-1) };
trans_data_t trans_get(zobrist_t key)
{
	unsigned long bucket = key % TRANS_NUM_BUCKETS;
	if (array[bucket].key == key)
	{
		return array[bucket].value;
	}
	return foo;
}

/**
 * Check if the transposition data is valid, according to whatever spec may be
 * in use for saying "invalid entry". Used for checking the returnvalue of get
 */
int trans_data_valid(trans_data_t foo)
{
	return (foo.flags != (uint8_t)(-1));
}

/******************************************************************************
 * Custom pebbles interface
 ******************************************************************************/

/********************** BOARD / SCREEN CONTENT ********************************/

char *board_strs[] = {
	"    a   b   c   d   e   f   g   h    ",
	"  .-------------------------------.  ",
	"8 |   |   |   |   |   |   |   |   | 8",
	"  |---+---+---+---+---+---+---+---|  ",
	"7 |   |   |   |   |   |   |   |   | 7",
	"  |---+---+---+---+---+---+---+---|  ",
	"6 |   |   |   |   |   |   |   |   | 6",
	"  |---+---+---+---+---+---+---+---|  ",
	"5 |   |   |   |   |   |   |   |   | 5",
	"  |---+---+---+---+---+---+---+---|  ",
	"4 |   |   |   |   |   |   |   |   | 4",
	"  |---+---+---+---+---+---+---+---|  ",
	"3 |   |   |   |   |   |   |   |   | 3",
	"  |---+---+---+---+---+---+---+---|  ",
	"2 |   |   |   |   |   |   |   |   | 2",
	"  |---+---+---+---+---+---+---+---|  ",
	"1 |   |   |   |   |   |   |   |   | 1",
	"  `-------------------------------'  ",
	"    a   b   c   d   e   f   g   h    ",
};
char *board_strs_flipped[] = {
	"    h   g   f   e   d   c   b   a    ",
	"  .-------------------------------.  ",
	"1 |   |   |   |   |   |   |   |   | 1",
	"  |---+---+---+---+---+---+---+---|  ",
	"2 |   |   |   |   |   |   |   |   | 2",
	"  |---+---+---+---+---+---+---+---|  ",
	"3 |   |   |   |   |   |   |   |   | 3",
	"  |---+---+---+---+---+---+---+---|  ",
	"4 |   |   |   |   |   |   |   |   | 4",
	"  |---+---+---+---+---+---+---+---|  ",
	"5 |   |   |   |   |   |   |   |   | 5",
	"  |---+---+---+---+---+---+---+---|  ",
	"6 |   |   |   |   |   |   |   |   | 6",
	"  |---+---+---+---+---+---+---+---|  ",
	"7 |   |   |   |   |   |   |   |   | 7",
	"  |---+---+---+---+---+---+---+---|  ",
	"8 |   |   |   |   |   |   |   |   | 8",
	"  `-------------------------------'  ",
	"    h   g   f   e   d   c   b   a    ",
};
#define BOARD_ROW 3 // the row of the opponent's sides lettering (a-h/h-a)
#define BOARD_COL 4 // the col of the left side numbering (1-8/8-1)
#define BOARD_ROW_SCALE 2
#define BOARD_COL_SCALE 4

#define BOARD_COLOR (FGND_BRWN | BGND_BLACK)
#define WHITE_COLOR (FGND_WHITE | BGND_BLACK)
#define BLACK_COLOR (FGND_RED | BGND_BLACK) // like the old-time chess sets

int flipped = 0;
int xyzzy = 0;

void set_text_color(int color) {
	int low = color & 0xf;
	int high = color & ~0xf;
	if (low == FGND_BLACK) {
		set_term_color(color);
	} else {
		low = (low + xyzzy) & 0xf;
		if (low == FGND_BLACK) low++;
		set_term_color(low | high);
	}
}

void draw_pieces(board_t *board, int color) {
	int square;
	bitboard_t pos;
	int piece = PAWN;
	while (piece <= KING) {
		pos = board->pos[color][piece];
		while (pos) {
			int row_offset, col_offset;
			square = BITSCAN(pos);
			if (flipped) {
				row_offset = BOARD_ROW_SCALE*(1 + ROW(square));
				col_offset = BOARD_COL_SCALE*(8 - COL(square));
			} else {
				row_offset = BOARD_ROW_SCALE*(8 - ROW(square));
				col_offset = BOARD_COL_SCALE*(1 + COL(square));
			}
			set_cursor_pos(BOARD_ROW + row_offset, BOARD_COL + col_offset);
			print(1, piecename[color][piece]);
			pos &= BB_ALLEXCEPT(square);
		}
		piece++;
	}
}

// I'd say this should be called only on redraw, but it also has the
// convenient effect of setting blank all unoccupied squares, which means
// fewer syscalls overall... unless you'd like to analyse the board state
// to determine what the most recently vacated square is.... etc.
void draw_board(board_t *board) {
	int i;
	set_text_color(BOARD_COLOR);
	if (flipped) {
		for (i = 0; i < ARRAY_SIZE(board_strs_flipped); i++) {
			set_cursor_pos(BOARD_ROW+i, BOARD_COL);
			print(strlen(board_strs_flipped[i]), board_strs_flipped[i]);
		}
	} else {
		for (i = 0; i < ARRAY_SIZE(board_strs); i++) {
			set_cursor_pos(BOARD_ROW+i, BOARD_COL);
			print(strlen(board_strs[i]), board_strs[i]);
		}
	}
	set_text_color(WHITE_COLOR);
	draw_pieces(board, WHITE);
	set_text_color(BLACK_COLOR);
	draw_pieces(board, BLACK);
}

enum player_mode {
	bot_black = 0,
	bot_white = 1,
	human = 2,
};
char *player_strs[] = {
	"you vs bistromath",
	"bistromath vs you",
	" you vs yourself ",
};
#define PLAYERSTR_ROW 1
#define PLAYERSTR_COL 14
#define PLAYERSTR_COLOR (FGND_BBLUE | BGND_BLACK)

enum player_mode player_mode = bot_black;

void draw_player_str() {
	set_text_color(PLAYERSTR_COLOR);
	set_cursor_pos(PLAYERSTR_ROW, PLAYERSTR_COL);
	assert(player_mode < ARRAY_SIZE(player_strs));
	print(strlen(player_strs[player_mode]),
	             player_strs[player_mode]);
}

#define STATUS_WHITEMOVE   "              [white to move]              "
#define STATUS_BLACKMOVE   "              [black to move]              "
#define STATUS_WHITEMATE   "          [0-1  white checkmated]          "
#define STATUS_BLACKMATE   "          [1-0  black checkmated]          "
#define STATUS_WHITERESIGN "            [0-1 white resigns]            "
#define STATUS_BLACKRESIGN "            [1-0 black resigns]            "
#define STATUS_DRAW3MOVE   " [1/2-1/2 game drawn by 3-fold repetition] "
#define STATUS_DRAW50MOVE  "   [1/2-1/2  game drawn by 50 move rule]   "
#define STATUS_DRAWSTALE   "     [1/2-1/2 game drawn by stalemate]     "
#define STATUS_DRAWNOMATE  "[1/2-1/2  neither side has mating material]"

#define STATUS_ROW 23
#define STATUS_COL 1
#define STATUS_COLOR (FGND_BBLUE | BGND_BLACK)

char *status_str = STATUS_WHITEMOVE;

// gentleperson's agreement: msg must be one of the predefined ones above
void draw_status_str() {
	assert(strlen(status_str) == strlen(STATUS_WHITEMOVE));
	set_text_color(STATUS_COLOR);
	set_cursor_pos(STATUS_ROW, STATUS_COL);
	print(strlen(status_str), status_str);
}

/************************* CONSOLE LIBRARY ************************************/

char *helptext[] = {
	"Commands available:              ",
	"a2a4   - move (e.g.)             ", //
	"e7e8Q  - move with pawn promote  ", //
	"go     - switch sides; bot goes  ", //
	"human  - let two people play     ", //
	"flip   - turn the board around   ", //
	"new    - start a new game        ", //
	"quiet  - don't think out loud    ", //
	"loud   - think out loud          ", //
	"hint   - what the bot would do   ", //
	"undo   - take back a move        ", //
	"resign - give up                 ", //
	"depth  - set/show difficulty     ", //
	"time   - set search time limit   ",
	"book   - turn on/off opening book", //
	"credit - show credit info        ", //
	"help   - show this               ", //
	"quit   - quit                    ", //
	"Sorry no algebraic move syntax.  ",
};

char *credittext[] = {
	"\"Bistromath\" is (C) 2013 Ben Blum",
	"Originally written  2008        ",
	"Engine finished     2010        ",
	"Ported to Pebbles   2012        ",
	"Questions/bugs? bblum@cs.cmu.edu",
	"(type \"help\" for commands)    ",
};

#define CONSOLE_HEIGHT 25
#define CONSOLE_WIDTH 80

// these defines could be a little more flexible
#define TEXT_WIDTH 34 // custom console dimensions
#define TEXT_HEIGHT (CONSOLE_HEIGHT-2)
#define TEXT_ROW 0 // where the custom console's top left corner is
#define TEXT_COL (CONSOLE_WIDTH-TEXT_WIDTH)
char textconsole[TEXT_HEIGHT][TEXT_WIDTH];
#define PROMPT "> "
#define TEXT_COLOR (FGND_GREEN | BGND_BLACK)
#define PROMPT_COLOR (FGND_BGRN | BGND_BLACK)

void init_console() {
	int i, j;
	for (i = 0; i < TEXT_HEIGHT; i++) { for (j = 0; j < TEXT_WIDTH; j++) {
		textconsole[i][j] = ' ';
	}}
}

// Printing e.g. help text would be more efficient with a print n lines function
// Future work! At least draw_console is separated out.
void print_line(const char *buf, unsigned int len) {
	if (len > TEXT_WIDTH) len = TEXT_WIDTH;
	/* Scroll. */
	int i;
	for (i = 1; i < TEXT_HEIGHT; i++) {
		strncpy(textconsole[i-1], textconsole[i], TEXT_WIDTH);
	}
	/* Command prompt always stays on the bottom. */
	strncpy(textconsole[TEXT_HEIGHT-1], buf, len);
	/* Pad rest of line with whitespace */
	for (i = len; i < TEXT_WIDTH; i++) {
		textconsole[TEXT_HEIGHT-1][i] = ' ';
	}
}

// Flush, essentially
void draw_console() {
	int i;
	set_text_color(TEXT_COLOR);
	for (i = 0; i < TEXT_HEIGHT; i++) {
		set_cursor_pos(TEXT_ROW + i, TEXT_COL);
		print(TEXT_WIDTH, textconsole[i]);
	}
}
void draw_prompt() {
	char buf[TEXT_WIDTH+1]; // Trac ticket #157. ARGGH
	set_text_color(PROMPT_COLOR);
	set_cursor_pos(TEXT_ROW + TEXT_HEIGHT, TEXT_COL);
	assert(strlen(PROMPT) < TEXT_WIDTH);
	snprintf(buf, TEXT_WIDTH, "%s%*s", PROMPT, TEXT_WIDTH-strlen(PROMPT), " ");
	buf[TEXT_WIDTH-1] = 0; // Trac ticket #157. ARGGH
	print(TEXT_WIDTH, buf);
	set_cursor_pos(TEXT_ROW + TEXT_HEIGHT, TEXT_COL + strlen(PROMPT));
}


// for other modules of the engine to print
int outbuf_size = TEXT_WIDTH;
char outbuf[TEXT_WIDTH];
void output() { print_line(outbuf, TEXT_WIDTH); draw_console(); }

/**************************** MAIN ROUTINES ***********************************/

int text_loud = 1;

void help() {
	int i;
	for (i = 0; i < ARRAY_SIZE(helptext); i++) {
		print_line(helptext[i], strlen(helptext[i]));
	}
}
void credits() {
	int i;
	for (i = 0; i < ARRAY_SIZE(credittext); i++) {
		print_line(credittext[i], strlen(credittext[i]));
	}
}

void redraw(board_t *board) {
	// clear screen
	int i;
	char spaces[CONSOLE_WIDTH+1]; // Trac ticket #157. ARGGH
	snprintf(spaces, CONSOLE_WIDTH, "%*s", CONSOLE_WIDTH, " ");
	spaces[CONSOLE_WIDTH-1] = 0; // Trac ticket #157. ARGGH
	set_text_color(FGND_BLACK | BGND_BLACK); // erase existing bgnd
	for (i = 0; i < CONSOLE_HEIGHT; i++) {
		set_cursor_pos(i, 0);
		print(CONSOLE_WIDTH, spaces);
	}
	// draw everything
	draw_board(board);
	draw_player_str();
	draw_status_str();
	draw_console();
	draw_prompt();
}

engine_t *e;
int gameover = 0;

int checkgameover() {
	int matetype;
	if (e->board->halfmoves >= 100) {
		status_str = STATUS_DRAW50MOVE;
		return 1;
	} else if (board_threefold_draw(e->board)) {
		status_str = STATUS_DRAW3MOVE;
		return 1;
	}
	/* Resignation is handled elsewhere */
	matetype = board_mated(e->board);
	/* check if a player is mated */
	if (matetype == BOARD_CHECKMATED) {
		status_str = e->board->tomove == WHITE ? STATUS_WHITEMATE
		                                       : STATUS_BLACKMATE;
		return 1;
	} else if (matetype == BOARD_STALEMATED) {
		status_str = STATUS_DRAWSTALE;
		return 1;
	} else if ((e->board->material[BLACK] == 0 && // Both players have 0
		    e->board->material[WHITE] == 0) ||
		   (e->board->material[BLACK] == 0 && // B has 0, W has N or B
		    ((e->board->material[WHITE] == eval_piecevalue[BISHOP] &&
		      e->board->pos[WHITE][BISHOP] != BB(0x0)) ||
		     (e->board->material[WHITE] == eval_piecevalue[KNIGHT] &&
		      e->board->pos[WHITE][KNIGHT] != BB(0x0)))) ||
		   (e->board->material[WHITE] == 0 && // W has 0, B has N or B
		    ((e->board->material[BLACK] == eval_piecevalue[BISHOP] &&
		      e->board->pos[BLACK][BISHOP] != BB(0x0)) ||
		     (e->board->material[BLACK] == eval_piecevalue[KNIGHT] &&
		      e->board->pos[BLACK][KNIGHT] != BB(0x0))))) {
		status_str = STATUS_DRAWNOMATE;
		return 1;
	} else {
		status_str = e->board->tomove == WHITE ? STATUS_WHITEMOVE
		                                       : STATUS_BLACKMOVE;
		return 0;
	}
}

void makemove() {
	draw_prompt();
	// TODO: Spawn a separate thread that can interrupt... somehow?
	char *str = engine_generatemove(e);
	char *errmsg;
	int x = engine_applymove(e, str, &errmsg);
	assert(x);
	free(str);
}

#define INBUF_LEN (2*TEXT_WIDTH)
static inline void print_string(const char *s) { print_line(s, strlen(s)); }

int main() {
	thr_init(4096);
	e = engine_init(getbestmove);
	char inbuf[INBUF_LEN];
	char *nogame = "No game is in progress.";
	assert(e != NULL);
	{
		int i;
		init_console();
		credits();
		for (i = ARRAY_SIZE(credittext); i < TEXT_HEIGHT; i++) {
			print_line("", 0);
		}
		redraw(e->board);
	}
	while(1) {
		int ret, need_redraw = 0;
		char inbuf2[TEXT_WIDTH];
		memset(inbuf, 0, sizeof(inbuf));
		ret = readline(INBUF_LEN-1, inbuf);
		assert(ret >= 0);
		if (inbuf[strlen(inbuf)-1] == '\n') {
			inbuf[strlen(inbuf)-1] = '\0';
		}
		if (strlen(inbuf) > TEXT_WIDTH-strlen(PROMPT)) {
			need_redraw = 1;
			snprintf(inbuf + TEXT_WIDTH - strlen(PROMPT) - 3,
				 3, "...");
		}
		/* echo user command */
		snprintf(inbuf2, TEXT_WIDTH, "%s%s", PROMPT, inbuf);
		print_line(inbuf2, strlen(inbuf2));
		/* process command */
		if (0 == strcmp(inbuf, "quit") || 0 == strcmp(inbuf, "exit")) {
			set_cursor_pos(CONSOLE_HEIGHT, 0);
			engine_destroy(e);
			thr_exit(0);
			return 0;
		/* Misc management */
		} else if (0 == strcmp(inbuf, "help")) {
			help();
		} else if (0 == strcmp(inbuf, "credit")) {
			credits();
		} else if (0 == strcmp(inbuf, "loud")) {
			if (text_loud) {
				print_string("This is as loud as I can be!");
			} else {
				print_string("Will now think out loud.");
				text_loud = 1;
			}
		} else if (0 == strcmp(inbuf, "quiet")) {
			if (!text_loud) {
				print_string("You don't need to ask twice.");
			} else {
				print_string("OK, I'll keep it to myself.");
				text_loud = 0;
			}
		} else if (0 == strcmp(inbuf, "flip")) {
			flipped = !flipped;
			if (!need_redraw) draw_board(e->board);
		} else if (0 == strcmp(inbuf, "book")) {
			book_enabled = !book_enabled;
			if (book_enabled) {
				print_string("Opening book on.");
				e->inbook = 1;
			} else {
				print_string("Opening book off.");
			}
		/* Easter eggs */
		} else if (0 == strncmp(inbuf, "punch", 5) ||
			   0 == strncmp(inbuf, "kill", 4)) {
			print_string("Keep your mind on the game.");
		} else if (0 == strncmp(inbuf, "hug", 3)) {
			print_string("If you think that'll help.");
		} else if (0 == strncmp(inbuf, "look", 4) ||
			   0 == strcmp(inbuf, "north") || 0 == strcmp(inbuf, "n") ||
			   0 == strcmp(inbuf, "east")  || 0 == strcmp(inbuf, "e") ||
			   0 == strcmp(inbuf, "south") || 0 == strcmp(inbuf, "s") ||
			   0 == strcmp(inbuf, "west")  || 0 == strcmp(inbuf, "w")) {
			print_string("This isn't that kind of game.");
		} else if (0 == strcmp(inbuf, "inventory")) {
			print_string("Nothing. This isn't bughouse.");
		} else if (0 == strcmp(inbuf, "ls")) {
			print_string("I'll pretend you didn't say that.");
		} else if (0 == strcmp(inbuf, "xyzzy")) {
			xyzzy++;
			if (xyzzy == 0x10) xyzzy = 0;
			need_redraw = 1;
		/* Game management */
		} else if (0 == strcmp(inbuf, "resign")) {
			// Future feature: ask "are you sure?" (also on "new")
			if (gameover) {
				print_string(nogame);
			} else {
				print_string("I'm sorry it had to be that way.");
				gameover = 1;
				status_str = e->board->tomove == WHITE ?
					 STATUS_WHITERESIGN : STATUS_BLACKRESIGN;
				draw_status_str();
			}
		} else if (0 == strcmp(inbuf, "hint")) {
			trans_data_t trans_data = trans_get(e->board->hash);
			if (gameover) {
				print_string(nogame);
			} else if (trans_data_valid(trans_data)) {
				char *move = move_tostring(TRANS_MOVE(trans_data));
				static unsigned int hint_index = 0;
				const char *words[] =
					{ "Consider %s.", "Why not %s?",
					  "How do you feel about %s?",
					  "If it were me, I'd play %s." };
				snprintf(inbuf2, TEXT_WIDTH,
					 words[hint_index++ % ARRAY_SIZE(words)],
					 move);
				print_line(inbuf2, strlen(inbuf2));
				free(move);
			} else {
				print_string("You're on your own here.");
			}
		} else if (0 == strcmp(inbuf, "new")) {
			gameover = 0;
			flipped = 0;
			player_mode = bot_black;
			status_str = STATUS_WHITEMOVE;
			engine_destroy(e);
			e = engine_init(getbestmove);
			assert(e != NULL);
			need_redraw = 1;
		} else if (0 == strcmp(inbuf, "human")) {
			if (gameover) {
				print_string(nogame);
			} else {
				player_mode = human;
				if (!need_redraw) draw_player_str();
			}
		} else if (0 == strncmp(inbuf, "depth", 5)) {
			// See trac bug #73. Argghhhh.
			char *endp;
			unsigned int n = strtol(&inbuf[6], &endp, 0);
			assert(searcher_max_depth < SEARCHER_MAX_DEPTH);
			assert(searcher_max_depth >= SEARCHER_MIN_DEPTH);
			if (inbuf[5] != ' ' || endp == &inbuf[6]) {
				print_string("Use \"depth N\" to limit max depth.");
				snprintf(inbuf2, TEXT_WIDTH, "Currently %u (0 = no limit)",
					 searcher_max_depth == SEARCHER_MAX_DEPTH-1 ?
					 0 : searcher_max_depth);
				print_string(inbuf2);
			} else {
				if (n >= SEARCHER_MAX_DEPTH || n == 0)
					n = SEARCHER_MAX_DEPTH-1;
				else if (n < SEARCHER_MIN_DEPTH)
					n = SEARCHER_MIN_DEPTH;
				searcher_max_depth = n;
				print_string("OK.");
			}
		} else if (0 == strncmp(inbuf, "time", 4)) {
			// Argghhhh, as above.
			char *endp;
			unsigned int n = strtol(&inbuf[5], &endp, 0);
			assert(searchtime >= 1);
			assert(searchtime <= SEARCHTIME_MAX);
			if (inbuf[4] != ' ' || endp == &inbuf[5]) {
				print_string("Use \"time N\" to limit search time.");
				snprintf(inbuf2, TEXT_WIDTH, "Currently %u (max %u)",
					 searchtime, SEARCHTIME_MAX);
				print_string(inbuf2);
			} else {
				if (n > SEARCHTIME_MAX || n == 0)
					n = SEARCHTIME_DEFAULT;
				searchtime = n;
				print_string("OK.");
			}
		} else if (0 == strcmp(inbuf, "undo")) {
			const char *words1[] =
				{ "We're at the beginning already.",
				  "My memories are hazy before this.",
				  "Stop, you're making me dizzy.",
				  "Can't take tail of empty list." };
			static unsigned int undo_index = 0;
			const char *words2[] =
				{ "Be more careful next time.",
				  "I won't judge you; I'm a computer.",
				  "Sure, as if time travel is easy.",
				  "You're pushing your luck, kiddo.",
				  "Next you'll ask for lessons?",
				};
			static unsigned int undo_index2 = 0;
			if (e->board->moves == 0) {
				print_string(gameover ?
					"Just accept it and move on." :
					words1[undo_index++ % ARRAY_SIZE(words1)]);
			} else {
				engine_undomove(e);
				if (player_mode == bot_white)
					player_mode = bot_black;
				else if (player_mode == bot_black)
					player_mode = bot_white;
				flipped = !flipped;
				gameover = checkgameover(); // refresh status string
				assert(!gameover && "Game over after undo??");
				if (!need_redraw) draw_player_str();
				if (!need_redraw) draw_board(e->board);
				if (!need_redraw) draw_status_str();
				print_string(words2[undo_index2++ %
				                    ARRAY_SIZE(words2)]);
			}
		/* Moving */
		} else if (0 == strcmp(inbuf, "go")) {
			if (gameover) {
				print_string(nogame);
			} else {
				player_mode = e->board->tomove == WHITE ?
					bot_white : bot_black;
				flipped = !flipped;
				if (!need_redraw) draw_player_str();
				if (!need_redraw) draw_board(e->board);
				makemove();
				if (!need_redraw) draw_board(e->board);
				gameover = checkgameover();
				if (!need_redraw) draw_status_str();
			}
		} else if (!!move_fromstring(inbuf)) {
			char *errmsg = NULL;
			if (gameover) {
				print_string(nogame);
			} else if (engine_applymove(e, inbuf, &errmsg)) {
				if ((gameover = checkgameover())) {
					print_string("Good game!");
				} else if (player_mode != human) {
					/* Make a move in response. */
					if (!need_redraw) draw_status_str();
					if (!need_redraw) draw_board(e->board);
					makemove();
					if (!need_redraw) draw_board(e->board);
					if ((gameover = checkgameover())) {
						print_string("Good game!");
					}
				} else {
					flipped = !flipped;
					if (!need_redraw) draw_board(e->board);
				}
				if (!need_redraw) draw_status_str();
			} else {
				assert(errmsg != NULL);
				print_string(errmsg);
			}
		/* Fail */
		} else {
			print_string(strlen(inbuf) ? "I didn't understand that." :
				"You'll have to be more specific.");
		}
		/* Update display to reflect changes */
		if (need_redraw) {
			redraw(e->board);
		} else {
			draw_console();
			draw_prompt();
		}
	}
	assert(0);
	return 31337;
}
