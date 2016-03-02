#include "factor.hh"

#include <iostream>
using namespace std;

namespace bn {

Factor::Factor(const Domain *domain) : _values(vector<double>(domain->size()))
{
	_domain = domain;
	_partition = 0.0;
}

const double&
Factor::operator[](unsigned i) const
{
    if (i < size()) return _values[i];
    else throw "Factor::operator[]: Index out of range.";
}

double&
Factor::operator[](unsigned i)
{
    if (i < size()) return _values[i];
    else throw "Factor::operator[]: Index out of range.";
}

ostream&
operator<<(ostream &os, const Factor &f)
{
    const Domain *domain = f._domain;
    int width = f.width();
    int size = f.size();

    os << "Factor(";
    os << "width:" << width << ", ";
    os << "size:" << size << ", ";
    os << "partition:" << f.partition() << ", ";
    os << *domain << ")";

    return os;
}

}