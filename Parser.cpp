#include "Parser.h"

#include <exception>
#include <vector>

using std::move;
using std::unique_ptr;
using std::string;

Parser::Parser() {
  SetupPrecedence();
}

Parser::Parser(const string& Str) {
  SetupPrecedence();
  SetupInput(Str);
}

void Parser::SetupInput(const string& Str) {
  mInputStream.str(Str);
}

void Parser::SetupPrecedence() {
  mBinopPrecedence["+"] = 100;
  mBinopPrecedence["-"] = 100;
  mBinopPrecedence["*"] = 200;
  mBinopPrecedence["/"] = 200;
  mBinopPrecedence["^"] = 300;
}

int Parser::GetTokPrecedence(const string& Op) const {
  try {
    const int TokPrec = mBinopPrecedence.at(op);
    return TokPrec;
  } catch (std::exception& e) {
    std::cerr << e.what() << '\n';
    return -1;
  }
}

tuple<Token, variant<string, double>> Parser::getNextToken() {
  return mCurrentToken = mLexer.getToken(mInputStream);
}

void Parser::PrintCurrentToken() const {
  using std::cout;
  using std::get;
  using std::endl;
  const Token T = get<0>(mCurrentToken);
  const auto V = get<1>(mCurrentToken);
  cout << "Current token: " << static_cast<int>(T) << " ";
  switch (T) {
    case Eof: cout << "Eof: " << get<string>(V); break;
    case LeftParenthesis: cout << "LeftParenthesis: " << get<string>(V); break;
    case RightParenthesis: cout << "RightParenthesis: " << get<string>(V); break;
    case Identifier: cout << "Identifier: " << get<string>(V); break;
    case Number: cout << "Number: " << get<double>(V); break;
    case Operator: cout << "Operator: " << get<string>(V); break;
    case Comma: cout << "Comma: " << get<string>(V); break;
  }
  cout << endl;
}

unique_ptr<ExprAST> Parser::LogError(const string& Str) {
  std::cerr << Str << std::endl;
  return nullptr;
}

unique_ptr<ExprAST> Parser::ParseNumberExpr() {
#ifdef DEBUG_PARSER
  std::cout << "unique_ptr<ExprAST> Parser::ParseNumberExpr()\n";
  PrintCurrentToken();
#endif
  auto Result = make_unique<NumberExprAst>(NumVal);
  getNextToken();
  return move(Result);
}

unique_ptr<ExprAST> Parser::ParseParenExpr() {
#ifdef DEBUG_PARSER
  std::cout << "unique_ptr<ExprAST> Parser::ParseParenExpr()\n";
  PrintCurrentToken();
#endif
  getNextToken(); // eat (.
  auto V = ParseExpression();
  if (!V)
    return nullptr;
  if (std::get<0>(mCurrentToken) != Token::RightParenthesis)
    return LogError("expected ')'");
  getNextToken(); // eat ).
  return V;
}

unique_ptr<ExprAST> Parser::ParseIdentifierExpr() {
  using std::get;
  using std::vector;
#ifdef DEBUG_PARSER
  std::cout << "unique_ptr<ExprAST> Parser::ParseIdentifierExpr()\n";
  PrintCurrentToken();
#endif
  const string IdName = get<string>(get<1>(mCurrentToken));
  getNextToken(); // eat identifier
  if (get<1>(mCurrentToken) != Token::LeftParenthesis) {
    // Simple variable ref.
    return std::make_unique<VariableExprAST>(IdName);
  }
  // '(' appears after an identifier, so this is a function call
  getNextToken();
  vector<unique_ptr<ExprAST>> Args;
  if (get<1>(mCurrentToken) != Token::RightParenthesis) {
    while (true) {
      if (auto Arg = ParseExpression())
        Args.push_back(move(Arg));
      else
        return nullptr;
      if (get<1>(mCurrentToken) == Token::RightParenthesis)
        break;
      if (get<1>(mCurrentToken) != Token::Comma)
        return LogError("Expected ')' or ',' in argument list");
      getNextToken();
    }
  }
  // Eat the ')'.
  getNextToken();
  return make_unique<CallExprAST>(IdName, move(Args));
}

unique_ptr<ExprAST> Parser::ParsePrimary() {
#ifdef DEBUG_PARSER
  std::cout << "unique_ptr<ExprAST> Parser::ParsePrimary()\n";
  PrintCurrentToken();
#endif
  switch (std::get<0>(mCurrentToken)) {
    case Token::Identifier: return ParseIdentifierExpr();
    case Token::Number: return ParseNumberExpr();
    case Token::LeftParenthesis: return ParseParenExpr();
    default: return LogError("unknown token when expecting an expression");
  }
}

unique_ptr<ExprAST> Parser::ParseExpression() {
#ifdef DEBUG_PARSER
  std::cout << "unique_ptr<ExprAST> Parser::ParseExpression()\n";
  PrintCurrentToken();
#endif
  auto LHS = ParsePrimary();
  if (!LHS)
    return nullptr;
  return ParseBinOpRHS(0, move(LHS));
}

unique_ptr<ExprAST> Parser::ParseBinOpRHS(int ExprPrec,
                                          unique_ptr<ExprAST> LHS) {
#ifdef DEBUG_PARSER
  std::cout << "unique_ptr<ExprAST> Parser::ParseBinOpRHS()\n";
  PrintCurrentToken();
#endif
  using std::get;
  string Op = get<string>(get<1>(mCurrentToken));
  // TODO: figure out what happens in the following code
  while (true) {
    int TokPrec = GetTokPrecedence(Op);
    if (TokPrec < ExprPrec)
      return LHS;
    auto BinOpToken = mCurrentToken;
    getNextToken();
    auto RHS = ParsePrimary();
    if (!RHS)
      return nullptr;
    int NextPrec = GetTokPrecedence(get<string>(get<1>(mCurrentToken)));
    if (TokPrec < NextPrec) {
      RHS = ParseBinOpRHS(TokPrec + 1, move(RHS));
      if (!RHS)
        return nullptr;
    }
    LHS = make_unique<BinaryExprAST>(BinOpToken, move(LHS), move(RHS));
  }
}

unique_ptr<FunctionAST> ParseTopLevelExpr() {
  if (auto E = ParseExpression()) {
    // Make an anonymous proto.
    auto Proto = make_unique<PrototypeAST>("", vector<string>());
    return make_unique<FunctionAST>(move(Proto), move(E));
  }
  return nullptr;
}
