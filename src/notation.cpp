/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2014 Marco Costalba, Joona Kiiski, Tord Romstad

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

#include <cassert>
#include <sstream>

#include "movegen.h"
#include "position.h"
#include "uci.h"

using namespace std;

static const char* PieceToChar[COLOR_NB] = { " PNBRQK", " pnbrqk" };


/// score_to_uci() converts a Value to a string suitable for use with the UCI
/// protocol specifications:
///
/// cp <x>     The score from the engine's point of view in centipawns.
/// mate <y>   Mate in y moves, not plies. If the engine is getting mated
///            use negative values for y.

string UCI::format_value(Value v, Value alpha, Value beta) {

  stringstream ss;

  if (abs(v) < VALUE_MATE_IN_MAX_PLY)
      ss << "cp " << v * 100 / PawnValueEg;
  else
      ss << "mate " << (v > 0 ? VALUE_MATE - v + 1 : -VALUE_MATE - v) / 2;

  ss << (v >= beta ? " lowerbound" : v <= alpha ? " upperbound" : "");

  return ss.str();
}


/// format_square() converts a Square to a string (g1, a7, etc.)

std::string UCI::format_square(Square s) {
  char ch[] = { 'a' + file_of(s), '1' + rank_of(s), 0 };
  return ch;
}


/// format_move() converts a Move to a string in coordinate notation
/// (g1f3, a7a8q, etc.). The only special case is castling moves, where we print
/// in the e1g1 notation in normal chess mode, and in e1h1 notation in chess960
/// mode. Internally castling moves are always encoded as "king captures rook".

string UCI::format_move(Move m, bool chess960) {

  Square from = from_sq(m);
  Square to = to_sq(m);

  if (m == MOVE_NONE)
      return "(none)";

  if (m == MOVE_NULL)
      return "0000";

  if (type_of(m) == CASTLING && !chess960)
      to = make_square(to > from ? FILE_G : FILE_C, rank_of(from));

  string move = format_square(from) + format_square(to);

  if (type_of(m) == PROMOTION)
      move += PieceToChar[BLACK][promotion_type(m)]; // Lower case

  return move;
}


/// to_move() takes a position and a string representing a move in
/// simple coordinate notation and returns an equivalent legal Move if any.

Move UCI::to_move(const Position& pos, string& str) {

  if (str.length() == 5) // Junior could send promotion piece in uppercase
      str[4] = char(tolower(str[4]));

  for (MoveList<LEGAL> it(pos); *it; ++it)
      if (str == format_move(*it, pos.is_chess960()))
          return *it;

  return MOVE_NONE;
}


///READDED
/// move_to_san() takes a position and a legal Move as input and returns its
/// short algebraic notation representation.

const string UCI::move_to_san(Position& pos, Move m) {

  if (m == MOVE_NONE)
      return "(none)";

  if (m == MOVE_NULL)
      return "(null)";

  assert(MoveList<LEGAL>(pos).contains(m));

  Bitboard others, b;
  string san;
  Color us = pos.side_to_move();
  Square from = from_sq(m);
  Square to = to_sq(m);
  Piece pc = pos.piece_on(from);
  PieceType pt = type_of(pc);

  if (type_of(m) == CASTLING)
      san = to > from ? "O-O" : "O-O-O";
  else
  {
      if (pt != PAWN)
      {
          san = PieceToChar[WHITE][pt]; // Upper case

          // A disambiguation occurs if we have more then one piece of type 'pt'
          // that can reach 'to' with a legal move.
          others = b = (pos.attacks_from(pc, to) & pos.pieces(us, pt)) ^ from;

          while (b)
          {
              Square s = pop_lsb(&b);
              if (!pos.legal(make_move(s, to), pos.pinned_pieces(us)))
                  others ^= s;
          }

          if (!others)
          { /* Disambiguation is not needed */ }

          else if (!(others & file_bb(from)))
              san += char(file_of(from) - FILE_A + 'a');

          else if (!(others & rank_bb(from)))
              san += char(rank_of(from) - RANK_1 + '1');

          else
              san += format_square(from);
      }
      else if (pos.capture(m))
          san = char(file_of(from) - FILE_A + 'a');

      if (pos.capture(m))
          san += 'x';

      san += format_square(to);

      if (type_of(m) == PROMOTION)
          san += string("=") + PieceToChar[WHITE][promotion_type(m)];
  }

  if (pos.gives_check(m, CheckInfo(pos)))
  {
      StateInfo st;
      pos.do_move(m, st);
      san += MoveList<LEGAL>(pos).size() ? "+" : "#";
      pos.undo_move(m);
  }

  return san;
}
