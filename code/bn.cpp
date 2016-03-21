#include "io.hh"
using namespace bn;

#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <unordered_map>
#include <regex>
using namespace std;


static unordered_map<string,bool> options;
static BN *model;


void
usage(const char *progname);

void
read_options(int argc, char *argv[]);

void
prompt();

void
parse_query(smatch result, string &target, string &evidence, unordered_set<const Variable*> &target_vars, unordered_set<const Variable*> &evidence_vars);

void
execute_query(smatch result);

void
parse_independence_assertion(smatch result, const Variable **var1, const Variable **var2, unordered_set<const Variable*> &evidence_vars);

void
execute_independence_assertion(smatch result);


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

	prompt();

	delete model;

	return 0;
}

void
usage(const char *progname)
{
	cout << "usage: " << progname << " /path/to/model.uai [OPTIONS]" << endl << endl;
	cout << "OPTIONS:" << endl;
	cout << "-b\tsolve query using bayes-ball" << endl;
	cout << "-h\tdisplay help information" << endl;
	cout << "-v\tverbose" << endl;
}

void
read_options(int argc, char *argv[])
{
	// default options
	options["verbose"] = false;
	options["help"] = false;
	options["bayes-ball"] = false;

	for (int i = 2; i < argc; ++i) {
		string option(argv[i]);
		if (option == "-h") {
			options["help"] = true;
		}
		else if (option == "-b") {
			options["bayes-ball"] = true;
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
	}

	regex quit_regex("quit");
	regex query_regex("query ([0-9]+(\\s*,\\s*[0-9]+)*)\\s*(\\|\\s*([0-9]+(\\s*,\\s*[0-9]+)*))?");
	regex independence_regex("ind ([0-9]+)\\s*,\\s*([0-9]+)\\s*(\\|\\s*([0-9]+(\\s*,\\s*[0-9]+)*))?");

	cout << ">> Query prompt:" << endl;
	while (cin) {
		cout << "? ";
		string line;
		getline(cin, line);

		smatch str_match_result;
		if (regex_match(line, str_match_result, query_regex)) {
			execute_query(str_match_result);
		}
		else if (regex_match(line, str_match_result, independence_regex)) {
			execute_independence_assertion(str_match_result);
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
parse_query(
	smatch result,
	string &target, string &evidence,
	unordered_set<const Variable*> &target_vars, unordered_set<const Variable*> &evidence_vars)
{
	regex whitespace_regex("\\s");

	target = result[1];
	target = regex_replace(target, whitespace_regex, "");
	evidence = result[4];
	evidence = regex_replace(evidence, whitespace_regex, "");

	string var = "";
	for (char& c: target) {
		if (c != ',') var += c;
		else {
			target_vars.insert(model->variables()[stoi(var)]);
			var = "";
		}
	}
	target_vars.insert(model->variables()[stoi(var)]);

	if (evidence != "") {
		var = "";
		for (char& c: evidence) {
			if (c != ',') var += c;
			else {
				evidence_vars.insert(model->variables()[stoi(var)]);
				var = "";
			}
		}
		evidence_vars.insert(model->variables()[stoi(var)]);
	}
}

void
execute_query(smatch result)
{
	string target;
	string evidence;
	unordered_set<const Variable*> target_vars;
	unordered_set<const Variable*> evidence_vars;
	parse_query(result, target, evidence, target_vars, evidence_vars);

	double uptime;
	Factor q = model->query(target_vars, evidence_vars, uptime, options);
	if (evidence != "") {
		cout << "P(" + target + "|" + evidence + ") =" << endl;
	}
	else {
		cout << "P(" + target + ") =" << endl;
	}
	cout << q;
	cout << ">> Executed in " << uptime << "ms." << endl << endl;
}


void
parse_independence_assertion(
	smatch result,
	const Variable **var1, const Variable **var2,
	unordered_set<const Variable*> &evidence_vars)
{
	regex whitespace_regex("\\s");

	string id1 = result[1];
	string id2 = result[2];

	string evidence = result[4];
	evidence = regex_replace(evidence, whitespace_regex, "");

	*var1 = model->variables()[stoi(id1)];
	*var2 = model->variables()[stoi(id2)];

	if (evidence != "") {
		string var = "";
		for (char& c: evidence) {
			if (c != ',') var += c;
			else {
				evidence_vars.insert(model->variables()[stoi(var)]);
				var = "";
			}
		}
		evidence_vars.insert(model->variables()[stoi(var)]);
	}
}

void
execute_independence_assertion(smatch result)
{
	const Variable *var1;
	const Variable *var2;
	unordered_set<const Variable*> evidence;
	parse_independence_assertion(result, &var1, &var2, evidence);
	cout << (model->m_separated(var1, var2, evidence, options["verbose"]) ? "true" : "false") << endl;
}
