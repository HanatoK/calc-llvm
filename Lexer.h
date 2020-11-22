#include <string>
#include <sstream>
#include <cctype>
#include <exception>
#include <iostream>

class Lexer {
public:
  // The lexer returns tokens [0-255] if it is an unknown character, otherwise one
  // of these for known things.
  enum Token {
    tok_eof = -1,
    // primary
    tok_leftparenthesis = -2,
    tok_rightparenthesis = -3,
    tok_identifier = -4,
    tok_number = -5,
    tok_operator = -6,
    tok_comma = -7,
  };
  Lexer(const std::string& input): mInputStr(input), mIdentifierStr(), mNumVal(0), mLastChar(' ') {}
  int getToken();
  int getToken(std::istringstream& iss);
  void printState(const int& t) const;
private:
  std::string mInputStr;
  std::string mIdentifierStr;
  std::string mOperatorStr;
  std::string mLeftParenthesis;
  std::string mRightParenthesis;
  std::string mComma;
  double mNumVal;
  char mLastChar;
};

int Lexer::getToken() {
  std::istringstream iss(mInputStr);
  int t = 256;
  do {
    t = getToken(iss);
    printState(t);
  } while (t != Token::tok_eof);
  return t;
}

int Lexer::getToken(std::istringstream& iss) {
  mIdentifierStr.clear();
  mOperatorStr.clear();
  mLeftParenthesis.clear();
  mRightParenthesis.clear();
  mComma.clear();
  mNumVal = 0;
  if (iss.eof()) {
    return Lexer::Token::tok_eof;
  }
  // Skip any whitespace
  while (std::isspace(mLastChar)) {
    if (iss.get(mLastChar)) {
      if (!std::isspace(mLastChar))
        // Find the first non-whitespace character
        break;
    } else {
      break;
    }
  }
  if (mLastChar == '(') {
    mLeftParenthesis = mLastChar;
    iss.get(mLastChar);
    return Lexer::Token::tok_leftparenthesis;
  }
  if (mLastChar == ')') {
    mRightParenthesis = mLastChar;
    iss.get(mLastChar);
    return Lexer::Token::tok_rightparenthesis;
  }
  if (mLastChar == ',') {
    mComma = mLastChar;
    iss.get(mLastChar);
    return Lexer::Token::tok_comma;
  }
  // identifier: [a-zA-Z][a-zA-Z0-9]*
  if (std::isalpha(mLastChar)) {
    mIdentifierStr += mLastChar;
    while (std::isalnum(mLastChar)) {
      if (iss.get(mLastChar)) {
        if (std::isalnum(mLastChar)) {
          mIdentifierStr += mLastChar;
        } else {
          break;
        }
      } else {
        break;
      }
    }
    return Lexer::Token::tok_identifier;
  }
  // identifier: '*' and '/' operators
  if (mLastChar == '*' || mLastChar == '/' || mLastChar == '-' ||
      mLastChar == '+' || mLastChar == '^' || mLastChar == '%') {
    mOperatorStr += mLastChar;
    iss.get(mLastChar);
    return Lexer::Token::tok_operator;
  }
  // identifier: numbers, potential '+' and '-' operators
  if (std::isdigit(mLastChar) || mLastChar == '.') {
    std::string NumStr;
    NumStr += mLastChar;
    int num_e = 0;
    int num_dot = (mLastChar == '.') ? 1 : 0;
    int num_digit = (std::isdigit(mLastChar)) ? 1 : 0;
    while (iss.get(mLastChar)) {
      if (mLastChar == 'e' || mLastChar == 'E') {
        // check if this is an 'e'
        if (num_digit >= 1) {
          NumStr += mLastChar;
          ++num_e;
        }
      } else if (std::isdigit(mLastChar)) {
        ++num_digit;
        NumStr += mLastChar;
      } else if (mLastChar == '.') {
        NumStr += mLastChar;
        ++num_dot;
      } else {
        // so the previous char should be an operator
        if (num_e > 0 && (mLastChar == '+' || mLastChar == '-')) {
          NumStr += mLastChar;
        } else {
          break;
        }
      }
    }
    try {
      mNumVal = std::stod(NumStr);
      return Lexer::Token::tok_number;
    }
    catch (std::exception& e) {
      std::cerr << e.what() << '\n';
      return 256;
    }
  }
  return int(mLastChar);
}

void Lexer::printState(const int& t) const {
  switch (t) {
    case -1: std::cout << "EOF" << std::endl; break;
    case -2: std::cout << "Left parenthesis: " << mLeftParenthesis << std::endl; break;
    case -3: std::cout << "Right parenthesis: " << mRightParenthesis << std::endl; break;
    case -4: std::cout << "Identifier: " << mIdentifierStr << std::endl; break;
    case -5: std::cout << "Number: " << mNumVal << std::endl; break;
    case -6: std::cout << "Operator: " << mOperatorStr << std::endl; break;
    case -7: std::cout << "Comma: " << mComma << std::endl; break;
    default: std::cout << "Unknown token: " << char(t) << std::endl; break;
  }
}
