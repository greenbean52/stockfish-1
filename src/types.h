/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2009 Marco Costalba

  Stockfish is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Stockfish is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#if !defined(TYPES_H_INCLUDED)
#define TYPES_H_INCLUDED

#if !defined(_MSC_VER)

#include <inttypes.h>

#else

typedef __int8 int8_t;
typedef unsigned __int8 uint8_t;
typedef __int16 int16;
typedef unsigned __int16 uint16_t;
typedef __int32 int32;
typedef unsigned __int32 uint32_t;
typedef __int64 int64;
typedef unsigned __int64 uint64_t;

typedef __int16 int16_t;
typedef __int64 int64_t;

#endif // !defined(_MSC_VER)

// Hash keys
typedef uint64_t Key;

// Bitboard type
typedef uint64_t Bitboard;


////
//// Compiler specific defines
////

// Quiet a warning on Intel compiler
#if !defined(__SIZEOF_INT__ )
#define __SIZEOF_INT__ 0
#endif

// Check for 64 bits for different compilers: Intel, MSVC and gcc
#if defined(__x86_64) || defined(_M_X64) || defined(_WIN64) || (__SIZEOF_INT__ > 4)
#define IS_64BIT
#endif

#if defined(IS_64BIT) && !defined(_WIN64) && (defined(__GNUC__) || defined(__INTEL_COMPILER))
#define USE_BSFQ
#endif

#endif // !defined(TYPES_H_INCLUDED)
