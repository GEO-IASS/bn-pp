#include "io.hh"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

using namespace std;

namespace bn {

bool
read_next_token(ifstream &input_file, string &token)
{
    while (input_file) {
        input_file >> token;
        if (token[0] != '#') return true;
        getline(input_file, token);  // ignore rest of line
    }
    return false;
}

bool
read_next_integer(ifstream &input_file, unsigned &i)
{
    string token;
    if (!read_next_token(input_file, token)) return false;
    i = stoi(token);
    return true;
}

bool
read_next_double(ifstream &input_file, double &d)
{
    string token;
    if (!read_next_token(input_file, token)) return false;
    d = stod(token);
    return true;
}

string
read_file_header(ifstream &input_file)
{
    string token;
    read_next_token(input_file, token);
    if (token.compare("BAYES") != 0)  {
        cerr << "ERROR! Expected 'BAYES' file header, found: " << token << endl;
    }
    return token;
}

void
read_variables(ifstream &input_file, unsigned &order, vector<const Variable*> &variables)
{
    read_next_integer(input_file, order);
    unsigned sz = 0;
    for (unsigned id = 0; id < order; ++id) {
        read_next_integer(input_file, sz);
        variables.push_back(new Variable(id, sz));
    }
}

int
read_uai_model(const char *filename, unsigned &order, vector<const Variable*> &variables)
{
    ifstream input_file(filename);
    if (input_file.is_open()) {
        read_file_header(input_file);
        read_variables(input_file, order, variables);
        input_file.close();
        return 0;
    }
    else {
        cerr << "Error: couldn't read file " << filename << endl;
        return -1;
    }
}

}
