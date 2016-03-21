#include "io.hh"
using namespace bn;

#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <unordered_map>
#include <regex>
#include <cmath>
#include <chrono>
using namespace std;


static unordered_map<string,bool> options;
static MN *model;
static unordered_map<unsigned,unsigned> evidence;


void
usage(const char *progname);

void
read_options(int argc, char *argv[]);

void
prompt();

void
execute_partition();

void
execute_marginals();


int
main(int argc, char *argv[])
{
	char *progname = argv[0];
	if (argc < 2) {
		usage(progname);
		exit(1);
	}

	read_options(argc, argv);
	if (options["help"]) {
		usage(progname);
		return 0;
	}

	char *model_filename = argv[1];
	if (read_uai_model(model_filename, &model)) {
		return -1;
	}

	char *evidence_filename = argv[2];
	if (read_uai_evidence(evidence_filename, evidence)) {
		return -2;
	}

	prompt();

	delete model;

	return 0;
}

void
usage(const char *progname)
{
	cout << "usage: " << progname << " /path/to/model.uai /path/to/evidence.evid [OPTIONS]" << endl << endl;
	cout << "OPTIONS:" << endl;
	cout << "-h\tdisplay help information" << endl;
	cout << "-v\tverbose" << endl;
}

void
read_options(int argc, char *argv[])
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

void
prompt()
{
	if (options["verbose"]) {
		cout << ">> Model:" << endl;
		cout << *model << endl;
		cout << ">> Evidence:" << endl;
		for (auto it : evidence) {
			cout << "Variable = " << it.first << ", Value = " << it.second << endl;
		}
		cout << endl;
	}

	regex quit_regex("quit");
	regex partition_regex("PR|pr|partition");
	regex marginals_regex("MAR|mar|marginals");

	cout << ">> Query prompt:" << endl;
	while (cin) {
		cout << "? ";
		string line;
		getline(cin, line);

		smatch str_match_result;
		if (regex_match(line, partition_regex)) {
			execute_partition();
		}
		else if (regex_match(line, marginals_regex)) {
			execute_marginals();
		}
		else if (regex_match(line, quit_regex)) {
			break;
		}
		else {
			cout << "Error: not a valid query." << endl;
		}
	}
}

void
execute_partition()
{
	auto start = chrono::steady_clock::now();
	double p = log10(model->partition(evidence));
	auto end = chrono::steady_clock::now();
	auto diff = chrono::duration <double, milli> (end-start).count();
	cout << "partition = " << p << endl << endl;
	cout << ">> Executed in " << diff << "ms." << endl << endl;
}

void
execute_marginals()
{
	cout << ">> Marginals:" << endl;
	auto start = chrono::steady_clock::now();
	vector<const Factor*> marginals = model->marginals(evidence);
	auto end = chrono::steady_clock::now();
	for (auto pf : marginals) {
		cout << *pf << endl;
		delete pf;
	}
	auto diff = chrono::duration <double, milli> (end-start).count();
	cout << ">> Executed in " << diff << "ms." << endl << endl;
}
