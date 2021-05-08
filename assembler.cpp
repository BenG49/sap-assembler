#include "lib.hpp"

enum Type
{
    WHITESPACE,
    COMMENT,

    LABEL,
    HEX_VAL, BIN_VAL, DEC_VAL,
    LBRAC, RBRAC,
    INSTR,

    TYPE_COUNT
};

// https://medium.com/factory-mind/regex-tutorial-a-simple-cheatsheet-by-examples-649dc1c3f285
// https://www.modula2.org/sb/env/index583.htm
const std::array<std::regex, TYPE_COUNT> regexes({
    std::regex("^\\s+"),
    std::regex("^;.*"),
    std::regex("^([A-z]+[A-z\\d]*):"),
    std::regex("^0[xX]([a-fA-F\\d]+)"),
    std::regex("^0[bB]([01]+)"),
    std::regex("^(\\d+)"),
    std::regex("^\\["),
    std::regex("^\\]"),
    std::regex("^([A-z]+)"),
});

const std::map<std::string, std::pair<int, int>> opcodes({
    // name, immediate opcode, pointer opcode
    {"nop", { NOP, 0 }},
    {"lda", { LDA+1, LDA }},
    {"sta", { STA, 0 }},
    {"add", { ADD+1, ADD }},
    {"sub", { SUB+1, SUB }},
    {"out", { OUT, 0 }},
    {"jmp", { JMP+1, JMP }},
    {"jc",  { JC+1, JC }},
    {"jz",  { JZ+1, JZ }},
    {"hlt", { HLT, 0 }},
});

std::map<std::string, int> labels;

struct Token
{
    int type;
    std::string val;

    int line;
    int col;
};

void error(std::string out, Token t)
{
    error(out, t.line, t.col);
}

std::vector<Token> parseTokens(std::string filename)
{
    std::ifstream file(filename);
    std::vector<Token> output;

    std::string line;
    std::smatch sm;

    int lineNum = 1;
    int col = 1;
    while (std::getline(file, line))
    {
        while (line.size() > 0)
        {
            // loop through all regexes
            for (int i = 0; i < regexes.size(); ++i)
            {
                // if match, set match var
                if (std::regex_search(line, sm, regexes[i]))
                {
                    // if token matters
                    if (i > COMMENT)
                    {
                        // doesn't have a capture group/value
                        if (sm.size() == 1)
                            output.push_back({ i, "", lineNum, col });
                        else
                        {
                            std::string m = sm.str(1);
                            // convert string to lowercase
                            std::transform(m.begin(), m.end(), m.begin(), ::tolower);
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

uint8_t parseVal(Token in)
{
    int out;
    if (in.type == HEX_VAL)
    {
        std::stringstream ss;
        ss << in.val;
        ss >> std::hex >> out;
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

// returns number of extra tokens to advance
int parseSingleArg(int valueId, int pointerId, int i, std::vector<Token> tokens, std::vector<std::string>& file)
{
    // token right after instruction
    Token arg = tokens[i+1];

    // if numeric
    if (arg.type >= HEX_VAL && arg.type <= DEC_VAL)
    {
        int val = parseVal(arg);

        // write instruction
        file.push_back(std::to_string(valueId));
        // write value byte
        file.push_back(std::to_string(val));

        return 1;
    }
    // pointer to value in RAM
    else if (arg.type == LBRAC)
    {
        // get value, advance past brackets
        arg = tokens[i+2];

        int val = parseVal(arg);

        // write instruction
        file.push_back(std::to_string(pointerId));
        // write value byte
        file.push_back(std::to_string(val));

        return 3;
    }

    return 0;
}

std::vector<std::string> write(std::vector<Token> tokens)
{
    // lookup table of the byte location of labels
    int bytes = 0;
    std::vector<std::string> out;

    // loop through tokens
    for (int i = 0; i < tokens.size(); ++i)
    {
        Token current = tokens[i];

        if (current.type == INSTR)
        {
            if (current.val == "nop" || current.val == "out" || current.val == "hlt")
                out.push_back(std::to_string(opcodes.at(current.val).first));
            else // instructions with arguments
            {
                if (current.val == "jmp" || current.val == "jc" || current.val == "jz")
                {
                    // jump to label
                    Token arg = tokens[i+1];
                    std::pair<int, int> pair = opcodes.at(current.val);
                    if (arg.type == INSTR)
                    {
                        out.push_back(std::to_string(pair.first));

                        // if referencing a non-indexed label, just put the label name for later
                        if (labels.find(arg.val) == labels.end())
                            out.push_back(arg.val);
                        else
                        {
                            int val = labels[arg.val];
                            out.push_back(std::to_string(val));
                        }

                        // advance past label jumping to
                        ++i;
                    }
                    // jump to value (can be pointer)
                    else
                        i += parseSingleArg(pair.first, pair.second, i, tokens, out);
                }
                else
                {
                    if (tokens[i+1].type == LBRAC && current.val == "sta")
                        error(typeerr, tokens[i+1]);

                    std::pair<int, int> pair = opcodes.at(current.val);
                    i += parseSingleArg(pair.first, pair.second, i, tokens, out);
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

            labels.insert({ current.val, i });
        }
    }

    return out;
}

void replaceLabels(std::vector<std::string> lines)
{
    std::ofstream file("out.txt");

    for (std::string line : lines)
    {
        try {
            stoi(line);
            file << line << std::endl;
        // if not a number, insert label location
        } catch (std::invalid_argument)
        {
            if (labels.find(line) == labels.end())
                error("Nonexistent label referenced!", -1, -1);
            file << labels[line] << std::endl;
        }
    }

    file.close();
}

int main(int argc, char *argv[])
{
    if (argc > 1)
        replaceLabels(write(parseTokens(argv[1])));
    else
        std::cout << "Must give a file argument!" << std::endl;

    return 0;
}