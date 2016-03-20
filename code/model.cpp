#include "model.hh"

#include <unordered_set>
#include <iostream>
#include <chrono>
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


Factor
BN::query(
	const unordered_set<const Variable*> &target,
	const unordered_set<const Variable*> &evidence,
	double &uptime,
	unordered_map<string,bool> &options) const
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
		Factor g = joint;
		for (auto pv : _variables) {
			if (evidence.find(pv) == evidence.end()) {
				g = g.sum_out(pv);
			}
		}
		f = f.divide(g);
	}

	auto end = chrono::steady_clock::now();
	auto diff = end - start;
	uptime = chrono::duration <double, milli> (diff).count();

	return f;
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

void
BN::accept(ModelVisitor &v) {
    v.dispatch(*this);
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

double
MN::partition(const unordered_map<unsigned,unsigned> &evidence) const
{
	Factor f = joint_distribution();
	f = f.conditioning(evidence);
	return f.partition();
}

vector<const Factor*>
MN::marginals(const unordered_map<unsigned,unsigned> &evidence) const
{
	Factor joint = joint_distribution();
	joint = joint.conditioning(evidence).normalize();

	vector<const Factor*> marg;
	for (auto pv : _variables) {
		marg.push_back(new Factor(marginal(pv, joint)));
	}
	return marg;
}

Factor
MN::marginal(const Variable *v, Factor &joint) const
{
	Factor f = joint;
	for (auto pv : _variables) {
		if (pv->id() != v->id()) {
			f = f.sum_out(pv);
		}
	}
	return f;
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

void
MN::accept(ModelVisitor &v) {
    v.dispatch(*this);
}

}
