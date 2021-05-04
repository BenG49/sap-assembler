#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <math.h>
#include <array>
#include <regex>
#include <map>

using namespace std;

const uint8_t NOP = 0;
const uint8_t LDA = 1;
const uint8_t STA = 3;
const uint8_t ADD = 5;
const uint8_t SUB = 7;
const uint8_t OUT = 9;
const uint8_t JMP = 10;
const uint8_t JC = 12;
const uint8_t JZ = 14;
const uint8_t HLT = 16;

const string numerr("Invalid number (>0xff) given");
const string typeerr("Invalid type (pointer given for memory address");
const string labelerr("Duplicate label");
const string iderr("Invalid identifier");


void error(string out, int line, int col)
{
    cout << out << " at line " << line << ", column " << col << endl;
    throw -1;
}
