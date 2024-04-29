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
#include "parser.h"
#include "scanner.h"
#include <fstream>
#include <iostream>

int main(int argc, char* argv[])
{
  if (argc != 2)
  {
    std::cout << "please run as ./chess_replay [input file]; say "
                 "./chess_replay /data/input/input.data";
    return -1;
  }

  const std::string input_file = argv[1];
  std::ifstream file;

  try
  {
    file.open(input_file);
    if (!file.is_open())
    {
      throw std::runtime_error(std::string("failed to open file [").append(input_file).append("]"));
    }

    ChessBoard board;
    TokenScanner scanner(file);
    PGNParser parser;
    for (const auto& token : scanner)
    {
      std::visit(overloaded{[&](const std::monostate& t)
                            {
                              if constexpr (PRINT_DEBUG_INFO)
                              {
                                std::cout << "[]" << std::endl;
                              }
                            },
                            [&](const auto& t)
                            {
                              if constexpr (PRINT_DEBUG_INFO)
                              {
                                std::cout << t << std::endl;
                              }
                            }},
                 token);
      auto action = parser.consume_token(token);
      if (action)
      {
        if (std::get_if<Finish>(&*action))
          break;

        board.apply(*action);
        if constexpr (PRINT_DEBUG_INFO)
        {
          std::cout << "\n NEW MOVE: " << *action << "\n" << board;
        }
      }
    }

    if (file.bad())
    {
      std::cout << "Failed to parse file [" << input_file << "]\n";
      return -1;
    }

    std::cout << board;
    return 0;
  }
  catch (const std::exception& e)
  {
    std::cout << "got exception while executing the program [" << e.what() << "] \n";
  }
  return -1;
}
