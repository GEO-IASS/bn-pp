#include "model.hh"
#include "graph.hh"

#include <unordered_set>
#include <forward_list>
#include <iostream>
#include <chrono>
#include <cassert>
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
	auto start = chrono::steady_clock::now();

	vector<const Variable*> variables;
	for (auto const pv : _variables) {
		variables.push_back(pv);
	}
	vector<const Factor*> factors;
	for (auto const pf : _factors) {
		factors.push_back(new Factor(pf->conditioning(evidence)));
	}
	Factor part = variable_elimination(variables, factors, options);
	assert(part[0] == part.partition());
	double p = part.partition();

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

	vector<const Factor*> factors;
	for (auto const pf : _factors) {
		factors.push_back(new Factor(pf->conditioning(evidence)));
	}

	vector<const Factor*> marg;
	for (auto const pv : _variables) {
		vector<const Variable*> vars;
		for (auto const pv2 : _variables) {
			if (pv2 != pv) {
				vars.push_back(pv2);
			}
		}
		marg.push_back(new Factor(variable_elimination(vars, factors, options).normalize()));
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

vector<const Factor*>
BN::sampling_order() const
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
				// cout << *_variables[id] << endl;
				// cout << *_factors[id] << endl;
				// cout << endl;
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

unordered_map<unsigned,unsigned>
BN::sampling() const
{
	static vector<const Factor*> order = sampling_order();
	unordered_map<unsigned,unsigned> valuation;
	for (auto pf : order) {
		unordered_map<unsigned,unsigned> sample = pf->sampling(valuation);
		for (auto it : sample) {
			valuation[it.first] = it.second;
		}
	}
	return valuation;
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
