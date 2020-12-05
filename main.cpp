#include "AbstractSyntaxTree.h"
#include "Lexer.h"
#include "Parser.h"
#include "Version.h"
#include "Driver.h"

int main() {
  std::string s{"(5+2)*8"};
  Parser p(s);
  Driver d(p);
  d.MainLoop();
//   s = "(-5+2)*8";
//   Lexer l;
//   l.getAllToken(s);
  return 0;
}
