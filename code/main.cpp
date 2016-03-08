#include "io.hh"
using namespace bn;

#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <unordered_map>
#include <regex>
using namespace std;


void
usage(const char *progname);

void
read_options(unordered_map<string,bool> &options,  int argc, char *argv[]);

void
prompt(const BN &model);

void
execute(const BN &model, string target, string evidence);

int
main(int argc, char *argv[])
{
	char *progname = argv[0];
	if (argc < 2) {
		usage(progname);
		exit(1);
	}

	unordered_map<string,bool> options;
	read_options(options, argc, argv);
	if (options["help"]) {
		usage(progname);
		return 0;
	}

	char *model_filename = argv[1];
	unsigned order;
	BN *model;
	read_uai_model(model_filename, order, &model);
	if (options["verbose"]) {
		cout << ">> Model:" << endl;
		cout << *model << endl;
	}

	prompt(*model);

	delete model;

	return 0;
}

void
usage(const char *progname)
{
	cout << "usage: " << progname << " /path/to/model.uai [OPTIONS]" << endl << endl;
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

void
prompt(const BN &model)
{
	cout << ">> Query prompt:" << endl;
	regex query_regex("query ([0-9](\\s*,\\s*[0-9])*)\\s*(\\|\\s*([0-9](\\s*,\\s*[0-9])*))?");
	regex whitespace_regex("\\s");
	regex quit_regex("quit");
	while (cin) {
		string line;
		std::smatch str_match_result;
		cout << "? ";
		getline(cin, line);
		if (regex_match(line, str_match_result, query_regex)) {
			string target = str_match_result[1];
			target = regex_replace(target, whitespace_regex, "");
			string evidence = str_match_result[4];
			evidence = regex_replace(evidence, whitespace_regex, "");
			execute(model, target, evidence);
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
execute(const BN &model, string target, string evidence)
{
	unordered_set<const Variable*> target_vars;
	string var = "";
	for (char& c: target) {
		if (c != ',') var += c;
		else {
			target_vars.insert(model.variables()[stoi(var)]);
			var = "";
		}
	}
	target_vars.insert(model.variables()[stoi(var)]);

	unordered_set<const Variable*> evidence_vars;
	if (evidence != "") {
		var = "";
		for (char& c: evidence) {
			if (c != ',') var += c;
			else {
				evidence_vars.insert(model.variables()[stoi(var)]);
				var = "";
			}
		}
		evidence_vars.insert(model.variables()[stoi(var)]);
	}

	Factor q = model.query(target_vars, evidence_vars);
	cout << q << endl;
}
