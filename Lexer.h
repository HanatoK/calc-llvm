#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <sstream>
#include <cctype>
#include <exception>
#include <iostream>
#include <tuple>
#include <variant>

using std::tuple;
using std::variant;
using std::istream;
using std::string;
using std::make_tuple;

enum class Token {
  Eof = -1,
  LeftParenthesis = -2,
  RightParenthesis = -3,
  Identifier = -4,
  Number = -5,
  Operator = -6,
  Comma = -7,
  Definition = -8,
  Extern = -9,
  Unknown = -255,
};

class Lexer {
public:
  Lexer();
  void clearState();
  tuple<Token, variant<string, double>> getAllToken(const string& str);
  tuple<Token, variant<string, double>> getToken(istream& inputStream);
private:
  char mLastChar;
};

#endif // LEXER_H
