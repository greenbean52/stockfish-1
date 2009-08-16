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


////
//// Includes
////

#include <cassert>
#include <sstream>
#include <map>

#include "material.h"

using std::string;

////
//// Local definitions
////

namespace {

  // Values modified by Joona Kiiski
  const Value BishopPairMidgameBonus = Value(109);
  const Value BishopPairEndgameBonus = Value(97);

  Key KNNKMaterialKey, KKNNMaterialKey;

}

////
//// Classes
////


/// See header for a class description. It is declared here to avoid
/// to include <map> in the header file.

class EndgameFunctions {

public:
  EndgameFunctions();
  EndgameEvaluationFunctionBase* getEEF(Key key) const;
  EndgameScalingFunctionBase* getESF(Key key, Color* c) const;

private:
  void add(const string& keyCode, EndgameEvaluationFunctionBase* f);
  void add(const string& keyCode, Color c, EndgameScalingFunctionBase* f);
  Key buildKey(const string& keyCode);

  struct ScalingInfo
  {
      Color col;
      EndgameScalingFunctionBase* fun;
  };

  std::map<Key, EndgameEvaluationFunctionBase*> EEFmap;
  std::map<Key, ScalingInfo> ESFmap;
};


////
//// Functions
////


/// Constructor for the MaterialInfoTable class

MaterialInfoTable::MaterialInfoTable(unsigned int numOfEntries) {

  size = numOfEntries;
  entries = new MaterialInfo[size];
  funcs = new EndgameFunctions();
  if (!entries || !funcs)
  {
      std::cerr << "Failed to allocate " << (numOfEntries * sizeof(MaterialInfo))
                << " bytes for material hash table." << std::endl;
      Application::exit_with_failure();
  }
}


/// Destructor for the MaterialInfoTable class

MaterialInfoTable::~MaterialInfoTable() {

  delete funcs;
  delete [] entries;
}


/// MaterialInfoTable::get_material_info() takes a position object as input,
/// computes or looks up a MaterialInfo object, and returns a pointer to it.
/// If the material configuration is not already present in the table, it
/// is stored there, so we don't have to recompute everything when the
/// same material configuration occurs again.

MaterialInfo* MaterialInfoTable::get_material_info(const Position& pos) {

  Key key = pos.get_material_key();
  int index = key & (size - 1);
  MaterialInfo* mi = entries + index;

  // If mi->key matches the position's material hash key, it means that we
  // have analysed this material configuration before, and we can simply
  // return the information we found the last time instead of recomputing it.
  if (mi->key == key)
      return mi;

  // Clear the MaterialInfo object, and set its key
  mi->clear();
  mi->key = key;

  // A special case before looking for a specialized evaluation function
  // KNN vs K is a draw.
  if (key == KNNKMaterialKey || key == KKNNMaterialKey)
  {
      mi->factor[WHITE] = mi->factor[BLACK] = 0;
      return mi;
  }

  // Let's look if we have a specialized evaluation function for this
  // particular material configuration. First we look for a fixed
  // configuration one, then a generic one if previous search failed.
  if ((mi->evaluationFunction = funcs->getEEF(key)) != NULL)
      return mi;

  else if (   pos.non_pawn_material(BLACK) == Value(0)
           && pos.piece_count(BLACK, PAWN) == 0
           && pos.non_pawn_material(WHITE) >= RookValueMidgame)
  {
      mi->evaluationFunction = &EvaluateKXK;
      return mi;
  }
  else if (   pos.non_pawn_material(WHITE) == Value(0)
           && pos.piece_count(WHITE, PAWN) == 0
           && pos.non_pawn_material(BLACK) >= RookValueMidgame)
  {
      mi->evaluationFunction = &EvaluateKKX;
      return mi;
  }
  else if (   pos.pawns() == EmptyBoardBB
           && pos.rooks() == EmptyBoardBB
           && pos.queens() == EmptyBoardBB)
  {
      // Minor piece endgame with at least one minor piece per side,
      // and no pawns.
      assert(pos.knights(WHITE) | pos.bishops(WHITE));
      assert(pos.knights(BLACK) | pos.bishops(BLACK));

      if (   pos.piece_count(WHITE, BISHOP) + pos.piece_count(WHITE, KNIGHT) <= 2
          && pos.piece_count(BLACK, BISHOP) + pos.piece_count(BLACK, KNIGHT) <= 2)
      {
          mi->evaluationFunction = &EvaluateKmmKm;
          return mi;
      }
  }

  // OK, we didn't find any special evaluation function for the current
  // material configuration. Is there a suitable scaling function?
  //
  // The code below is rather messy, and it could easily get worse later,
  // if we decide to add more special cases.  We face problems when there
  // are several conflicting applicable scaling functions and we need to
  // decide which one to use.
  Color c;
  EndgameScalingFunctionBase* sf;

  if ((sf = funcs->getESF(key, &c)) != NULL)
  {
      mi->scalingFunction[c] = sf;
      return mi;
  }

  if (   pos.non_pawn_material(WHITE) == BishopValueMidgame
      && pos.piece_count(WHITE, BISHOP) == 1
      && pos.piece_count(WHITE, PAWN) >= 1)
      mi->scalingFunction[WHITE] = &ScaleKBPK;

  if (   pos.non_pawn_material(BLACK) == BishopValueMidgame
      && pos.piece_count(BLACK, BISHOP) == 1
      && pos.piece_count(BLACK, PAWN) >= 1)
      mi->scalingFunction[BLACK] = &ScaleKKBP;

  if (   pos.piece_count(WHITE, PAWN) == 0
      && pos.non_pawn_material(WHITE) == QueenValueMidgame
      && pos.piece_count(WHITE, QUEEN) == 1
      && pos.piece_count(BLACK, ROOK) == 1
      && pos.piece_count(BLACK, PAWN) >= 1)
      mi->scalingFunction[WHITE] = &ScaleKQKRP;

  else if (   pos.piece_count(BLACK, PAWN) == 0
           && pos.non_pawn_material(BLACK) == QueenValueMidgame
           && pos.piece_count(BLACK, QUEEN) == 1
           && pos.piece_count(WHITE, ROOK) == 1
           && pos.piece_count(WHITE, PAWN) >= 1)
      mi->scalingFunction[BLACK] = &ScaleKRPKQ;

  if (pos.non_pawn_material(WHITE) + pos.non_pawn_material(BLACK) == Value(0))
  {
      if (pos.piece_count(BLACK, PAWN) == 0)
      {
          assert(pos.piece_count(WHITE, PAWN) >= 2);
          mi->scalingFunction[WHITE] = &ScaleKPsK;
      }
      else if (pos.piece_count(WHITE, PAWN) == 0)
      {
          assert(pos.piece_count(BLACK, PAWN) >= 2);
          mi->scalingFunction[BLACK] = &ScaleKKPs;
      }
      else if (pos.piece_count(WHITE, PAWN) == 1 && pos.piece_count(BLACK, PAWN) == 1)
      {
          mi->scalingFunction[WHITE] = &ScaleKPKPw;
          mi->scalingFunction[BLACK] = &ScaleKPKPb;
      }
  }

  // Compute the space weight
  if (pos.non_pawn_material(WHITE) + pos.non_pawn_material(BLACK) >=
      2*QueenValueMidgame + 4*RookValueMidgame + 2*KnightValueMidgame)
  {
      int minorPieceCount =  pos.piece_count(WHITE, KNIGHT)
                           + pos.piece_count(BLACK, KNIGHT)
                           + pos.piece_count(WHITE, BISHOP)
                           + pos.piece_count(BLACK, BISHOP);

      mi->spaceWeight = minorPieceCount * minorPieceCount;
  }

  // Evaluate the material balance

  int sign;
  Value egValue = Value(0);
  Value mgValue = Value(0);

  for (c = WHITE, sign = 1; c <= BLACK; c++, sign = -sign)
  {
    // No pawns makes it difficult to win, even with a material advantage
    if (   pos.piece_count(c, PAWN) == 0
        && pos.non_pawn_material(c) - pos.non_pawn_material(opposite_color(c)) <= BishopValueMidgame)
    {
        if (   pos.non_pawn_material(c) == pos.non_pawn_material(opposite_color(c))
            || pos.non_pawn_material(c) < RookValueMidgame)
            mi->factor[c] = 0;
        else
        {
            switch (pos.piece_count(c, BISHOP)) {
            case 2:
                mi->factor[c] = 32;
                break;
            case 1:
                mi->factor[c] = 12;
                break;
            case 0:
                mi->factor[c] = 6;
                break;
            }
        }
    }

    // Bishop pair
    if (pos.piece_count(c, BISHOP) >= 2)
    {
        mgValue += sign * BishopPairMidgameBonus;
        egValue += sign * BishopPairEndgameBonus;
    }

    // Knights are stronger when there are many pawns on the board.  The
    // formula is taken from Larry Kaufman's paper "The Evaluation of Material
    // Imbalances in Chess":
    // http://mywebpages.comcast.net/danheisman/Articles/evaluation_of_material_imbalance.htm
    mgValue += sign * Value(pos.piece_count(c, KNIGHT)*(pos.piece_count(c, PAWN)-5)*16);
    egValue += sign * Value(pos.piece_count(c, KNIGHT)*(pos.piece_count(c, PAWN)-5)*16);

    // Redundancy of major pieces, again based on Kaufman's paper:
    if (pos.piece_count(c, ROOK) >= 1)
    {
        Value v = Value((pos.piece_count(c, ROOK) - 1) * 32 + pos.piece_count(c, QUEEN) * 16);
        mgValue -= sign * v;
        egValue -= sign * v;
    }
  }
  mi->mgValue = int16_t(mgValue);
  mi->egValue = int16_t(egValue);
  return mi;
}


/// EndgameFunctions member definitions. This class is used to store the maps
/// of end game and scaling functions that MaterialInfoTable will query for
/// each key. The maps are constant and are populated only at construction,
/// but are per-thread instead of globals to avoid expensive locks.

EndgameFunctions::EndgameFunctions() {

  KNNKMaterialKey = buildKey("KNNK");
  KKNNMaterialKey = buildKey("KKNN");

  add("KPK",   &EvaluateKPK);
  add("KKP",   &EvaluateKKP);
  add("KBNK",  &EvaluateKBNK);
  add("KKBN",  &EvaluateKKBN);
  add("KRKP",  &EvaluateKRKP);
  add("KPKR",  &EvaluateKPKR);
  add("KRKB",  &EvaluateKRKB);
  add("KBKR",  &EvaluateKBKR);
  add("KRKN",  &EvaluateKRKN);
  add("KNKR",  &EvaluateKNKR);
  add("KQKR",  &EvaluateKQKR);
  add("KRKQ",  &EvaluateKRKQ);
  add("KBBKN", &EvaluateKBBKN);
  add("KNKBB", &EvaluateKNKBB);

  add("KNPK",    WHITE, &ScaleKNPK);
  add("KKNP",    BLACK, &ScaleKKNP);
  add("KRPKR",   WHITE, &ScaleKRPKR);
  add("KRKRP",   BLACK, &ScaleKRKRP);
  add("KBPKB",   WHITE, &ScaleKBPKB);
  add("KBKBP",   BLACK, &ScaleKBKBP);
  add("KBPPKB",  WHITE, &ScaleKBPPKB);
  add("KBKBPP",  BLACK, &ScaleKBKBPP);
  add("KBPKN",   WHITE, &ScaleKBPKN);
  add("KNKBP",   BLACK, &ScaleKNKBP);
  add("KRPPKRP", WHITE, &ScaleKRPPKRP);
  add("KRPKRPP", BLACK, &ScaleKRPKRPP);
  add("KRPPKRP", WHITE, &ScaleKRPPKRP);
  add("KRPKRPP", BLACK, &ScaleKRPKRPP);
}

Key EndgameFunctions::buildKey(const string& keyCode) {

    assert(keyCode.length() > 0 && keyCode[0] == 'K');
    assert(keyCode.length() < 8);

    std::stringstream s;
    bool upcase = false;

    // Build up a fen substring with the given pieces, note
    // that the fen string could be of an illegal position.
    for (size_t i = 0; i < keyCode.length(); i++)
    {
        if (keyCode[i] == 'K')
            upcase = !upcase;

        s << char(upcase? toupper(keyCode[i]) : tolower(keyCode[i]));
    }
    s << 8 - keyCode.length() << "/8/8/8/8/8/8/8 w -";
    return Position(s.str()).get_material_key();
}

void EndgameFunctions::add(const string& keyCode, EndgameEvaluationFunctionBase* f) {

  EEFmap.insert(std::pair<Key, EndgameEvaluationFunctionBase*>(buildKey(keyCode), f));
}

void EndgameFunctions::add(const string& keyCode, Color c, EndgameScalingFunctionBase* f) {

  ScalingInfo s = {c, f};
  ESFmap.insert(std::pair<Key, ScalingInfo>(buildKey(keyCode), s));
}

EndgameEvaluationFunctionBase* EndgameFunctions::getEEF(Key key) const {

  std::map<Key, EndgameEvaluationFunctionBase*>::const_iterator it(EEFmap.find(key));
  return (it != EEFmap.end() ? it->second : NULL);
}

EndgameScalingFunctionBase* EndgameFunctions::getESF(Key key, Color* c) const {

  std::map<Key, ScalingInfo>::const_iterator it(ESFmap.find(key));
  if (it == ESFmap.end())
      return NULL;

  *c = it->second.col;
  return it->second.fun;
}
