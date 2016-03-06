#ifndef _BN_FACTOR_H_
#define _BN_FACTOR_H_

#include "domain.hh"

#include <vector>

namespace bn {

class Factor {
public:
    Factor(const Domain *domain, std::vector<double> values, double partition);
    Factor(const Domain *domain, double value);
    Factor(double value = 1.0);
    Factor(const Factor &f);
    Factor(Factor &&f);
    ~Factor();

    Factor &operator=(Factor &&f);
    Factor operator*(const Factor &f);
    void operator*=(const Factor &f);

    unsigned size()    const { return _domain->size();  }
    unsigned width()   const { return _domain->width(); }
    double partition() const { return _partition; }

    const double &operator[](unsigned i) const;
    double &operator[](unsigned i);

    Factor sum_out(const Variable *variable) const;
    Factor product(const Factor &f) const;

    friend std::ostream &operator<<(std::ostream &os, const Factor &f);

private:
    const Domain *_domain;
    std::vector<double> _values;
    double _partition;
};

}

#endif