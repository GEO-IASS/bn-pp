#ifndef _BN_FACTOR_H_
#define _BN_FACTOR_H_

#include "domain.hh"

#include <vector>

namespace bn {

class Factor {
public:
    Factor(const Domain *domain);

    unsigned size()    const { return _domain->size();  }
    unsigned width()   const { return _domain->width(); }
    double partition() const { return _partition; }

    const double &operator[](unsigned i) const;
    double &operator[](unsigned i);

    void partition(double p) { _partition = p; }

    friend std::ostream &operator<<(std::ostream &os, const Factor &f);

private:
    const Domain *_domain;
    std::vector<double> _values;
    double _partition;
};

}

#endif