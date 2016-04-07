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
static vector<string> positional;

static BN *model;
static unordered_map<unsigned,unsigned> evidence;


void
usage(const char *progname);

void
read_parameters(int argc, char *argv[]);

void
prompt();

void
execute_task();

void
execute_partition();

void
execute_marginals();

void
execute_query(smatch result);

void
execute_independence_assertion(smatch result);

void
execute_sampling();

void
execute_roots();

void
execute_leaves();

int
main(int argc, char *argv[])
{
	char *progname = argv[0];
	if (argc < 2) {
		usage(progname);
		exit(1);
	}

	read_parameters(argc, argv);
	if (options["help"]) {
		usage(progname);
		return 0;
	}

	string model_filename = positional[0];
	if (options["verbose"]) {
		cout << ">> Reading file " << model_filename << " ..." << endl;
	}
	if (read_uai_model(model_filename, &model)) {
		return -1;
	}
	if (options["verbose"]) {
		cout << *model << " ..." << endl;
	}

	if (positional.size() > 1) {
		string evidence_filename = positional[1];
		if (options["verbose"]) {
			cout << ">> Reading file " << evidence_filename << " ..." << endl;
		}
		if (read_uai_evidence(evidence_filename, evidence)) {
			return -2;
		}
	}

	execute_task();

	delete model;

	return 0;
}

void
usage(const char *progname)
{
	cout << "usage: " << progname << " /path/to/model.uai [/path/to/evidence.uai.evid TASK] [OPTIONS]" << endl;
	cout << endl;
	cout << "TASK:" << endl;
	cout << "-pr\tcompute partition function" << endl;
	cout << "-mar\tcompute marginals" << endl;
	cout << endl;
	cout << "OPTIONS:" << endl;
	cout << "-ve\tsolve inference using variable elimination" << endl;
	cout << "-bb\tsolve inference using bayes-ball" << endl;
	cout << "-h\tdisplay help information" << endl;
	cout << "-v\tverbose" << endl;
}

void
read_parameters(int argc, char *argv[])
{
	// default options
	options["partition"] = false;
	options["marginals"] = false;
	options["bayes-ball"] = false;
	options["variable-elimination"] = false;
	options["verbose"] = false;
	options["help"] = false;

	for (int i = 1; i < argc; ++i) {

		string param(argv[i]);

		if (param == "-pr") {
			options["partition"] = true;
		}
		else if (param == "-mar") {
			options["marginals"] = true;
		}
		else if (param == "-ve") {
			options["variable-elimination"] = true;
		}
		else if (param == "-bb") {
			options["bayes-ball"] = true;
		}
		else if (param == "-v") {
			options["verbose"] = true;
		}
		else if (param == "-h") {
			options["help"] = true;
		}
		else if (param[0] == '-') {
			cerr << "Error: invalid option `" << param << "'." << endl << endl;
			usage(argv[0]);
			exit(-1);
		}
		else {
			positional.push_back(param);
		}
	}
}

void
execute_task()
{
	if (options["partition"] || options["marginals"]) {
		if (options["partition"]) execute_partition();
		if (options["marginals"]) execute_marginals();
	}
	else {
		prompt();
	}
}

void
execute_partition()
{
	double uptime;
	double p = model->partition(evidence, uptime);

	cout << ">> Partition = " << p << endl;

	if (options["verbose"] && !evidence.empty()) {
		cout << ">> Evidence:" << endl;
		for (auto it : evidence) {
			cout << "Variable = " << it.first << ", Value = " << it.second << endl;
		}
		cout << endl;
	}

	cout << ">> Executed in " << uptime << "ms." << endl << endl;
}

void
execute_marginals()
{
	double uptime;
	vector<const Factor*> marginals = model->marginals(evidence, uptime);

	cout << ">> Marginals:" << endl;
	for (auto pf : marginals) {
		cout << *pf << endl;
		delete pf;
	}

	if (options["verbose"] && !evidence.empty()) {
		cout << ">> Evidence:" << endl;
		for (auto it : evidence) {
			cout << "Variable = " << it.first << ", Value = " << it.second << endl;
		}
		cout << endl;
	}

	cout << ">> Executed in " << uptime << "ms." << endl << endl;
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
	regex sample_regex("sample");
	regex roots_regex("roots");
	regex leaves_regex("leaves");

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
		else if (regex_match(line, sample_regex)) {
			execute_sampling();
		}
		else if (regex_match(line, roots_regex)) {
			execute_roots();
		}
		else if (regex_match(line, leaves_regex)) {
			execute_leaves();
		}
		else if (regex_match(line, quit_regex)) {
			break;
		}
		else {
			cout << "Error: not a valid query." << endl << endl;
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
		q = model->query_ve(target_vars, evidence_vars, options, uptime);
	}
	else {
		q = model->query(target_vars, evidence_vars, options, uptime);
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

void
execute_sampling()
{
	unordered_map<unsigned,unsigned> sample = model->sampling();
	cout << "sample = " << endl;
	for (auto it : sample) {
		unsigned id = it.first;
		unsigned val = it.second;
		cout << id << " -> " << val << endl;
	}
	cout << endl;
}

void
execute_roots()
{
	vector<const Variable*> roots = model->roots();
	cout << ">> Roots: ";
	cout << roots[0]->id();
	for (unsigned i = 1; i < roots.size(); ++i) {
		cout << ", " << roots[i]->id();
	}
	cout << endl << endl;
}

void
execute_leaves()
{
	vector<const Variable*> leaves = model->leaves();
	cout << ">> Leaves: ";
	cout << leaves[0]->id();
	for (unsigned i = 1; i < leaves.size(); ++i) {
		cout << ", " << leaves[i]->id();
	}
	cout << endl << endl;
}
