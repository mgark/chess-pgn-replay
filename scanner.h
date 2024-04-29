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
#include "tokens.h"
#include <fstream>
#include <stdexcept>
#include <variant>

class TokenScanner
{
public:
private:
  std::istream& file_;
  Token current_token_;
  char c_;

public:
  TokenScanner(std::istream& file) : file_(file) {}

  struct Iterator
  {
    using iterator_category = std::input_iterator_tag;
    using value_type = Token;
    using pointer = Token*;
    using reference = Token&;

    TokenScanner* scanner_;
    bool repeat_ = false;

    Iterator() : scanner_(nullptr) {}
    Iterator(TokenScanner* scanner) : scanner_(scanner) { scan_token(); }
    Iterator& operator++()
    {
      scan_token();
      return *this;
    }

    Token& operator*() { return scanner_->current_token_; }
    Token* operator->() { return &scanner_->current_token_; }

    friend bool operator==(const Iterator& l, const Iterator& r)
    {
      return l.scanner_ == r.scanner_;
    }
    friend bool operator!=(const Iterator& l, const Iterator& r) { return !operator==(l, r); }

    bool is_token_separator(char c) { return c == ' ' || c == '\n' || c == '\t'; }

  private:
    void scan_token()
    {
      bool expecting_new_token = true;
      bool token_terminated = false;
      scanner_->current_token_.emplace<0>();
      while (!token_terminated && !scanner_->file_.eof() && (repeat_ || scanner_->file_.get(scanner_->c_)))
      {
        if (expecting_new_token && is_token_separator(scanner_->c_))
        {
          repeat_ = false;
          continue;
        }

        if (expecting_new_token)
        {
          switch (scanner_->c_)
          {
          case '[':
            scanner_->current_token_ = LeftBraceToken();
            break;
          case ']':
            scanner_->current_token_ = RightBraceToken();
            break;
          case '(':
            scanner_->current_token_ = LeftParenthesisToken();
            break;
          case ')':
            scanner_->current_token_ = RightParenthesisToken();
            break;
          case '\"':
            scanner_->current_token_ = StringToken();
            break;
          case '.':
            scanner_->current_token_ = PeriodToken();
            break;
          case '*':
            scanner_->current_token_ = AsterkixToken();
            break;
          case '{':
            scanner_->current_token_ = BraceComment();
            break;
          case '$':
            scanner_->current_token_ = NumericGlyphToken();
            break;
          case ';':
            scanner_->current_token_ = LineComment();
            break;
          case '%':
            scanner_->current_token_ = EscapeToken();
            break;
          default:
            if (std::isdigit(scanner_->c_) || std::isalpha(scanner_->c_))
            {
              scanner_->current_token_ = SymbolToken();
            }
            else
            {
              throw std::runtime_error(
                std::string("bad format. expecing digit / character, but got [")
                  .append(std::string(1, scanner_->c_))
                  .append("]"));
            }
          }

          expecting_new_token = false;
        }

        {
          token_terminated = std::visit(
            overloaded{
              [](std::monostate& t) { return true; },
              [&](auto& t)
              {
                AcceptResult r = t.accept(scanner_->c_);
                switch (r)
                {
                case AcceptResult::INVALID:
                {
                  throw std::runtime_error(
                    std::string("got unexpected char [").append(std::string(1, scanner_->c_)).append("]"));
                }
                case AcceptResult::TERMINATED_CONSUMED:
                {
                  repeat_ = false;
                  return true;
                }
                case AcceptResult::TERMINATED_NONCONSUMED:
                {
                  // need to start parsing new token, but the current symbol has to be re-tried!
                  using token_type = std::decay_t<decltype(t)>;
                  if constexpr (std::is_same_v<token_type, SymbolToken>)
                  {
                    if (t.number_only_)
                    {
                      // shall be inserted as IntegerToken!
                      Token integer_token =
                        IntegerToken{std::get<SymbolToken>(scanner_->current_token_).value_};
                      scanner_->current_token_ = integer_token;
                    }
                  }

                  repeat_ = true;
                  return true;
                }
                case AcceptResult::CONSUMED:
                {
                  repeat_ = false;
                  return false;
                }
                default:
                {
                  throw std::runtime_error(
                    std::string("got unexpected return value from accept function [")
                      .append(std::to_string(static_cast<size_t>(r)))
                      .append("]"));
                  break;
                }
                }
              }},
            scanner_->current_token_);
        }
      }

      // finish reading the file!
      if (!token_terminated)
        scanner_ = nullptr;
    }
  };

  friend Iterator;
  Iterator begin() { return Iterator(this); }
  Iterator end() { return {}; }

  bool is_bad() const { return file_.bad(); }
};
