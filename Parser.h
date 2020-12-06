#ifndef PARSER_H
#define PARSER_H

#include <map>
#include <iostream>
#include <string>
#include <memory>
#include <tuple>
#include <variant>
#include <sstream>

#include "Lexer.h"
#include "AbstractSyntaxTree.h"

using std::map;
using std::string;
using std::unique_ptr;
using std::variant;
using std::tuple;
using std::istringstream;
using std::make_unique;

class Parser {
public:
  Parser();
  Parser(const string& Str);
  Parser(const Parser& p);
  string getInputString() const;
  int GetBinaryPrecedence(const string& Op) const;
  int GetUnaryPrecedence(const string& Op) const;
  bool IsRightAssociative(const string& Op) const;
  tuple<Token, variant<string, double>> getNextToken();
  tuple<Token, variant<string, double>> getCurrentToken() const;
  void PrintCurrentToken() const;
  unique_ptr<ExprAST> ParseNumberExpr();
  unique_ptr<ExprAST> ParseParenExpr();
  unique_ptr<ExprAST> ParseIdentifierExpr();
  unique_ptr<ExprAST> ParsePrimary();
  unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec, unique_ptr<ExprAST> LHS);
  unique_ptr<ExprAST> ParseExpression();
  unique_ptr<ExprAST> ParseUnaryOpRHS(int ExprPrec);
  unique_ptr<FunctionAST> ParseTopLevelExpr();
private:
  void SetupPrecedence();
  void SetupInput(const string& Str);
  map<string, int> mBinaryOpPrecedence;
  map<string, bool> mRightAssociative;
  map<string, int> mUnaryOpPrecedence;
  string mInputString;
  istringstream mInputStream;
  tuple<Token, variant<string, double>> mCurrentToken;
  Lexer mLexer;
};

#endif // PARSER_H
