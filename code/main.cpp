#include "io.hh"
using namespace bn;

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
using namespace std;


void
usage(const char *filename);

void
read_options(unordered_map<string,bool> &options,  int argc, char *argv[]);


int
main(int argc, char *argv[])
{
	char *progname = argv[0];
	if (argc < 2) {
		usage(progname);
		exit(1);
	}

	char *filename = argv[1];
	unordered_map<string,bool> options;
	read_options(options, argc, argv);
	if (options["help"]) {
		usage(progname);
		return 0;
	}

	unsigned order;
	vector<Variable*> variables;
	vector<Factor*> factors;
	read_uai_model(filename, order, variables, factors);

	if (options["verbose"]) {
		cout << ">> Variables" << endl;
		for (auto pv : variables) {
			cout << *pv << endl;
		}
		cout << endl;
		cout << ">> Factors" << endl;
		for (auto pf : factors) {
			cout << *pf << endl;
		}
	}

	return 0;
}

void
usage(const char *filename)
{
	cout << "usage: " << filename << " /path/to/model.uai [OPTIONS]" << endl << endl;
	cout << "OPTIONS:" << endl;
	cout << "-h\tdisplay help information" << endl;
	cout << "-v\tverbose" << endl;
}

void
read_options(unordered_map<string,bool> &options, int argc, char *argv[])
{
	// default options
	options["verbose"] = false;
	options["help"] = false;

	for (int i = 2; i < argc; ++i) {
		string option(argv[i]);
		if (option == "-h") {
			options["help"] = true;
		}
		else if (option == "-v") {
			options["verbose"] = true;
		}
	}
}
