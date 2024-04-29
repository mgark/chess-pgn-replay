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

#include <array>
#include <assert.h>
#include <iostream>
#include <map>
#include <ostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "common.h"
#include "moves.h"

class ChessBoard
{
  using CoordinatesToChar = std::unordered_set<Coordinates, CoordinatesHash>;
  CoordinatesToChar coordinates_to_pieces_;

  struct Cell
  {
    bool is_white;
    char piece;
    bool double_move = false;
  };

  std::vector<std::vector<Cell>> board_;
  std::vector<Cell> double_pawn_moves_;
  static constexpr size_t _N_ = 8;

public:
  ChessBoard()
  {
    clear();

    const std::string high_rank{"RNBQKBNR"};
    const std::string low_rank{"PPPPPPPP"};

    for (size_t i = 0; i <= _N_ - 1; ++i)
    {
      board_[r('8')][i].piece = high_rank[i];
      board_[r('8')][i].is_white = false;
      board_[r('7')][i].piece = low_rank[i];
      board_[r('7')][i].is_white = false;
    }
    for (size_t i = 0; i <= _N_ - 1; ++i)
    {
      board_[r('1')][i].piece = high_rank[i];
      board_[r('1')][i].is_white = true;
      board_[r('2')][i].piece = low_rank[i];
      board_[r('2')][i].is_white = true;
    }
  }

  Cell get(Coordinates c) const { return board_[*c.x][*c.y]; }
  void clear() { board_ = {_N_, std::vector<Cell>(_N_, {false, '.'})}; }

  void apply(const Moves& move)
  {
    return std::visit(
      overloaded{[&](const NextMove& val)
                 {
                   INTERNAL_ASSERT(val.piece != '\0');

                   Coordinates dst = val.dst;
                   Coordinates src = val.src;
                   INTERNAL_ASSERT(dst.y.has_value());

                   CoordinatesToChar src_candidates;
                   {
                     if (!src.y && !src.x)
                     {
                       for (size_t x = 0; x <= _N_ - 1; ++x)
                       {
                         for (size_t y = 0; y <= _N_ - 1; ++y)
                         {
                           const Cell& c = board_[x][y];
                           if (c.piece == val.piece && c.is_white == val.is_white_move)
                             src_candidates.emplace(x, y);
                         }
                       }
                     }
                     else if (!src.y)
                     {
                       int x = src.x.value();
                       for (size_t y = 0; y <= _N_ - 1; ++y)
                       {
                         const Cell& c = board_[x][y];
                         if (c.piece == val.piece && c.is_white == val.is_white_move)
                           src_candidates.emplace(x, y);
                       }
                     }
                     else if (!src.x)
                     {
                       int y = src.y.value();
                       for (size_t x = 0; x <= _N_ - 1; ++x)
                       {
                         const Cell& c = board_[x][y];
                         if (c.piece == val.piece && c.is_white == val.is_white_move)
                           src_candidates.emplace(x, y);
                       }
                     }
                     else
                     {
                       src_candidates.emplace(src.x, src.y);
                     }
                   }
                   INTERNAL_ASSERT(!src_candidates.empty());

                   CoordinatesToChar dst_candidates;
                   {
                     if (!dst.y)
                     {
                       int x = dst.x.value();
                       for (size_t y = 0; y <= _N_ - 1; ++y)
                       {
                         const Cell& c = board_[x][y];
                         if (c.piece == '.' || (val.capture /*&& c.is_white != val.is_white_move*/))
                           dst_candidates.emplace(x, y);
                       }
                     }
                     else if (!dst.x)
                     {
                       int y = dst.y.value();
                       for (size_t x = 0; x <= _N_ - 1; ++x)
                       {
                         const Cell& c = board_[x][y];
                         if (c.piece == '.' || (val.capture /*&& c.is_white != val.is_white_move*/))
                           dst_candidates.emplace(x, y);
                       }
                     }
                     else
                     {
                       dst_candidates.emplace(dst.x, dst.y);
                     }
                   }
                   INTERNAL_ASSERT(!dst_candidates.empty());

                   size_t matches = 0;
                   Coordinates final_src;
                   Coordinates final_dst;
                   bool found_match = false;
                   for (const auto& src : src_candidates)
                   {
                     for (const auto& dst : dst_candidates)
                     {
                       bool locked = is_locked(src, dst, val.capture, val.is_white_move);
                       if (locked)
                       {
                         continue;
                       }

                       switch (val.piece)
                       {
                       case 'P':
                         matches += can_move_pawn(src, dst, val.capture, val.is_white_move);
                         break;
                       case 'R':
                         matches += can_move_rook(src, dst, val.capture, val.is_white_move);
                         break;
                       case 'Q':
                         matches += can_move_queen(src, dst, val.capture, val.is_white_move);
                         break;
                       case 'N':
                         matches += can_move_knight(src, dst, val.capture, val.is_white_move);
                         break;
                       case 'B':
                         matches += can_move_bishop(src, dst, val.capture, val.is_white_move);
                         break;
                       case 'K':
                         matches += can_move_king(src, dst, val.capture, val.is_white_move);
                         break;
                       default:
                         INTERNAL_ASSERT(false);
                         break;
                       }

                       if (!found_match && matches == 1)
                       {
                         final_src = src;
                         final_dst = dst;
                         found_match = true;

                         // let's make sure to clear double move flag if the pawn
                         // has moved or if it has been captured
                         if (val.piece == 'P')
                         {
                           Cell& src_cell = board_[*final_src.x][*final_src.y];
                           if (src_cell.double_move)
                             src_cell.double_move = false;
                         }
                         else if (val.capture)
                         {
                           Cell& dst_cell = board_[*final_dst.x][*final_dst.y];
                           if (dst_cell.double_move)
                           {
                             INTERNAL_ASSERT(dst_cell.piece == 'P');
                             dst_cell.double_move = false;
                           }
                         }
                       }
                     }
                   }
                   INTERNAL_ASSERT(matches == 1);

                   if (val.promote_piece)
                   {
                     // apply promotion
                     board_[*final_dst.x][*final_dst.y].piece = *val.promote_piece;
                   }
                   else
                   {
                     // regular move
                     board_[*final_dst.x][*final_dst.y].piece = val.piece;
                   }

                   // rest the src cell
                   board_[*final_src.x][*final_src.y].piece = '.';

                   // set the right yor
                   board_[*final_dst.x][*final_dst.y].is_white = val.is_white_move;
                 },
                 [&](const QueenCastling& t)
                 {
                   if (t.is_white_move)
                   {
                     INTERNAL_ASSERT(is_free_cell({r('1'), f('c')}));
                     INTERNAL_ASSERT(is_free_cell({r('1'), f('d')}));

                     board_[r('1')][f('c')] = board_[r('1')][f('e')]; // move the king to 'c1'
                     board_[r('1')][f('e')].piece = '.'; // clear the king's original square
                     board_[r('1')][f('d')] = board_[r('1')][f('a')]; // move the rook to 'd1'
                     board_[r('1')][f('a')].piece = '.'; // clear the rook's original square
                   }
                   else
                   {
                     INTERNAL_ASSERT(is_free_cell({r('8'), f('c')}));
                     INTERNAL_ASSERT(is_free_cell({r('8'), f('d')}));

                     board_[r('8')][f('c')] = board_[r('8')][f('e')]; // move the king to 'c8'
                     board_[r('8')][f('e')].piece = '.'; // clear the king's original square
                     board_[r('8')][f('d')] = board_[r('8')][f('a')]; // move the rook to 'd8'
                     board_[r('8')][f('a')].piece = '.'; // clear the rook's original square
                   }
                 },
                 [&](const Ignore& t)
                 {
                   // do nothing
                 },
                 [&](const KingCastling& t)
                 {
                   if (t.is_white_move)
                   {
                     INTERNAL_ASSERT(is_free_cell({r('1'), f('g')}));
                     INTERNAL_ASSERT(is_free_cell({r('1'), f('f')}));

                     board_[r('1')][f('g')] = board_[r('1')][f('e')]; // king moves to 'g1'
                     board_[r('1')][f('e')].piece = '.'; // clear the king's original square
                     board_[r('1')][f('f')] = board_[r('1')][f('h')]; // rook moves to 'f1'
                     board_[r('1')][f('h')].piece = '.'; // clear the rook's original square
                   }
                   else
                   {
                     INTERNAL_ASSERT(is_free_cell({r('8'), f('g')}));
                     INTERNAL_ASSERT(is_free_cell({r('8'), f('f')}));

                     board_[r('8')][f('g')] = board_[r('8')][f('e')]; // king moves to 'g8'
                     board_[r('8')][f('e')].piece = '.'; // clear the king's original square
                     board_[r('8')][f('f')] = board_[r('8')][f('h')]; // rook moves to 'f8'
                     board_[r('8')][f('h')].piece = '.'; // clear the rook original square
                   }
                 },
                 [&](const auto& t) {}},
      move);
  }

  bool in_range(int idx) { return 0 <= idx && idx <= (int)_N_ - 1; }

  bool is_locked(Coordinates src, Coordinates dst, bool capture, bool is_white_move)
  {
    struct Direction
    {
      int dx;
      int dy;
    };

    std::array<Direction, _N_> d{{{-1, 0}, {-1, 1}, {0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1}}};
    auto is_king_under_attack = [&](int direction, Coordinates attacker)
    {
      const Cell& c = board_[*attacker.x][*attacker.y];
      if (c.piece == '.' || c.is_white == is_white_move)
      {
        return false;
      }

      if (direction & 1)
      {
        // odd, that means diagonal moves, so B / Q may still be attacking the king
        return (c.piece == 'Q' || c.piece == 'B');
      }
      else
      {
        // even, that means horizontal/vertical moves, so R / Q may still be attacking the king
        return (c.piece == 'Q' || c.piece == 'R');
      }
    };

    // first let's find the angle at which king is
    int idx = -1;
    Coordinates king;
    for (size_t i = 0; i <= _N_ - 1; ++i)
    {
      Coordinates ray = src;
      do
      {
        *ray.x += d[i].dx;
        *ray.y += d[i].dy;
      } while (in_range(*ray.x) && in_range(*ray.y) && board_[*ray.x][*ray.y].piece == '.');
      if (in_range(*ray.x) && in_range(*ray.y))
      {
        const Cell& c = board_[*ray.x][*ray.y];
        if (c.piece == 'K' && c.is_white == is_white_move)
        {
          idx = i;
          king = ray;
          break;
        }
      }
    }

    if (idx != -1)
    {
      INTERNAL_ASSERT(king.y && king.x);
      int opposite_idx = (idx + _N_ / 2) % _N_;
      Coordinates ray = src;
      do
      {
        *ray.x += d[opposite_idx].dx;
        *ray.y += d[opposite_idx].dy;
      } while (ray != dst && in_range(*ray.x) && in_range(*ray.y) && board_[*ray.x][*ray.y].piece == '.');
      if (in_range(*ray.x) && in_range(*ray.y) && board_[*ray.x][*ray.y].piece != '.')
      {
        const Cell& c = board_[*ray.x][*ray.y];
        if (ray == dst)
        {
          // we must be attackign it
          INTERNAL_ASSERT(c.is_white != is_white_move);
          INTERNAL_ASSERT(capture);

          // let's assume it was captured, but second piece in line may still be checking our king,
          // so need to check if the is the case
          *ray.x += d[opposite_idx].dx;
          *ray.y += d[opposite_idx].dy;
          if (in_range(*ray.x) && in_range(*ray.y))
          {
            return is_king_under_attack(idx, ray);
          }
        }
        else
        {
          return is_king_under_attack(idx, ray);
        }
      }
    }

    return false;
  }

  bool can_move_pawn(Coordinates src, Coordinates dst, bool capture, bool is_white_move)
  {
    int dx;
    int dy = std::abs(*dst.y - *src.y);
    bool first_move;
    if (is_white_move)
    {
      first_move = (r('2') == src.x);
      dx = *src.x - *dst.x;
    }
    else
    {
      first_move = (r('7') == src.x);
      dx = *dst.x - *src.x;
    }

    bool result = false;
    if (capture)
    {
      result = (dx == 1 && dy == 1);

      // detect en passant
      Cell& dest_cell = board_[*dst.x][*dst.y];
      if (result && dest_cell.piece == '.')
      {
        Coordinates x;
        int d = *dst.y - *src.y;
        x.y = *src.y + d;
        x.x = *src.x;
        INTERNAL_ASSERT(in_range(*x.y));

        Cell& captured_cell = board_[*x.x][*x.y];
        INTERNAL_ASSERT(captured_cell.piece == 'P');
        INTERNAL_ASSERT(captured_cell.is_white == !is_white_move);
        {
          INTERNAL_ASSERT(captured_cell.double_move);
          captured_cell.piece = '.';
          captured_cell.double_move = false;
        }

        return true;
      }
    }
    else
    {
      if (dx == 2)
      {
        result = (first_move && dy == 0);
        if (result)
        {
          constexpr std::array<int, 2> d{1, -1};

          // need to make sure the pawn can move two cells forward, which means they must be free
          Coordinates ray = src;
          *ray.x += d[is_white_move];
          if (!is_free_cell(ray))
            return false;

          *ray.x += d[is_white_move];
          return is_free_cell(ray) && (board_[*dst.x][*dst.y].double_move = true, true);
        }
      }
      else if (dx == 1)
      {
        result = (dy == 0);
      }
    }

    return result && is_valid_dest(dst, capture, is_white_move);
  }

  bool can_move_rook(Coordinates src, Coordinates dst, bool capture, bool is_white_move)
  {
    if (*src.x == *dst.x && *src.y != *dst.y)
    {
      Coordinates ray = src;
      if (*src.y < *dst.y)
      {
        // horizontal right move
        while (ray.y < *dst.y)
        {
          ++*ray.y;
          if (dst != ray)
          {
            if (!is_free_cell(ray))
              return false;
          }
        }
      }
      else
      {
        // horizontal left move
        while (ray.y > *dst.y)
        {
          --*ray.y;
          if (dst != ray)
          {
            if (!is_free_cell(ray))
              return false;
          }
        }
      }

      return is_valid_dest(dst, capture, is_white_move);
    }
    else if (*src.y == *dst.y && *src.x != *dst.x)
    {
      Coordinates ray = src;
      if (*src.x < *dst.x)
      {
        // vertical down move
        while (ray.x < *dst.x)
        {
          ++*ray.x;
          if (dst != ray)
          {
            if (!is_free_cell(ray))
              return false;
          }
        }
      }
      else
      {
        // vertical up move
        while (ray.x > *dst.x)
        {
          --*ray.x;
          if (dst != ray)
          {
            if (!is_free_cell(ray))
              return false;
          }
        }
      }

      return is_valid_dest(dst, capture, is_white_move);
    }
    else
    {
      return false;
    }
  }

  bool can_move_queen(Coordinates src, Coordinates dst, bool capture, bool is_white_move)
  {
    return can_move_rook(src, dst, capture, is_white_move) || can_move_bishop(src, dst, capture, is_white_move);
  }

  bool can_move_knight(Coordinates src, Coordinates dst, bool capture, bool is_white_move)
  {
    int dx = std::abs(*dst.x - *src.x);
    int dy = std::abs(*dst.y - *src.y);
    bool result = ((dx == 1 && dy == 2) || (dx == 2 && dy == 1));
    return result && is_valid_dest(dst, capture, is_white_move);
  }

  bool can_move_bishop(Coordinates src, Coordinates dst, bool capture, bool is_white_move)
  {
    int dx = std::abs(*dst.x - *src.x);
    int dy = std::abs(*dst.y - *src.y);
    if (dx >= 1 && dy >= 1 && dx == dy)
    {
      Coordinates ray = src;
      if (*dst.x > *src.x && *dst.y > *src.y)
      {
        // down right
        while (*ray.x < *dst.x)
        {
          ++*ray.x;
          ++*ray.y;
          if (dst != ray)
          {
            if (!is_free_cell(ray))
              return false;
          }
        }
      }
      else if (*dst.x > *src.x && *dst.y < *src.y)
      {
        // down left
        while (*ray.x < *dst.x)
        {
          ++*ray.x;
          --*ray.y;
          if (dst != ray)
          {
            if (!is_free_cell(ray))
              return false;
          }
        }
      }
      else if (*dst.x<*src.x&& * dst.y> * src.y)
      {
        // up right
        while (*ray.x > *dst.x)
        {
          --*ray.x;
          ++*ray.y;
          if (dst != ray)
          {
            if (!is_free_cell(ray))
              return false;
          }
        }
      }
      else if (*dst.x < *src.x && *dst.y < *src.y)
      {
        // up left
        while (*ray.x > *dst.x)
        {
          --*ray.x;
          --*ray.y;
          if (dst != ray)
          {
            if (!is_free_cell(ray))
              return false;
          }
        }
      }
      else
      {
        INTERNAL_ASSERT(true);
      }

      return is_valid_dest(dst, capture, is_white_move);
    }
    else
    {
      return false;
    }
  }

  bool can_move_king(Coordinates src, Coordinates dst, bool capture, bool is_white_move)
  {
    int dx = std::abs(*src.x - *dst.x);
    int dy = std::abs(*src.y - *dst.y);
    bool result = (dx || dy) && dx <= 1 && dy <= 1;
    return result && is_valid_dest(dst, capture, is_white_move);
  }

  bool is_free_cell(Coordinates c) { return board_[*c.x][*c.y].piece == '.'; }
  bool is_valid_dest(Coordinates dst, bool capture, bool is_white_move)
  {
    auto& dst_cell = board_[*dst.x][*dst.y];
    if (capture)
    {
      return (dst_cell.is_white == !is_white_move && dst_cell.piece != 'K');
    }
    else
    {
      return (dst_cell.piece == '.');
    }
  }

  void manualy_set_cell(Coordinates c, Cell cell)
  {
    INTERNAL_ASSERT(c.x.has_value());
    INTERNAL_ASSERT(c.y.has_value());
    INTERNAL_ASSERT(c.y <= _N_ - 1 && c.y >= 0);
    INTERNAL_ASSERT(c.x <= _N_ - 1 && c.x >= 0);
    board_[*c.x][*c.y] = cell;
  }

  friend std::ostream& operator<<(std::ostream& o, const ChessBoard& b)
  {
    for (size_t x = 0; x <= _N_ - 1; ++x)
    {
      for (size_t y = 0; y <= _N_ - 1; ++y)
      {
        if (y > 0)
        {
          o << "|";
        }

        char yor = b.board_[x][y].is_white ? 'w' : 'b';
        o << (b.board_[x][y].piece == '.' ? "  " : std::string({yor, b.board_[x][y].piece}));
      }

      o << "\n";
    }

    return o;
  }
};
