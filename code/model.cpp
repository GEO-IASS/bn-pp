#include "model.hh"

using namespace std;

namespace bn {

BN::BN(string name, vector<Variable*> &variables, vector<Factor*> &factors) :
	_name(name),
	_variables(variables),
	_factors(factors)
{
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

ostream&
operator<<(ostream &os, const BN &bn)
{
	os << ">> Variables" << endl;
	for (auto pv : bn._variables) {
		os << *pv << endl;
	}
	os << endl;
	os << ">> Factors" << endl;
	for (auto pf : bn._factors) {
		os << *pf << endl;
	}
	return os;
}

}