#include <algorithm>
#include <exception>
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
const uint8_t HLT = 12;

const string numerr("Invalid number (>0xff) given");
const string typeerr("Invalid type (pointer given for memory address");
const string labelerr("Duplicate label");
const string iderr("Invalid identifier");

enum Type
{
    LABEL,
    HEX_VAL, BIN_VAL, DEC_VAL,
    LBRAC, RBRAC,
    INSTR,

    IGNORED_TYPES,
    ENDL,
    COMMENT,

    TYPE_COUNT
};

// https://medium.com/factory-mind/regex-tutorial-a-simple-cheatsheet-by-examples-649dc1c3f285
// https://www.modula2.org/sb/env/index583.htm
const array<regex, TYPE_COUNT> regexes({
    regex("^([A-z]+[A-z\\d]*):\\s*"),
    regex("^0[xX]([a-fA-F\\d]+)\\s*"),
    regex("^0[bB]([01]+)\\s*"),
    regex("^(\\d+)\\s*"),
    regex("^\\[\\s*"),
    regex("^\\]\\s*"),
    regex("^\\s*([A-z]+)\\s*"),
    regex("^\\s*(\r\n|\r|\n)"),
    regex("^\\s*;.*")
});

struct Token
{
    int type;
    string val;

    int line;
    int col;
};

void error(string out, int line, int col)
{
    cout << out << " at line " << line << ", column " << col << endl;
    throw -1;
}
void error(string out, Token t)
{
    error(out, t.line, t.col);
}

vector<Token> parseTokens(string filename)
{
    ifstream file(filename);
    vector<Token> output;

    string line;
    smatch sm;

    int lineNum = 1;
    int col = 1;
    while (getline(file, line))
    {
        // loop through all regexes
        while (line.size() > 0)
        {
            for (int i = 0; i < regexes.size(); ++i)
            {
                // if match, set match var
                if (regex_search(line, sm, regexes[i]))
                {
                    if (i < IGNORED_TYPES)
                    {
                        // doesn't have a capture group/value
                        if (sm.size() == 1)
                            output.push_back({ i, "", lineNum, col });
                        else
                        {
                            string m = sm.str(1);
                            // convert string to lowercase
                            transform(m.begin(), m.end(), m.begin(), ::tolower);
                            output.push_back({ i, m, lineNum, col });
                        }
                    }

                    // remove match from string (might be more efficient than regex_replace ?)
                    line =  line.substr(sm.length(), line.size()-1);
                    col += sm.length();
                }
            }
        }

        ++lineNum;
        col = 1;
    }

    file.close();
    return output;
}

void writeToFile(int val, ofstream *file, bool eol)
{
    (*file) << "0x" << hex << val;
    if (eol)
        (*file) << endl;
    else
        (*file) << ' ';
}

uint8_t parseVal(Token in)
{
    int out;
    if (in.type == HEX_VAL)
    {
        stringstream ss;
        ss << in.val;
        ss >> hex >> out;
    }
    else if (in.type == BIN_VAL)
    {
        out = 0;
        for (int i = 0; i < in.val.size(); ++i)
        {
            char c = in.val[in.val.size()-i-1];
            if (c == '1')
                out += pow(2, i);
        }
    }
    else
        out = stoi(in.val);
                    
    if (out > 0xff)
        error(numerr, in);
    
    return out;
}

void parseSingleArg(int instrId, int *i, vector<Token> *tokens, ofstream *file)
{
    // advance token to arg
    ++(*i);
    // token right after instruction
    Token arg = (*tokens)[*i];
    int val;

    // if numeric
    if (arg.type >= HEX_VAL && arg.type <= DEC_VAL)
    {
        val = parseVal(arg);

        // write instruction
        writeToFile(instrId+1, file, false);
    }
    // pointer to value in RAM
    else if (arg.type == LBRAC)
    {
        // get value, advance past brackets
        arg = (*tokens)[*i+1];
        *i += 2;

        val = parseVal(arg);

        // write instruction
        writeToFile(instrId, file, false);
    }

    // write value byte
    writeToFile(val, file, true);
}

void write(string outfile, vector<Token> *tokens)
{
    ofstream file(outfile);

    // lookup table of the byte location of labels
    map<string, int> labels;
    int bytes = 0;

    // loop through tokens
    for (int i = 0; i < tokens->size(); ++i)
    {
        Token current = (*tokens)[i];

        if (current.type == INSTR)
        {
            if (current.val == "nop")
                writeToFile(NOP, &file, true);
            else if (current.val == "out")
                writeToFile(OUT, &file, true);
            else if (current.val == "hlt")
                writeToFile(HLT, &file, true);

            else // instructions with arguments
            {
                if (current.val == "lda")
                    parseSingleArg(LDA, &i, tokens, &file);
                else if (current.val == "sta")
                {
                    Token arg = (*tokens)[i+1];
                    if (arg.type == LBRAC)
                        error(typeerr, arg);

                    parseSingleArg(STA, &i, tokens, &file);
                }
                else if (current.val == "add")
                    parseSingleArg(ADD, &i, tokens, &file);
                else if (current.val == "sub")
                    parseSingleArg(SUB, &i, tokens, &file);
                else if (current.val == "jmp")
                {
                    // jump to label
                    if ((*tokens)[i+1].type == INSTR)
                    {
                        ++i;
                        int val = labels[(*tokens)[i].val];

                        writeToFile(JMP, &file, false);
                        writeToFile(val, &file, true);
                    } else
                        parseSingleArg(JMP, &i, tokens, &file);
                }

                ++bytes;
            }
            
            ++bytes;
        }
        // add label to the lookup table (if label doesn't already exist)
        else if (current.type == LABEL)
        {
            if (labels.find(current.val) != labels.end())
                error(labelerr, current);

            labels.insert(pair<string, int>(current.val, bytes));
        }
    }

    file.close();
}

int main(int argc, char *argv[])
{
    if (argc > 1)
    {
        vector<Token> i = parseTokens(argv[1]);

        write("out.txt", &i);
    }
    else
        cout << "Must give a file argument!" << endl;

    return 0;
}