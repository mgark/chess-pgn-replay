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

#include "board.h"
#include "common.h"
#include "moves.h"
#include "parser.h"
#include "scanner.h"
#include <assert.h>
#include <exception>
#include <sstream>

void test_move_parser()
{
  size_t N;
  try
  {
    // simple pawn move
    {
      N = 1;
      Moves m = MoveFactory()(std::string{"e4"}, true);
      assert(m.index() == 2);
      const NextMove& n = std::get<NextMove>(m);
      assert(n.is_white_move == true);
      assert(n.dst.x == 4);
      assert(n.dst.y == 4);
      assert(n.piece == 'P');
    }

    {
      N = 2;
      Moves m = MoveFactory()(std::string{"h1"}, true);
      assert(m.index() == 2);
      const NextMove& n = std::get<NextMove>(m);
      assert(n.is_white_move == true);
      assert(n.dst.x == 7);
      assert(n.dst.y == 7);
      assert(n.piece == 'P');
      assert(!n.src.y.has_value());
      assert(!n.src.x.has_value());
    }

    // promotion
    {
      N = 3;
      Moves m = MoveFactory()(std::string{"a1=Q"}, false);
      assert(m.index() == 2);
      const NextMove& n = std::get<NextMove>(m);
      assert(n.is_white_move == false);
      assert(n.dst.x == 7);
      assert(n.dst.y == 0);
      assert(n.piece == 'P');
      assert(n.promote_piece == 'Q');
      assert(!n.src.y.has_value());
      assert(!n.src.x.has_value());
    }

    {
      N = 4;
      Moves m = MoveFactory()(std::string{"a7xb8=Q"}, false);
      assert(m.index() == 2);
      const NextMove& n = std::get<NextMove>(m);
      assert(n.is_white_move == false);
      assert(n.dst.x == 0);
      assert(n.dst.y == 1);
      assert(n.piece == 'P');
      assert(n.src.y == 0);
      assert(n.src.x == 1);
    }

    {
      N = 5;
      Moves m = MoveFactory()(std::string{"axb"}, false);
      assert(m.index() == 2);
      const NextMove& n = std::get<NextMove>(m);
      assert(n.is_white_move == false);
      assert(!n.dst.x.has_value());
      assert(n.dst.y == 1);
      assert(n.piece == 'P');
      assert(n.src.y == 0);
      assert(!n.src.x.has_value());
    }
  }
  catch (const std::exception& e)
  {
    std::cout << "\n Test [" << N << "] E: [" << e.what() << "]\n";
  }
}

void test_king_move()
{
  ChessBoard b;
  b.clear();
  bool is_white_move = true;
  b.manualy_set_cell({1, 1}, {is_white_move, 'K'});

  assert(b.can_move_king({1, 1}, {0, 0}, false, is_white_move));
  assert(b.can_move_king({1, 1}, {2, 2}, false, is_white_move));
  assert(b.can_move_king({1, 1}, {0, 1}, false, is_white_move));
  assert(b.can_move_king({1, 1}, {1, 0}, false, is_white_move));
  assert(b.can_move_king({1, 1}, {2, 1}, false, is_white_move));
  assert(b.can_move_king({1, 1}, {1, 2}, false, is_white_move));

  assert(!b.can_move_king({1, 1}, {3, 3}, false, is_white_move));
  assert(!b.can_move_king({1, 1}, {1, 1}, false, is_white_move));

  // set the other king next the original king position
  b.manualy_set_cell({0, 0}, {!is_white_move, 'K'});
  assert(!b.can_move_king({1, 1}, {0, 0}, false, is_white_move));
  assert(!b.can_move_king({1, 1}, {0, 0}, true, is_white_move));

  // set the pawn next to the original king position
  {
    b.manualy_set_cell({0, 0}, {!is_white_move, 'P'});
    assert(!b.can_move_king({1, 1}, {0, 0}, false, is_white_move));
    assert(b.can_move_king({1, 1}, {0, 0}, true, is_white_move));

    b.manualy_set_cell({0, 0}, {is_white_move, 'P'});
    assert(!b.can_move_king({1, 1}, {0, 0}, false, is_white_move));
    assert(!b.can_move_king({1, 1}, {0, 0}, true, is_white_move));
  }
}

void test_bishop_move()
{
  ChessBoard b;
  b.clear();
  Coordinates src{1, 1};
  bool is_white_move = true;

  // basic test
  {
    b.manualy_set_cell(src, {is_white_move, 'B'});

    assert(b.can_move_bishop(src, {0, 0}, false, is_white_move));
    assert(b.can_move_bishop(src, {2, 2}, false, is_white_move));
    assert(b.can_move_bishop(src, {0, 2}, false, is_white_move));
    assert(b.can_move_bishop(src, {2, 0}, false, is_white_move));
    assert(b.can_move_bishop(src, {3, 3}, false, is_white_move));

    assert(!b.can_move_bishop(src, {1, 1}, false, is_white_move));
    assert(!b.can_move_bishop(src, {1, 2}, false, is_white_move));
    assert(!b.can_move_bishop(src, {4, 1}, false, is_white_move));
  }

  // down right move test
  {
    b.manualy_set_cell(src, {is_white_move, '.'}); // reset
                                                   //
    src = {0, 0};
    b.manualy_set_cell(src, {is_white_move, 'B'});

    Coordinates dst{7, 7};
    assert(b.can_move_bishop(src, dst, false, is_white_move));
    // capture move
    {
      b.manualy_set_cell(dst, {!is_white_move, 'B'});
      assert(b.can_move_bishop(src, dst, true, is_white_move));
      assert(!b.can_move_bishop(src, dst, false, is_white_move));
      b.manualy_set_cell(dst, {is_white_move, '.'}); // reset
    }

    assert(!b.can_move_bishop(src, {2, 4}, false, is_white_move));
    assert(!b.can_move_bishop(src, {1, 2}, false, is_white_move));
    assert(!b.can_move_bishop(src, {4, 1}, false, is_white_move));
  }

  // down left move test
  {

    b.manualy_set_cell(src, {is_white_move, '.'}); // reset
                                                   //
    src = {0, 7};
    b.manualy_set_cell(src, {is_white_move, 'B'});

    Coordinates dst{7, 0};
    assert(b.can_move_bishop(src, dst, false, is_white_move));
    // capture move
    {
      b.manualy_set_cell(dst, {!is_white_move, 'B'});
      assert(b.can_move_bishop(src, dst, true, is_white_move));
      assert(!b.can_move_bishop(src, dst, false, is_white_move));
      b.manualy_set_cell(dst, {is_white_move, '.'}); // reset
    }

    assert(!b.can_move_bishop(src, {7, 7}, false, is_white_move));
    assert(!b.can_move_bishop(src, {2, 4}, false, is_white_move));
    assert(!b.can_move_bishop(src, {1, 2}, false, is_white_move));
    assert(!b.can_move_bishop(src, {4, 1}, false, is_white_move));
  }

  // up right move test
  {

    b.manualy_set_cell(src, {is_white_move, '.'}); // reset
                                                   //
    src = {7, 0};
    b.manualy_set_cell(src, {is_white_move, 'B'});

    Coordinates dst{0, 7};
    assert(b.can_move_bishop(src, dst, false, is_white_move));
    // capture move
    {
      b.manualy_set_cell(dst, {!is_white_move, 'B'});
      assert(b.can_move_bishop(src, dst, true, is_white_move));
      assert(!b.can_move_bishop(src, dst, false, is_white_move));
      b.manualy_set_cell(dst, {is_white_move, '.'}); // reset
    }

    assert(!b.can_move_bishop(src, {7, 7}, false, is_white_move));
    assert(!b.can_move_bishop(src, {2, 4}, false, is_white_move));
    assert(!b.can_move_bishop(src, {1, 2}, false, is_white_move));
    assert(!b.can_move_bishop(src, {4, 1}, false, is_white_move));
  }

  // up left move test
  {

    b.manualy_set_cell(src, {is_white_move, '.'}); // reset
                                                   //
    src = {7, 7};
    b.manualy_set_cell(src, {is_white_move, 'B'});

    Coordinates dst{0, 0};
    assert(b.can_move_bishop(src, dst, false, is_white_move));
    // capture move
    {
      b.manualy_set_cell(dst, {!is_white_move, 'B'});
      assert(b.can_move_bishop(src, dst, true, is_white_move));
      assert(!b.can_move_bishop(src, dst, false, is_white_move));

      // test same yor capture/move
      b.manualy_set_cell(dst, {is_white_move, 'B'});
      assert(!b.can_move_bishop(src, dst, true, is_white_move));
      assert(!b.can_move_bishop(src, dst, false, is_white_move));

      b.manualy_set_cell(dst, {is_white_move, '.'}); // reset
    }

    assert(!b.can_move_bishop(src, {0, 7}, true, is_white_move));
    assert(!b.can_move_bishop(src, {7, 7}, false, is_white_move));
    assert(!b.can_move_bishop(src, {2, 4}, false, is_white_move));
    assert(!b.can_move_bishop(src, {1, 2}, false, is_white_move));
    assert(!b.can_move_bishop(src, {4, 1}, false, is_white_move));
  }
}

void test_knight_move()
{
  ChessBoard b;
  b.clear();
  Coordinates src{2, 2};
  bool is_white_move = true;

  // down right
  {
    b.manualy_set_cell(src, {is_white_move, 'N'});

    Coordinates dst{4, 3};
    assert(b.can_move_knight(src, dst, false, is_white_move));
    // capture move
    {
      b.manualy_set_cell(dst, {!is_white_move, 'B'});
      assert(b.can_move_knight(src, dst, true, is_white_move));
      assert(!b.can_move_knight(src, dst, false, is_white_move));

      // test same yor capture/move
      b.manualy_set_cell(dst, {is_white_move, 'B'});
      assert(!b.can_move_knight(src, dst, true, is_white_move));
      assert(!b.can_move_knight(src, dst, false, is_white_move));

      b.manualy_set_cell(dst, {is_white_move, '.'}); // reset
    }
  }

  // down left
  {
    b.manualy_set_cell(src, {is_white_move, 'N'});

    Coordinates dst{4, 1};
    assert(b.can_move_knight(src, dst, false, is_white_move));
    // capture move
    {
      b.manualy_set_cell(dst, {!is_white_move, 'B'});
      assert(b.can_move_knight(src, dst, true, is_white_move));
      assert(!b.can_move_knight(src, dst, false, is_white_move));

      // test same yor capture/move
      b.manualy_set_cell(dst, {is_white_move, 'B'});
      assert(!b.can_move_knight(src, dst, true, is_white_move));
      assert(!b.can_move_knight(src, dst, false, is_white_move));

      b.manualy_set_cell(dst, {is_white_move, '.'}); // reset
    }
  }

  // up right
  {
    b.manualy_set_cell(src, {is_white_move, 'N'});

    Coordinates dst{0, 3};
    assert(b.can_move_knight(src, dst, false, is_white_move));
    // capture move
    {
      b.manualy_set_cell(dst, {!is_white_move, 'B'});
      assert(b.can_move_knight(src, dst, true, is_white_move));
      assert(!b.can_move_knight(src, dst, false, is_white_move));

      // test same yor capture/move
      b.manualy_set_cell(dst, {is_white_move, 'B'});
      assert(!b.can_move_knight(src, dst, true, is_white_move));
      assert(!b.can_move_knight(src, dst, false, is_white_move));

      b.manualy_set_cell(dst, {is_white_move, '.'}); // reset
    }
  }

  // up left
  {
    b.manualy_set_cell(src, {is_white_move, 'N'});

    Coordinates dst{0, 1};
    assert(b.can_move_knight(src, dst, false, is_white_move));
    // capture move
    {
      b.manualy_set_cell(dst, {!is_white_move, 'B'});
      assert(b.can_move_knight(src, dst, true, is_white_move));
      assert(!b.can_move_knight(src, dst, false, is_white_move));

      // test same yor capture/move
      b.manualy_set_cell(dst, {is_white_move, 'B'});
      assert(!b.can_move_knight(src, dst, true, is_white_move));
      assert(!b.can_move_knight(src, dst, false, is_white_move));

      b.manualy_set_cell(dst, {is_white_move, '.'}); // reset
    }
  }
}

void test_rook_move()
{
  ChessBoard b;
  b.clear();
  Coordinates src{0, 0};
  bool is_white_move = true;

  // horizontal right
  {
    b.manualy_set_cell(src, {is_white_move, 'R'});

    Coordinates dst{0, 7};
    assert(b.can_move_rook(src, dst, false, is_white_move));
    // capture move
    {
      b.manualy_set_cell(dst, {!is_white_move, 'B'});
      assert(b.can_move_rook(src, dst, true, is_white_move));
      assert(!b.can_move_rook(src, dst, false, is_white_move));

      // test same yor capture/move
      b.manualy_set_cell(dst, {is_white_move, 'B'});
      assert(!b.can_move_rook(src, dst, true, is_white_move));
      assert(!b.can_move_rook(src, dst, false, is_white_move));

      b.manualy_set_cell(dst, {is_white_move, '.'}); // reset
    }
  }

  // horizontal left
  {
    b.manualy_set_cell(src, {is_white_move, '.'}); // reset

    src = {0, 7};
    b.manualy_set_cell(src, {is_white_move, 'R'});

    Coordinates dst{0, 0};
    assert(b.can_move_rook(src, dst, false, is_white_move));
    // capture move
    {
      b.manualy_set_cell(dst, {!is_white_move, 'B'});
      assert(b.can_move_rook(src, dst, true, is_white_move));
      assert(!b.can_move_rook(src, dst, false, is_white_move));

      // test same yor capture/move
      b.manualy_set_cell(dst, {is_white_move, 'B'});
      assert(!b.can_move_rook(src, dst, true, is_white_move));
      assert(!b.can_move_rook(src, dst, false, is_white_move));

      b.manualy_set_cell(dst, {is_white_move, '.'}); // reset
    }
  }

  // vertical up
  {
    b.manualy_set_cell(src, {is_white_move, '.'}); // reset

    src = {7, 0};
    b.manualy_set_cell(src, {is_white_move, 'R'});

    Coordinates dst{0, 0};
    assert(b.can_move_rook(src, dst, false, is_white_move));
    // capture move
    {
      b.manualy_set_cell(dst, {!is_white_move, 'B'});
      assert(b.can_move_rook(src, dst, true, is_white_move));
      assert(!b.can_move_rook(src, dst, false, is_white_move));

      // test same yor capture/move
      b.manualy_set_cell(dst, {is_white_move, 'B'});
      assert(!b.can_move_rook(src, dst, true, is_white_move));
      assert(!b.can_move_rook(src, dst, false, is_white_move));

      b.manualy_set_cell(dst, {is_white_move, '.'}); // reset
    }
  }

  // vertical down
  {
    b.manualy_set_cell(src, {is_white_move, '.'}); // reset

    src = {0, 0};
    b.manualy_set_cell(src, {is_white_move, 'R'});

    Coordinates dst{7, 0};
    assert(b.can_move_rook(src, dst, false, is_white_move));
    // capture move
    {
      b.manualy_set_cell(dst, {!is_white_move, 'B'});
      assert(b.can_move_rook(src, dst, true, is_white_move));
      assert(!b.can_move_rook(src, dst, false, is_white_move));

      // test same yor capture/move
      b.manualy_set_cell(dst, {is_white_move, 'B'});
      assert(!b.can_move_rook(src, dst, true, is_white_move));
      assert(!b.can_move_rook(src, dst, false, is_white_move));

      b.manualy_set_cell(dst, {is_white_move, '.'}); // reset
    }
  }
}

void test_pawn_move()
{
  bool white_move = true;
  // white move
  {
    ChessBoard b;
    b.clear();
    Coordinates src{6, 1};
    b.manualy_set_cell(src, {white_move, 'P'});

    Coordinates dst{5, 1};
    assert(b.can_move_pawn(src, dst, false, white_move));
    {
      // diff yor, no capture
      b.manualy_set_cell(dst, {!white_move, 'B'});
      assert(!b.can_move_pawn(src, dst, false, white_move));
      b.manualy_set_cell(dst, {white_move, '.'}); // reset
    }

    // capture test 1
    {
      Coordinates dst{5, 0};
      b.manualy_set_cell(dst, {!white_move, 'B'});
      assert(b.can_move_pawn(src, dst, true, white_move));
      assert(!b.can_move_pawn(src, dst, false, white_move)); // diff yor but not capture
      assert(!b.can_move_pawn(src, dst, true, !white_move)); // same yor
    }

    // capture test 2
    {
      Coordinates dst{5, 2};
      b.manualy_set_cell(dst, {!white_move, 'B'});
      assert(b.can_move_pawn(src, dst, true, white_move));
      assert(!b.can_move_pawn(src, dst, false, white_move)); // diff yor but not capture
      assert(!b.can_move_pawn(src, dst, true, !white_move)); // same yor
    }

    // first move
    dst = {4, 1};
    assert(b.can_move_pawn(src, dst, false, white_move));
  }

  // black move
  {
    ChessBoard b;
    b.clear();
    Coordinates src{1, 1};
    b.manualy_set_cell(src, {!white_move, 'P'});

    Coordinates dst{2, 1};
    assert(b.can_move_pawn(src, dst, false, !white_move));
    {
      // diff yor, no capture
      b.manualy_set_cell(dst, {white_move, 'B'});
      assert(!b.can_move_pawn(src, dst, false, !white_move));
      b.manualy_set_cell(dst, {!white_move, '.'}); // reset
    }

    // capture test 1
    {
      Coordinates dst{2, 0};
      b.manualy_set_cell(dst, {white_move, 'B'});
      assert(b.can_move_pawn(src, dst, true, !white_move));
      assert(!b.can_move_pawn(src, dst, false, !white_move)); // diff yor but not capture
      assert(!b.can_move_pawn(src, dst, true, white_move));   // same yor
    }

    // capture test 2
    {
      Coordinates dst{2, 2};
      b.manualy_set_cell(dst, {white_move, 'B'});
      assert(b.can_move_pawn(src, dst, true, !white_move));
      assert(!b.can_move_pawn(src, dst, false, !white_move)); // diff yor but not capture
      assert(!b.can_move_pawn(src, dst, true, white_move));   // same yor
    }

    // first move
    dst = {3, 1};
    assert(b.can_move_pawn(src, dst, false, !white_move));
  }
}

void test_pawn_board_moves()
{
  // pawn move
  {
    ChessBoard b;
    b.clear();
    bool white_move = true;
    Coordinates src{1, 1};
    Coordinates dst{2, 1};
    b.manualy_set_cell(src, {!white_move, 'P'});
    b.manualy_set_cell(dst, {white_move, '.'});
    b.apply(MoveFactory()(std::string{"b6"}, !white_move));
    assert(b.get(dst).piece == 'P');
    assert(!b.get(dst).is_white);
    assert(b.get(src).piece == '.');

    b.manualy_set_cell({3, 2}, {white_move, 'P'});
    assert(b.get({3, 2}).piece == 'P');
    assert(b.get({2, 1}).piece == 'P');
    b.apply(MoveFactory()(std::string{"cxb6"}, white_move));
    assert(b.get({3, 2}).piece == '.');
  }

  // pawn promotion #1
  {
    ChessBoard b;
    b.clear();
    bool white_move = true;
    Coordinates src{1, 1};
    b.manualy_set_cell(src, {white_move, 'P'});
    b.apply(MoveFactory()(std::string{"b8=Q"}, white_move));
    assert(b.get(src).piece == '.');
    assert(b.get({0, 1}).piece == 'Q');
  }

  // pawn promotion #2
  {
    ChessBoard b;
    b.clear();
    bool white_move = true;
    Coordinates src{1, 1};
    b.manualy_set_cell(src, {white_move, 'P'});
    b.apply(MoveFactory()(std::string{"b8/R"}, white_move));
    assert(b.get(src).piece == '.');
    assert(b.get({0, 1}).piece == 'R');
  }

  // pawn promotion #3
  {
    ChessBoard b;
    b.clear();
    bool white_move = true;
    Coordinates src{1, 1};
    b.manualy_set_cell(src, {white_move, 'P'});
    b.apply(MoveFactory()(std::string{"b8(B)"}, white_move));
    assert(b.get(src).piece == '.');
    assert(b.get({0, 1}).piece == 'B');
  }

  // pawn promotion #4
  {
    ChessBoard b;
    b.clear();
    bool white_move = true;
    Coordinates src{1, 1};
    b.manualy_set_cell(src, {white_move, 'P'});
    b.apply(MoveFactory()(std::string{"b8Q"}, white_move));
    assert(b.get(src).piece == '.');
    assert(b.get({0, 1}).piece == 'Q');
  }
}

void test_pawn_en_passant_board_moves()
{
  // white pawn performs en passant capture of a black pawn to the left
  {
    ChessBoard b;
    b.clear();
    bool white_move = true;
    Coordinates orig_black_pawn{1, 1};
    Coordinates orig_white_pawn{3, 2};
    Coordinates white_pawn_dst{2, 1};
    b.manualy_set_cell(orig_black_pawn, {!white_move, 'P'});
    b.manualy_set_cell(orig_white_pawn, {white_move, 'P'});
    b.manualy_set_cell(white_pawn_dst, {white_move, '.'});
    b.apply(MoveFactory()(std::string{"b5"}, !white_move));
    b.apply(MoveFactory()(std::string{"cxb6"}, white_move));

    assert(b.get(white_pawn_dst).piece == 'P');
    assert(b.get(white_pawn_dst).is_white);
    assert(b.get(orig_black_pawn).piece == '.');
    assert(!b.get(orig_black_pawn).double_move);
    assert(b.get(orig_white_pawn).piece == '.');
    assert(!b.get(orig_white_pawn).double_move);
  }

  // white pawn performs en passant capture of a black pawn to the right
  {
    ChessBoard b;
    b.clear();
    bool white_move = true;
    Coordinates orig_black_pawn{1, 1};
    Coordinates orig_white_pawn{3, 0};
    Coordinates white_pawn_dst{2, 1};
    b.manualy_set_cell(orig_black_pawn, {!white_move, 'P'});
    b.manualy_set_cell(orig_white_pawn, {white_move, 'P'});
    b.manualy_set_cell(white_pawn_dst, {white_move, '.'});

    b.apply(MoveFactory()(std::string{"b5"}, !white_move));

    b.apply(MoveFactory()(std::string{"axb6"}, white_move));

    assert(b.get(white_pawn_dst).piece == 'P');
    assert(b.get(white_pawn_dst).is_white);
    assert(b.get(orig_black_pawn).piece == '.');
    assert(!b.get(orig_black_pawn).double_move);
    assert(b.get(orig_white_pawn).piece == '.');
    assert(!b.get(orig_white_pawn).double_move);
  }

  // black pawn performs en passant capture of a black pawn
  {
    ChessBoard b;
    b.clear();
    bool white_move = true;
    Coordinates orig_black_pawn{4, 0};
    Coordinates orig_white_pawn{6, 1};
    Coordinates black_pawn_dst{5, 1};
    b.manualy_set_cell(orig_black_pawn, {!white_move, 'P'});
    b.manualy_set_cell(orig_white_pawn, {white_move, 'P'});
    b.manualy_set_cell(black_pawn_dst, {white_move, '.'});

    b.apply(MoveFactory()(std::string{"b4"}, white_move));

    b.apply(MoveFactory()(std::string{"axb"}, !white_move));

    assert(b.get(black_pawn_dst).piece == 'P');
    assert(!b.get(black_pawn_dst).is_white);
    assert(b.get(orig_black_pawn).piece == '.');
    assert(!b.get(orig_black_pawn).double_move);
    assert(b.get(orig_white_pawn).piece == '.');
    assert(!b.get(orig_white_pawn).double_move);

    b.apply(MoveFactory()(std::string{"b2"}, !white_move));

    Coordinates another_white_pawn{5, 1};
    b.manualy_set_cell(another_white_pawn, {white_move, 'P'});
    b.apply(MoveFactory()(std::string{"b4"}, white_move));

    Coordinates another_black_pawn{4, 0};
    b.manualy_set_cell(another_black_pawn, {!white_move, 'P'});
    b.apply(MoveFactory()(std::string{"a3"}, !white_move));
  }
}

void test_knight_board_moves()
{
  // Knight move #1
  {
    ChessBoard b;
    b.clear();
    bool white_move = true;
    Coordinates k1{2, 0};
    Coordinates k2{2, 2};
    Coordinates dst{0, 1};
    b.manualy_set_cell(k1, {white_move, 'N'});
    b.manualy_set_cell(k2, {white_move, 'N'});
    b.manualy_set_cell(dst, {!white_move, '.'});
    b.apply(MoveFactory()(std::string{"Na6b8"}, white_move));
    assert(b.get(dst).piece == 'N');
    assert(b.get(dst).is_white);
  }

  // Knight move #2
  {
    ChessBoard b;
    b.clear();
    bool white_move = true;
    Coordinates n1{2, 0};
    Coordinates n2{2, 2};
    Coordinates dst{0, 1};
    b.manualy_set_cell(n1, {white_move, 'N'});
    b.manualy_set_cell(n2, {white_move, 'N'});
    b.manualy_set_cell(dst, {!white_move, '.'});
    b.apply(MoveFactory()(std::string{"Nab8"}, white_move));
    assert(b.get(dst).piece == 'N');
    assert(b.get(dst).is_white);
  }
}

void test_castling()
{
  {
    ChessBoard b;
    b.clear();
    bool white_move = true;
    Coordinates r{7, 7};
    Coordinates k{7, 4};
    b.manualy_set_cell(r, {white_move, 'R'});
    b.manualy_set_cell(k, {white_move, 'K'});
    b.apply(MoveFactory()(std::string{"O-O"}, white_move));
    assert(b.get(r).piece == '.');
    assert(b.get(k).piece == '.');
    assert(b.get({7, 6}).piece == 'K');
    assert(b.get({7, 5}).piece == 'R');
  }

  {
    ChessBoard b;
    b.clear();
    bool white_move = true;
    Coordinates r{7, 0};
    Coordinates k{7, 4};
    b.manualy_set_cell(r, {white_move, 'R'});
    b.manualy_set_cell(k, {white_move, 'K'});
    b.apply(MoveFactory()(std::string{"O-O-O"}, white_move));
    assert(b.get(r).piece == '.');
    assert(b.get(k).piece == '.');
    assert(b.get({7, 2}).piece == 'K');
    assert(b.get({7, 3}).piece == 'R');
    if (PRINT_DEBUG_INFO)
      std::cout << b;
  }
}

void test_locked_moves()
{
  // two nights, one is protecting the king
  {
    ChessBoard b;
    b.clear();

    bool white_move = true;
    Coordinates white_knight1{7, 2};
    Coordinates white_knight2{7, 4};
    Coordinates white_king{7, 3};
    Coordinates black_rook{7, 0};
    Coordinates dst{5, 3};

    b.manualy_set_cell(white_knight1, {white_move, 'N'});
    b.manualy_set_cell(white_knight2, {white_move, 'N'});
    b.manualy_set_cell(white_king, {white_move, 'K'});
    b.manualy_set_cell(black_rook, {!white_move, 'R'});

    assert(b.is_locked(white_knight1, dst, false, white_move));
    assert(!b.is_locked(white_knight2, dst, false, white_move));
    b.apply(MoveFactory()(std::string{"Nd3"}, white_move));

    assert(b.get(dst).piece == 'N');
    assert(b.get(dst).is_white);
    assert(b.get(white_knight1).piece == 'N');
    assert(b.get(white_knight1).is_white);
    assert(b.get(white_knight2).piece == '.');
  }

  // Bishop is protecting the king from enemy Queen
  {
    ChessBoard b;
    b.clear();

    bool white_move = true;
    Coordinates black_bishop{2, 2};
    Coordinates black_king{1, 1};
    Coordinates white_queen{7, 7};
    Coordinates dst{7, 7};

    b.manualy_set_cell(black_bishop, {!white_move, 'B'});
    b.manualy_set_cell(black_king, {!white_move, 'K'});
    b.manualy_set_cell(white_queen, {white_move, 'Q'});

    assert(!b.is_locked(black_bishop, dst, true, !white_move));
    assert(!b.is_locked(black_bishop, {6, 6}, false, !white_move));
    b.apply(MoveFactory()(std::string{"Bxh1"}, !white_move)); // kill queen

    assert(b.get(dst).piece == 'B');
    assert(!b.get(dst).is_white);
    assert(b.get(black_bishop).piece == '.');
  }
}

bool verify(const std::string& pgn, const std::string& expected, int N)
{
  try
  {
    ChessBoard b;
    std::istringstream s(pgn);
    TokenScanner scanner(s);
    PGNParser parser;
    for (const auto& token : scanner)
    {
      auto action = parser.consume_token(token);
      if (action)
      {
        if (std::get_if<Finish>(&*action))
          break;

        b.apply(*action);
      }
    }

    std::stringstream o;
    o << b;

    std::cout << "---------------------\n" << o.str();
    const std::string output = o.str();
    return output == expected;
  }
  catch (const std::exception& e)
  {
    std::cout << "[" << N << "] caught exception: [" << e.what() << "]\n";
    assert(false);
  }
  return false;
}

void test_rav()
{
  const std::string expected = R"(bR|bN|bB|bQ|bK|bB|bN|bR
bP|bP|bP|bP|bP|bP|bP|bP
  |  |  |  |  |  |  |  
  |  |  |  |  |  |  |  
  |  |  |  |  |  |  |  
  |  |  |  |  |  |  |  
wP|wP|wP|wP|wP|wP|wP|wP
wR|wN|wB|wQ|wK|wB|wN|wR
)";
  assert(verify("(asdfasdf {asdfasd)(f})", expected, 1));
}

void integration_tests()
{
  //#1
  {
    const std::string pgn = R"(
[Event "F/S Return Match"]
[Site "Belgrade, Serbia JUG"]
[Date "1992.11.04"]
[Round "29"]
[White "Fischer, Robert J."]
[Black "Spassky, Boris V."]
[Result "1/2-1/2"]
%alkjalksdf
1. e4 .. e5 2. Nf3 Nc6 $122 3. Bb5 a6 {This opening is called the Ruy Lopez.}
4. Ba4 Nf6 5. O-O Be7 6. Re1 b5 7. Bb3 d6 8. c3 O-O 9. h3 Nb8 10. d4 Nbd7;akljalksdf
11. c4 c6 12. cxb5 axb5 13. Nc3 Bb7 14. Bg5 b4 15. Nb1 h6 16. Bh4 c5 17. dxe5;asdf
%alkjasdkflasdflk
Nxe4 18. Bxe7 Qxe7 19. exd6$122 Qf6 20.$6 Nbd2$ Nxd6 21. Nc4 Nxc4 22. Bxc4 Nb6 ;
23. Ne5 e.p. Rae8 24. Bxf7+ Rxf7 25. Nxf7 Rxe1+ 26. Qxe1 Kxf7 27. Qe3 Qg5 28. Qxg5
hxg5 29. b3 Ke6 30. a3 Kd6 31. axb4 cxb4 32. Ra5 Nd5 33. f3 Bc8 34. Kf2 Bf5; alskjalskdfasdflkj
35. Ra7 g6 36. Ra6+ Kc5 37. Ke1 Nf4 38. g3 Nxh3 39. Kd2 Kb5 40. Rd6 Kc5 41. Ra6
({After the game, Anand explained that this was the critical moment. There was a rather "messy" line for black with: }
27 ..Qf4 $1 28 Rg7+ Kd6 29 Rxe6+ Kd5 30 Rd7+ ({ or even} 30 c4+ ) 30 ..Nxd7 31 c4+ Qxc4 (31 ..Kxc4 $2 32 Re4+ Qxe4+ 33 fxe4) 32 Qg8 Rxb2+ 33 Kxb2 Bg7+ (33 ..Rb8+ $2 34 Rb6+ $1) (33 ..Qb4+ 34 Kc2 Qc4+ 35 Kd1 Qd4+ 36 Ke2 {and the king escapes to safety on the kingside.}) 34 Re5+ Kxe5 35 Qxc4 Kxf5+ {and after the storm has settled, black has a small material advantage, but it will be a draw due to the king never being able to find shelter from the many queen checks. However, Anand felt sure that Bareev [which he confirmed after the game] was expecting a repetition here with Bg6+ and Bf5, so balked at the idea of playing the messy line in preference for an easy life. He was in for a shock...} )
Nf2 42. g4 Bd3 43. Re6 1/2-1/2
)";
    const std::string expected = R"(  |  |  |  |  |  |  |  
  |  |  |  |  |  |  |  
  |  |  |  |wR|  |bP|  
  |  |bK|  |  |  |bP|  
  |bP|  |  |  |  |wP|  
  |wP|  |bB|  |wP|  |  
  |  |  |wK|  |bN|  |  
  |  |  |  |  |  |  |  
)";
    assert(verify(pgn, expected, 1));
  }

  //#2
  {
    const std::string pgn = R"(
[ECO "C67"]

1. e4 e5 2. Nf3 Nc6 3. Bb5 Nf6 4. O-O Nxe4 5. d4 Nd6 6. Bxc6 dxc6 7. dxe5
Nf5 8. Qxd8+ Kxd8 9. Nc3 Ke8 10. h3 Be7 11. Bf4 Be6 12. g4 Nh4 13. Nxh4
Bxh4 14. Kg2 Be7 15. Rfd1 Rd8 16. f3 h5 17. Ne2 c5 18. Ng3 hxg4 19. hxg4
Rxd1 20. Rxd1 c6 21. Nh5 Rh7 22. Be3 b6 23. b3 c4 24. Nf4 cxb3 25. axb3 c5
26. Nxe6 fxe6 27. b4 cxb4 28. Ra1 a5 29. Bxb6 Bg5 30. Rxa5 Bf4 31. Bg1 Kf7
32. Rb5 Rh8 33. Rxb4 Bxe5 34. f4 Rb8 35. Ra4 Bd6 36. Ra7+ Ke8 37. Rxg7 Rb2
38. Rg8+ Kd7 39. Kf3 Rxc2 40. Be3 Rc3 41. Rg5 Ra3 42. Rh5 Kc6 43. Rh8 Kd7
44. Rh7+ Kc6 45. Rh2 Bc5 46. Re2 Kd7 47. Kf2 Be7 48. Rc2 Bd6 49. Rc4 Rb3
50. Kf3 Ke7 51. Rc6 Bb8 52. Rc2 Kf7 53. Rd2 Rb7 54. Rd3 Kg6 55. Rd8 Ba7 56.
Rd6 Kf7 57. Bxa7 Rxa7 58. g5 Ra3+ 59. Kg4 Ra1 60. Rd7+ Kg6 61. Re7 Rg1+ 62.
Kf3 Re1 63. Kf2 Re4 64. Kg3 Kf5 65. Rf7+ Kg6 66. Rf6+ Kg7 67. Kf3 Re1 68.
Rh6 Kf7 69. Kf2 Re4 70. Kg3 Re1 71. Rf6+ Kg7 72. Kf3 Kg8 73. Kg4 Kg7 74.
Kf3 Kg8 75. Rh6 Kg7 76. Rh2 Kg6 77. Rc2 Rf1+ 78. Ke3 Re1+ 79. Kf2 Re4 80.
Kf3 Re1 81. Rc5 Rf1+ 82. Ke3 Re1+ 83. Kf2 Re4 84. Kf3 Re1 85. Rc3 Kf5 86.
Rc5+ Kg6 87. Rc7 Rf1+ 88. Ke3 Re1+ 89. Kf2 Re4 90. Kf3 Re1 91. Rc8 Kf7 92.
Rc4 Rf1+ 93. Ke2 Ra1 94. Rc7+ Kf8 95. Kf3 Rg1 96. Ke4 Re1+ 97. Kd4 Kg8 98.
Rb7 Kf8 99. g6 Rg1 100. Rf7+ Ke8 101. Rg7 Kf8 1/2-1/2
)";
    const std::string expected = R"(  |  |  |  |  |bK|  |  
  |  |  |  |  |  |wR|  
  |  |  |  |bP|  |wP|  
  |  |  |  |  |  |  |  
  |  |  |wK|  |wP|  |  
  |  |  |  |  |  |  |  
  |  |  |  |  |  |  |  
  |  |  |  |  |  |bR|  
)";
    assert(verify(pgn, expected, 2));
  }

  {

    const std::string pgn = R"([Event "GMA"]
[Site "Wijk aan Zee NED"]
[Date "2004.01.19"]
[Round "8"]
[White "Anand,V"]
[Black "Bareev,E"]
[Result "1-0"]
[WhiteElo "2766"]
[BlackElo "2714"]
[EventDate "2004.01.10"]
[ECO "C11"]
[Annotator "Henderson"]

1 e4 e6 
{After his previous shocking results with the French, Bareev sat with his head in his hands before replying, contemplating his options. The omens didn't look good for the Russian, however. After about a minute he picked up the e-pawn and before it got to e6 it had slipped out of his hand and noisily rolled across the board! }
2 d4 d5 3 Nc3 Nf6 4 Bg5 dxe4 5 Nxe4 Be7 6 Bxf6 Bxf6 7 Nf3 O-O 8 Qd2 Nd7 9 O-O-O Be7 10 Bd3 b6 11 h4 Bb7 12 Neg5 Nf6 13 c3 Bxf3 14 gxf3 c5 15 dxc5 Qc7 16 Kb1 bxc5 17 Rdg1 Rfd8 18 Qc2 h6 19 Bh7+ Kf8 20 Nxf7 
{Now the fireworks begin! }
20 ..Kxf7 21 Qg6+ Kf8 22 Qxg7+ Ke8 23 Re1 
(23 Bc2 $1 {offered better prospects according to Anand.} )
23 ..Rd6 24 Qh8+ Bf8 25 Bg6+ Ke7 26 Rhg1 Rb6 27 Bf5 Kf7 $2 28 Bg6+ Ke7 29 Bc2 $1 Kf7 
30 Rg6 Qf4 31 Reg1 e5 32 Rg7+ Ke6 33 R1g6 Rab8 34 Qg8+ Kd6 35 Rxf6+ Qxf6 36 Rg6 Kc7 37 Rxf6 Rxf6 38 Qh7+ Kb6 39 Be4 Rd6 40 h5 a6 41 Qf7 Rd2 42 a3 Rd1+ 43 Kc2 Rd6 44 b4 cxb4 45 axb4 Rdd8 46 Qe6+ Rd6 47 Qc4 Rf6 48 Qd5 
1-0 
)";
    const std::string expected = R"(  |bR|  |  |  |bB|  |  
  |  |  |  |  |  |  |  
bP|bK|  |  |  |bR|  |bP
  |  |  |wQ|bP|  |  |wP
  |wP|  |  |wB|  |  |  
  |  |wP|  |  |wP|  |  
  |  |wK|  |  |wP|  |  
  |  |  |  |  |  |  |  
)";

    assert(verify(pgn, expected, 3));
  }

  //#4
  {
    const std::string pgn = R"(
[Event "GMA"]
[Site "Wijk aan Zee NED"]
[Date "2004.01.20"]
[Round "9"]
[White "Shirov,A"]
[Black "Sokolov,I"]
[Result "1-0"]
[WhiteElo "2736"]
[BlackElo "2706"]
[EventDate "2004.01.10"]
[ECO "C72"]
[Annotator "Henderson"]

1 e4 e5 2 Nf3 Nc6 3 Bb5 a6 4 Ba4 d6 5 O-O Bg4 6 h3 h5 7 Bxc6+ bxc6 8 d4 Qf6 
({This is an old line that has been played a couple of times in the last few years. The modern line is:} 8 ..Bxf3 9 Qxf3 exd4) 
9 Nbd2 Be6 
{quite a surprise for Shirov, he expected either g7-g5 or Ne7} 
10 Nb3 
({Preventing any ideas g7-g5.} 10 c3 g5 11 Qa4 Ne7 12 dxe5 dxe5 13 Nc4 Bxc4 14 Qxc4 Bh6 {and with g5-g4 coming, black is probably more than ok here} )
10 ..Qg6 11 Ng5 
({Threatens to exchange on e6 which would give white a clear edge} 11 Qd3 f6 {and black is fine} (11 ..Bxh3 12 Nh4 $1) )
11 ..Bd7 
(11 ..Bc4 12 Re1 Be7 13 Na5 $1 ( 13 Nf3 f6 {and black is ok}) 13 ..Bxg5 14 Nxc4 Bxc1 15 Rxc1 f6 {and white is better due to the development and state of black's queenside pawns.} )
12 dxe5 $1 
{Being better developed, white rightly seeks to open as many lines as possible. }
12 ..dxe5 13 f4 exf4 14 Bxf4 Be7 $2 
({The decisive mistake. Black's only option was:} 14 ..f6 15 Nf3 Bxh3 16 Nh4 Qg4 17 Qxg4 Bxg4 18 Ng6 Rh7 19 Bxc7 {with white having a small advantage.} )
15 Qd2 
{Now white has a big advantage} 
15 ..Rd8 
(15 ..f6 {is one move too late - and with the extra tempo compared to the previous note. White now wins by:} 16 Rad1 Rd8 17 Bxc7 Qxg5 18 Qd3 $1 Bxh3 19 Qxh3 $18 )
16 Nxf7 $1 
{As Shirov put it: "No need to calculate everything, with two pawns and a rook for the two light pieces white is, materially speaking, fine and still far ahead in development." }
16 ..Qxf7 17 Bxc7 Qe6 ( 17 ..Nf6 18 Bxd8 Bxd8 19 e5 $18 )
18 Bxd8 
({an interesting alternative calculated by Shirov was:} 18 Nd4 Qc4 19 b3 Qc5 20 Bxd8 (20 e5 Rc8 21 Bd6 Bxd6 22 exd6 Nf6 {and black is ok}) 20 ..Bxd8 21 e5 Bb6 22 c3 Ne7 {and white is much better but it is less clear than in the game} )
18 ..Bxd8 19 Kh1 $1 
({Less clear is:} 19 Nc5 Bb6 20 Qb4 Ba7 (20 ..Bxc5+ 21 Qxc5) 21 Kh1 Qd6 22 Nxa6 Qxb4 23 Nxb4 Nf6 24 e5 (24 Rae1 h4 { with Nh5 coming and a very unclear position}) 24 ..Ne4 {and suddenly black has some serious threats} )
19 ..Nf6 $6 
({Granted a difficult position for Sokolov, however he could have put up greater resistance with:} 19 ..Be7 20 Rad1 Nf6 21 e5 Ne4 22 Qe3 Qg6 {this may be much better than in the game, however white still holds an obvious advantage. })
20 Nc5 Qe7 21 Rad1 Bc8 
({the alternatives fair no better:} 21 ..Qxc5 22 Rxf6 $1 Bxf6 23 Qxd7+ Kf8 24 Qc8+ $18)(21 ..Bb6 22 e5 Nd5 23 Nxd7 Qxd7 24 c4 $18)
22 e5 Nd5 23 Ne4 Qxe5 
(23 ..Bc7 24 Nd6+ $1 Bxd6 25 exd6 Qxd6 26 c4 $18 )
24 Rde1 Be7 25 c4 Bb4 26 Nc3 
1-0 ")";
    const std::string expected = R"(  |  |bB|  |bK|  |  |bR
  |  |  |  |  |  |bP|  
bP|  |bP|  |  |  |  |  
  |  |  |bN|bQ|  |  |bP
  |bB|wP|  |  |  |  |  
  |  |wN|  |  |  |  |wP
wP|wP|  |wQ|  |  |wP|  
  |  |  |  |wR|wR|  |wK
)";
    assert(verify(pgn, expected, 5));
  }
  {
    const std::string pgn = R"(
[Event "GMA"]
[Site "Wijk aan Zee NED"]
[Date "2004.01.20"]
[Round "9"]
[White "Bologan,V"]
[Black "Akopian,Vl"]
[Result "1-0"]
[WhiteElo "2679"]
[BlackElo "2693"]
[EventDate "2004.01.10"]
[ECO "D48"]

1. d4 Nf6 2. c4 e6 3. Nf3 d5 4. Nc3 c6 5. e3 Nbd7 6. Bd3 dxc4 7. Bxc4 b5 8.
Bd3 Bb7 9. O-O a6 10. e4 c5 11. d5 Qc7 12. dxe6 fxe6 13. Bc2 Bd6 14. Ng5
Nf8 15. f4 h6 16. e5 O-O-O 17. exd6 Rxd6 18. Qe2 hxg5 19. Ne4 Bxe4 20. Bxe4
c4 21. Bc2 g4 22. Be3 Rd5 23. g3 Rdh5 24. Rf2 N8d7 25. a4 Qc6 26. axb5 axb5
27. b3 c3 28. Bd4 Rh3 29. Qd3 Nd5 30. Bxg7 Nc5 31. Qd4 R8h5 32. f5 Nb4 33.
Be5 Qd5 34. Ra8+ Qxa8 35. Qxc5+ Nc6 36. Be4 Qa1+ 37. Rf1 Qa7 38. Qxa7 Nxa7
39. Bxc3 exf5 40. Bxf5+ Kb7 41. Bxg4 Rxh2 42. Rf7+ Kb8 43. Bxh5 Rxh5 44.
Kg2 Nc6 45. g4 Rd5 46. Bf6 Kc8 47. Kg3 Rd7 48. Rf8+ Kc7 49. g5 Rd1 50. Kf2
Rd2+ 51. Kf3 Rd3+ 52. Kf4 Rd1 53. g6 Nd4 54. Rf7+ Kd6 55. Bxd4 Rxd4+ 56.
Kf5 Rd5+ 57. Kg4 Ke6 58. Rf3 Rd1 59. g7 1-0
)";
    const std::string expected = R"(  |  |  |  |  |  |  |  
  |  |  |  |  |  |wP|  
  |  |  |  |bK|  |  |  
  |bP|  |  |  |  |  |  
  |  |  |  |  |  |wK|  
  |wP|  |  |  |wR|  |  
  |  |  |  |  |  |  |  
  |  |  |bR|  |  |  |  
)";
    assert(verify(pgn, expected, 6));
  }

  {
    const std::string pgn = R"(
1. e4 e5
2. Nf3 Nf6
3. d4 exd4
4. e5 Ne4
5. Qxd4 d5 
6. exd6 e.p
)";
    const std::string expected = R"(bR|bN|bB|bQ|bK|bB|  |bR
bP|bP|bP|  |  |bP|bP|bP
  |  |  |wP|  |  |  |  
  |  |  |  |  |  |  |  
  |  |  |wQ|bN|  |  |  
  |  |  |  |  |wN|  |  
wP|wP|wP|  |  |wP|wP|wP
wR|wN|wB|  |wK|wB|  |wR
)";
    assert(verify(pgn, expected, 7));
  }

  {
    const std::string pgn = R"(
1.d4 Nf6 2.c4 c5 3.d5 e6 4.Nc3 exd5 5.cxd5 d6 6.e4 g6 7.f4 Bg7 8.Nf3 O-O 9.Be2 Re8 10.Nd2 a6 11.a4 Nbd7 12.O-O Rb8 13.a5 Qc7 14.Qc2 b5 15.axb6 Nxb6 16.Bf3 c4 17.Kh1 Bh6 18.Ne2 Qc5 19.b4 cxb3 20.Qxb3 Qc7 21.Qd3 Bd7 22.Rxa6 Nbxd5 23.Qd4 Bb5 24.Ra7 Qc5 25.Qxc5 dxc5 26.exd5 Bxe2 27.Bxe2 Rxe2 28.d6 Bf8 29.f5 gxf5 30.Ra6 Rd8 31.Nc4 Re4 32.Ne3 Rxd6 33.Ra8 f4 34.Nf5 Rd5 35.g4 fxg3 36.Nxg3 Re8 37.Ra2 Bg7 38.Nf5 Kf8 39.Nxg7 Kxg7 40.Rg2+ Kf8 41.Bh6+ Ke7 42.Bg5 Rxg5 43.Rxg5 Rg8 44.Rxc5 Rg6 45.Rc8 Nd5 46.Ra8 Rf6 47.Rb1 Rf5 48.Rh8 h5 49.Re1+ Kf6 50.Rh6+ Kg7 51.Rd6 Kh7 52.Ra6 Nc7 53.Raa1 Ne6 54.Rf1 Rxf1+ 55.Rxf1 Kg6 56.Kg2 Nd4 57.Ra1 Kg5 58.Ra5+ f5 59.h4+ Kf4 60.Ra6 Ne2 61.Rg6 Nd4 62.Rg8 Nc6 63.Rg5 Ne5 64.Kh3 Nd3 65.Rxh5 Nf2+ 66.Kg2 Kg4 67.Rh8 Nd3 68.Rg8+ Kxh4 69.Kf3 Kh5 70.Re8 Kg5 1/2-1/2
)";
    const std::string expected = R"(  |  |  |  |wR|  |  |  
  |  |  |  |  |  |  |  
  |  |  |  |  |  |  |  
  |  |  |  |  |bP|bK|  
  |  |  |  |  |  |  |  
  |  |  |bN|  |wK|  |  
  |  |  |  |  |  |  |  
  |  |  |  |  |  |  |  
)";
    assert(verify(pgn, expected, 8));
  }

  {
    const std::string pgn = R"(
[Event "London"]
[Site "?"]
[Date "1912.??.??"]
[Round "?"]
[White "Lasker, Edward"]
[Black "Thomas, George"]
[Result "1-0"]
[ECO "A83"]
[Annotator "Wall"]
[Plycount "35"]
[Eventdate "1912.??.??"]
[Sourcedate "2013.04.01"]

1. d4 {Her honour's pawn.  THE TWO GENTLEMENOF VERONA.  Act 1, Scene 3.} 1... f5
2. e4 fxe4 3. Nc3 {Rise up knight.  2 HENRY VI, Act 5, Scene 1} 3... Nf6 {It is
a merry knight.  THE MERRY WIVES OF WINDSOR.  Act 2, Scene 1} 4. Bg5 {The bishop
will be overborne by thee.  1 KING HENRY VI, Act 5, Scene 1} 4... e6 5. Nxe4 {I
say his horse comes.  THE TAMING OF THE SHREW.  Act 3, Scene 2} 5... Be7 6. Bxf6
{An uproar, I dare warrant, begun through malice of the bishop's men.  1 KING
HENRY VI, Act 3, Scene 1} 6... Bxf6 {Bishop, farewell.  3 KING HENRY VI, Act 4,
Scene 5} 7. Nf3 {The worthy knight.  LOVE'S LABORS LOST,  Act 5, Scene 1} 7...
b6 8. Ne5 {And I have horse will follow where the game makes way.  TITUS
ANDRONICUS, Act 2, Scene 2} 8... O-O 9. Bd3 Bb7 10. Qh5 {She shall be a high and
mighty queen. KING RICHARD III, Act 4, Scene 4} 10... Qe7 {It is his highness'
pleasure that the queen appear.  THE WINTER'S TALE, Act 3, Scene 2} 11. Qxh7+
{Come hither, come $1  Come, come, and take a queen.  ANTONY AND CLEOPATRA, Act 5,
Scene 2} 11... Kxh7 {So that we fled; the king unto the queen.  3 KING HENRY VI,
Act 2, Scene 2} 12. Nxf6+ {There, give it your...horse.  OTHELLO, Act 4, Scene
1} 12... Kh6 {What must the king do now; must he submit $2  KING RICHARD II, Act
3, Scene 3} 13. Neg4+ {Come knight; come knight.  TWELFTH KNIGHT, Act 2, Scene
3} 13... Kg5 {The king is render'd lost.  ALL'S WELL THAT ENDS WELL, Act 1,
Scene 3} 14. h4+ {Uneasy lies the head that wears the crown.  2 HENRY IV, Act 3,
Scene 1} 14... Kf4 {The king is almost wounded to the death.  2 KING HENRY IV,
Act 1, Scene 1} 15. g3+ Kf3 {The king will labour still to save his life.  2
KING HENRY VI, Act 3, Scene 1} 16. Be2+ Kg2 {Help, lords $1  The king is dead.  2
KING HENRY VI, Act 3, Scene 2} 17. Rh2+ {What says my bully-rook $2  THE MERRY
WIVES OF WINDSOR, Act 1, Scene 3} 17... Kg1 {Most degenerate king $1  KING RICHARD
II, Act 2, Scene 1} 18. Kd2# {Ay...the king is dead.  KING RICHARD III, Act 2,
Scene 2.  Good king, to be so mightily abused.  TITUS ANDRONICUS, Act 2, Scene
3.  So thou, that hast no unkind mate.  THE WINTER'S TALE, Act 2, Scene 1} 1-0
)";
    const std::string expected = R"(bR|bN|  |  |  |bR|  |  
bP|bB|bP|bP|bQ|  |bP|  
  |bP|  |  |bP|wN|  |  
  |  |  |  |  |  |  |  
  |  |  |wP|  |  |wN|wP
  |  |  |  |  |  |wP|  
wP|wP|wP|wK|wB|wP|  |wR
wR|  |  |  |  |  |bK|  
)";
    assert(verify(pgn, expected, 9));
  }

  {
    const std::string pgn = R"(
[Event "FICS rated standard game"]
[Site "FICS freechess.org"]
[FICSGamesDBGameNo "700321693"]
[White "exeComp"]
[Black "Horsian"]
[WhiteElo "2745"]
[BlackElo "2867"]
[WhiteRD "41.0"]
[BlackRD "49.4"]
[TimeControl "900+1"]
[Date "2024-03-06"]
[Time "13:15:00"]
[Duration "0:35:06"]
[WhiteClock "0:15:00.000"]
[BlackClock "0:15:00.000"]
[Result "1/2-1/2"]
[LongResult "Game drawn by the 50 move rule"]

1. e4 e5 2. Nf3 Nc6 3. Bb5 Nf6 4. O-O Nxe4 5. Re1 Nd6 6. Nxe5 Be7 7. Bf1 Nxe5 8. Rxe5 O-O 9. d4 Bf6 10. Re1 Re8 11. c3 Rxe1 12. Qxe1 Nf5 13. Bf4 h6 14. Bd3 d5 15. Nd2 Be6 16. Qe2 b6 17. Nf3 c5 18. h3 cxd4 19. g4 Nh4 20. Nxh4 Bxh4 21. cxd4 Bf6 22. Qe3 Qe8 23. Be5 Bxe5 24. Qxe5 Qa4 25. Qf4 Qb4 26. Rb1 Rc8 27. a3 Qb3 28. Qd2 Bd7 29. Kg2 g6 30. Qd1 Qxd1 31. Rxd1 Kf8 32. f4 b5 33. Kf3 Ke7 34. Re1+ Kd6 35. Rh1 Ke7 36. h4 b4 37. Re1+ Kd6 38. axb4 Rb8 39. Ra1 Rxb4 40. Ra6+ Ke7 41. Rxa7 Ke8 42. Ra8+ Ke7 43. Ra7 Ke8 44. Ra3 Ke7 45. h5 gxh5 46. gxh5 Rxb2 47. Ra7 Rb3 48. Ke2 Rb2+ 49. Ke3 Kd6 50. Ra8 Rb6 51. f5 Rb3 52. Kd2 Rb2+ 53. Ke3 Ke7 54. Kf4 Rb3 55. Be2 f6 56. Rh8 Rb4 57. Rh7+ Kd6 58. Ke3 Rb3+ 59. Kf4 Rb2 60. Bf3 Rb4 61. Rxh6 Rxd4+ 62. Kg3 Bxf5 63. Rxf6+ Ke5 64. Rf7 Rb4 65. Rxf5+ Kxf5 66. Bxd5 Kf6 67. h6 Kg6 68. Kf3 Kxh6 69. Ke3 Kg5 70. Kd3 Rb1 71. Kd4 Kf4 72. Bg2 Rb4+ 73. Kc5 Rb1 74. Kc4 Rc1+ 75. Kd3 Rc7 76. Bh1 Rd7+ 77. Kc4 Rd1 78. Ba8 Ke5 79. Bg2 Rd4+ 80. Kc3 Rd1 81. Ba8 Ra1 82. Bg2 Ra7 83. Kc4 Rc7+ 84. Kd3 Kd6 85. Kd4 Rh7 86. Ke4 Rh4+ 87. Kf5 Kc7 88. Ke5 Kb6 89. Kd5 Kc7 90. Bf3 Rb4 91. Ke5 Kd7 92. Bh1 Rb1 93. Bf3 Rb3 94. Bd1 Ra3 95. Ke4 Kd6 96. Bf3 Rc3 97. Bh1 Rb3 98. Bg2 Rb4+ 99. Kf5 Ra4 100. Bf3 Ra1 101. Ke4 Rb1 102. Bg2 Ra1 103. Bf3 Rb1 104. Kd4 Ke6 105. Bg4+ Kf6 106. Bf3 Ra1 107. Bb7 Rb1 108. Bg2 Ra1 109. Bb7 Rb1 110. Bg2 Rb4+ 111. Kd3 Ke5 112. Ba8 Rb1 113. Bg2 Kd6 114. Kd4 Rb5 115. Kc4 Rb1 116. Ba8 Kc7 117. Kd4 Kb8 118. Bg2 Ra1  {Game drawn by the 50 move rule} 1/2-1/2
)";
    const std::string expected = R"(  |bK|  |  |  |  |  |  
  |  |  |  |  |  |  |  
  |  |  |  |  |  |  |  
  |  |  |  |  |  |  |  
  |  |  |wK|  |  |  |  
  |  |  |  |  |  |  |  
  |  |  |  |  |  |wB|  
bR|  |  |  |  |  |  |  
)";
    assert(verify(pgn, expected, 10));
  }

  {
    const std::string pgn = R"(
[Event "GMA"]
[Site "Wijk aan Zee NED"]
[Date "2004.01.25"]
[Round "13"]
[White "Kramnik,V"]
[Black "Leko,P"]
[Result "1/2-1/2"]
[WhiteElo "2777"]
[BlackElo "2722"]
[EventDate "2004.01.10"]
[ECO "E15"]

1. d4 Nf6 2. c4 e6 3. Nf3 b6 4. g3 Ba6 5. b3 Bb4+ 6. Bd2 Be7 7. Bg2 c6 8.
Ne5 d5 9. Bc3 O-O 10. Nd2 c5 11. dxc5 Bxc5 12. O-O Bb7 13. e3 Be7 14. Rc1
Nbd7 15. Nxd7 Qxd7 16. Bxf6 Bxf6 17. cxd5 exd5 18. Ne4 Be5 19. Qd3 Rad8 20.
Ng5 g6 21. Nf3 Bf6 22. Rfd1 Rc8 23. Nd4 Rxc1 24. Rxc1 Rc8 25. Rd1 Qc7 26.
Ne2 Qc2 27. Nf4 Qxa2 28. Nxd5 Bxd5 29. Bxd5 Rc7 30. Bc4 Qa3 31. e4 b5 32.
Bxb5 Rc3 33. Qd7 Qxb3 34. e5 Be7 35. Be2 Qe6 36. Qxe6 fxe6 37. Rd7 Kf8 38.
Rxa7 Rc2 39. Kf1 Rc1+ 40. Kg2 Rc2 41. Kf3 g5 42. Ra8+ Kg7 43. Bd3 Rc5 44.
Ke4 Rc6 45. f4 gxf4 46. gxf4 Kf7 47. Ra4 Bd8 48. Bc4 Ke7 49. Kd3 Bc7 50.
Ra8 Bd8 51. Ra7+ Bc7 52. Bb5 Rc5 53. Kd4 Rd5+ 54. Kc4 Kd8 55. f5 Rd4+ 56.
Kc5 Rd5+ 57. Kc4 Rd4+ 58. Kxd4 Bb6+ 59. Kd3 Bxa7 60. f6 Bg1 61. h3 Bc5 62.
Ke4 Ba3 63. h4 Bb2 64. Kf4 h6 65. Bc4 1/2-1/2
)";
    const std::string expected = R"(  |  |  |bK|  |  |  |  
  |  |  |  |  |  |  |  
  |  |  |  |bP|wP|  |bP
  |  |  |  |wP|  |  |  
  |  |wB|  |  |wK|  |wP
  |  |  |  |  |  |  |  
  |bB|  |  |  |  |  |  
  |  |  |  |  |  |  |  
)";
    assert(verify(pgn, expected, 11));
  }

  {
    const std::string pgn = R"(
[Event "GMA"]
[Site "Wijk aan Zee NED"]
[Date "2004.01.10"]
[Round "1"]
[White "Akopian,Vl"]
[Black "Kramnik,V"]
[Result "1-0"]
[WhiteElo "2693"]
[BlackElo "2777"]
[EventDate "2004.01.10"]
[ECO "B90"]
[Annotator "Henderson"]

1 e4 c5 2 Nf3 d6 3 d4 cxd4 4 Nxd4 Nf6 5 Nc3 a6 
{Akopian explained that the last time he played a main line Najdorf was against Nigel Short at the Lucerne FIDE World Championship 1997. Normally he would play something like a Moscow Variation with Bb5+ or even a Kopec System with 3 c3 followed by Bd3. However, he suspected that this is perhaps what Kramnik thought he would play, so instead opted for the main line Najdorf - coincidentally the first time Kramnik has played this as Black.} 
6 Be3 Ng4 
({The Kasparov System; pioneered by the man himself to avoid all the complications of the double-edged Perenyi System with} 6... e6 7 g4 e5 8 Nf5 $5 g6 9 g5) 
7 Bg5 h6 8 Bh4 g5 9 Bg3 Bg7 10 h3 Ne5 11 f3 Nbc6 12 Bf2 Be6 13 Qd2 Nxd4 14 Bxd4 Qa5 15 a3 O-O 16 h4 Ng6 17 hxg5 hxg5 18 b4 Qc7 
19 Ne2 $5 $146 
({Now after some 40 minutes thought, Kramnik opts to prevent the exchange of bishops as this would leave his king in a vulnerable position. 
Up to now the stem game we have been following is Anand-Ponomariov from their Mainz match last year. However Akopian comes up an interesting novelty, aimed at re-routing the knight to d4 from where it can exploit f5.} 19 Bxg7 Kxg7 20 O-O-O Rh8 21 Rxh8 Rxh8 22 Kb2 f6 23 g3 Ne5 24 f4 gxf4 25 gxf4 Nc4+ 26 Bxc4 Qxc4 27 e5 dxe5 28 fxe5 f5 29 Qg2+ Kf7 30 Rd3 Qf4 31 Qxb7 Rh2 32 Qf3 Qxe5 33 Re3 Qd4 34 Re2 Rh8 35 Re4 Qf6 36 Re3 Qd4 37 Qe2 Qc4 38 Qg2 Qd4 39 Qe2 Qc4 40 Qg2 Qd4 {1/2-1/2 Anand,V-Ponomariov,R/Mainz GER 2002/The Week in Chess 406 (40)})
19 ..f6 20 Bb2 $1 Bf7 21 Nd4 d5 22 exd5 Qe5+ 
({Kramnik by now didn't feel very comfortable with his position - and obviously felt his king was more at risk with the alternatives:} 22 ..Qg3+ 23 Kd1 $1 {followed by a Nf5 is good for white.})
( 22 ..Bxd5 23 Bd3 Nf4 24 Bh7+ Kf7 25 g3 Ne6 26 O-O-O $1 Nxd4 27 Qxd4 Rfd8 ( 27 ..Bxf3 $2 28 Qd3 $1) 28 Qg4 $1) 
23 Be2 Qxd5 
({Exchanging queens wasn't any better for Kramnik, as the ending also favoured white:} 23 ..Bxd5 24 O-O-O $1 Qf4 25 Bd3 Qxd2+ 26 Kxd2 $1 Ne5 ( 26 ..Nf4 27 c4 Nxd3 28 Kxd3 Bf7 29 c5 $1) 27 Nf5 Nxd3 28 Kxd3 e6 29 Ne7+ Kf7 30 Nxd5 exd5) 
24 O-O-O Rfc8 $6 
{Akopian felt that this is where Kramnik really lost the thread of the game - much better, he thought, was the natural 24 ..Rfd8 } 
25 Bd3 Ne5 26 Be4 Qa2 27 Nf5 
({The safe option. However, on reflection, Akopian discovered after the game that he could have just taken on b7 having missed a finesse in a tricky position:} 27 Bxb7 Nc4 28 Qc3 Nb6 29 Nc6 ( 29 Bxc8 Na4 $1) 29 ..Rxc6 30 Bxc6 Rc8 31 Qd4 $1)
27 ..Nc4 28 Qc3 Rc7 $4 
({A big blunder from the World Champion. Kramnik told Akopian after the game that he didn't fancy the endgame after the exchange of queens. Akopian believed the ending wasn't all that decisive for white:} 28 ..Qxb2+ 29 Qxb2 Nxb2 30 Kxb2 Rc7 {Instead, Kramnik fails to spot the stunning riposte coming up.} )
29 Rh7 $1 Qxb2+ 
({Take my word for it: there's a forced mate if you take the rook} 29 ..Kxh7 30 Nxe7+ Kh6 31 Rh1+ Bh5 32 g4 Qxb2+ 33 Qxb2 Nxb2 34 Rxh5# )
({ Also no better is} 29 ..Nxb2 30 Rxg7+ Kf8 31 Qxb2 Qxb2+ 32 Kxb2 e6 33 Rxf7+ Kxf7 34 Bxb7 {with an easily won ending.})
30 Qxb2 Nxb2 31 Rxg7+ Kf8 32 Rh1 $1 
{Much stronger than "simply" going material ahead after Kxb2} 
1-0
)";
    const std::string expected = R"(bR|  |  |  |  |bK|  |  
  |bP|bR|  |bP|bB|wR|  
bP|  |  |  |  |bP|  |  
  |  |  |  |  |wN|bP|  
  |wP|  |  |wB|  |  |  
wP|  |  |  |  |wP|  |  
  |bN|wP|  |  |  |wP|  
  |  |wK|  |  |  |  |wR
)";
    assert(verify(pgn, expected, 12));
  }
}

int main()
{
  test_move_parser();
  test_king_move();
  test_bishop_move();
  test_knight_move();
  test_rook_move();
  test_pawn_move();
  test_pawn_board_moves();
  test_knight_board_moves();
  test_castling();
  test_pawn_en_passant_board_moves();
  test_locked_moves();
  test_rav();
  integration_tests();
  return 0;
}
