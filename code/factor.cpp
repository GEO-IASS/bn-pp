#include "factor.hh"

#include <iostream>
#include <iomanip>
using namespace std;

namespace bn {

Factor::Factor(const Domain *domain, double value) : _values(vector<double>(domain->size()))
{
	_domain = domain;
	_partition = domain->size() * value;
}

Factor::Factor(double value) :
    _domain(new Domain()),
    _values(vector<double>(1, value)),
    _partition(value)
{
}

Factor::Factor(Factor &&f)
{
    _domain = f._domain;
    _values = f._values;
    _partition = f._partition;
    f._domain = nullptr;
    f._values.clear();
    f._partition = 0.0;
}

Factor::~Factor()
{
	delete _domain;
}


Factor&
Factor::operator=(Factor &&f)
{
    if (this != &f) {
        delete _domain;
        _values.clear();
        _domain = f._domain;
        _values = f._values;
        _partition = f._partition;
        f._domain = nullptr;
        f._values.clear();
        f._partition = 0.0;
    }
    return *this;
}

Factor
Factor::operator*(const Factor &f)
{
    return product(f);
}

void
Factor::operator*=(const Factor &f)
{
    *this = product(f);
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

Factor
Factor::product(const Factor &f) const
{
    const Domain *d1 = this->_domain;
    const Domain *d2 = f._domain;

    Domain *new_domain = new Domain(*d1, *d2);
    unsigned width = new_domain->width();
    unsigned size = new_domain->size();
    Factor new_factor(new_domain, 0.0);

    vector<unsigned> valuation(width, 0);
    double partition = 0;
    for (unsigned i = 0; i < size; ++i) {
        // find position in linearization of consistent valuation
        unsigned pos1 = d1->position_consistent_valuation(valuation, *new_domain);
        unsigned pos2 = d2->position_consistent_valuation(valuation, *new_domain);

        // set product factor value
        double value = (*this)[pos1] * f[pos2];
        new_factor[i] = value;
        partition += value;

        // find next valuation
        new_domain->next_valuation(valuation);
    }
    new_factor._partition = partition;
    return new_factor;
}

ostream&
operator<<(ostream &os, const Factor &f)
{
    const Domain *domain = f._domain;
    int width = f.width();
    int size = f.size();
    double partition = f._partition;

    os << "Factor(";
    os << "width:" << width << ", ";
    os << "size:" << size << ", ";
    os << "partition:" << partition << ")" << endl;

    // scope
    for (int i = 0; i < width; ++i) {
        os << (*domain)[i]->id() << " ";
    }
    os << endl;

    // values
    vector<unsigned> valuation(width, 0);
    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < width; ++j) {
            os << valuation[j] << " ";
        }
        os << ": " << fixed << setprecision(5) << f[i] << endl;
        domain->next_valuation(valuation);
    }

    return os;
}

}
