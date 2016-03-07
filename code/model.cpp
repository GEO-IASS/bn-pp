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
		vector<const Variable*> children;
		_children[v->id()] = children;
	}

	for (unsigned i = 0; i < _variables.size(); ++i) {
		const Variable *v = _variables[i];
		vector<const Variable*> scope = _factors[i]->domain().scope();
		vector<const Variable*> parents(scope.begin()+1, scope.end());
		_parents[v->id()] = parents;
		for (auto p : parents) {
			_children[p->id()].push_back(v);
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
BN::query(std::vector<Variable*> &target, std::vector<Variable*> &evidence)
{
	return joint_distribution();
}

void
BN::markov_independence(const Variable* v) const
{
	unordered_set<unsigned> nd;
	for (auto pv : _variables) {
		nd.insert(pv->id());
	}
	nd.erase(nd.find(v->id()));

	for (auto pv : _parents.find(v->id())->second) {
		nd.erase(nd.find(pv->id()));
	}
	for (auto id : descendants(v)) {
		nd.erase(nd.find(id));
	}

	cout << "descendants: " << *v << endl;
	for (auto id : nd) {
		cout << " " << id;
	}
	cout <<endl;
}

unordered_set<unsigned>
BN::descendants(const Variable *v) const
{
	unordered_set<unsigned> desc;
	if (!_children.find(v->id())->second.empty()) {
		for (auto pv : _children.find(v->id())->second) {
			desc.insert(pv->id());
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
		vector<const Variable*> parents = bn._parents.find(pv1->id())->second;
		vector<const Variable*> children = bn._children.find(pv1->id())->second;
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