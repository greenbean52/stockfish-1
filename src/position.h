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


#if !defined(POSITION_H_INCLUDED)
#define POSITION_H_INCLUDED

// Disable some silly and noisy warning from MSVC compiler
#if defined(_MSC_VER)

// Forcing value to bool 'true' or 'false' (performance warning)
#pragma warning(disable: 4800)

// Conditional expression is constant
#pragma warning(disable: 4127)


#endif

////
//// Includes
////

#include "bitboard.h"
#include "color.h"
#include "direction.h"
#include "move.h"
#include "piece.h"
#include "square.h"
#include "value.h"


////
//// Constants
////

/// FEN string for the initial position
const std::string StartPosition = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

/// Maximum number of plies per game (220 should be enough, because the
/// maximum search depth is 100, and during position setup we reset the
/// move counter for every non-reversible move).
const int MaxGameLength = 220;


////
//// Types
////

/// Castle rights, encoded as bit fields

enum CastleRights {
  NO_CASTLES  = 0,
  WHITE_OO    = 1,
  BLACK_OO    = 2,
  WHITE_OOO   = 4,
  BLACK_OOO   = 8,
  ALL_CASTLES = 15
};

/// Game phase
enum Phase {
  PHASE_ENDGAME = 0,
  PHASE_MIDGAME = 128
};


/// The StateInfo struct stores information we need to restore a Position
/// object to its previous state when we retract a move. Whenever a move
/// is made on the board (by calling Position::do_move), an StateInfo object
/// must be passed as a parameter.

struct StateInfo {
  Key key, pawnKey, materialKey;
  int castleRights, rule50;
  Square epSquare;
  Value mgValue, egValue;
  Value npMaterial[2];

  PieceType capture;
  Bitboard checkersBB;
  StateInfo* previous;
};


/// The position data structure. A position consists of the following data:
///
///    * For each piece type, a bitboard representing the squares occupied
///      by pieces of that type.
///    * For each color, a bitboard representing the squares occupied by
///      pieces of that color.
///    * A bitboard of all occupied squares.
///    * A bitboard of all checking pieces.
///    * A 64-entry array of pieces, indexed by the squares of the board.
///    * The current side to move.
///    * Information about the castling rights for both sides.
///    * The initial files of the kings and both pairs of rooks. This is
///      used to implement the Chess960 castling rules.
///    * The en passant square (which is SQ_NONE if no en passant capture is
///      possible).
///    * The squares of the kings for both sides.
///    * Hash keys for the position itself, the current pawn structure, and
///      the current material situation.
///    * Hash keys for all previous positions in the game for detecting
///      repetition draws.
///    * A counter for detecting 50 move rule draws.

class Position {

  friend class MaterialInfo;
  friend class EndgameFunctions;

public:
  enum GamePhase {
      MidGame,
      EndGame
  };

  // Constructors
  Position() {};
  Position(const Position& pos);
  Position(const std::string& fen);

  // Text input/output
  void from_fen(const std::string& fen);
  const std::string to_fen() const;
  void print(Move m = MOVE_NONE) const;

  // Copying
  void copy(const Position& pos);
  void flipped_copy(const Position& pos);

  // The piece on a given square
  Piece piece_on(Square s) const;
  PieceType type_of_piece_on(Square s) const;
  Color color_of_piece_on(Square s) const;
  bool square_is_empty(Square s) const;
  bool square_is_occupied(Square s) const;
  Value midgame_value_of_piece_on(Square s) const;
  Value endgame_value_of_piece_on(Square s) const;

  // Side to move
  Color side_to_move() const;

  // Bitboard representation of the position
  Bitboard empty_squares() const;
  Bitboard occupied_squares() const;
  Bitboard pieces_of_color(Color c) const;
  Bitboard pieces_of_type(PieceType pt) const;
  Bitboard pieces_of_color_and_type(Color c, PieceType pt) const;
  Bitboard pawns() const;
  Bitboard knights() const;
  Bitboard bishops() const;
  Bitboard rooks() const;
  Bitboard queens() const;
  Bitboard kings() const;
  Bitboard rooks_and_queens() const;
  Bitboard bishops_and_queens() const;
  Bitboard sliders() const;
  Bitboard pawns(Color c) const;
  Bitboard knights(Color c) const;
  Bitboard bishops(Color c) const;
  Bitboard rooks(Color c) const;
  Bitboard queens(Color c) const;
  Bitboard kings(Color c) const;
  Bitboard rooks_and_queens(Color c) const;
  Bitboard bishops_and_queens(Color c) const;
  Bitboard sliders_of_color(Color c) const;

  // Number of pieces of each color and type
  int piece_count(Color c, PieceType pt) const;

  // The en passant square
  Square ep_square() const;

  // Current king position for each color
  Square king_square(Color c) const;

  // Castling rights
  bool can_castle_kingside(Color c) const;
  bool can_castle_queenside(Color c) const;
  bool can_castle(Color c) const;
  Square initial_kr_square(Color c) const;
  Square initial_qr_square(Color c) const;

  // Attack bitboards
  Bitboard sliding_attacks(Square s, Direction d) const;
  Bitboard ray_attacks(Square s, SignedDirection d) const;
  Bitboard pawn_attacks(Color c, Square s) const;

  template<PieceType>
  Bitboard piece_attacks(Square s) const;

  // Bitboards for pinned pieces and discovered check candidates
  Bitboard discovered_check_candidates(Color c) const;
  Bitboard pinned_pieces(Color c, Bitboard& p) const;
  Bitboard pinned_pieces(Color c) const;

  // Checking pieces
  Bitboard checkers() const;

  // Piece lists
  Square piece_list(Color c, PieceType pt, int index) const;

  // Attack information for a given square
  bool square_is_attacked(Square s, Color c) const;
  Bitboard attacks_to(Square s) const;
  Bitboard attacks_to(Square s, Color c) const;
  bool is_check() const;
  bool pawn_attacks_square(Color c, Square f, Square t) const;

  template<PieceType>
  Bitboard piece_attacks_square(Square f, Square t) const; // Dispatch at compile-time

  bool piece_attacks_square(Piece p, Square f, Square t) const; // Dispatch at run-time

  // Properties of moves
  bool pl_move_is_legal(Move m) const;
  bool pl_move_is_legal(Move m, Bitboard pinned) const;
  bool move_is_check(Move m) const;
  bool move_is_check(Move m, Bitboard dcCandidates) const;
  bool move_is_capture(Move m) const;
  bool move_is_deep_pawn_push(Move m) const;
  bool move_is_pawn_push_to_7th(Move m) const;
  bool move_is_passed_pawn_push(Move m) const;
  bool move_was_passed_pawn_push(Move m) const;
  bool move_attacks_square(Move m, Square s) const;

  // Information about pawns
  bool pawn_is_passed(Color c, Square s) const;
  static bool pawn_is_passed(Bitboard theirPawns, Color c, Square s);
  static bool pawn_is_isolated(Bitboard ourPawns, Square s);
  static bool pawn_is_doubled(Bitboard ourPawns, Color c, Square s);

  // Open and half-open files
  static bool file_is_open(Bitboard pawns, File f);
  static bool file_is_half_open(Bitboard pawns, File f);

  // Weak squares
  bool square_is_weak(Square s, Color c) const;

  // Doing and undoing moves
  void saveState();
  void do_move(Move m, StateInfo& st);
  void do_move(Move m, StateInfo& st, Bitboard dcCandidates);
  void undo_move(Move m);
  void do_null_move(StateInfo& st);
  void undo_null_move();

  // Static exchange evaluation
  int see(Square from, Square to) const;
  int see(Move m) const;
  int see(Square to) const;

  // Accessing hash keys
  Key get_key() const;
  Key get_pawn_key() const;
  Key get_material_key() const;

  // Incremental evaluation
  Value mg_value() const;
  Value eg_value() const;
  Value non_pawn_material(Color c) const;
  Phase game_phase() const;
  template<GamePhase> Value pst_delta(Piece piece, Square from, Square to) const;

  // Game termination checks
  bool is_mate() const;
  bool is_draw() const;

  // Check if one side threatens a mate in one
  bool has_mate_threat(Color c);

  // Number of plies since the last non-reversible move
  int rule_50_counter() const;

  // Other properties of the position
  bool opposite_colored_bishops() const;
  bool has_pawn_on_7th(Color c) const;

  // Reset the gamePly variable to 0
  void reset_game_ply();

  // Position consistency check, for debugging
  bool is_ok(int* failedStep = NULL) const;

  // Static member functions:
  static void init_zobrist();
  static void init_piece_square_tables();

private:

  // Initialization helper functions (used while setting up a position)
  void clear();
  void put_piece(Piece p, Square s);
  void allow_oo(Color c);
  void allow_ooo(Color c);

  // Helper functions for doing and undoing moves
  void do_capture_move(PieceType capture, Color them, Square to);
  void do_castle_move(Move m);
  void do_promotion_move(Move m);
  void do_ep_move(Move m);
  void undo_castle_move(Move m);
  void undo_promotion_move(Move m);
  void undo_ep_move(Move m);
  void find_checkers();

  template<PieceType Piece>
  void update_checkers(Bitboard* pCheckersBB, Square ksq, Square from, Square to, Bitboard dcCandidates);

  template<bool FindPinned>
  Bitboard hidden_checkers(Color c) const;

  // Computing hash keys from scratch (for initialization and debugging)
  Key compute_key() const;
  Key compute_pawn_key() const;
  Key compute_material_key() const;

  // Computing incremental evaluation scores and material counts
  template<GamePhase> Value pst(Color c, PieceType pt, Square s) const;
  template<GamePhase> Value compute_value() const;
  Value compute_non_pawn_material(Color c) const;

  // Bitboards
  Bitboard byColorBB[2], byTypeBB[8];

  // Board
  Piece board[64];

  // Piece counts
  int pieceCount[2][8]; // [color][pieceType]

  // Piece lists
  Square pieceList[2][8][16]; // [color][pieceType][index]
  int index[64];

  // Other info
  Square kingSquare[2];
  Color sideToMove;
  int gamePly;
  Key history[MaxGameLength];
  File initialKFile, initialKRFile, initialQRFile;
  StateInfo startState;
  StateInfo* st;

  // Static variables
  static int castleRightsMask[64];
  static Key zobrist[2][8][64];
  static Key zobEp[64];
  static Key zobCastle[16];
  static Key zobMaterial[2][8][16];
  static Key zobSideToMove;
  static Value MgPieceSquareTable[16][64];
  static Value EgPieceSquareTable[16][64];
};


////
//// Inline functions
////

inline Piece Position::piece_on(Square s) const {
  return board[s];
}

inline Color Position::color_of_piece_on(Square s) const {
  return color_of_piece(piece_on(s));
}

inline PieceType Position::type_of_piece_on(Square s) const {
  return type_of_piece(piece_on(s));
}

inline bool Position::square_is_empty(Square s) const {
  return piece_on(s) == EMPTY;
}

inline bool Position::square_is_occupied(Square s) const {
  return !square_is_empty(s);
}

inline Value Position::midgame_value_of_piece_on(Square s) const {
  return piece_value_midgame(piece_on(s));
}

inline Value Position::endgame_value_of_piece_on(Square s) const {
  return piece_value_endgame(piece_on(s));
}

inline Color Position::side_to_move() const {
  return sideToMove;
}

inline Bitboard Position::occupied_squares() const {
  return byTypeBB[0];
}

inline Bitboard Position::empty_squares() const {
  return ~(occupied_squares());
}

inline Bitboard Position::pieces_of_color(Color c) const {
  return byColorBB[c];
}

inline Bitboard Position::pieces_of_type(PieceType pt) const {
  return byTypeBB[pt];
}

inline Bitboard Position::pieces_of_color_and_type(Color c, PieceType pt) const {
  return pieces_of_color(c) & pieces_of_type(pt);
}

inline Bitboard Position::pawns() const {
  return pieces_of_type(PAWN);
}

inline Bitboard Position::knights() const {
  return pieces_of_type(KNIGHT);
}

inline Bitboard Position::bishops() const {
  return pieces_of_type(BISHOP);
}

inline Bitboard Position::rooks() const {
  return pieces_of_type(ROOK);
}

inline Bitboard Position::queens() const {
  return pieces_of_type(QUEEN);
}

inline Bitboard Position::kings() const {
  return pieces_of_type(KING);
}

inline Bitboard Position::rooks_and_queens() const {
  return rooks() | queens();
}

inline Bitboard Position::bishops_and_queens() const {
  return bishops() | queens();
}

inline Bitboard Position::sliders() const {
  return bishops() | queens() | rooks();
}

inline Bitboard Position::pawns(Color c) const {
  return pieces_of_color_and_type(c, PAWN);
}

inline Bitboard Position::knights(Color c) const {
  return pieces_of_color_and_type(c, KNIGHT);
}

inline Bitboard Position::bishops(Color c) const {
  return pieces_of_color_and_type(c, BISHOP);
}

inline Bitboard Position::rooks(Color c) const {
  return pieces_of_color_and_type(c, ROOK);
}

inline Bitboard Position::queens(Color c) const {
  return pieces_of_color_and_type(c, QUEEN);
}

inline Bitboard Position::kings(Color c) const {
  return pieces_of_color_and_type(c, KING);
}

inline Bitboard Position::rooks_and_queens(Color c) const {
  return rooks_and_queens() & pieces_of_color(c);
}

inline Bitboard Position::bishops_and_queens(Color c) const {
  return bishops_and_queens() & pieces_of_color(c);
}

inline int Position::piece_count(Color c, PieceType pt) const {
  return pieceCount[c][pt];
}

inline Square Position::piece_list(Color c, PieceType pt, int index) const {
  return pieceList[c][pt][index];
}

inline Square Position::ep_square() const {
  return st->epSquare;
}

inline Square Position::king_square(Color c) const {
  return kingSquare[c];
}

inline bool Position::can_castle_kingside(Color side) const {
  return st->castleRights & (1+int(side));
}

inline bool Position::can_castle_queenside(Color side) const {
  return st->castleRights & (4+4*int(side));
}

inline bool Position::can_castle(Color side) const {
  return can_castle_kingside(side) || can_castle_queenside(side);
}

inline Square Position::initial_kr_square(Color c) const {
  return relative_square(c, make_square(initialKRFile, RANK_1));
}

inline Square Position::initial_qr_square(Color c) const {
  return relative_square(c, make_square(initialQRFile, RANK_1));
}

inline Bitboard Position::pawn_attacks(Color c, Square s) const {
  return StepAttackBB[piece_of_color_and_type(c, PAWN)][s];
}

template<PieceType Piece> // Knight and King
inline Bitboard Position::piece_attacks(Square s) const {
  return StepAttackBB[Piece][s];
}

template<>
inline Bitboard Position::piece_attacks<PAWN>(Square s) const {
  return StepAttackBB[piece_of_color_and_type(opposite_color(sideToMove), PAWN)][s];
}

template<>
inline Bitboard Position::piece_attacks<BISHOP>(Square s) const {
  return bishop_attacks_bb(s, occupied_squares());
}

template<>
inline Bitboard Position::piece_attacks<ROOK>(Square s) const {
  return rook_attacks_bb(s, occupied_squares());
}

template<>
inline Bitboard Position::piece_attacks<QUEEN>(Square s) const {
  return piece_attacks<ROOK>(s) | piece_attacks<BISHOP>(s);
}

inline Bitboard Position::checkers() const {
  return st->checkersBB;
}

inline bool Position::is_check() const {
  return st->checkersBB != EmptyBoardBB;
}

inline bool Position::pawn_attacks_square(Color c, Square f, Square t) const {
  return bit_is_set(pawn_attacks(c, f), t);
}

template<PieceType Piece>
Bitboard Position::piece_attacks_square(Square f, Square t) const {
  return bit_is_set(piece_attacks<Piece>(f), t);
}

inline Bitboard Position::attacks_to(Square s, Color c) const {

  return attacks_to(s) & pieces_of_color(c);
}

inline bool Position::square_is_attacked(Square s, Color c) const {

  return attacks_to(s, c) != EmptyBoardBB;
}

inline bool Position::pawn_is_passed(Color c, Square s) const {
  return !(pawns(opposite_color(c)) & passed_pawn_mask(c, s));
}

inline bool Position::pawn_is_passed(Bitboard theirPawns, Color c, Square s) {
  return !(theirPawns & passed_pawn_mask(c, s));
}

inline bool Position::pawn_is_isolated(Bitboard ourPawns, Square s) {
  return !(ourPawns & neighboring_files_bb(s));
}

inline bool Position::pawn_is_doubled(Bitboard ourPawns, Color c, Square s) {
  return ourPawns & squares_behind(c, s);
}

inline bool Position::file_is_open(Bitboard pawns, File f) {
  return !(pawns & file_bb(f));
}

inline bool Position::file_is_half_open(Bitboard pawns, File f) {
  return !(pawns & file_bb(f));
}

inline bool Position::square_is_weak(Square s, Color c) const {
  return !(pawns(c) & outpost_mask(opposite_color(c), s));
}

inline Key Position::get_key() const {
  return st->key;
}

inline Key Position::get_pawn_key() const {
  return st->pawnKey;
}

inline Key Position::get_material_key() const {
  return st->materialKey;
}

template<Position::GamePhase Ph>
inline Value Position::pst(Color c, PieceType pt, Square s) const {
  return (Ph == MidGame ? MgPieceSquareTable[piece_of_color_and_type(c, pt)][s]
                        : EgPieceSquareTable[piece_of_color_and_type(c, pt)][s]);
}

template<Position::GamePhase Ph>
inline Value Position::pst_delta(Piece piece, Square from, Square to) const {
  return (Ph == MidGame ? MgPieceSquareTable[piece][to] - MgPieceSquareTable[piece][from]
                        : EgPieceSquareTable[piece][to] - EgPieceSquareTable[piece][from]);
}

inline Value Position::mg_value() const {
  return st->mgValue;
}

inline Value Position::eg_value() const {
  return st->egValue;
}

inline Value Position::non_pawn_material(Color c) const {
  return st->npMaterial[c];
}

inline Phase Position::game_phase() const {

  // Values modified by Joona Kiiski
  static const Value MidgameLimit = Value(15713);
  static const Value EndgameLimit = Value(4428);

  Value npm = non_pawn_material(WHITE) + non_pawn_material(BLACK);

  if (npm >= MidgameLimit)
      return PHASE_MIDGAME;
  else if(npm <= EndgameLimit)
      return PHASE_ENDGAME;
  else
      return Phase(((npm - EndgameLimit) * 128) / (MidgameLimit - EndgameLimit));
}

inline bool Position::move_is_deep_pawn_push(Move m) const {

  Color c = side_to_move();
  return   piece_on(move_from(m)) == piece_of_color_and_type(c, PAWN)
        && relative_rank(c, move_to(m)) > RANK_4;
}

inline bool Position::move_is_pawn_push_to_7th(Move m) const {

  Color c = side_to_move();
  return   piece_on(move_from(m)) == piece_of_color_and_type(c, PAWN)
        && relative_rank(c, move_to(m)) == RANK_7;
}

inline bool Position::move_is_passed_pawn_push(Move m) const {

  Color c = side_to_move();
  return   piece_on(move_from(m)) == piece_of_color_and_type(c, PAWN)
        && pawn_is_passed(c, move_to(m));
}

inline bool Position::move_was_passed_pawn_push(Move m) const {

  Color c = opposite_color(side_to_move());
  return   piece_on(move_to(m)) == piece_of_color_and_type(c, PAWN)
        && pawn_is_passed(c, move_to(m));
}

inline int Position::rule_50_counter() const {

  return st->rule50;
}

inline bool Position::opposite_colored_bishops() const {

  return   piece_count(WHITE, BISHOP) == 1
        && piece_count(BLACK, BISHOP) == 1
        && square_color(piece_list(WHITE, BISHOP, 0)) != square_color(piece_list(BLACK, BISHOP, 0));
}

inline bool Position::has_pawn_on_7th(Color c) const {

  return pawns(c) & relative_rank_bb(c, RANK_7);
}

inline bool Position::move_is_capture(Move m) const {

  // Move must not be MOVE_NONE !

  return (!square_is_empty(move_to(m)) && !move_is_castle(m)) || move_is_ep(m);
}

#endif // !defined(POSITION_H_INCLUDED)
