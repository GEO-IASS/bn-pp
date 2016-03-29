#include "io.hh"
#include "utils.hh"
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
execute_query(smatch result);

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
	cout << "-ve\tsolve query using variable elimination" << endl;
	cout << "-bb\tsolve query using bayes-ball" << endl;
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
	options["variable-elimination"] = false;

	for (int i = 2; i < argc; ++i) {
		string option(argv[i]);
		if (option == "-h") {
			options["help"] = true;
		}
		else if (option == "-ve") {
			options["variable-elimination"] = true;
		}
		else if (option == "-bb") {
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
	regex query_regex("query ([^\\|]+)\\s*(\\|\\s*(.*))?");
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
execute_query(smatch result)
{
	// parse query
	regex whitespace_regex("\\s");
	string target   = result[1]; target   = regex_replace(target,   whitespace_regex, "");
	string evidence = result[3]; evidence = regex_replace(evidence, whitespace_regex, "");

	unordered_set<const Variable*> target_vars;
	unordered_set<const Variable*> evidence_vars;
	parse_vars_set(model, target, target_vars);
	if (evidence != "") {
		parse_vars_set(model, evidence, evidence_vars);
	}

	// solve query
	double uptime;
	Factor q;
	if (options["variable-elimination"]) {
		q = model->query_ve(target_vars, evidence_vars, uptime, options);
	}
	else {
		q = model->query(target_vars, evidence_vars, uptime, options);
	}

	// print results
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
execute_independence_assertion(smatch result)
{
	string id1 = result[1];
	string id2 = result[2];
	const Variable *var1 = model->variables()[stoi(id1)];
	const Variable *var2 = model->variables()[stoi(id2)];

	unordered_set<const Variable*> evidence_vars;
	string evidence = result[4];
	if (evidence != "") {
		parse_vars_set(model, evidence, evidence_vars);
	}

	cout << (model->m_separated(var1, var2, evidence_vars, options["verbose"]) ? "true" : "false") << endl << endl;
}
