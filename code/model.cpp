#include "model.hh"

#include <unordered_set>
#include <iostream>
using namespace std;

namespace bn {

BN::BN(string name, vector<Variable*> &variables, vector<Factor*> &factors) :
	_name(name),
	_variables(variables),
	_factors(factors)
{
	for (unsigned i = 0; i < _variables.size(); ++i) {
		const Variable *v = _variables[i];
		unordered_set<const Variable*> children;
		_children[v] = children;
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

BN::~BN()
{
	for (auto pv : _variables) {
		delete pv;
	}
	for (auto pf : _factors) {
		delete pf;
	}
}

Factor
BN::joint_distribution()
{
	Factor f(1.0);
	for (auto pf : _factors) {
		f *= *pf;
	}
	return f;
}

Factor
BN::query(const unordered_set<const Variable*> &target, const unordered_set<const Variable*> &evidence)
{
	static Factor joint = joint_distribution();

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
	return f;
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

ostream&
operator<<(ostream &os, const BN &bn)
{
	os << ">> Variables" << endl;
	for (auto pv1 : bn._variables) {
		unordered_set<const Variable*> parents = bn._parents.find(pv1)->second;
		unordered_set<const Variable*> children = bn._children.find(pv1)->second;
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
		bn.markov_independence(pv1);
	}
	os << endl;
	os << ">> Factors" << endl;
	for (auto pf : bn._factors) {
		os << *pf << endl;
	}
	return os;
}

}