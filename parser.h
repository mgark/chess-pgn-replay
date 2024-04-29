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

#include "common.h"
#include "moves.h"
#include "tokens.h"
#include <assert.h>
#include <bits/ranges_cmp.h>
#include <functional>
#include <memory>
#include <optional>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <variant>

enum class State
{
  Init,
  ParsingLeftBracket,
  ParsingHeaderName,
  ParsingHeaderValue,
  ParsingRightBracket,
  ParsingMove,
  ParsingNumberIndication,
  ParsingPeriod,
  ParsingLeftParenthesis,
  ParsingRightParenthesis,
  ParsingComment,
  Finished
};

struct MoveFactory
{

  Moves operator()(const std::string& val, bool white_turn) const
  {
    if (std::ranges::equal(val, std::string{"e"}) || std::ranges::equal(val, std::string{"p"}))
    {
      // no need to capture 'en passant' capture as it is implicitly derived anyway
      return Ignore{};
    }
    if (std::ranges::equal(val, std::string{"O-O"}))
    {
      return KingCastling{white_turn};
    }
    else if (std::ranges::equal(val, std::string{"O-O-O"}))
    {
      return QueenCastling{white_turn};
    }
    else if (std::ranges::equal(val, std::string{"1-0"}))
    {
      return Finish{TerminationMarker::WHITE_WON};
    }
    else if (std::ranges::equal(val, std::string{"0-1"}))
    {
      return Finish{TerminationMarker::BLAKC_WON};
    }
    else if (std::ranges::equal(val, std::string{"1/2-1/2"}))
    {
      return Finish{TerminationMarker::EVEN};
    }
    else
    {
      // regular move!
      NextMove next_move;
      next_move.orig_token = val;
      auto c = rbegin(val);
      auto needs_one_char = [&val, &c]()
      {
        if (rend(val) == c)
        {
          throw std::runtime_error(
            std::string("bad symbol val to pare next move: ").append(begin(val), end(val)));
        }
      };
      auto has_more_char = [&val, &c]() { return (rend(val) != c); };

      // is_white_move
      {
        next_move.is_white_move = white_turn;
      }

      // try to identify checkmate / ':' capture / check
      // could be potentialy up to 2 of these special moves
      for (size_t i = 1; i <= 2; ++i)
      {
        needs_one_char();
        if (*c == '#')
        {
          next_move.checkmate = true;
          ++c;
        }
        else if (*c == '+')
        {
          next_move.check = true;
          ++c;
        }
        else if (*c == ':')
        {
          next_move.capture = true;
          ++c;
        }
        else
        {
          break;
        }
      }

      if (*c == ')')
      {
        // another way of indicating a promotion
        ++c;
        needs_one_char();
      }

      if (all_possible_pieces().find(*c) != end(all_possible_pieces()))
      {
        // promotion!
        next_move.promote_piece = *c;
        ++c;

        needs_one_char();
        if (*c == '=' || *c == '/' || *c == '(')
          ++c;
      }

      // to
      {
        if (has_more_char() && '1' <= *c && '8' >= *c) // if *src* rank provided
        {
          next_move.dst.x = r(*c);
          ++c;
        }

        if (has_more_char() && 'a' <= *c && 'h' >= *c) // if *src* file provided
        {
          next_move.dst.y = f(*c);
          ++c;
        }
      }

      if (!has_more_char())
      {
        // this is a pawn move
        next_move.piece = 'P';
        return next_move;
      }

      if (*c == 'x' || *c == ':')
      {
        next_move.capture = true;
        ++c;
      }

      // from_row/from_y
      {
        if ('1' <= *c && '8' >= *c) // if *src* rank provided
        {
          next_move.src.x = r(*c);
          ++c;
        }
        if (has_more_char() && 'a' <= *c && 'h' >= *c) // if *src* file provided
        {
          next_move.src.y = f(*c);
          ++c;
        }
      }

      if (has_more_char())
      {
        // actual explicit non-pawn piece
        if (all_possible_pieces().find(*c) != end(all_possible_pieces()))
        {
          next_move.piece = *c;
          ++c;
        }
        else
        {
          throw std::runtime_error(
            std::string("was expecting a piece - bad symbol in next move: ").append(begin(val), end(val)));
        }
      }
      else
      {
        // pawn is an implicit piece
        next_move.piece = 'P';
      }

      if (has_more_char())
      {
        throw std::runtime_error(
          std::string("was NOT expecting a piece - extra symbols in next move: ").append(begin(val), end(val)));
      }
      return next_move;
    }
  }
};

class PGNParser
{
  struct status
  {
    std::unique_ptr<std::function<Moves(const std::string&)>> emit_move;
    std::unordered_map<std::type_index /*event*/, State /*target state*/> transitions;
  };

  std::unordered_map<State, status> automaton_;
  State state_{State::Init};
  int paranthesis_count_{0};
  bool white_turn = false;

public:
  PGNParser()
  {
    auto& init_status = automaton_[State::Init];
    init_status.transitions.emplace(LeftBraceToken::Event, State::ParsingLeftBracket);
    {
      auto& brace_open_status = automaton_[State::ParsingLeftBracket];
      brace_open_status.transitions.emplace(SymbolToken::Event, State::ParsingHeaderName);
      {
        auto& header_name_status = automaton_[State::ParsingHeaderName];
        header_name_status.transitions.emplace(StringToken::Event, State::ParsingHeaderValue);
        {
          auto& header_value_status = automaton_[State::ParsingHeaderValue];
          header_value_status.transitions.emplace(RightBraceToken::Event, State::ParsingRightBracket);
          {
            auto& brace_close_status = automaton_[State::ParsingRightBracket];
            brace_close_status.transitions.emplace(LeftBraceToken::Event, State::ParsingLeftBracket); // header loop as many headers can provided!
            brace_close_status.transitions.emplace(IntegerToken::Event, State::ParsingNumberIndication); // will define transitions for ParsingNumberIndication below
            brace_close_status.transitions.emplace(
              SymbolToken::Event, State::ParsingMove); // will define transitions for ParsingMove below
          }
        }
      }
    }

    init_status.transitions.emplace(IntegerToken::Event, State::ParsingNumberIndication);
    {
      auto& number_indication_status = automaton_[State::ParsingNumberIndication];
      number_indication_status.transitions.emplace(PeriodToken::Event, State::ParsingPeriod);
      {
        auto& period_status = automaton_[State::ParsingPeriod];
        period_status.transitions.emplace(
          PeriodToken::Event, State::ParsingPeriod); // self-loop is possible to ensure many periods can be chained
        period_status.transitions.emplace(SymbolToken::Event, State::ParsingMove);
      }
      number_indication_status.transitions.emplace(SymbolToken::Event, State::ParsingMove); // will handle white move below
    }

    init_status.transitions.emplace(SymbolToken::Event, State::ParsingMove);
    {
      auto& move_status = automaton_[State::ParsingMove];
      move_status.emit_move = std::make_unique<std::function<Moves(const std::string&)>>(
        [this](const std::string& val)
        {
          this->white_turn = !this->white_turn;
          Moves current = MoveFactory()(val, this->white_turn);
          return current;
        });
      move_status.transitions.emplace(SymbolToken::Event, State::ParsingMove);
      move_status.transitions.emplace(IntegerToken::Event, State::ParsingNumberIndication);
      // move_status.transitions.emplace(PeriodToken::Event, State::ParsingPeriod);
    }

    // Terminating states
    automaton_[State::Init].transitions.emplace(AsterkixToken::Event, State::Finished);
    automaton_[State::ParsingHeaderName].transitions.emplace(AsterkixToken::Event, State::Finished);
    automaton_[State::ParsingHeaderValue].transitions.emplace(AsterkixToken::Event, State::Finished);
    automaton_[State::ParsingRightBracket].transitions.emplace(AsterkixToken::Event, State::Finished);
    automaton_[State::ParsingMove].transitions.emplace(AsterkixToken::Event, State::Finished);
    automaton_[State::ParsingNumberIndication].transitions.emplace(AsterkixToken::Event, State::Finished);
    automaton_[State::ParsingPeriod].transitions.emplace(AsterkixToken::Event, State::Finished);
    automaton_[State::ParsingLeftParenthesis].transitions.emplace(AsterkixToken::Event, State::Finished);
    automaton_[State::ParsingRightParenthesis].transitions.emplace(AsterkixToken::Event, State::Finished);
    automaton_[State::ParsingComment].transitions.emplace(AsterkixToken::Event, State::Finished);
  }

  std::optional<Moves> consume_token(const Token& token)
  {
    return std::visit(overloaded{[](const std::monostate&) -> std::optional<Moves>
                                 {
                                   INTERNAL_ASSERT(false);
                                   return {};
                                 },
                                 [&](const auto& t) -> std::optional<Moves>
                                 {
                                   using token_type = std::decay_t<decltype(t)>;
                                   auto event = token_type::Event;
                                   // skip some dummy tokens!
                                   if (event == BraceComment::Event || event == LineComment::Event ||
                                       event == EscapeToken::Event || event == NumericGlyphToken::Event)
                                   {
                                     return {};
                                   }

                                   if (event == LeftParenthesisToken::Event)
                                   {
                                     ++paranthesis_count_;
                                     return {};
                                   }
                                   if (event == RightParenthesisToken::Event)
                                   {
                                     --paranthesis_count_;
                                     return {};
                                   }
                                   INTERNAL_ASSERT(paranthesis_count_ >= 0);

                                   auto& state = automaton_[state_];
                                   INTERNAL_ASSERT(!state.transitions.empty());
                                   auto possible_it = state.transitions.find(event);
                                   if (possible_it == end(state.transitions))
                                   {
                                     // let's allow periods after parsing move
                                     if (state_ == State::ParsingMove && event == PeriodToken::Event)
                                       return {};

                                     std::stringstream ss;
                                     ss << "event[" << typeid(token_type).name() << "] ";
                                     ss << "cannot transition to any knownwn state ";
                                     ss << "from state [" << (size_t)state_ << "]";
                                     throw std::runtime_error(ss.str());
                                   }
                                   else
                                   {
                                     state_ = possible_it->second;
                                     auto new_state_it = automaton_.find(state_);
                                     if (state_ == State::Finished)
                                     {
                                       return paranthesis_count_ > 0 ? std::optional<Moves>{} : Finish();
                                     }

                                     INTERNAL_ASSERT(new_state_it != end(automaton_));
                                     if (state_ == State::ParsingMove && new_state_it->second.emit_move)
                                     {
                                       if constexpr (requires { t.value_; })
                                       {
                                         return paranthesis_count_ > 0
                                           ? std::optional<Moves>{}
                                           : (*new_state_it->second.emit_move)(t.value_);
                                       }
                                     }
                                   }

                                   return {};
                                 }},
                      token);
    return {};
  }
};
