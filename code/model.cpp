#include "model.hh"
#include "graph.hh"

#include <unordered_set>
#include <forward_list>
#include <iostream>
#include <chrono>
#include <cassert>
#include <cmath>
using namespace std;

namespace bn {

Model::Model(string name, vector<Variable*> &variables, vector<Factor*> &factors) :
	_name(name),
	_variables(variables),
	_factors(factors)
{
}

Model::~Model()
{
	for (auto pv : _variables) {
		delete pv;
	}
	for (auto pf : _factors) {
		delete pf;
	}
}

Factor
Model::joint_distribution() const
{
	Factor f(1.0);
	for (auto pf : _factors) {
		f *= *pf;
	}
	return f;
}

Factor
Model::joint_distribution(const unordered_map<unsigned,unsigned> &evidence) const
{
	Factor f(1.0);
	for (auto pf : _factors) {
		f *= pf->conditioning(evidence);
	}
	return f;
}

double
Model::partition(
	const unordered_map<unsigned,unsigned> &evidence,
	unordered_map<string,bool> &options,
	double &uptime) const
{
	auto start = chrono::steady_clock::now();

	Factor f = joint_distribution(evidence);
	double p = f.partition();

	auto end = chrono::steady_clock::now();
	auto diff = end - start;
	uptime = chrono::duration <double, milli> (diff).count();

	return p;
}

vector<const Factor*>
Model::marginals(
	const unordered_map<unsigned,unsigned> &evidence,
	unordered_map<string,bool> &options,
	double &uptime) const
{
	auto start = chrono::steady_clock::now();

	Factor joint = joint_distribution(evidence).normalize();

	vector<const Factor*> marg;
	for (auto pv : _variables) {
		marg.push_back(new Factor(marginal(pv, joint)));
	}

	auto end = chrono::steady_clock::now();
	auto diff = end - start;
	uptime = chrono::duration <double, milli> (diff).count();

	return marg;
}

Factor
Model::marginal(const Variable *v, Factor &joint) const
{
	Factor f = joint;
	for (auto pv : _variables) {
		if (pv->id() != v->id()) {
			f = f.sum_out(pv);
		}
	}
	return f;
}


BN::BN(string name, vector<Variable*> &variables, vector<Factor*> &factors) : Model(name, variables, factors)
{
	for (auto pv : _variables) {
		unordered_set<const Variable*> children;
		_children[pv] = children;
	}

	for (unsigned i = 0; i < _variables.size(); ++i) {
		const Variable *v = _variables[i];
		vector<const Variable*> scope = _factors[i]->domain().scope();
		unordered_set<const Variable*> parents(scope.begin()+1, scope.end());
		_parents[v] = parents;
		for (auto p : parents) {
			_children[p].insert(v);
		}
	}
}

const vector<const Variable*>
BN::roots() const
{
	vector<const Variable*> vars;
	for (auto const pv : _variables) {
		if (_parents.find(pv)->second.empty()) {
			vars.push_back(pv);
		}
	}
	return vars;
}

const vector<const Variable*>
BN::leaves() const
{
	vector<const Variable*> vars;
	for (auto const pv : _variables) {
		if (_children.find(pv)->second.empty()) {
			vars.push_back(pv);
		}
	}
	return vars;
}


Factor
BN::query(
	const unordered_set<const Variable*> &target,
	const unordered_set<const Variable*> &evidence,
	unordered_map<string,bool> &options,
	double &uptime) const
{
	auto start = chrono::steady_clock::now();

	Factor joint(1.0);
	if (options["bayes-ball"]) {
		unordered_set<const Variable*> Np, Ne, F;
		bayes_ball(target, evidence, F, Np, Ne);
		for (auto const pv : Np) {
			unsigned id = pv->id();
			joint *= *_factors[id];
		}

		if (options["verbose"]) {
			cout << ">> Requisite probability nodes Np:" << endl;
			for (auto const pv : Np) {
				cout << *pv << endl;
			}
			cout << endl;

			cout << ">> Requisite observation nodes Ne" << endl;
			for (auto const pv: Ne) {
				cout << *pv << endl;
			}
			cout << endl;
		}
	}
	else {
		joint = joint_distribution();
	}

	Factor f = joint;
	for (auto pv : _variables) {
		if (target.find(pv) == target.end() && evidence.find(pv) == evidence.end()) {
			f = f.sum_out(pv);
		}
	}
	if (!evidence.empty()) {
		Factor g = f;
		for (auto pv : target) {
			g = g.sum_out(pv);
		}
		f = f.divide(g);
	}

	auto end = chrono::steady_clock::now();
	auto diff = end - start;
	uptime = chrono::duration <double, milli> (diff).count();

	return f;
}

Factor
BN::query_ve(
	const unordered_set<const Variable*> &target,
	const unordered_set<const Variable*> &evidence,
	unordered_map<string,bool> &options,
	double &uptime) const
{
	auto start = chrono::steady_clock::now();

	vector<const Variable*> variables;
	vector<const Factor*> factors;
	if (options["bayes-ball"]) {
		unordered_set<const Variable*> Np, Ne, F;
		bayes_ball(target, evidence, F, Np, Ne);
		for (auto pv : Np) {
			if (target.find(pv) == target.end() && evidence.find(pv) == evidence.end()) {
				variables.push_back(pv);
			}
			factors.push_back(_factors[pv->id()]);
		}
	}
	else {
		for (auto pv : _variables) {
			if (target.find(pv) == target.end() && evidence.find(pv) == evidence.end()) {
				variables.push_back(pv);
			}
			factors.push_back(_factors[pv->id()]);
		}
	}

	Factor f = variable_elimination(variables, factors, options);
	if (!evidence.empty()) {
		Factor g = f;
		for (auto pv : target) {
			g = g.sum_out(pv);
		}
		f = f.divide(g);
	}

	auto end = chrono::steady_clock::now();
	auto diff = end - start;
	uptime = chrono::duration <double, milli> (diff).count();

	return f;
}

double
BN::partition(
	const unordered_map<unsigned,unsigned> &evidence,
	unordered_map<string,bool> &options,
	double &uptime) const
{
	double p = -1.0;

	auto start = chrono::steady_clock::now();

	if (options["logical-sampling"]) {
		double delta = 0.05;
		double epsilon = 0.05;
		p = logical_sampling(evidence, delta, epsilon);
	}
	else if (options["likelihood-weighting"]) {
		double delta = 0.05;
		double epsilon = 0.05;
		p = likelihood_weighting(evidence, delta, epsilon);
	}
	else if (options["gibbs-sampling"]) {
		long unsigned M = 100000;
		long unsigned burn_in = 10000;
		p = gibbs_sampling(evidence, M, burn_in);
	}
	// variable elimination by default
	else {
		vector<const Variable*> variables;
		for (auto const pv : _variables) {
			if (evidence.find(pv->id()) == evidence.end()) {
				variables.push_back(pv);
			}
		}
		vector<const Factor*> factors;
		for (auto const pf : _factors) {
			factors.push_back(new Factor(pf->conditioning(evidence)));
		}
		Factor part = variable_elimination(variables, factors, options);
		assert(part[0] == part.partition());
		p = part.partition();
		for (auto const pf : factors) {
			delete pf;
		}
		factors.clear();
	}

	auto end = chrono::steady_clock::now();
	auto diff = end - start;
	uptime = chrono::duration <double, milli> (diff).count();

	return p;
}

vector<const Factor*>
BN::marginals(
	const unordered_map<unsigned,unsigned> &evidence,
	unordered_map<string,bool> &options,
	double &uptime) const
{
	auto start = chrono::steady_clock::now();

	vector<const Factor*> marg;

	if (options["sum-product"]) {
		FactorGraph g = sum_product();
		for (auto const pv : _variables) {
			marg.push_back(new Factor(g.marginal(pv)));
		}
	}
	// variable elimination by default
	else {
		vector<const Factor*> factors;
		for (auto const pf : _factors) {
			factors.push_back(new Factor(pf->conditioning(evidence)));
		}

		for (auto const pv : _variables) {
			vector<const Variable*> vars;
			for (auto const pv2 : _variables) {
				if (pv2 != pv) {
					vars.push_back(pv2);
				}
			}
			marg.push_back(new Factor(variable_elimination(vars, factors, options).normalize()));
		}

		for (auto const pf : factors) {
			delete pf;
		}
	}

	auto end = chrono::steady_clock::now();
	auto diff = end - start;
	uptime = chrono::duration <double, milli> (diff).count();

	return marg;
}

Factor
BN::variable_elimination(
	vector<const Variable*> &variables,
	vector<const Factor*> &factors,
	unordered_map<string,bool> &options) const
{
	// initialize result
	Factor result(1.0);

	// choose elimination ordering
	vector<const Variable*> vars = variables;

	if (options["min-fill"] || options["weighted-min-fill"] || options["min-degree"]) {
		vector<const Variable*> model_variables(_variables.begin(), _variables.end());
		Graph g(model_variables, factors);

		unsigned width = 0;
		vector<unsigned> ids = g.ordering(variables, width, options);

		for (unsigned i = 0; i < ids.size(); ++i) {
			vars[i] = _variables.at(ids[i]);
		}

		if (options["verbose"]) {
			unsigned width = g.order_width(vars);
			cout << ">> Original elimination order (width = " << width << ")" << endl;
			cout << "  ";
			for (auto const pv : vars) {
				cout << " " << pv->id();
			}
			cout << endl << endl;
		}
	}

	forward_list<const Variable*> ordering(vars.begin(), vars.end());

	// initialize buckets
	unordered_map<unsigned,unordered_set<const Factor*>> buckets;

	// initialize new_factor_lst
	vector<const Factor*> new_factor_lst;

	for (auto pv : ordering) {
		unordered_set<const Factor*> bfactors;
		buckets[pv->id()] = bfactors;
	}
	for (auto pf : factors) {
		bool in_bucket = false;
		for (auto pv : ordering) {
			if (pf->domain().in_scope(pv)) {
				buckets[pv->id()].insert(pf);
				in_bucket = true;
				break;
			}
		}
		if (!in_bucket) {
			result *= *pf;
		}
	}

	// eliminate all variables
	while (!ordering.empty()) {
		const Variable *var = ordering.front();
		ordering.pop_front();

		// eliminate var
		Factor prod(1.0);
		for (auto pf : buckets[var->id()]) {
			prod *= *pf;
		}
		Factor *new_factor = new Factor(prod.sum_out(var));
		new_factor_lst.push_back(new_factor);

		// stop if finished
		if (buckets.empty()) {
			result *= *new_factor;
			break;
		}

		// update bucket list with new factor
		bool in_bucket = false;
		for (auto pv : ordering) {
			if (new_factor->domain().in_scope(pv)) {
				buckets[pv->id()].insert(new_factor);
				in_bucket = true;
				break;
			}
		}
		if (!in_bucket) {
			result *= *new_factor;
		}
	}

	for (auto pf : new_factor_lst) {
		delete pf;
	}

	return result;
}

void
BN::bayes_ball(const unordered_set<const Variable*> &J, const unordered_set<const Variable*> &K, const unordered_set<const Variable*> &F, unordered_set<const Variable*> &Np, unordered_set<const Variable*> &Ne) const
{
	// Initialize all nodes as neither visited, nor marked on the top, nor marked on the bottom.
	unordered_set<const Variable*> visited, top, bottom;

	// Create a schedule of nodes to be visited,
	// initialized with each node in J to be visited as if from one of its children.
	vector<const Variable*> schedule;
	vector<bool> origin; // true if from children, false from parent
	for (auto const j : J) {
		schedule.push_back(j);
		origin.push_back(true);
	}

	// While there are still nodes scheduled to be visited:
	while (!schedule.empty()) {

		// Pick any node j scheduled to be visited and remove it from the schedule.
		// Either j was scheduled for a visit from a parent, a visit from a child, or both.
		const Variable *j = schedule.back(); schedule.pop_back();
		bool from_child = origin.back();  origin.pop_back();

		// Mark j as visited.
		visited.insert(j);

		// If j not in K and the visit to j is from a child:
		if (K.find(j) == K.end() && from_child) {

			// if the top of j is not marked,
			// then mark its top and schedule each of its parents to be visited;
			if (top.find(j) == top.end()) {
				top.insert(j);
				unordered_set<const Variable*> parents = _parents.at(j);
				for (auto const pa : parents) {
					schedule.push_back(pa);
					origin.push_back(true);
				}
			}

			// if j not in F and the bottom of j is not marked,
			// then mark its bottom and schedule each of its children to be visited.
			if (F.find(j) == F.end() && bottom.find(j) == bottom.end()) {
				bottom.insert(j);
				unordered_set<const Variable*> children = _children.at(j);
				for (auto const ch : children) {
					schedule.push_back(ch);
					origin.push_back(false);
				}
			}
		}
		// If the visit to j is from a parent:
		else if (!from_child) {

			// If j in K and the top of j is not marked,
			// then mark its top and schedule each of its parents to be visited;
			if (K.find(j) != K.end() && top.find(j) == top.end()) {
				top.insert(j);
				unordered_set<const Variable*> parents = _parents.at(j);
				for (auto const pa : parents) {
					schedule.push_back(pa);
					origin.push_back(true);
				}
			}

			// If j not in K and the bottom of j is not marked,
			// then mark its bottom and schedule each of its children to be visited.
			if (K.find(j) == K.end() && bottom.find(j) == bottom.end()) {
				bottom.insert(j);
				unordered_set<const Variable*> children = _children.at(j);
				for (auto const ch : children) {
					schedule.push_back(ch);
					origin.push_back(false);
				}
			}
		}
	}

	// The requisite probability nodes Np are those nodes marked on top.
	for (auto const pv : top) {
		Np.insert(pv);
	}

	// The requisite observation nodes Ne are those nodes in K marked as visited.
	for (auto const pv : K) {
		if (visited.find(pv) != visited.end()) {
			Ne.insert(pv);
		}
	}
}


double
BN::logical_sampling(const unordered_map<unsigned,unsigned> &evidence, double delta, double epsilon) const
{
	double lp = 0.1;
	unsigned long M = 3*log(2/delta) / pow(epsilon,2) * 1/lp;
	long unsigned N = 0;
	for (long unsigned i = 0; i < M; ++i) {
		unordered_map<unsigned,unsigned> sample = sampling();
		bool consistent = true;
		for (auto it : evidence) {
			if (sample.at(it.first) != it.second) {
				consistent = false;
				break;
			}
		}
		if (consistent) ++N;
	}
	return 1.0*N/M;
}

unordered_map<unsigned,unsigned>
BN::sampling() const
{
	static vector<const Factor*> order = topological_sampling_order();
	unordered_map<unsigned,unsigned> valuation;
	for (auto pf : order) {
		unordered_map<unsigned,unsigned> sample = pf->sampling(valuation);
		for (auto it : sample) {
			valuation[it.first] = it.second;
		}
	}
	return valuation;
}

vector<const Factor*>
BN::topological_sampling_order() const
{
	vector<const Factor*> order;

	unordered_map<unsigned,unsigned> variables_to_sample;
	for (auto pv : _variables) {
		unsigned id = pv->id();
		unsigned nparents = _parents.find(pv)->second.size();
		variables_to_sample[id] = nparents;
	}

	while (!variables_to_sample.empty()) {
		unordered_map<unsigned,unsigned> new_variables_to_sample;
		unordered_set<unsigned> ready_to_sample;

		for (auto it : variables_to_sample) {
			unsigned id = it.first;
			unsigned left_to_sample = it.second;
			if (left_to_sample == 0) {
				ready_to_sample.insert(id);
				order.push_back(_factors[id]);
			}
			else {
				new_variables_to_sample[id] = left_to_sample;
			}
		}

		for (auto it : new_variables_to_sample) {
			unsigned id = it.first;
			for (auto id2 : ready_to_sample) {
				if (_factors[id]->domain().in_scope(id2)) {
					new_variables_to_sample[id]--;
				}
			}
		}

		variables_to_sample = new_variables_to_sample;
	}

	return order;
}


double
BN::likelihood_weighting(const unordered_map<unsigned,unsigned> &evidence, double delta, double epsilon) const
{
	static vector<const Factor*> order = topological_sampling_order();

	// initialization
	double U = 1.0;
	for (auto const pf : _factors) {
		U *= pf->max();
	}
	double Nstar = 4*log(2/delta)*(1+epsilon)/(pow(epsilon,2));
	double N, M;
	N = M = 0.0;

	// sampling
	while (N < Nstar) {
		double W = 1.0;
		unordered_map<unsigned,unsigned> valuation;
		for (auto const pf : order) {
			const Domain &d = pf->domain();
			const Variable *Xi = d[0];
			unsigned id = Xi->id();
			if (evidence.find(id) == evidence.end()) {
				unordered_map<unsigned,unsigned> sample = pf->sampling(valuation);
				for (auto it : sample) {
					valuation[it.first] = it.second;
				}
			}
			else {
				valuation[id] = evidence.find(id)->second;
				Factor f = pf->conditioning(valuation);
				assert(f.size() == 1);
				assert(f[0] == f.partition());
				W *= f[0];
			}
		}
		assert(W > 0.0);
		N += W/U;
		++M;
	}

	return U*N/M;
}

double
BN::gibbs_sampling(const unordered_map<unsigned,unsigned> &evidence, long unsigned M, long unsigned burn_in) const
{
	// pre-compute probabilities p(X|MB(X))
	vector<const Factor*> blanket_factors;
	for (auto const pf : _factors) {
		const Domain &d = pf->domain();
		const Variable *X = d[0];

		unordered_set<const Variable*> MB = markov_blanket(X);
		MB.insert(X);

		Factor factor(1.0);
		for (auto const pv : MB) {
			unordered_set<const Variable*> pa = parents(pv);
			Factor f(*_factors.at(pv->id()));
			for (auto const pv2 : pa) {
				if (MB.find(pv2) == MB.end()) {
					f = f.sum_out(pv2);
				}
			}
			factor *= f;
		}
		blanket_factors.push_back(new Factor(factor.divide(factor.sum_out(X))));
	}

	// initialize valuation
	unordered_map<unsigned,unsigned> valuation;
	for (auto const pv : _variables) {
		unsigned id = pv->id();
		if (evidence.find(id) == evidence.end()) {
			valuation[id] = 0;
		}
		else {
			valuation[id] = evidence.find(id)->second;
		}
	}

	// compute samplings from p(Xi|MB(Xi))
	long unsigned N = 0;
	for (long unsigned i = 0; i < M+burn_in; ++i) {

		// generate new valuation
		for (auto const pf : blanket_factors) {
			const Domain &d = pf->domain();
			const Variable *X = d[0];
			valuation.erase(valuation.find(X->id()));
			unordered_map<unsigned,unsigned> sample = pf->sampling(valuation);
			for (auto it : sample) {
				valuation[it.first] = it.second;
			}
		}

		if (i < burn_in) continue;

		// check if new valuation is consistent with evidence
		bool consistent = true;
		for (auto it : evidence) {
			if (valuation.at(it.first) != it.second) {
				consistent = false;
				break;
			}
		}
		if (consistent) ++N;
	}

	for (auto const pf : blanket_factors) {
		delete pf;
	}
	blanket_factors.clear();

	return 1.0*N/M;
}

FactorGraph
BN::sum_product(void) const
{
	vector<const Variable*> variables;
	for (auto const pv : _variables) {
		variables.push_back(pv);
	}
	vector<const Factor*> factors;
	for (auto const pf : _factors) {
		factors.push_back(pf);
	}
	FactorGraph g(variables, factors);

	unsigned iterations = g.update(10000, 0.001);
	// cout << ">> Number of iterations = " <<  iterations << endl;

	return g;
}

bool
BN::m_separated(const Variable *v1, const Variable *v2, const unordered_set<const Variable*> evidence, bool verbose) const
{
	unordered_map<const Variable*,unordered_set<const Variable*>> graph;

	// compute ancestor variables
	unordered_set<const Variable*> vars(evidence);
	vars.insert(v1);
	vars.insert(v2);
	unordered_set<const Variable*> all_ancestors = ancestors(vars);
	for (auto const pv : vars) {
		all_ancestors.insert(pv);
	}

	// initialization
	for (auto const pv: all_ancestors) {
		unordered_set<const Variable*> neighboors;
		graph[pv] = neighboors;
	}

	// build sub-graph
	for (auto const pv: all_ancestors) {
		unordered_set<const Variable*> pa = _parents.find(pv)->second;
		for (auto const p_pa : pa) {
			graph[pv].insert(p_pa);
			graph[p_pa].insert(pv);

			// moralization
			for (auto const p_pa2 : pa) {
				if (p_pa != p_pa2) {
					graph[p_pa].insert(p_pa2);
					graph[p_pa2].insert(p_pa);
				}
			}
		}
	}

	// remove evidence variables
	for (auto const e : evidence) {
		graph.erase(e);
		for (auto it : graph) {
			const Variable *v = it.first;
			unordered_set<const Variable*> neighboors = it.second;
			if (neighboors.find(e) != neighboors.end()) {
				graph[v].erase(e);
			}
		}
	}

	if (verbose) {
		cout << ">> Graph:" <<endl;
		for (auto const it : graph) {
			const Variable *v = it.first;
			unordered_set<const Variable*> neighboors = it.second;
			cout << "variable id=" << v->id() << ", neighboors={ ";
			for (auto const pv : neighboors) {
				cout << pv->id() << " ";
			}
			cout << "}" << endl;
		}
	}

	// path search in graph
	vector<const Variable*> stack;
	unordered_set<const Variable*> visited;
	stack.push_back(v1);
	while (!stack.empty()) {
		const Variable *v = stack.back(); stack.pop_back();
		visited.insert(v);
		if (v->id() == v2->id()) return false;
		else {
			unordered_set<const Variable*> neighboors = graph.find(v)->second;
			for (auto const pn : neighboors) {
				if (visited.find(pn) == visited.end()) {
					stack.push_back(pn);
				}
			}
		}
	}
	return true;
}

unordered_set<const Variable*>
BN::markov_blanket(const Variable *v) const
{
	unordered_set<const Variable*> MB;
	for (auto const pv : parents(v)) {
		MB.insert(pv);
	}
	for (auto const pv : children(v)) {
		MB.insert(pv);
		for (auto const pv2 : parents(pv)) {
			if (pv2->id() != v->id()) {
				MB.insert(pv2);
			}
		}
	}
	return MB;
}

unordered_set<const Variable*>
BN::markov_independence(const Variable* v) const
{
	unordered_set<const Variable*> nd;
	for (auto pv : _variables) {
		nd.insert(pv);
	}
	nd.erase(nd.find(v));

	for (auto pv : _parents.find(v)->second) {
		nd.erase(nd.find(pv));
	}
	for (auto id : descendants(v)) {
		nd.erase(nd.find(id));
	}
	return nd;
}

unordered_set<const Variable*>
BN::descendants(const Variable *v) const
{
	unordered_set<const Variable*> desc;
	if (!_children.find(v)->second.empty()) {
		for (auto pv : _children.find(v)->second) {
			desc.insert(pv);
			for (auto d : descendants(pv)) {
				desc.insert(d);
			}
		}
	}
	return desc;
}

unordered_set<const Variable*>
BN::ancestors(const Variable *v) const
{
	unordered_set<const Variable*> anc;
	if (!_parents.find(v)->second.empty()) {
		for (auto pv : _parents.find(v)->second) {
			if (anc.find(pv) == anc.end()) {
				anc.insert(pv);
				for (auto d : ancestors(pv)) {
					anc.insert(d);
				}
			}
		}
	}
	return anc;
}

unordered_set<const Variable*>
BN::ancestors(const unordered_set<const Variable*> &vars) const
{
	unordered_set<const Variable*> anc;
	for (auto const v : vars) {
		if (!_parents.find(v)->second.empty()) {
			for (auto pv : _parents.find(v)->second) {
				if (anc.find(pv) == anc.end()) {
					anc.insert(pv);
					for (auto d : ancestors(pv)) {
						anc.insert(d);
					}
				}
			}
		}
	}
	return anc;
}

void
BN::write(ostream& os) const
{
	os << "BAYES:" << endl;
	os << ">> Variables" << endl;
	for (auto pv1 : _variables) {
		unordered_set<const Variable*> parents = _parents.find(pv1)->second;
		unordered_set<const Variable*> children = _children.find(pv1)->second;
		os << *pv1 << ", ";
		os << "parents:{";
		for (auto pv2 : parents) {
			os << " " << pv2->id();
		}
		os << " }, ";
		os << "children:{";
		for (auto pv2 : children) {
			os << " " << pv2->id();
		}
		os << " }" << endl;
		markov_independence(pv1);
	}
	os << endl;
	os << ">> Factors" << endl;
	for (auto pf : _factors) {
		os << *pf << endl;
	}
}

ostream&
operator<<(ostream &os, const BN &bn)
{
	bn.write(os);
	return os;
}


MN::MN(string name, vector<Variable*> &variables, vector<Factor*> &factors) : Model(name, variables, factors)
{
	for (auto pv : _variables) {
		unordered_set<const Variable*> neighbors;
		_neighbors[pv] = neighbors;
	}

	for (auto pf : _factors) {
		vector<const Variable*> scope = pf->domain().scope();
		for (auto pv1 : scope) {
			for (auto pv2: scope) {
				if (pv1->id() != pv2->id()) {
					_neighbors[pv1].insert(pv2);
				}
			}
		}
	}
}

void
MN::write(ostream& os) const
{
	os << "MARKOV:" << endl;
	os << ">> Variables" << endl;
	for (auto pv1 : _variables) {
		unordered_set<const Variable*> neighbors = _neighbors.find(pv1)->second;
		os << *pv1 << ", ";
		os << "neighbors:{";
		for (auto pv2 : neighbors) {
			os << " " << pv2->id();
		}
		os << " }" << endl;
	}
	os << endl;
	os << ">> Factors" << endl;
	for (auto pf : _factors) {
		os << *pf << endl;
	}
}

ostream&
operator<<(ostream &os, const MN &mn)
{
	mn.write(os);
	return os;
}

}
