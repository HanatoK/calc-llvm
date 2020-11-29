#include "AbstractSyntaxTree.h"
#include "Lexer.h"
#include "Parser.h"

int main() {
  std::string s{"5+2*8"};
  Parser p(s);
  p.ParseTopLevelExpr();
  return 0;
}
