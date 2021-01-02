#include "Lexer.h"

const extern map<string, Token> keywords = {{"extern", Token::Extern},
                                            {"def", Token::Definition},
                                            {"if", Token::If},
                                            {"then", Token::Then},
                                            {"else", Token::Else},
                                            {"for", Token::For},
                                            {"in", Token::In}};

Lexer::Lexer(): mCurrentPosition(0) {}

Lexer::Lexer(const string& input): Lexer() {
  AppendString(input);
}

void Lexer::AppendString(const string& input) {
  mInputString.append(input);
}

bool Lexer::CurrentChar(char& c) const {
  if (mCurrentPosition >= mInputString.size()) {
    return false;
  } else {
    c = mInputString[mCurrentPosition];
    return true;
  }
}

tuple<Token, variant<string, double>> Lexer::getToken() {
  char c = ' ';
  if (mCurrentPosition >= mInputString.size()) {
    return make_tuple(Token::Eof, string{"EOF"});
  }
  CurrentChar(c);
  while (std::isspace(c)){
    ++mCurrentPosition;
    if (CurrentChar(c)) {
      if (!std::isspace(c)) {
        // if we meet the first non-whitespace character, break the loop
        break;
      }
    } else {
      // EOF
      return make_tuple(Token::Eof, string{c});
    }
  }
  Token t;
  string result{c};
  bool match_in_switch = true;
  switch (c) {
    case '(': t = Token::LeftParenthesis; break;
    case ')': t = Token::RightParenthesis; break;
    case ',': t = Token::Comma; break;
    case ';': t = Token::Semicolon; break;
    case '=': t = Token::Assignment; break;
    case '*':
    case '/':
    case '+':
    case '-':
    case '^': t = Token::Operator; break;
    default: match_in_switch = false; // may be a digit or alphabet
  }
  if (match_in_switch) {
    ++mCurrentPosition;
    CurrentChar(c);
    return make_tuple(t, result);
  } else {
    // check identifier [_a-zA-Z][_a-zA-Z0-9]*
    if (std::isalpha(c) || c == '_') {
      do {
        ++mCurrentPosition;
        if (CurrentChar(c)) {
          if (std::isalnum(c) || c == '_') result += c;
          else break;
        } else {
          break;
        }
      } while (true);
      const auto find_result = keywords.find(result);
      if (find_result != keywords.end()) {
        return make_tuple(find_result->second, result);
      } else {
        return make_tuple(Token::Identifier, result);
      }
    } else if (std::isdigit(c) || c == '.') {
      t = Token::Number;
      int num_e = 0;
      int num_dot = (c == '.') ? 1 : 0;
      int num_digit = (std::isdigit(c)) ? 1 : 0;
      ++mCurrentPosition;
      while (CurrentChar(c)) {
        if (c == 'e' || c == 'E') {
          // check if this is an 'e'
          if (num_digit >= 1) {
            result += c;
            ++num_e;
          } else {
            t = Token::Unknown;
            break;
          }
        } else if (std::isdigit(c)) {
          ++num_digit;
          result += c;
        } else if (c == '.') {
          ++num_dot;
          result += c;
        } else {
          if (num_e == 1 && (c == '+' || c == '-')) {
            // 'e' can only appear once
            result += c;
          } else {
            break;
          }
        }
        ++mCurrentPosition;
      }
      try {
        const double number = std::stod(result);
        return make_tuple(t, number);
      } catch (std::exception& e) {
        std::cerr << e.what() << '\n';
        return make_tuple(t, 0.0);
      }
    } else {
      ++mCurrentPosition;
    }
    return make_tuple(Token::Unknown, result);
  }
}
