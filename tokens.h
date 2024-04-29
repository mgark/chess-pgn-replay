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
#include <cctype>
#include <ostream>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <variant>
#include <vector>

enum class AcceptResult
{
  CONSUMED,
  TERMINATED_CONSUMED,
  TERMINATED_NONCONSUMED,
  INVALID
};

template <class T>
concept token = requires(T t)
{
  {
    t.accept('a')
    } -> std::same_as<AcceptResult>;
};

struct StringToken
{
  static constexpr char name[] = "StringToken";
  static std::type_index Event;

  std::string value_;
  AcceptResult accept(char c)
  {
    if (c == '\"')
    {
      if (value_.empty())
      {
        // no need to store quotes in our string!
        return AcceptResult::CONSUMED;
      }
      else if (value_.back() == '\\')
      {
        // so this is not really terminate character, but just another char
        value_.pop_back(); // remove previous escaped char
        value_.push_back(c);
        return AcceptResult::CONSUMED;
      }
      else
      {
        return AcceptResult::TERMINATED_CONSUMED;
      }
    }
    else if (c == '\\')
    {
      // TODO: TEST sequences of backslashes!
      if (value_.empty() || value_.back() == '\\')
        value_.push_back(c);

      return AcceptResult::CONSUMED;
    }
    else if (std::isprint(c))
    {
      if (!value_.empty() && value_.back() == '\\')
      {
        // backslash is not printed so this input is invalid!
        return AcceptResult::INVALID;
      }

      value_.push_back(c);
      return AcceptResult::CONSUMED;
    }
    else
    {
      return AcceptResult::INVALID;
    }
  }
};

struct PeriodToken
{
  static constexpr char name[] = "PeriodToken";
  static std::type_index Event;

  AcceptResult accept(char c)
  {
    if (c == '.')
    {
      return AcceptResult::TERMINATED_CONSUMED;
    }
    else
    {
      return AcceptResult::TERMINATED_NONCONSUMED;
    }
  }
};

struct AsterkixToken
{
  static std::type_index Event;
  static constexpr char name[] = "AsterixToken";
  AcceptResult accept(char c) { return AcceptResult::TERMINATED_CONSUMED; }
};

struct LeftBraceToken
{
  static std::type_index Event;
  static constexpr char name[] = "LeftBraceToken";
  AcceptResult accept(char c) { return AcceptResult::TERMINATED_CONSUMED; }
};

struct RightBraceToken
{
  static std::type_index Event;
  static constexpr char name[] = "RightBraceToken";
  AcceptResult accept(char c) { return AcceptResult::TERMINATED_CONSUMED; }
};

struct LeftParenthesisToken
{
  static std::type_index Event;
  static constexpr char name[] = "LeftParenthesisToken";
  AcceptResult accept(char c) { return AcceptResult::TERMINATED_CONSUMED; }
};

struct RightParenthesisToken
{
  static std::type_index Event;
  static constexpr char name[] = "RightParenthesisToken";
  AcceptResult accept(char c) { return AcceptResult::TERMINATED_CONSUMED; }
};

struct NumericAnnotationToken
{
  static std::type_index Event;
  static constexpr char name[] = "NumericAnnotationToken";
  AcceptResult accept(char c) { return AcceptResult::TERMINATED_CONSUMED; }
};

struct BraceComment
{
  static std::type_index Event;
  static constexpr char name[] = "BraceComment";

  AcceptResult accept(char c)
  {
    // we just skip comments
    if (c == '}')
    {
      return AcceptResult::TERMINATED_CONSUMED;
    }
    else
    {
      return AcceptResult::CONSUMED;
    }
  }
};

struct LineComment
{
  static std::type_index Event;
  static constexpr char name[] = "LineComment";

  AcceptResult accept(char c)
  {
    // we just skip comments
    if (c == '\n')
    {
      return AcceptResult::TERMINATED_CONSUMED;
    }
    else
    {
      return AcceptResult::CONSUMED;
    }
  }
};

struct EscapeToken
{
  static std::type_index Event;
  static constexpr char name[] = "EscapeToken";

  AcceptResult accept(char c)
  {
    // we just skip comments
    if (c == '\n')
    {
      return AcceptResult::TERMINATED_CONSUMED;
    }
    else
    {
      return AcceptResult::CONSUMED;
    }
  }
};

struct NumericGlyphToken
{
  static std::type_index Event;
  static constexpr char name[] = "NumericGlyphToken";
  bool first = true;

  AcceptResult accept(char c)
  {
    // we just skip comments
    if (std::isdigit(c) || (first && c == '$'))
    {
      first = false;
      return AcceptResult::CONSUMED;
    }
    else
    {
      return AcceptResult::TERMINATED_NONCONSUMED;
    }
  }
};

struct SymbolToken
{
  static std::type_index Event;
  static constexpr char name[] = "SymbolToken";

  std::string value_;
  bool number_only_ = true;

  static bool is_symbol_char(char c)
  {
    return std::isdigit(c) || std::isalpha(c) || c == ':' || c == '-' || c == '_' || c == '+' ||
      c == '=' || c == '#' || c == '/';
  }

  AcceptResult accept(char c)
  {
    if (is_symbol_char(c))
    {
      if (!std::isdigit(c))
        number_only_ = false;

      value_.push_back(c);
      return AcceptResult::CONSUMED;
    }
    else
    {
      return AcceptResult::TERMINATED_NONCONSUMED;
    }
  }
};

struct IntegerToken
{
  static std::type_index Event;
  static constexpr char name[] = "IntegerToken";

  std::string value_;
  AcceptResult accept(char c)
  {
    throw std::runtime_error("IntegerToken::accept is not meant to be called");
  }
};

using Token =
  std::variant<std::monostate, StringToken, PeriodToken, AsterkixToken, LeftBraceToken, RightBraceToken, LeftParenthesisToken, RightParenthesisToken,
               NumericAnnotationToken, SymbolToken, IntegerToken, BraceComment, LineComment, EscapeToken, NumericGlyphToken>;

inline std::ostream& operator<<(std::ostream& o, const token auto& t)
{
  using token_type = std::decay_t<decltype(t)>;
  o << "[" << token_type::name << "],";
  return o;
}

inline std::ostream& operator<<(std::ostream& o, const std::monostate& null)
{
  o << "[null]";
  return o;
}

inline std::type_index StringToken::Event = typeid(StringToken);
inline std::type_index PeriodToken::Event = typeid(PeriodToken);
inline std::type_index AsterkixToken::Event = typeid(AsterkixToken);
inline std::type_index LeftBraceToken::Event = typeid(LeftBraceToken);
inline std::type_index RightBraceToken::Event = typeid(RightBraceToken);
inline std::type_index LeftParenthesisToken::Event = typeid(LeftParenthesisToken);
inline std::type_index RightParenthesisToken::Event = typeid(RightParenthesisToken);
inline std::type_index NumericAnnotationToken::Event = typeid(NumericAnnotationToken);
inline std::type_index SymbolToken::Event = typeid(SymbolToken);
inline std::type_index IntegerToken::Event = typeid(IntegerToken);
inline std::type_index BraceComment::Event = typeid(BraceComment);
inline std::type_index LineComment::Event = typeid(LineComment);
inline std::type_index EscapeToken::Event = typeid(EscapeToken);
inline std::type_index NumericGlyphToken::Event = typeid(NumericGlyphToken);
