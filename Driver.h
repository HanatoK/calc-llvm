#include "Parser.h"

class Driver {
public:
  Driver(const Parser& p): mParser(p) {}
  void HandleTopLevelExpression();
  void MainLoop();
private:
  Parser mParser;
};
