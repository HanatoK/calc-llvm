#include "AbstractSyntaxTree.h"
#include "Lexer.h"
#include "Parser.h"
#include "Version.h"
#include "Driver.h"

int main() {
  std::cin >> s;
  std::cout << "Input string: " << s << std::endl;
  Parser p(s);
  Driver d(p);
  d.MainLoop();
//   s = "(-5+2)*8";
//   Lexer l;
//   l.getAllToken(s);
  return 0;
}
