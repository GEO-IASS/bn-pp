#include "io.hh"
#include "utils.hh"
#include "model.hh"
#include "graph.hh"
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

void
execute_width();

void
execute_stats();

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
		cout << *model << endl;
	}

	if (positional.size() > 1) {
		string evidence_filename = positional[1];
		if (options["verbose"]) {
			cout << ">> Reading file " << evidence_filename << " ..." << endl;
		}
		if (read_uai_evidence(evidence_filename, evidence)) {
			return -2;
		}
		if (options["verbose"] && !evidence.empty()) {
			cout << ">> Evidence:" << endl;
			for (auto it : evidence) {
				cout << "Variable = " << it.first << ",\tValue = " << it.second << endl;
			}
			cout << endl;
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
	cout << "-ls\tsolve inference using logical sampling" << endl;
	cout << "-ve\tsolve inference using variable elimination" << endl;
	cout << "-mf\tvariable elimination using min-fill heuristic" << endl;
	cout << "-wmf\tvariable elimination using weighted min-fill heuristic" << endl;
	cout << "-md\tvariable elimination using min-degree heuristic" << endl;
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

	options["logical-sampling"] = false;

	options["variable-elimination"] = false;
	options["bayes-ball"] = false;
	options["min-fill"] = false;
	options["weighted-min-fill"] = false;
	options["min-degree"] = false;

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
		else if (param == "-ls") {
			options["logical-sampling"] = true;
		}
		else if (param == "-ve") {
			options["variable-elimination"] = true;
		}
		else if (param == "-mf") {
			options["min-fill"] = true;
		}
		else if (param == "-wmf") {
			options["weighted-min-fill"] = true;
		}
		else if (param == "-md") {
			options["min-degree"] = true;
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

	if (options["verbose"]) {
		cout << ">> Computing partition for evidence ..." << endl;
	}
	double p = model->partition(evidence, options, uptime);
	cout << ">> Partition = " << p << endl;
	cout << ">> Executed in " << uptime << "ms." << endl << endl;
}

void
execute_marginals()
{
	double uptime;
	vector<const Factor*> marginals = model->marginals(evidence, options, uptime);

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
	regex query_regex("query ([^\\|]+)\\s*(\\|\\s*(.*))?");
	regex independence_regex("ind ([0-9]+)\\s*,\\s*([0-9]+)\\s*(\\|\\s*([0-9]+(\\s*,\\s*[0-9]+)*))?");

	regex sample_regex("sample");

	regex stats_regex("stats");
	regex roots_regex("roots");
	regex leaves_regex("leaves");

	regex width_regex("width");

	regex quit_regex("quit");

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
		else if (regex_match(line, stats_regex)) {
			execute_stats();
		}
		else if (regex_match(line, roots_regex)) {
			execute_roots();
		}
		else if (regex_match(line, leaves_regex)) {
			execute_leaves();
		}
		else if (regex_match(line, width_regex)) {
			execute_width();
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

void
execute_width()
{
	vector<const Variable*> vars(model->variables().begin(), model->variables().end());
	vector<const Factor*> factors(model->factors().begin(), model->factors().end());

	Graph g(vars, factors);
	unsigned original_width, min_fill_width, weighted_min_fill_width, min_degree;

	// original order
	original_width = g.order_width(vars);
	cout << endl;
	cout << ">> Original elimination order          (width = " << original_width << ")" << endl;
	if (options["verbose"]) {
		cout << "  ";
		for (auto const pv : vars) {
			cout << " " << pv->id();
		}
		cout << endl << endl;
	}

	// min-degree order
	unordered_map<string,bool> order_options;
	order_options["min-degree"] = true;

	vector<unsigned> ids = g.ordering(vars, min_degree, order_options);
	cout << ">> Min-degree elimination order        (width = " << min_degree << ")" << endl;
	if (options["verbose"]) {
		cout << "  ";
		for (auto id : ids) {
			cout << " " << id;
		}
		cout << endl << endl;
	}

	// min-fill order
	order_options.clear();
	order_options["min-fill"] = true;

	ids = g.ordering(vars, min_fill_width, order_options);
	cout << ">> Min-fill elimination order          (width = " << min_fill_width << ")" << endl;
	if (options["verbose"]) {
		cout << "  ";
		for (auto id : ids) {
			cout << " " << id;
		}
		cout << endl << endl;
	}

	// weighted min-fill order
	order_options.clear();
	order_options["weighted-min-fill"] = true;

	ids = g.ordering(vars, weighted_min_fill_width, order_options);
	cout << ">> Weighted min-fill elimination order (width = " << weighted_min_fill_width << ")" << endl;
	if (options["verbose"]) {
		cout << "  ";
		for (auto id : ids) {
			cout << " " << id;
		}
		cout << endl;
	}
	cout << endl;
}

void
execute_stats()
{
	unsigned nvars, nfactors;
	unsigned nroots, nleaves, ndisconnected;
	unsigned maxcard;
	unsigned maxparents, maxchildren;
	unsigned nparams;
	double minprob, maxprob, maxpartition;

	vector<Variable*> variables = model->variables();
	vector<Factor*> factors = model->factors();

	nvars = variables.size();
	nfactors = factors.size();

	nroots = nleaves = ndisconnected = 0;
	maxparents = maxchildren = 0;
	maxcard = 0;
	for (auto const pv : variables) {
		unsigned card = pv->size();
		maxcard = (card > maxcard) ? card : maxcard;

		unordered_set<const Variable*> pa = model->parents(pv);
		unsigned nparents = pa.size();

		unordered_set<const Variable*> ch = model->children(pv);
		unsigned nchildren = ch.size();

		if (nparents == 0)  ++nroots;
		if (nchildren == 0) ++nleaves;
		if (nparents == 0 && nchildren == 0) ++ndisconnected;
		maxparents  = (nparents > maxparents)   ? nparents  : maxparents;
		maxchildren = (nchildren > maxchildren) ? nchildren : maxchildren;
	}

	nparams = 0;
	minprob = 1.0;
	maxprob = 0.0;
	maxpartition = 0.0;
	for (auto const pf : factors) {
		const Domain &domain = pf->domain();

		unsigned size = domain.size();
		nparams += size-1;

		double partition = pf->partition();
		maxpartition = (partition > maxpartition) ? partition : maxpartition;

		for (unsigned i = 0; i < size; ++i) {
			double prob = (*pf)[i];
			minprob = (prob < minprob) ? prob : minprob;
			maxprob = (prob > maxprob) ? prob : maxprob;
		}
	}

	string name = model->name();
	cout << ">> Stats (" << name << ")" << endl;
	cout << ">> variables = " << nvars << ", factors = " << nfactors << endl;
	cout << ">> roots = " << nroots << ", leaves = " << nleaves << ", disconnected = " << ndisconnected << endl;
	cout << ">> max number of parents = " << maxparents << ", max number of children = " << maxchildren << endl;
	cout << ">> max domain size = " << maxcard << endl;
	cout << ">> number of parameters = " << nparams << endl;
	cout << ">> lowest probability = " << minprob << ", highest probability = " << maxprob << endl;
	cout << ">> max partition = " << maxpartition << endl;
	cout << endl;
}
