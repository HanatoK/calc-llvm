#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <sstream>
#include <cctype>
#include <exception>
#include <iostream>
#include <tuple>
#include <variant>
#include <map>

using std::tuple;
using std::variant;
using std::istream;
using std::string;
using std::make_tuple;
using std::map;

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
  Semicolon = -10,
  If = -11,
  Then = -12,
  Else = -13,
  For = -14,
  In = -15,
  Assignment = -16,
  Unknown = -255,
};

const extern map<string, Token> keywords;

class Lexer {
public:
  Lexer();
  Lexer(const string& input);
  void AppendString(const string& input);
  tuple<Token, variant<string, double>> getToken();
  [[nodiscard]] string str() const {return mInputString;}
private:
  string mInputString;
  size_t mCurrentPosition;
  bool CurrentChar(char& c) const;
};

#endif // LEXER_H
