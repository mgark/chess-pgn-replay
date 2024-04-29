/*
 * Copyright(c) 2024-present Mykola Garkusha.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>

inline auto& all_possible_pieces()
{
  static std::unordered_set<char> pieces{'P', 'N', 'B', 'R', 'Q', 'K'};
  return pieces;
}

enum class TerminationMarker
{
  MANUAL,
  WHITE_WON,
  BLAKC_WON,
  EVEN
};

struct Coordinates
{
  std::optional<int> x;
  std::optional<int> y;

  bool operator==(const Coordinates& other) const { return x == other.x && y == other.y; }
};

struct CoordinatesHash
{
  size_t operator()(const Coordinates& c) const { return *c.y ^ *c.x; }
};

struct NextMove
{
  char piece = '\0';
  bool is_white_move;
  bool capture = false;
  bool check = false;
  bool checkmate = false;
  Coordinates src;
  Coordinates dst;
  std::optional<char> promote_piece;
  std::string orig_token;
};

struct KingCastling
{
  bool is_white_move;
};

struct QueenCastling
{
  bool is_white_move;
};

struct Finish
{
  TerminationMarker marker = TerminationMarker::MANUAL;
};

struct Ignore
{
};

using Moves = std::variant<KingCastling, QueenCastling, NextMove, Finish, Ignore>;

inline std::ostream& operator<<(std::ostream& o, const Moves& val)
{
  std::visit(
    overloaded{[&](const KingCastling& v) { o << v.is_white_move; },
               [&](const QueenCastling& v) { o << v.is_white_move; },
               [&](const NextMove& v) { o << v.orig_token; }, [&](const Ignore& v) { o << "ignore"; },
               [&](const Finish& v) { o << (size_t)v.marker; }},
    val);
  return o;
}
