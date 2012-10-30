#include <iostream>
#include <octave/oct.h>
#include <octave/octave.h>
#include <octave/parse.h>
#include <octave/toplev.h>

class TiXmlElement;

enum ValueType {
  ArbitraryType,
  ScalarType,
  VectorType,
  MatrixType,
  StringType
};

struct Param {
  Param(std::string n, std::string eq, TiXmlElement *e) : name(n), equ(eq), ele(e) {}
  std::string name, equ;
  TiXmlElement *ele;
};

void octaveEvalRet(std::string str, TiXmlElement *e=NULL);
std::string octaveGetRet(ValueType expectedType=ArbitraryType);
int fillParam(TiXmlElement *e);
void initializeOctave();
int fillParam(std::vector<Param> param);
