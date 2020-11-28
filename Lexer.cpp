#include "Lexer.h"

Lexer::Lexer(): mLastChar(' ') {}

void Lexer::clearState() {
  mLastChar = ' ';
}

tuple<Token, variant<string, double>> Lexer::getToken(const string& str) {
  clearState();
  std::istringstream iss(str);
  auto result = getToken(iss);
  do {
    switch (std::get<0>(result)) {
      case Token::Eof: {
        std::cout << std::get<string>(std::get<1>(result)) << std::endl;
        break;
      }
      case Token::LeftParenthesis: {
        std::cout << "Left parenthesis: ";
        std::cout << std::get<string>(std::get<1>(result)) << std::endl;
        break;
      }
      case Token::RightParenthesis: {
        std::cout << "Right parenthesis: ";
        std::cout << std::get<string>(std::get<1>(result)) << std::endl;
        break;
      }
      case Token::Identifier: {
        std::cout << "Identifier: ";
        std::cout << std::get<string>(std::get<1>(result)) << std::endl;
        break;
      }
      case Token::Number: {
        std::cout << "Number: ";
        std::cout << std::get<double>(std::get<1>(result)) << std::endl;
        break;
      }
      case Token::Operator: {
        std::cout << "Operator: ";
        std::cout << std::get<string>(std::get<1>(result)) << std::endl;
        break;
      }
      case Token::Comma: {
        std::cout << "Comma:";
        std::cout << std::get<string>(std::get<1>(result)) << std::endl;
        break;
      }
      default: {
        std::cout << "Unknown token: ";
        std::cout << std::get<string>(std::get<1>(result)) << std::endl;
        break;
      }
    }
    result = getToken(iss);
  } while (std::get<0>(result) != Token::Eof);
  return result;
}

tuple<Token, variant<string, double>> Lexer::getToken(istream& inputStream) {
  if (inputStream.eof()) {
    return make_tuple(Token::Eof, string{"EOF"});
  }
  // skip any whitespace
  while (std::isspace(mLastChar)) {
    if (inputStream.get(mLastChar)) {
      if (!std::isspace(mLastChar)) {
        // if we meet the first non-whitespace character, break the loop
        break;
      }
    } else {
      // EOF
      return make_tuple(Token::Eof, string{mLastChar});
      break;
    }
  }
  if (mLastChar == '(') {
    const Token t = Token::LeftParenthesis;
    const string result{mLastChar};
    inputStream.get(mLastChar);
    return make_tuple(t, result);
  }
  if (mLastChar == ')') {
    const Token t = Token::RightParenthesis;
    const string result{mLastChar};
    inputStream.get(mLastChar);
    return make_tuple(t, result);
  }
  if (mLastChar == ',') {
    const Token t = Token::Comma;
    const string result{mLastChar};
    inputStream.get(mLastChar);
    return make_tuple(t, result);
  }
  // [a-zA-Z][a-zA-Z0-9]*
  if (std::isalpha(mLastChar)) {
    const Token t = Token::Identifier;
    string result{mLastChar};
    while (std::isalnum(mLastChar)) {
      if (inputStream.get(mLastChar)) {
        if (std::isalnum(mLastChar)) {
          result += mLastChar;
        } else {
          break;
        }
      } else {
        break;
      }
    }
    return make_tuple(t, result);
  }
  // operators
  if (mLastChar == '*' || mLastChar == '/' || mLastChar == '-' ||
      mLastChar == '+' || mLastChar == '^' || mLastChar == '%') {
    const Token t = Token::Operator;
    const string result{mLastChar};
    inputStream.get(mLastChar);
    return make_tuple(t, result);
  }
  // numbers
  if (std::isdigit(mLastChar) || mLastChar == '.') {
    const Token t = Token::Number;
    string NumStr{mLastChar};
    int num_e = 0;
    int num_dot = (mLastChar == '.') ? 1 : 0;
    int num_digit = (std::isdigit(mLastChar)) ? 1 : 0;
    while (inputStream.get(mLastChar)) {
      if (mLastChar == 'e' || mLastChar == 'E') {
        // check if this is an 'e'
        if (num_digit >= 1) {
          // any 'e' must appear after at least a number
          NumStr += mLastChar;
          ++num_e;
        } else {
          // TODO: what will happen here?
        }
      } else if (std::isdigit(mLastChar)) {
        ++num_digit;
        NumStr += mLastChar;
      } else if (mLastChar == '.') {
        NumStr += mLastChar;
        ++num_dot;
      } else {
        if (num_e > 0 && (mLastChar == '+' || mLastChar == '-')) {
          NumStr += mLastChar;
        } else {
          break;
        }
      }
    }
    try {
      const double number = std::stod(NumStr);
      return make_tuple(t, number);
    } catch (std::exception& e) {
      std::cerr << e.what() << '\n';
      return make_tuple(t, 0.0);
    }
  }
  return make_tuple(Token::Unknown, string{mLastChar});
}
