#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <math.h>
#include <array>
#include <regex>
#include <map>

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

const std::string numerr("Invalid number (>0xff) given");
const std::string typeerr("Invalid type (pointer given for memory address");
const std::string labelerr("Duplicate label");
const std::string iderr("Invalid identifier");


void error(std::string out, int line, int col)
{
    std::cout << out << " at line " << line << ", column " << col << std::endl;
    throw -1;
}
