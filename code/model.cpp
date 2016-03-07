#include "model.hh"

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
	}
	os << endl;
	os << ">> Factors" << endl;
	for (auto pf : bn._factors) {
		os << *pf << endl;
	}
	return os;
}

}